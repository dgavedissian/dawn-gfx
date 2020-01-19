/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "Common.h"

class Textured3DCube : public Example {
public:
    Mesh box_;
    ProgramHandle program_;
    TextureHandle texture_;

    void start() override {
        // Load shaders.
        auto vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("shaders/cube_textured.vert"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("shaders/cube_textured.frag"));
        program_ = r.createProgram();
        r.attachShader(program_, vs);
        r.attachShader(program_, fs);
        r.linkProgram(program_);

        // Load texture.
        texture_ = util::loadTexture(r, util::media("wall.jpg"));

        // Create box.
        box_ = MeshBuilder{r}.normals(true).texcoords(true).createBox(10.0f);
    }

    void render(float dt) override {
        r.setRenderQueueClear({0.0f, 0.0f, 0.2f});

        // Calculate matrices.
        static float angle = 0.0f;
        angle += M_PI / 4.0f * dt;  // 45 degrees per second.
        Mat4 model = Mat4::Translate(Vec3{0.0f, 0.0f, -50.0f}).ToFloat4x4() *
                     Mat4::RotateX(M_PI / 8.0f) * Mat4::RotateY(angle);
        static Mat4 view = Mat4::identity;
        static Mat4 proj = util::createProjMatrix(r, 0.1f, 1000.0f, 60.0f, aspect());
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

DEFINE_MAIN(Textured3DCube);
