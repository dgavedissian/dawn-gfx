/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "Common.h"

class BasicIndexBuffer : public Example {
public:
    VertexBufferHandle vb_;
    IndexBufferHandle ib_;
    ProgramHandle program_;

    void start() override {
        // Load shaders.
        auto vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("shaders/basic_colour.vert"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("shaders/basic_colour.frag"));
        program_ = r.createProgram({vs, fs});

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

DEFINE_MAIN(BasicIndexBuffer);
