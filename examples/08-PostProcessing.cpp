/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "Common.h"

class PostProcessing : public Example {
public:
    Mesh box_;
    ProgramHandle box_program_;

    ProgramHandle post_process_;
    FrameBufferHandle fb_handle_;

    void start() override {
        // Load shaders.
        auto vs = util::loadShader(r, ShaderStage::Vertex, util::media("shaders/cube_solid.vert"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("shaders/cube_solid.frag"));
        box_program_ = r.createProgram({vs, fs});

        // Create box.
        box_ = MeshBuilder{r}.normals(true).texcoords(true).createBox(10.0f);

        // Set up frame buffer.
        fb_handle_ = r.createFrameBuffer(width(), height(), TextureFormat::RGBA8);

        // Load post process shader.
        auto pp_vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("shaders/post_process.vert"));
        auto pp_fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("shaders/post_process.frag"));
        post_process_ = r.createProgram({pp_vs, pp_fs});
    }

    void render(float dt) override {
        // Set up render queue to frame buffer.
        r.startRenderQueue(fb_handle_);
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
        r.setVertexBuffer(box_.vb);
        r.setIndexBuffer(box_.ib);
        r.submit(box_program_, box_.index_count);

        // Set up render queue to draw fsq to backbuffer.
        r.startRenderQueue();
        r.setRenderQueueClear({0.0f, 0.2f, 0.0f});

        // Draw fb.
        r.setTexture(1, r.getFrameBufferTexture(fb_handle_, 0));
        r.submitFullscreenQuad(post_process_);
    }

    void stop() override {
        r.deleteProgram(post_process_);
        r.deleteProgram(box_program_);
    }
};

DEFINE_MAIN(PostProcessing);
