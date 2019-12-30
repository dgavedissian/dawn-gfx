/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#include <dawn-gfx/Renderer.h>
#include <dawn-gfx/Shader.h>
#include <dawn-gfx/MeshBuilder.h>

#include <iostream>
#include <fstream>
#include <chrono>
#include <random>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef DGA_EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

using namespace dw::gfx;

class StdoutLogger : public Logger {
public:
    void log(LogLevel level, const std::string& str) const override {
        (level == LogLevel::Error ? std::cerr : std::cout) << str << std::endl;
    }
};

class Example {
public:
    Example() : r(logger_), frame_start_time_(std::chrono::steady_clock::now()) {
        r.init(RendererType::OpenGL, 1024, 768, "Example", InputCallbacks{}, true);
    }

    void tick() {
        render(dt_);
        if (!r.frame()) {
            running_ = false;
        }

        // Update delta time.
        auto now = std::chrono::steady_clock::now();
        dt_ = std::chrono::duration_cast<std::chrono::duration<float>>(now - frame_start_time_)
                  .count();
        frame_start_time_ = now;
    }

    bool running() const {
        return running_;
    }

    u16 width() const {
        return r.backbufferSize().x;
    }

    u16 height() const {
        return r.backbufferSize().y;
    }

    float aspect() const {
        return static_cast<float>(r.backbufferSize().x) / static_cast<float>(r.backbufferSize().y);
    }

    virtual void start() = 0;
    virtual void render(float dt) = 0;
    virtual void stop() = 0;

    Renderer r;

private:
    StdoutLogger logger_;
    float dt_ = 1.0f / 60.0f;
    std::chrono::steady_clock::time_point frame_start_time_;
    bool running_ = true;
};

#define TEST_CLASS(test_name) class test_name : public Example

// Utils
namespace util {
Mat4 createProjMatrix(float n, float f, float fov_y, float aspect) {
    auto tangent = static_cast<float>(tan(fov_y * M_DEGTORAD_OVER_2));  // tangent of half fovY
    float v = n * tangent * 2;                                          // half height of near plane
    float h = v * aspect;                                               // half width of near plane
    return Mat4::OpenGLPerspProjRH(n, f, h, v);
}

uint createFullscreenQuad(Renderer& r, VertexBufferHandle& vb) {
    // clang-format off
    float vertices[] = {
        // Position       | UV
        -1.0f, -1.0f, 0.0f, 0.0f,
        3.0f,  -1.0f, 2.0f, 0.0f,
        -1.0f,  3.0f, 0.0f, 2.0f};
    // clang-format on
    VertexDecl decl;
    decl.begin()
        .add(VertexDecl::Attribute::Position, 2, VertexDecl::AttributeType::Float)
        .add(VertexDecl::Attribute::TexCoord0, 2, VertexDecl::AttributeType::Float)
        .end();
    vb = r.createVertexBuffer(Memory(vertices, sizeof(vertices)), decl);
    return 3;
}

ShaderHandle loadShader(Renderer& r, ShaderStage type, const std::string& source_file) {
    std::ifstream shader(source_file);
    std::string shader_source((std::istreambuf_iterator<char>(shader)),
                              std::istreambuf_iterator<char>());
    auto spv_result = compileGLSL(shader_source, type);
    if (!spv_result) {
        throw std::runtime_error("Compile error: " + spv_result.error().compile_error);
    }
    return r.createShader(type, spv_result.value().entry_point,
                          Memory(std::move(spv_result.value().spirv)));
}

TextureHandle loadTexture(Renderer& r, const std::string& texture) {
    int width, height, bpp;
    stbi_uc* buffer = stbi_load(texture.c_str(), &width, &height, &bpp, 4);
    if (!buffer) {
        throw std::runtime_error("Failed to load " + texture);
    }
    Memory data(buffer, static_cast<u32>(width * height * 4),
                [](byte* buffer) { stbi_image_free(buffer); });
    return r.createTexture2D(static_cast<u16>(width), static_cast<u16>(height),
                             TextureFormat::RGBA8, std::move(data));
}

std::string media(const std::string& media_name) {
#ifdef DGA_EMSCRIPTEN
    return "/media/" + media_name;
#else
    return "../../examples/media" + media_name;
#endif
}
}  // namespace util

TEST_CLASS(BasicVertexBuffer) {
public:
    VertexBufferHandle vb_;
    ProgramHandle program_;

    void start() override {
        // Load shaders.
        auto vs = util::loadShader(r, ShaderStage::Vertex, util::media("/shaders/basic_colour.vs"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("/shaders/basic_colour.fs"));
        program_ = r.createProgram();
        r.attachShader(program_, vs);
        r.attachShader(program_, fs);
        r.linkProgram(program_);

        struct Vertex {
            float x;
            float y;
            u32 colour;
        };
        Vertex vertices[] = {
            // Little-endian, so colours are 0xAABBGGRR.
            {0.0f, 0.5f, 0xff0000ff},    // Vertex 1: Red
            {-0.5f, -0.5f, 0xff00ff00},  // Vertex 2: Green
            {0.5f, -0.5f, 0xffff0000}    // Vertex 3: Blue
        };
        VertexDecl decl;
        decl.begin()
            .add(VertexDecl::Attribute::Position, 2, VertexDecl::AttributeType::Float)
            .add(VertexDecl::Attribute::Colour, 4, VertexDecl::AttributeType::Uint8, true)
            .end();
        vb_ = r.createVertexBuffer(Memory(vertices, sizeof(vertices)), decl);
    }

    void render(float) override {
        r.setRenderQueueClear({0.0f, 0.0f, 0.2f});
        r.setVertexBuffer(vb_);
        r.submit(program_, 3);
    }

    void stop() override {
        r.deleteProgram(program_);
        r.deleteVertexBuffer(vb_);
    }
};

TEST_CLASS(BasicIndexBuffer) {
public:
    VertexBufferHandle vb_;
    IndexBufferHandle ib_;
    ProgramHandle program_;

    void start() override {
        // Load shaders.
        auto vs = util::loadShader(r, ShaderStage::Vertex, util::media("/shaders/basic_colour.vs"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("/shaders/basic_colour.fs"));
        program_ = r.createProgram();
        r.attachShader(program_, vs);
        r.attachShader(program_, fs);
        r.linkProgram(program_);

        float vertices[] = {
            -0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  // Top-left
            0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  // Top-right
            0.5f,  -0.5f, 0.0f, 0.0f, 1.0f,  // Bottom-right
            -0.5f, -0.5f, 1.0f, 1.0f, 1.0f   // Bottom-left
        };
        VertexDecl decl;
        decl.begin()
            .add(VertexDecl::Attribute::Position, 2, VertexDecl::AttributeType::Float)
            .add(VertexDecl::Attribute::Colour, 3, VertexDecl::AttributeType::Float)
            .end();
        vb_ = r.createVertexBuffer(Memory(vertices, sizeof(vertices)), decl);

        u32 elements[] = {0, 2, 1, 2, 0, 3};
        ib_ = r.createIndexBuffer(Memory(elements, sizeof(elements)), IndexBufferType::U32);
    }

    void render(float) override {
        r.setRenderQueueClear({0.0f, 0.0f, 0.2f});
        r.setVertexBuffer(vb_);
        r.setIndexBuffer(ib_);
        r.submit(program_, 6);
    }

    void stop() override {
        r.deleteProgram(program_);
        r.deleteVertexBuffer(vb_);
    }
};

TEST_CLASS(TransientIndexBuffer) {
public:
    ProgramHandle program_;

    void start() override {
        // Load shaders.
        auto vs = util::loadShader(r, ShaderStage::Vertex, util::media("/shaders/basic_colour.vs"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("/shaders/basic_colour.fs"));
        program_ = r.createProgram();
        r.attachShader(program_, vs);
        r.attachShader(program_, fs);
        r.linkProgram(program_);
    }

    void render(float dt) override {
        r.setRenderQueueClear({0.0f, 0.0f, 0.2f});

        static float angle = 0.0f;
        angle += dt;
        float size_multiplier = 1.0f;  //((float)sin(angle) + 1.0f) * 0.25f;
        float vertices[] = {
            -0.5f * size_multiplier, 0.5f * size_multiplier,  1.0f, 0.0f, 0.0f,  // Top-left
            0.5f * size_multiplier,  0.5f * size_multiplier,  0.0f, 1.0f, 0.0f,  // Top-right
            0.5f * size_multiplier,  -0.5f * size_multiplier, 0.0f, 0.0f, 1.0f,  // Bottom-right
            -0.5f * size_multiplier, -0.5f * size_multiplier, 1.0f, 1.0f, 1.0f   // Bottom-left
        };
        VertexDecl decl;
        decl.begin()
            .add(VertexDecl::Attribute::Position, 2, VertexDecl::AttributeType::Float)
            .add(VertexDecl::Attribute::Colour, 3, VertexDecl::AttributeType::Float)
            .end();
        auto tvb = *r.allocTransientVertexBuffer(sizeof(vertices) / decl.stride(), decl);
        auto* vertex_data = reinterpret_cast<float*>(r.getTransientVertexBufferData(tvb));
        memcpy(vertex_data, vertices, sizeof(vertices));

        u16 elements[] = {0, 2, 1, 2, 0, 3};
        auto tib = *r.allocTransientIndexBuffer(sizeof(elements) / sizeof(elements[0]));
        auto* index_data = reinterpret_cast<u16*>(r.getTransientIndexBufferData(tib));
        memcpy(index_data, elements, sizeof(elements));

        r.setVertexBuffer(tvb);
        r.setIndexBuffer(tib);
        r.submit(program_, 6);
    }

    void stop() override {
        r.deleteProgram(program_);
    }
};

TEST_CLASS(Textured3DCube) {
public:
    Mesh box_;
    ProgramHandle program_;
    TextureHandle texture_;

    void start() override {
        // Load shaders.
        auto vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("/shaders/cube_textured.vs"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("/shaders/cube_textured.fs"));
        program_ = r.createProgram();
        r.attachShader(program_, vs);
        r.attachShader(program_, fs);
        r.linkProgram(program_);
        r.setUniform("diffuse_texture", 0);
        r.submit(program_);

        // Load texture.
        texture_ = util::loadTexture(r, util::media("/wall.jpg"));

        // Create box.
        box_ = MeshBuilder{r}.normals(true).texcoords(true).createBox(10.0f);
    }

    void render(float dt) override {
        r.setRenderQueueClear({0.0f, 0.0f, 0.2f});

        // Calculate matrices.
        static float angle = 0.0f;
        angle += M_PI / 4.0f * dt;  // 45 degrees per second.
        Mat4 model = Mat4::Translate(Vec3{0.0f, 0.0f, -50.0f}).ToFloat4x4() * Mat4::RotateY(angle);
        static Mat4 view = Mat4::identity;
        static Mat4 proj = util::createProjMatrix(0.1f, 1000.0f, 60.0f, aspect());
        r.setUniform("model_matrix", model);
        r.setUniform("mvp_matrix", proj * view * model);
        r.setUniform("light_direction", Vec3{1.0f, 1.0f, 1.0f}.Normalized());

        // Set vertex buffer and submit.
        r.setTexture(texture_, 0);
        r.setVertexBuffer(box_.vb);
        r.setIndexBuffer(box_.ib);
        r.submit(program_, box_.index_count);
    }

    void stop() override {
        r.deleteProgram(program_);
    }
};

TEST_CLASS(Textured3DCubeNormalMap) {
public:
    Mesh box_;
    ProgramHandle program_;
    TextureHandle surface_texture_;
    TextureHandle normal_texture_;

    void start() override {
        // Load shaders.
        auto vs = util::loadShader(r, ShaderStage::Vertex,
                                   util::media("/shaders/cube_textured_normal_map.vs"));
        auto fs = util::loadShader(r, ShaderStage::Fragment,
                                   util::media("/shaders/cube_textured_normal_map.fs"));
        program_ = r.createProgram();
        r.attachShader(program_, vs);
        r.attachShader(program_, fs);
        r.linkProgram(program_);
        r.setUniform("diffuse_texture", 0);
        r.setUniform("normal_map_texture", 1);
        r.submit(program_);

        // Load texture.
        surface_texture_ = util::loadTexture(r, util::media("/stone_wall.jpg"));
        normal_texture_ = util::loadTexture(r, util::media("/stone_wall_normal.jpg"));

        // Create box.
        box_ = MeshBuilder{r}.normals(true).texcoords(true).tangents(true).createBox(10.0f);
    }

    void render(float dt) override {
        r.setRenderQueueClear({0.0f, 0.0f, 0.2f});

        // Calculate matrices.
        static float angle = 0.0f;
        angle += M_PI / 4.0f * dt;  // 45 degrees per second.
        Mat4 model = Mat4::Translate(Vec3{0.0f, 0.0f, -50.0f}).ToFloat4x4() * Mat4::RotateY(angle);
        static Mat4 view = Mat4::identity;
        static Mat4 proj = util::createProjMatrix(0.1f, 1000.0f, 60.0f, aspect());
        r.setUniform("model_matrix", model);
        r.setUniform("mvp_matrix", proj * view * model);
        r.setUniform("light_direction", Vec3{1.0f, 1.0f, 1.0f}.Normalized());

        // Set vertex buffer and submit.
        r.setTexture(surface_texture_, 0);
        r.setTexture(normal_texture_, 1);
        r.setVertexBuffer(box_.vb);
        r.setIndexBuffer(box_.ib);
        r.submit(program_, box_.index_count);
    }

    void stop() override {
        r.deleteProgram(program_);
    }
};

TEST_CLASS(PostProcessing) {
public:
    Mesh box_;
    ProgramHandle box_program_;

    VertexBufferHandle fsq_vb_;
    ProgramHandle post_process_;
    FrameBufferHandle fb_handle_;

    void start() override {
        // Load shaders.
        auto vs = util::loadShader(r, ShaderStage::Vertex, util::media("/shaders/cube_solid.vs"));
        auto fs = util::loadShader(r, ShaderStage::Fragment, util::media("/shaders/cube_solid.fs"));
        box_program_ = r.createProgram();
        r.attachShader(box_program_, vs);
        r.attachShader(box_program_, fs);
        r.linkProgram(box_program_);
        r.setUniform("diffuse_colour", Vec3{1.0f, 1.0f, 1.0f});
        r.submit(box_program_);

        // Create box.
        box_ = MeshBuilder{r}.normals(true).texcoords(true).createBox(10.0f);

        // Create full screen quad.
        util::createFullscreenQuad(r, fsq_vb_);

        // Set up frame buffer.
        fb_handle_ = r.createFrameBuffer(width(), height(), TextureFormat::RGB8);

        // Load post process shader.
        auto pp_vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("/shaders/post_process.vs"));
        auto pp_fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("/shaders/post_process.fs"));
        post_process_ = r.createProgram();
        r.attachShader(post_process_, pp_vs);
        r.attachShader(post_process_, pp_fs);
        r.linkProgram(post_process_);
        r.setUniform("input_texture", 0);
        r.submit(post_process_);
    }

    void render(float dt) override {
        // Calculate matrices.
        static float angle = 0.0f;
        angle += M_PI / 4.0f * dt;  // 45 degrees per second.
        Mat4 model = Mat4::Translate(Vec3{0.0f, 0.0f, -50.0f}).ToFloat4x4() * Mat4::RotateY(angle);
        static Mat4 view = Mat4::identity;
        static Mat4 proj = util::createProjMatrix(0.1f, 1000.0f, 60.0f, aspect());
        r.setUniform("model_matrix", model);
        r.setUniform("mvp_matrix", proj * view * model);
        r.setUniform("light_direction", Vec3{1.0f, 1.0f, 1.0f}.Normalized());

        // Set up render queue to frame buffer.
        r.startRenderQueue(fb_handle_);
        r.setRenderQueueClear({0.0f, 0.0f, 0.2f});

        // Set vertex buffer and submit.
        r.setVertexBuffer(box_.vb);
        r.setIndexBuffer(box_.ib);
        r.submit(box_program_, box_.index_count);

        // Set up render queue to draw fsq to backbuffer.
        r.startRenderQueue();
        r.setRenderQueueClear({0.0f, 0.2f, 0.0f});

        // Draw fb.
        r.setTexture(r.getFrameBufferTexture(fb_handle_, 0), 0);
        r.setVertexBuffer(fsq_vb_);
        r.submit(post_process_, 3);
    }

    void stop() override {
        r.deleteProgram(post_process_);
        r.deleteVertexBuffer(fsq_vb_);
        r.deleteProgram(box_program_);
    }
};

TEST_CLASS(DeferredShading) {
public:
    Mesh ground_;
    Mesh sphere_;
    ProgramHandle ground_program_;
    ProgramHandle sphere_program_;

    TextureHandle texture_;

    ProgramHandle post_process_;

    VertexBufferHandle fsq_vb_;
    FrameBufferHandle gbuffer_;

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
            auto vs =
                util::loadShader(r, ShaderStage::Vertex, util::media("/shaders/light_pass.vs"));
            auto fs = util::loadShader(r, ShaderStage::Fragment,
                                       util::media("/shaders/point_light_pass.fs"));
            program_ = r.createProgram();
            r.attachShader(program_, vs);
            r.attachShader(program_, fs);
            r.linkProgram(program_);

            r.setUniform("screen_size", screen_size);
            r.setUniform("gb0_texture", 0);
            r.setUniform("gb1_texture", 1);
            r.setUniform("gb2_texture", 2);
            r.setUniform("light_colour", colour.rgb());
            r.setUniform("linear_term", linear_term);
            r.setUniform("quadratic_term", quadratic_term);
            r.submit(program_);
            sphere_ = MeshBuilder{r}.normals(false).texcoords(false).createSphere(
                light_sphere_radius_, 8, 8);
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
        auto vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("/shaders/object_gbuffer.vs"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("/shaders/object_gbuffer.fs"));
        ground_program_ = r.createProgram();
        r.attachShader(ground_program_, vs);
        r.attachShader(ground_program_, fs);
        r.linkProgram(ground_program_);
        r.setUniform("diffuse_texture", 0);
        r.setUniform("texcoord_scale", Vec2{kGroundSize / 5.0f, kGroundSize / 5.0f});
        r.submit(ground_program_);

        sphere_program_ = r.createProgram();
        r.attachShader(sphere_program_, vs);
        r.attachShader(sphere_program_, fs);
        r.linkProgram(sphere_program_);
        r.setUniform("diffuse_texture", 0);
        r.setUniform("texcoord_scale", Vec2{1.0f, 1.0f});
        r.submit(sphere_program_);

        // Create ground.
        ground_ = MeshBuilder{r}.normals(true).texcoords(true).createPlane(kGroundSize * 2.0f,
                                                                           kGroundSize * 2.0f);

        // Create ground.
        sphere_ = MeshBuilder{r}.normals(true).texcoords(true).createSphere(3.0f);

        // Load texture.
        texture_ = util::loadTexture(r, util::media("/wall.jpg"));

        // Create full screen quad.
        util::createFullscreenQuad(r, fsq_vb_);

        // Set up frame buffer.
        auto format = TextureFormat::RGBA32F;
        gbuffer_ = r.createFrameBuffer({r.createTexture2D(width(), height(), format, Memory()),
                                        r.createTexture2D(width(), height(), format, Memory()),
                                        r.createTexture2D(width(), height(), format, Memory())});

        // Load post process shader.
        auto pp_vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("/shaders/post_process.vs"));
        auto pp_fs = util::loadShader(r, ShaderStage::Fragment,
                                      util::media("/shaders/deferred_ambient_light_pass.fs"));
        post_process_ = r.createProgram();
        r.attachShader(post_process_, pp_vs);
        r.attachShader(post_process_, pp_fs);
        r.linkProgram(post_process_);
        r.setUniform("gb0_texture", 0);
        r.setUniform("gb1_texture", 1);
        r.setUniform("gb2_texture", 2);
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

        // Calculate matrices.
        Mat4 model = Mat4::RotateX(math::pi * -0.5f);
        static Mat4 view = (Mat4::Translate(Vec3{0.0f, 30.0f, 40.0f}).ToFloat4x4() *
                            Mat4::RotateX(math::pi * -0.25f))
                               .Inverted();
        static Mat4 proj = util::createProjMatrix(0.1f, 1000.0f, 60.0f, aspect());
        r.setUniform("model_matrix", model);
        r.setUniform("mvp_matrix", proj * view * model);

        // Draw ground.
        r.setVertexBuffer(ground_.vb);
        r.setIndexBuffer(ground_.ib);
        r.setTexture(texture_, 0);
        r.submit(ground_program_, ground_.index_count);

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
        r.setVertexBuffer(fsq_vb_);
        r.submit(post_process_, 3);

        // Update and draw point lights.
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
    }

    void stop() override {
        r.deleteProgram(post_process_);
        r.deleteVertexBuffer(fsq_vb_);
        r.deleteTexture(texture_);
        r.deleteProgram(ground_program_);
    }
};

int main() {
    auto example = std::make_unique<PostProcessing>();
    example->start();
#ifdef DGA_EMSCRIPTEN
    // void emscripten_set_main_loop(em_callback_func func, int fps, int simulate_infinite_loop);
    emscripten_set_main_loop_arg([](void* arg) { reinterpret_cast<Example*>(arg)->tick(); },
                                 reinterpret_cast<void*>(example.get()), 0, 1);
    std::cout << "After loop." << std::endl;
#else
    while (example->running()) {
        example->tick();
    }
#endif
    example->stop();
}
