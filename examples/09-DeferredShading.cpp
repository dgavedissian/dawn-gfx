/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "Common.h"

//#define DEBUG_GBUFFER

class PointLight {
public:
    PointLight(Renderer& r, Colour colour, float linear_term, float quadratic_term,
               const Vec2& screen_size)
        : r(r) {
        // Calculate radius.
        float light_max = std::fmaxf(std::fmaxf(colour.r(), colour.g()), colour.b());
        float min_light_level = 256.0f / 4.0f;
        light_sphere_radius_ =
            (-linear_term +
             std::sqrt(linear_term * linear_term -
                       4.0f * quadratic_term * (1.0f - min_light_level * light_max))) /
            (2.0f * quadratic_term);

        setPosition(Vec3::zero);

        // Load shaders.
        auto vs = util::loadShader(r, ShaderStage::Vertex,
                                   util::media("shaders/deferred_shading/light_pass.vert"));
        auto fs = util::loadShader(r, ShaderStage::Fragment,
                                   util::media("shaders/deferred_shading/light_pass_point.frag"));
        program_ = r.createProgram();
        r.attachShader(program_, vs);
        r.attachShader(program_, fs);
        r.linkProgram(program_);

        r.setUniform("screen_size", screen_size);
        r.setUniform("light_colour", colour.rgb());
        r.setUniform("linear_term", linear_term);
        r.setUniform("quadratic_term", quadratic_term);
        r.submit(program_);
        sphere_ =
            MeshBuilder{r}.normals(false).texcoords(false).createSphere(light_sphere_radius_, 8, 8);
    }

    ~PointLight() {
        r.deleteProgram(program_);
    }

    void setPosition(const Vec3& position) {
        position_ = position;
        model_ = Mat4::Translate(position);
    }

    void draw(const Mat4& view_matrix, const Mat4& proj_matrix) {
        Mat4 mvp = proj_matrix * view_matrix * model_;

        // Invert culling when inside the light sphere.
        Vec3 view_space_position = (view_matrix * Vec4(position_, 1.0f)).xyz();
        if (view_space_position.LengthSq() < (light_sphere_radius_ * light_sphere_radius_)) {
            r.setStateCullFrontFace(CullFrontFace::CW);
        }

        // Disable depth, and enable blending.
        r.setStateDisable(RenderState::Depth);
        r.setStateEnable(RenderState::Blending);
        r.setStateBlendEquation(BlendEquation::Add, BlendFunc::One, BlendFunc::One);

        // Draw sphere.
        r.setVertexBuffer(sphere_.vb);
        r.setIndexBuffer(sphere_.ib);
        r.setUniform("mvp_matrix", mvp);
        r.setUniform("light_position", position_);
        r.submit(program_, sphere_.index_count);
    }

private:
    Renderer& r;
    Mesh sphere_;
    ProgramHandle program_;

    Vec3 position_;
    Mat4 model_;

    float light_sphere_radius_;
};

class DeferredShading : public Example {
public:
    Mesh ground_;
    Mesh sphere_;
    ProgramHandle ground_program_;
    ProgramHandle sphere_program_;

    TextureHandle texture_;

    ProgramHandle post_process_;

    FrameBufferHandle gbuffer_;

    struct PointLightInfo {
        std::unique_ptr<PointLight> light;
        float angle_offset = 0.0f;
        Vec3 origin;
    };
    std::vector<PointLightInfo> point_lights;

    struct SphereInfo {
        Vec3 position;
    };
    std::vector<SphereInfo> spheres;

    std::mt19937 random_engine_{1};  // start with the same seed each time, for determinism.

    static constexpr auto kLightCount = 30;
    static constexpr auto kSphereCount = 50;
    static constexpr auto kGroundSize = 30.0f;

    void start() override {
        // Create render queue for setting initial uniform state.
        r.startRenderQueue();

        // Load shaders.
        auto vs = util::loadShader(r, ShaderStage::Vertex,
                                   util::media("shaders/deferred_shading/object_gbuffer.vert"));
        auto fs = util::loadShader(r, ShaderStage::Fragment,
                                   util::media("shaders/deferred_shading/object_gbuffer.frag"));
        ground_program_ = r.createProgram();
        r.attachShader(ground_program_, vs);
        r.attachShader(ground_program_, fs);
        r.linkProgram(ground_program_);
        r.setUniform("texcoord_scale", Vec2{kGroundSize / 5.0f, kGroundSize / 5.0f});
        r.submit(ground_program_);

        sphere_program_ = r.createProgram();
        r.attachShader(sphere_program_, vs);
        r.attachShader(sphere_program_, fs);
        r.linkProgram(sphere_program_);
        r.setUniform("texcoord_scale", Vec2{1.0f, 1.0f});
        r.submit(sphere_program_);

        // Create ground.
        ground_ = MeshBuilder{r}.normals(true).texcoords(true).createPlane(kGroundSize * 2.0f,
                                                                           kGroundSize * 2.0f);

        // Create ground.
        sphere_ = MeshBuilder{r}.normals(true).texcoords(true).createSphere(3.0f, 15, 15);

        // Load texture.
        texture_ = util::loadTexture(r, util::media("wall.jpg"));

        // Set up frame buffer.
        auto format = TextureFormat::RGBA32F;
        gbuffer_ = r.createFrameBuffer(
            {r.createTexture2D(width(), height(), format, Memory(), false, true),
             r.createTexture2D(width(), height(), format, Memory(), false, true),
             r.createTexture2D(width(), height(), format, Memory(), false, true)});

        // Load post process shader.
        auto pp_vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("shaders/post_process.vert"));
#ifdef DEBUG_GBUFFER
        auto pp_fs =
            util::loadShader(r, ShaderStage::Fragment,
                             util::media("shaders/deferred_shading/deferred_debug_gbuffer.frag"));
#else
        auto pp_fs = util::loadShader(
            r, ShaderStage::Fragment,
            util::media("shaders/deferred_shading/deferred_ambient_light_pass.frag"));
#endif
        post_process_ = r.createProgram();
        r.attachShader(post_process_, pp_vs);
        r.attachShader(post_process_, pp_fs);
        r.linkProgram(post_process_);
        r.setUniform("ambient_light", Vec3{0.1f, 0.1f, 0.1f});
        r.submit(post_process_);

        // Lights.
        std::array<float, 2> intervals = {0.5f, 1.0f};
        std::array<float, 2> weights = {0.0f, 1.0f};
        std::piecewise_linear_distribution<float> colour_channel_distribution(
            intervals.begin(), intervals.end(), weights.begin());
        std::normal_distribution<float> radius_distribution(/* mean radius */ 5.0f);
        std::uniform_real_distribution<float> angle_offset_distribution(-math::pi, math::pi);
        std::uniform_real_distribution<float> position_axis_distribution(-kGroundSize, kGroundSize);
        std::uniform_real_distribution<float> position_height_distribution(3.0f, 5.0f);
        for (int i = 0; i < kLightCount; ++i) {
            PointLightInfo light;
            light.light = std::make_unique<PointLight>(
                r,
                Colour{colour_channel_distribution(random_engine_),
                       colour_channel_distribution(random_engine_),
                       colour_channel_distribution(random_engine_)},
                0.18f, 0.11f, Vec2{static_cast<float>(width()), static_cast<float>(height())});
            light.angle_offset = angle_offset_distribution(random_engine_);
            light.origin.x = position_axis_distribution(random_engine_);
            light.origin.y = position_height_distribution(random_engine_);
            light.origin.z = position_axis_distribution(random_engine_);
            point_lights.emplace_back(std::move(light));
        }

        // Spheres.
        for (int i = 0; i < kSphereCount; ++i) {
            spheres.emplace_back(SphereInfo{Vec3{position_axis_distribution(random_engine_), 0.0f,
                                                 position_axis_distribution(random_engine_)}});
        }
    }

    void render(float dt) override {
        // Set up gbuffer render queue.
        r.startRenderQueue(gbuffer_);
        r.setRenderQueueClear({0.0f, 0.0f, 0.0f});

        // Calculate view and proj matrices.
        static Mat4 view = (Mat4::Translate(Vec3{0.0f, 30.0f, 40.0f}).ToFloat4x4() *
                            Mat4::RotateX(math::pi * -0.25f))
                               .Inverted();
        static Mat4 proj = util::createProjMatrix(r, 0.1f, 1000.0f, 60.0f, aspect());

        // Draw ground.
        {
            Mat4 model = Mat4::RotateX(math::pi * -0.5f);
            r.setUniform("model_matrix", model);
            r.setUniform("mvp_matrix", proj * view * model);

            r.setVertexBuffer(ground_.vb);
            r.setIndexBuffer(ground_.ib);
            r.setTexture(texture_, 0);
            r.submit(ground_program_, ground_.index_count);
        }

        // Draw spheres.
        for (const auto& sphere_info : spheres) {
            Mat4 model = Mat4::Translate(sphere_info.position);
            r.setUniform("model_matrix", model);
            r.setUniform("mvp_matrix", proj * view * model);
            r.setVertexBuffer(sphere_.vb);
            r.setIndexBuffer(sphere_.ib);
            r.setTexture(texture_, 0);
            r.submit(sphere_program_, sphere_.index_count);
        }

        // Set up backbuffer render queue.
        r.startRenderQueue();
        r.setRenderQueueClear({0.0f, 0.0f, 0.0f});

        // Draw fb.
        r.setTexture(r.getFrameBufferTexture(gbuffer_, 0), 0);
        r.setTexture(r.getFrameBufferTexture(gbuffer_, 1), 1);
        r.setTexture(r.getFrameBufferTexture(gbuffer_, 2), 2);
        r.submitFullscreenQuad(post_process_);

        // Update and draw point lights.
#ifndef DEBUG_GBUFFER
        static float angle = 0.0f;
        angle += dt;
        for (auto& light_info : point_lights) {
            light_info.light->setPosition(
                Vec3(light_info.origin.x + sin(angle + light_info.angle_offset) * 5.0f -
                         cos(angle - light_info.angle_offset) * 4.0f,
                     light_info.origin.y,
                     light_info.origin.z - sin(angle + light_info.angle_offset * 0.5f) * 5.5f +
                         cos(angle + light_info.angle_offset * 0.8f) * 6.0f));
            r.setTexture(r.getFrameBufferTexture(gbuffer_, 0), 0);
            r.setTexture(r.getFrameBufferTexture(gbuffer_, 1), 1);
            r.setTexture(r.getFrameBufferTexture(gbuffer_, 2), 2);
            light_info.light->draw(view, proj);
        }
#endif
    }

    void stop() override {
        r.deleteProgram(post_process_);
        r.deleteTexture(texture_);
        r.deleteProgram(ground_program_);
    }
};

DEFINE_MAIN(DeferredShading);
