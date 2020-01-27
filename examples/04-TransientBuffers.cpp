/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "Common.h"

class TransientIndexBuffer : public Example {
public:
    ProgramHandle program_;

    void start() override {
        // Load shaders.
        auto vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("shaders/basic_colour.vert"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("shaders/basic_colour.frag"));
        program_ = r.createProgram({vs, fs});
    }

    void render(float dt) override {
        r.setRenderQueueClear({0.0f, 0.0f, 0.2f});

        static float angle = 0.0f;
        angle += dt;
        float size_multiplier = (float(sin(angle)) + 1.5f) / 4;
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

DEFINE_MAIN(TransientIndexBuffer);
