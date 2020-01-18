/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "Common.h"

class Simple3D : public Example {
public:
    VertexBufferHandle vb_;
    IndexBufferHandle ib_;
    ProgramHandle program_;

    void start() override {
        // Load shaders.
        auto vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("shaders/basic_colour_3d.vert"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("shaders/basic_colour.frag"));
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

    void render(float dt) override {
        r.setRenderQueueClear({0.0f, 0.0f, 0.2f});

        // Calculate matrices.
        static float angle = 0.0f;
        angle += M_PI / 4.0f * dt;  // 45 degrees per second.
        Mat4 model = Mat4::Translate(Vec3{0.0f, 0.0f, -2.0f}).ToFloat4x4() * Mat4::RotateY(angle);
        static Mat4 view = Mat4::identity;
        Mat4 proj = util::createProjMatrix(0.1f, 1000.0f, 60.0f, aspect());
        r.setUniform("u.mvp_matrix", proj * view * model);

        r.setVertexBuffer(vb_);
        r.setIndexBuffer(ib_);
        r.submit(program_, 6);
    }

    void stop() override {
        r.deleteProgram(program_);
        r.deleteVertexBuffer(vb_);
    }
};

DEFINE_MAIN(Simple3D);
