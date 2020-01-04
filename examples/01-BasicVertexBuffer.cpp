/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#include "Common.h"

/*
    auto vertex_shader_result = compileGLSL(ShaderStage::Vertex, R"(
        #version 450
        #extension GL_ARB_separate_shader_objects : enable

        layout(location = 0) out vec3 fragColor;

        vec2 positions[3] = vec2[](
            vec2(0.0, -0.5),
            vec2(0.5, 0.5),
            vec2(-0.5, 0.5)
        );

        vec3 colors[3] = vec3[](
            vec3(1.0, 0.0, 0.0),
            vec3(0.0, 1.0, 0.0),
            vec3(0.0, 0.0, 1.0)
        );

        void main() {
            gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
            fragColor = colors[gl_VertexIndex];
        })");
    auto fragment_shader_result = compileGLSL(ShaderStage::Fragment, R"(#version 450
        #extension GL_ARB_separate_shader_objects : enable

        layout(location = 0) in vec3 fragColor;

        layout(location = 0) out vec4 outColor;

        void main() {
            outColor = vec4(fragColor, 1.0);
        })");
    if (!vertex_shader_result) {
        throw std::runtime_error(vertex_shader_result.error().compile_error);
    }
    auto& vertex_shader = *vertex_shader_result;
    if (!fragment_shader_result) {
        throw std::runtime_error(fragment_shader_result.error().compile_error);
    }
    auto& fragment_shader = *fragment_shader_result;
 */

class BasicVertexBuffer : public Example {
public:
    VertexBufferHandle vb_;
    ProgramHandle program_;

    void start() override {
        // Load shaders.
        auto vs = util::loadShader(r, ShaderStage::Vertex, util::media("shaders/basic_colour.vs"));
        auto fs =
            util::loadShader(r, ShaderStage::Fragment, util::media("shaders/basic_colour.fs"));
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
