/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Renderer.h"
#include "RenderContext.h"
#include "Logger.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace dw {
namespace gfx {
class SamplerCacheGL {
public:
    void setMaxSupportedAnisotropy(float max_supported_anisotropy);

    // Find a sampler object given a set of sampler flags. If the object does not exist, create it.
    GLuint findOrCreate(RenderItem::SamplerInfo info);

    // Clear the cache.
    void clear();

private:
    std::unordered_map<RenderItem::SamplerInfo, GLuint> cache_;
    float max_supported_anisotropy_;
};

class RenderContextGL : public RenderContext {
public:
    explicit RenderContextGL(Logger& logger);
    ~RenderContextGL() override;

    // Window management. Executed on the main thread.
    tl::expected<void, std::string> createWindow(u16 width, u16 height, const std::string& title,
                                                 InputCallbacks desc) override;
    void destroyWindow() override;
    void processEvents() override;
    bool isWindowClosed() const override;
    Vec2i windowSize() const override;
    Vec2 windowScale() const override;
    Vec2i framebufferSize() const override;

    // Command buffer processing. Executed on the render thread.
    void startRendering() override;
    void stopRendering() override;
    void processCommandList(std::vector<RenderCommand>& command_list) override;
    bool frame(const Frame* frame) override;

    // Variant walker methods. Executed on the render thread.
    void operator()(const cmd::CreateVertexBuffer& c);
    void operator()(const cmd::UpdateVertexBuffer& c);
    void operator()(const cmd::DeleteVertexBuffer& c);
    void operator()(const cmd::CreateIndexBuffer& c);
    void operator()(const cmd::UpdateIndexBuffer& c);
    void operator()(const cmd::DeleteIndexBuffer& c);
    void operator()(const cmd::CreateShader& c);
    void operator()(const cmd::DeleteShader& c);
    void operator()(const cmd::CreateProgram& c);
    void operator()(const cmd::AttachShader& c);
    void operator()(const cmd::LinkProgram& c);
    void operator()(const cmd::DeleteProgram& c);
    void operator()(const cmd::CreateTexture2D& c);
    void operator()(const cmd::DeleteTexture& c);
    void operator()(const cmd::CreateFrameBuffer& c);
    void operator()(const cmd::DeleteFrameBuffer& c);
    template <typename T> void operator()(const T& c) {
        static_assert(!std::is_same<T, T>::value, "Unimplemented RenderCommand");
    }

private:
    float max_supported_anisotropy_;

    // Window.
    GLFWwindow* window_;
    u16 backbuffer_width_;
    u16 backbuffer_height_;
    Vec2 window_scale_;

    // Input callbacks.
    std::function<void(Key::Enum key, Modifier::Enum modifier, bool pressed)> on_key_;
    std::function<void(const std::string& input)> on_char_input_;
    std::function<void(MouseButton::Enum button, bool pressed)> on_mouse_button_;
    std::function<void(const Vec2i& position)> on_mouse_move_;
    std::function<void(const Vec2& offset)> on_mouse_scroll_;

    GLuint vao_;
    VertexDecl current_vertex_decl;

    // Vertex and index buffers.
    struct VertexBufferData {
        GLuint vertex_buffer;
        VertexDecl decl;
        GLenum usage;
        size_t size;
    };
    struct IndexBufferData {
        GLuint element_buffer;
        GLenum type;
        GLenum usage;
        size_t size;
    };
    std::unordered_map<VertexBufferHandle, VertexBufferData> vertex_buffer_map_;
    std::unordered_map<IndexBufferHandle, IndexBufferData> index_buffer_map_;

    // Shaders and programs.
    struct ProgramData {
        GLuint program;
        std::unordered_map<std::string, GLint> uniform_location_map;
    };
    std::unordered_map<ShaderHandle, GLuint> shader_map_;
    std::unordered_map<ProgramHandle, ProgramData> program_map_;

    // Textures.
    std::unordered_map<TextureHandle, GLuint> texture_map_;
    SamplerCacheGL sampler_cache_;

    // Frame buffers.
    struct FrameBufferData {
        GLuint frame_buffer;
        GLuint depth_render_buffer;
        u16 width;
        u16 height;
        std::vector<TextureHandle> textures;
    };
    std::unordered_map<FrameBufferHandle, FrameBufferData> frame_buffer_map_;

    // Helper functions.
    void setupVertexArrayAttributes(const VertexDecl& decl, uint vb_offset);
};
}  // namespace gfx
}  // namespace dw
