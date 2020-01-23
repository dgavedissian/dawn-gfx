/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include <dawn-gfx/Renderer.h>
#include <dawn-gfx/Input.h>
#include <dawn-gfx/Shader.h>
#include <imgui.h>

// Header only ImGui backend.

namespace dw {
namespace gfx {

class ImGuiBackend {
public:
    ImGuiBackend(Renderer& r, ImGuiIO& io);
    ~ImGuiBackend();

    inline void newFrame();
    inline void render(ImDrawData* draw_data);

private:
    Renderer& r_;
    ImGuiIO& io_;

    // Renderer resources.
    VertexDecl vertex_decl_;
    ProgramHandle shader_program_;
};

// Implementation.
ImGuiBackend::ImGuiBackend(Renderer& r, ImGuiIO& io) : r_(r), io_(io) {
    io.BackendPlatformName = io.BackendRendererName = "dawn-gfx";

    // TODO: Resize this on screen size change.
    // TODO: Fill others settings of the io structure later.
    io.DisplaySize.x = r_.backbufferSize().x / r_.windowScale().x;
    io.DisplaySize.y = r_.backbufferSize().y / r_.windowScale().y;
    io.DisplayFramebufferScale.x = r_.windowScale().x;
    io.DisplayFramebufferScale.y = r_.windowScale().y;
    io.IniFilename = nullptr;

    // Load font texture atlas.
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    TextureHandle handle =
        r_.createTexture2D(static_cast<u16>(width), static_cast<u16>(height), TextureFormat::RGBA8,
                           Memory(pixels, width * height * 4));
    io.Fonts->TexID = reinterpret_cast<void*>(static_cast<dga::uintptr>(static_cast<u32>(handle)));

    // Set up key map.
    io.KeyMap[ImGuiKey_Tab] = Key::Tab;
    io.KeyMap[ImGuiKey_LeftArrow] = Key::Left;
    io.KeyMap[ImGuiKey_RightArrow] = Key::Right;
    io.KeyMap[ImGuiKey_UpArrow] = Key::Up;
    io.KeyMap[ImGuiKey_DownArrow] = Key::Down;
    io.KeyMap[ImGuiKey_PageUp] = Key::PageUp;
    io.KeyMap[ImGuiKey_PageDown] = Key::PageDown;
    io.KeyMap[ImGuiKey_Home] = Key::Home;
    io.KeyMap[ImGuiKey_End] = Key::End;
    io.KeyMap[ImGuiKey_Delete] = Key::Delete;
    io.KeyMap[ImGuiKey_Backspace] = Key::Backspace;
    io.KeyMap[ImGuiKey_Enter] = Key::Enter;
    io.KeyMap[ImGuiKey_Escape] = Key::Escape;
    io.KeyMap[ImGuiKey_A] = Key::A;
    io.KeyMap[ImGuiKey_C] = Key::C;
    io.KeyMap[ImGuiKey_V] = Key::V;
    io.KeyMap[ImGuiKey_X] = Key::X;
    io.KeyMap[ImGuiKey_Y] = Key::Y;
    io.KeyMap[ImGuiKey_Z] = Key::Z;

    // Set up vertex declaration.
    vertex_decl_.begin()
        .add(VertexDecl::Attribute::Position, 2, VertexDecl::AttributeType::Float)
        .add(VertexDecl::Attribute::TexCoord0, 2, VertexDecl::AttributeType::Float)
        .add(VertexDecl::Attribute::Colour, 4, VertexDecl::AttributeType::Uint8, true)
        .end();

    // Create shader.
    auto compiled_vs_result = compileGLSL(ShaderStage::Vertex, R"(
        #version 450 core

        layout(location = 0) in vec2 in_position;
        layout(location = 1) in vec2 in_texcoord;
        layout(location = 2) in vec4 in_colour;

        layout(binding = 0) uniform PerFrame {
            mat4 proj_matrix;
        };

        layout(location = 0) out VertexData {
            vec2 texcoord;
            vec4 colour;
        } o;

        void main()
        {
            gl_Position = proj_matrix * vec4(in_position, 0.0, 1.0);
            o.texcoord = in_texcoord;
            o.colour = in_colour;
        }
    )");
    if (!compiled_vs_result) {
        throw std::runtime_error("Failed to compile ImGui vertex shader: " +
                                 compiled_vs_result.error().compile_error);
    }
    auto compiled_fs_result = compileGLSL(ShaderStage::Fragment, R"(
        #version 450 core

        layout(location = 0) in VertexData {
            vec2 texcoord;
            vec4 colour;
        } i;

        layout(binding = 1) uniform sampler2D ui_texture;

        layout(location = 0) out vec4 out_colour;

        void main()
        {
            out_colour = i.colour * texture(ui_texture, i.texcoord);
        }
    )");
    if (!compiled_fs_result) {
        throw std::runtime_error("Failed to compile ImGui fragment shader: " +
                                 compiled_vs_result.error().compile_error);
    }
    auto vs = r_.createShader(ShaderStage::Vertex, compiled_vs_result->entry_point,
                              Memory(std::move(compiled_vs_result->spirv)));
    auto fs = r_.createShader(ShaderStage::Fragment, compiled_fs_result->entry_point,
                              Memory(std::move(compiled_fs_result->spirv)));
    shader_program_ = r_.createProgram();
    r_.attachShader(shader_program_, vs);
    r_.attachShader(shader_program_, fs);
    r_.linkProgram(shader_program_);
    r_.deleteShader(vs);
    r_.deleteShader(fs);
    r_.submit(shader_program_);
}

ImGuiBackend::~ImGuiBackend() {
    r_.deleteProgram(shader_program_);
}

void ImGuiBackend::newFrame() {
}

void ImGuiBackend::render(ImDrawData* draw_data) {
    if (!draw_data) {
        return;
    }

    // Give up if we have no draw surface.
    if (io_.DisplaySize.x == 0.0f || io_.DisplaySize.y == 0.0f) {
        return;
    }

    // Setup projection matrix.
    Mat4 proj_matrix = Mat4::OpenGLOrthoProjRH(-1.0f, 1.0f, io_.DisplaySize.x, io_.DisplaySize.y) *
                       Mat4::Translate(-io_.DisplaySize.x * 0.5f, io_.DisplaySize.y * 0.5f, 0.0f) *
                       Mat4::Scale(1.0f, -1.0f, 1.0f);
    r_.setUniform("proj_matrix", proj_matrix);

    // Create a new render queue specific for the UI elements.
    r_.startRenderQueue();

    // Process command lists.
    for (int n = 0; n < draw_data->CmdListsCount; ++n) {
        auto cmd_list = draw_data->CmdLists[n];

        // Update GPU buffers.
        auto& vtx_buffer = cmd_list->VtxBuffer;
        auto& idx_buffer = cmd_list->IdxBuffer;
        auto tvb = r_.allocTransientVertexBuffer(vtx_buffer.Size, vertex_decl_);
        if (!tvb) {
            return;
        }
        auto tib = r_.allocTransientIndexBuffer(idx_buffer.Size);
        if (!tib) {
            return;
        }
        memcpy(r_.getTransientVertexBufferData(*tvb), vtx_buffer.Data,
               vtx_buffer.Size * sizeof(ImDrawVert));
        memcpy(r_.getTransientIndexBufferData(*tib), idx_buffer.Data,
               idx_buffer.Size * sizeof(ImDrawIdx));

        // Execute draw commands.
        uint offset = 0;
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i) {
            const ImDrawCmd* cmd = &cmd_list->CmdBuffer[cmd_i];
            if (cmd->UserCallback) {
                cmd->UserCallback(cmd_list, cmd);
            } else {
                // Set render state.
                r_.setStateEnable(RenderState::Blending);
                r_.setStateBlendEquation(BlendEquation::Add, BlendFunc::SrcAlpha,
                                         BlendFunc::OneMinusSrcAlpha);
                r_.setStateDisable(RenderState::CullFace);
                r_.setStateDisable(RenderState::Depth);
                r_.setScissor(static_cast<u16>(cmd->ClipRect.x * io_.DisplayFramebufferScale.x),
                              static_cast<u16>(cmd->ClipRect.y * io_.DisplayFramebufferScale.y),
                              static_cast<u16>((cmd->ClipRect.z - cmd->ClipRect.x) *
                                               io_.DisplayFramebufferScale.x),
                              static_cast<u16>((cmd->ClipRect.w - cmd->ClipRect.y) *
                                               io_.DisplayFramebufferScale.y));

                // Set resources.
                r_.setTexture(1, TextureHandle{static_cast<TextureHandle::base_type>(
                                     reinterpret_cast<dga::uintptr>(cmd->TextureId))});
                r_.setVertexBuffer(*tvb);
                r_.setIndexBuffer(*tib);

                // Draw.
                r_.submit(shader_program_, cmd->ElemCount, offset);
            }
            offset += cmd->ElemCount;
        }
    }
}

}  // namespace gfx
}  // namespace dw
