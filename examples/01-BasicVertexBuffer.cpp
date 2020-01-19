/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "Common.h"

class BasicVertexBuffer : public Example {
public:
    VertexBufferHandle vb_;
    ProgramHandle program_;

    void start() override {
        // Load shaders.
        auto vs =
            util::loadShader(r, ShaderStage::Vertex, util::media("shaders/basic_colour.vert"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("shaders/basic_colour.frag"));
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

DEFINE_MAIN(BasicVertexBuffer);
