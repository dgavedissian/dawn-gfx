/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#include <dawn-gfx/RHIRenderer.h>
#include <dawn-gfx/Shader.h>
#include <iostream>
#include <fstream>

using namespace dw;

class StdoutLogger : public Logger {
public:
    void log(LogLevel level, const std::string& str) override {
        (level == LogLevel::Error ? std::cerr : std::cout) << str << std::endl;
    }
};

class Example {
public:
    Example() : r(logger_) {
        r.init(RendererType::OpenGL, 1024, 768, "Example", true);
    }

    u16 width() const {
        return r.backbufferSize().x;
    }

    u16 height() const {
        return r.backbufferSize().y;
    }

    virtual void start() {
    }
    virtual void render(float dt) {
    }
    virtual void stop() {
    }

    StdoutLogger logger_;
    RHIRenderer r;
};

#define TEST_CLASS(test_name) class test_name : public Example
#define TEST_BODY(test_name)                                                      \
public:                                                                           \
    test_name() : Example() { \
    }

// Utils
namespace util {
Mat4 createProjMatrix(float n, float f, float fov_y, float aspect) {
    auto tangent = static_cast<float>(tan(fov_y * dw::M_DEGTORAD_OVER_2));  // tangent of half fovY
    float v = n * tangent * 2;  // half height of near plane
    float h = v * aspect;       // half width of near plane
    return Mat4::OpenGLPerspProjRH(n, f, h, v);
}

uint createFullscreenQuad(RHIRenderer& r, VertexBufferHandle& vb) {
    // clang-format off
    float vertices[] = {
        // Position   | UV
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

ShaderHandle loadShader(RHIRenderer& r, ShaderStage type, const std::string& source_file) {
    std::ifstream shader(source_file);
    std::string shader_source((std::istreambuf_iterator<char>(shader)),
                    std::istreambuf_iterator<char>());
    auto spv_result = compileGLSL(shader_source, type);
    return r.createShader(type, Memory(std::move(spv_result.value())));
}
}  // namespace util

TEST_CLASS(RHIBasicVertexBuffer) {
TEST_BODY(RHIBasicVertexBuffer);

    VertexBufferHandle vb_;
    ProgramHandle program_;

    void start() override {
        // Load shaders.
        auto vs = util::loadShader(r, ShaderStage::Vertex, "../../examples/media/shaders/test.vs");
        auto fs = util::loadShader(r, ShaderStage::Fragment, "../../examples/media/shaders/test.fs");
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
        r.setViewClear(0, {0.0f, 0.0f, 0.2f, 1.0f});
        r.setVertexBuffer(vb_);
        r.submit(0, program_, 3);
    }

    void stop() override {
        r.deleteProgram(program_);
        r.deleteVertexBuffer(vb_);
    }
};

/*
TEST_CLASS(RHIBasicIndexBuffer) {
TEST_BODY(RHIBasicIndexBuffer);

    VertexBufferHandle vb_;
    IndexBufferHandle ib_;
    ProgramHandle program_;

    void start() override {
        module<FileSystem>()->setWorkingDir("../media/examples");

        // Load shaders.
        auto vs = util::loadShader(context(), ShaderStage::Vertex, "shaders/test.vs");
        auto fs = util::loadShader(context(), ShaderStage::Fragment, "shaders/test.fs");
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
        r.setVertexBuffer(vb_);
        r.setIndexBuffer(ib_);
        r.submit(0, program_, 6);
    }

    void stop() override {
        r.deleteProgram(program_);
        r.deleteVertexBuffer(vb_);
    }
};

TEST_CLASS(RHITransientIndexBuffer) {
TEST_BODY(RHITransientIndexBuffer);

    ProgramHandle program_;

    void start() override {
        module<FileSystem>()->setWorkingDir("../examples");

        // Load shaders.
        auto vs = util::loadShader(context(), ShaderStage::Vertex, "shaders/test.vs");
        auto fs = util::loadShader(context(), ShaderStage::Fragment, "shaders/test.fs");
        program_ = r.createProgram();
        r.attachShader(program_, vs);
        r.attachShader(program_, fs);
        r.linkProgram(program_);
    }

    void render(float dt) override {
        r.setViewClear(0, {0.0f, 0.0f, 0.2f, 1.0f});

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
        auto tvb = r.allocTransientVertexBuffer(sizeof(vertices) / decl.stride(), decl);
        float* vertex_data = (float*)r.getTransientVertexBufferData(tvb);
        memcpy(vertex_data, vertices, sizeof(vertices));

        u16 elements[] = {0, 2, 1, 2, 0, 3};
        auto tib = r.allocTransientIndexBuffer(sizeof(elements) / sizeof(elements[0]));
        u16* index_data = (u16*)r.getTransientIndexBufferData(tib);
        memcpy(index_data, elements, sizeof(elements));

        r.setVertexBuffer(tvb);
        r.setIndexBuffer(tib);
        r.submit(0, program_, 6);
    }

    void stop() override {
        r.deleteProgram(program_);
    }
};

TEST_CLASS(RHITextured3DCube) {
TEST_BODY(RHITextured3DCube);

    SharedPtr<CustomMeshRenderable> box_;
    ProgramHandle program_;

    // Uses the higher level wrapper which provides loading from files.
    UniquePtr<Texture> texture_resource_;

    void start() override {
        module<FileSystem>()->setWorkingDir("../media/examples");

        // Load shaders.
        auto vs = util::loadShader(context(), ShaderStage::Vertex, "shaders/cube_textured.vs");
        auto fs =
            util::loadShader(context(), ShaderStage::Fragment, "shaders/cube_textured.fs");
        program_ = r.createProgram();
        r.attachShader(program_, vs);
        r.attachShader(program_, fs);
        r.linkProgram(program_);
        r.setUniform("wall_sampler", 0);
        r.submit(0, program_);

        // Load texture.
        File texture_file{context(), "wall.jpg"};
        texture_resource_ = makeUnique<Texture>(context());
        texture_resource_->beginLoad("wall.jpg", texture_file);
        texture_resource_->endLoad();

        // Create box.
        box_ = MeshBuilder{context()}.normals(true).texcoords(true).createBox(10.0f);
    }

    void render(float dt) override {
        // Calculate matrices.
        static float angle = 0.0f;
        angle += M_PI / 3.0f * dt;  // 60 degrees per second.
        Mat4 model = Mat4::Translate(Vec3{0.0f, 0.0f, -50.0f}).ToFloat4x4() * Mat4::RotateY(angle);
        static Mat4 view = Mat4::identity;
        static Mat4 proj =
            util::createProjMatrix(0.1f, 1000.0f, 60.0f, static_cast<float>(width()) / height());
        r.setUniform("model_matrix", model);
        r.setUniform("mvp_matrix", proj * view * model);
        r.setUniform("light_direction", Vec3{1.0f, 1.0f, 1.0f}.Normalized());

        // Set vertex buffer and submit.
        r.setViewClear(0, {0.0f, 0.0f, 0.2f, 1.0f});
        r.setTexture(texture_resource_->internalHandle(), 0);
        r.setVertexBuffer(box_->vertexBuffer()->internalHandle());
        r.setIndexBuffer(box_->indexBuffer()->internalHandle());
        r.submit(0, program_, box_->indexBuffer()->indexCount());
    }

    void stop() override {
        r.deleteProgram(program_);
    }
};

TEST_CLASS(RHIPostProcessing) {
TEST_BODY(RHIPostProcessing);

    SharedPtr<CustomMeshRenderable> box_;
    ProgramHandle box_program_;

    VertexBufferHandle fsq_vb_;
    ProgramHandle post_process_;
    FrameBufferHandle fb_handle_;

    void start() override {
        module<FileSystem>()->setWorkingDir("../media/examples");

        // Load shaders.
        auto vs = util::loadShader(context(), ShaderStage::Vertex, "shaders/cube_solid.vs");
        auto fs = util::loadShader(context(), ShaderStage::Fragment, "shaders/cube_solid.fs");
        box_program_ = r.createProgram();
        r.attachShader(box_program_, vs);
        r.attachShader(box_program_, fs);
        r.linkProgram(box_program_);

        // Create box.
        box_ = MeshBuilder{context()}.normals(true).texcoords(true).createBox(10.0f);

        // Create full screen quad.
        util::createFullscreenQuad(r, fsq_vb_);

        // Set up frame buffer.
        fb_handle_ = r.createFrameBuffer(1280, 800, TextureFormat::RGB8);

        // Load post process shader.
        auto pp_vs =
            util::loadShader(context(), ShaderStage::Vertex, "shaders/post_process.vs");
        auto pp_fs =
            util::loadShader(context(), ShaderStage::Fragment, "shaders/post_process.fs");
        post_process_ = r.createProgram();
        r.attachShader(post_process_, pp_vs);
        r.attachShader(post_process_, pp_fs);
        r.linkProgram(post_process_);
        r.setUniform("in_sampler", 0);
        r.submit(0, post_process_);
    }

    void render(float dt) override {
        // Calculate matrices.
        static float angle = 0.0f;
        angle += M_PI / 3.0f * dt;  // 60 degrees per second.
        Mat4 model = Mat4::Translate(Vec3{0.0f, 0.0f, -50.0f}).ToFloat4x4() * Mat4::RotateY(angle);
        static Mat4 view = Mat4::identity;
        static Mat4 proj =
            util::createProjMatrix(0.1f, 1000.0f, 60.0f, static_cast<float>(width()) / height());
        r.setUniform("model_matrix", model);
        r.setUniform("mvp_matrix", proj * view * model);
        r.setUniform("light_direction", Vec3{1.0f, 1.0f, 1.0f}.Normalized());

        // Set up views.
        r.setViewClear(0, {0.0f, 0.0f, 0.2f, 1.0f});
        r.setViewFrameBuffer(0, fb_handle_);
        r.setViewClear(1, {0.0f, 0.2f, 0.0f, 1.0f});
        r.setViewFrameBuffer(1, FrameBufferHandle{0});

        // Set vertex buffer and submit.
        r.setVertexBuffer(box_->vertexBuffer()->internalHandle());
        r.setIndexBuffer(box_->indexBuffer()->internalHandle());
        r.submit(0, box_program_, box_->indexBuffer()->indexCount());

        // Draw fb.
        r.setTexture(r.getFrameBufferTexture(fb_handle_, 0), 0);
        r.setVertexBuffer(fsq_vb_);
        r.submit(1, post_process_, 3);
    }

    void stop() override {
        r.deleteProgram(post_process_);
        r.deleteVertexBuffer(fsq_vb_);
        r.deleteProgram(box_program_);
    }
};

TEST_CLASS(RHIDeferredShading) {
TEST_BODY(RHIDeferredShading);

    SharedPtr<CustomMeshRenderable> ground_;
    ProgramHandle cube_program_;

    // Uses the higher level wrapper which provides loading from files.
    UniquePtr<Texture> texture_resource_;

    ProgramHandle post_process_;

    VertexBufferHandle fsq_vb_;
    FrameBufferHandle gbuffer_;

    class PointLight : public Object {
    public:
        DW_OBJECT(PointLight);

        PointLight(Context* ctx, float radius, const Vec2& screen_size)
            : Object{ctx}, r{module<Renderer>()->rhi()}, light_sphere_radius_{radius * 4} {
            setPosition(Vec3::zero);

            // Load shaders.
            auto vs =
                util::loadShader(context(), ShaderStage::Vertex, "shaders/light_pass.vs");
            auto fs = util::loadShader(context(), ShaderStage::Fragment,
                                       "shaders/point_light_pass.fs");
            program_ = r.createProgram();
            r.attachShader(program_, vs);
            r.attachShader(program_, fs);
            r.linkProgram(program_);
            r.setUniform("screen_size", screen_size);
            r.setUniform("gb0", 0);
            r.setUniform("gb1", 1);
            r.setUniform("gb2", 2);
            r.setUniform("radius", radius);
            r.submit(0, program_);
            sphere_ = MeshBuilder{context_}.normals(false).texcoords(false).createSphere(
                light_sphere_radius_, 8, 8);
        }

        ~PointLight() {
            r.deleteProgram(program_);
        }

        void setPosition(const Vec3& position) {
            position_ = position;
            model_ = Mat4::Translate(position);
        }

        void draw(uint view, const Mat4& view_matrix, const Mat4& proj_matrix) {
            Mat4 mvp = proj_matrix * view_matrix * model_;

            // Invert culling when inside the light sphere.
            Vec3 view_space_position = (view_matrix * Vec4(position_, 1.0f)).xyz();
            if (view_space_position.LengthSq() < (light_sphere_radius_ * light_sphere_radius_)) {
                r.setStateCullFrontFace(CullFrontFace::CW);
            }

            // Disable depth, and enable blending.
            r.setStateDisable(RenderState::Depth);
            r.setStateEnable(RenderState::Blending);
            r.setStateBlendEquation(BlendEquation::Add, BlendFunc::One,
                                     BlendFunc::One);

            // Draw sphere.
            r.setVertexBuffer(sphere_->vertexBuffer()->internalHandle());
            r.setIndexBuffer(sphere_->indexBuffer()->internalHandle());
            r.setUniform("mvp_matrix", mvp);
            r.setUniform("light_position", position_);
            r.submit(view, program_, sphere_->indexBuffer()->indexCount());
        }

    private:
        RHIRenderer* r;
        SharedPtr<CustomMeshRenderable> sphere_;
        ProgramHandle program_;

        Vec3 position_;
        Mat4 model_;

        float light_sphere_radius_;
    };

    Vector<UniquePtr<PointLight>> point_lights;

    void start() override {
        module<FileSystem>()->setWorkingDir("../media/examples");

        // Load shaders.
        auto vs =
            util::loadShader(context(), ShaderStage::Vertex, "shaders/object_gbuffer.vs");
        auto fs =
            util::loadShader(context(), ShaderStage::Fragment, "shaders/object_gbuffer.fs");
        cube_program_ = r.createProgram();
        r.attachShader(cube_program_, vs);
        r.attachShader(cube_program_, fs);
        r.linkProgram(cube_program_);
        r.setUniform("wall_sampler", 0);
        r.setUniform("texcoord_scale", Vec2{10.0f, 10.0f});
        r.submit(0, cube_program_);

        // Create ground.
        ground_ = MeshBuilder{context_}.normals(true).texcoords(true).createPlane(250.0f, 250.0f);

        // Load texture.
        File texture_file{context(), "wall.jpg"};
        texture_resource_ = makeUnique<Texture>(context());
        texture_resource_->beginLoad("wall.jpg", texture_file);
        texture_resource_->endLoad();

        // Create full screen quad.
        util::createFullscreenQuad(r, fsq_vb_);

        // Set up frame buffer.
        auto format = TextureFormat::RGBA32F;
        gbuffer_ = r.createFrameBuffer({r.createTexture2D(width(), height(), format, Memory()),
                                         r.createTexture2D(width(), height(), format, Memory()),
                                         r.createTexture2D(width(), height(), format, Memory())});

        // Load post process shader.
        auto pp_vs =
            util::loadShader(context(), ShaderStage::Vertex, "shaders/post_process.vs");
        auto pp_fs = util::loadShader(context(), ShaderStage::Fragment,
                                      "shaders/deferred_ambient_light_pass.fs");
        post_process_ = r.createProgram();
        r.attachShader(post_process_, pp_vs);
        r.attachShader(post_process_, pp_fs);
        r.linkProgram(post_process_);
        r.setUniform("gb0_sampler", 0);
        r.setUniform("gb1_sampler", 1);
        r.setUniform("gb2_sampler", 2);
        r.setUniform("ambient_light", Vec3{0.02f, 0.02f, 0.02f});
        r.submit(0, post_process_);

        // Lights.
        for (int x = -3; x <= 3; x++) {
            for (int z = -3; z <= 3; z++) {
                point_lights.emplace_back(makeUnique<PointLight>(
                    context(), 10.0f,
                    Vec2{static_cast<float>(width()), static_cast<float>(height())}));
                point_lights.back()->setPosition(Vec3(x * 30.0f, -9.0f, z * 30.0f - 40.0f));
            }
        }
    }

    void render(float dt) override {
        // Set up views.
        r.setViewClear(0, {0.0f, 0.0f, 0.0f, 1.0f});
        r.setViewFrameBuffer(0, gbuffer_);
        r.setViewClear(1, {0.0f, 0.0f, 0.0f, 1.0f});
        r.setViewFrameBuffer(1, FrameBufferHandle{0});

        // Calculate matrices.
        Mat4 model = Mat4::Translate(Vec3{0.0f, -10.0f, 0.0f}).ToFloat4x4() *
                     Mat4::RotateX(math::pi * -0.5f);
        static Mat4 view = Mat4::Translate(Vec3{0.0f, 0.0f, 50.0f}).ToFloat4x4().Inverted();
        static Mat4 proj =
            util::createProjMatrix(0.1f, 1000.0f, 60.0f, static_cast<float>(width()) / height());
        r.setUniform("model_matrix", model);
        r.setUniform("mvp_matrix", proj * view * model);

        // Set vertex buffer and submit.
        r.setVertexBuffer(ground_->vertexBuffer()->internalHandle());
        r.setIndexBuffer(ground_->indexBuffer()->internalHandle());
        r.setTexture(texture_resource_->internalHandle(), 0);
        r.submit(0, cube_program_, ground_->indexBuffer()->indexCount());

        // Draw fb.
        r.setTexture(r.getFrameBufferTexture(gbuffer_, 0), 0);
        r.setTexture(r.getFrameBufferTexture(gbuffer_, 1), 1);
        r.setTexture(r.getFrameBufferTexture(gbuffer_, 2), 2);
        r.setVertexBuffer(fsq_vb_);
        r.submit(1, post_process_, 3);

        // Update and draw point lights.
        static float angle = 0.0f;
        angle += dt;
        int light_counter = 0;
        for (int x = -3; x <= 3; x++) {
            for (int z = -3; z <= 3; z++) {
                point_lights[light_counter]->setPosition(Vec3(
                    x * 20.0f + sin(angle) * 10.0f, -8.0f, z * 20.0f - 30.0f + cos(angle) * 10.0f));
                r.setTexture(r.getFrameBufferTexture(gbuffer_, 0), 0);
                r.setTexture(r.getFrameBufferTexture(gbuffer_, 1), 1);
                r.setTexture(r.getFrameBufferTexture(gbuffer_, 2), 2);
                point_lights[light_counter]->draw(1, view, proj);
                light_counter++;
            }
        }
    }

    void stop() override {
        r.deleteProgram(post_process_);
        r.deleteVertexBuffer(fsq_vb_);
        r.deleteProgram(cube_program_);
    }
};
*/

int main() {
    RHIBasicVertexBuffer example;
    example.start();
    while(true) {
        example.render(0.1f);
        if(!example.r.frame()) {
            break;
        }
    }
    example.stop();
}
