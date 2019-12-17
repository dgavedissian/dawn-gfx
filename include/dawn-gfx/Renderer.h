/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Base.h"
#include "dawn-gfx/detail/Handle.h"
#include "Math.h"
#include "Colour.h"
#include "dawn-gfx/detail/Memory.h"
#include "Logger.h"
#include "VertexDecl.h"

#include <dga/barrier.h>
#include <tl/expected.hpp>
#include <vector>
#include <variant>
#include <unordered_map>
#include <string>
#include <array>
#include <atomic>
#include <thread>

#define DW_MAX_TEXTURE_SAMPLERS 8
#define DW_MAX_TRANSIENT_VERTEX_BUFFER_SIZE (1 << 20)
#define DW_MAX_TRANSIENT_INDEX_BUFFER_SIZE (1 << 20)

namespace dw {
// Handles.
namespace detail {
struct VertexBufferTag {};
struct TransientVertexBufferTag {};
struct IndexBufferTag {};
struct TransientIndexBufferTag {};
struct ShaderTag {};
struct ProgramTag {};
struct TextureTag {};
struct FrameBufferTag {};
}  // namespace detail
using VertexBufferHandle = Handle<detail::VertexBufferTag, -1>;
using TransientVertexBufferHandle = Handle<detail::TransientVertexBufferTag, -1>;
using IndexBufferHandle = Handle<detail::IndexBufferTag, -1>;
using TransientIndexBufferHandle = Handle<detail::TransientIndexBufferTag, -1>;
using ShaderHandle = Handle<detail::ShaderTag, -1>;
using ProgramHandle = Handle<detail::ProgramTag, -1>;
using TextureHandle = Handle<detail::TextureTag, -1>;
using FrameBufferHandle = Handle<detail::FrameBufferTag, -1>;

// Renderer type.
enum class RendererType { Null, OpenGL, D3D12, Vulkan };

// Shader type.
enum class ShaderStage { Vertex, Geometry, Fragment };

// Buffer usage.
enum class BufferUsage {
    Static,   // Never modified.
    Dynamic,  // Modified occasionally.
    Stream    // Modified every time.
};

// Index buffer type.
enum class IndexBufferType { U16, U32 };

// Texture format.
/*
 * RGBA16S
 * ^   ^ ^
 * |   | +-- [ ]Unorm
 * |   |     [F]loat
 * |   |     [S]norm
 * |   |     [I]nt
 * |   |     [U]int
 * |   +---- Number of bits per component
 * +-------- Components
 */
enum class TextureFormat {
    // Colour formats.
    A8,
    R8,
    R8I,
    R8U,
    R8S,
    R16,
    R16I,
    R16U,
    R16F,
    R16S,
    R32I,
    R32U,
    R32F,
    RG8,
    RG8I,
    RG8U,
    RG8S,
    RG16,
    RG16I,
    RG16U,
    RG16F,
    RG16S,
    RG32I,
    RG32U,
    RG32F,
    RGB8,
    RGB8I,
    RGB8U,
    RGB8S,
    BGRA8,
    RGBA8,
    RGBA8I,
    RGBA8U,
    RGBA8S,
    RGBA16,
    RGBA16I,
    RGBA16U,
    RGBA16F,
    RGBA16S,
    RGBA32I,
    RGBA32U,
    RGBA32F,
    RGBA4,
    RGB5A1,
    RGB10A2,
    RG11B10F,
    // Depth formats.
    D16,
    D24,
    D24S8,
    D32,
    D16F,
    D24F,
    D32F,
    D0S8,
    Count
};

// Render states.
enum class RenderState { CullFace, Depth, Blending };
enum class CullFrontFace { CCW, CW };
enum class PolygonMode { Fill, Wireframe };
enum class BlendFunc {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate
};
enum class BlendEquation { Add, Subtract, ReverseSubtract, Min, Max };

// Render commands.
namespace cmd {
struct CreateVertexBuffer {
    VertexBufferHandle handle;
    Memory data;
    uint size;
    VertexDecl decl;
    BufferUsage usage;
};

struct UpdateVertexBuffer {
    VertexBufferHandle handle;
    Memory data;
    uint offset;
};

struct DeleteVertexBuffer {
    VertexBufferHandle handle;
};

struct CreateIndexBuffer {
    IndexBufferHandle handle;
    Memory data;
    uint size;
    IndexBufferType type;
    BufferUsage usage;
};

struct UpdateIndexBuffer {
    IndexBufferHandle handle;
    Memory data;
    uint offset;
};

struct DeleteIndexBuffer {
    IndexBufferHandle handle;
};

struct CreateShader {
    ShaderHandle handle;
    ShaderStage stage;
    std::string entry_point;
    Memory data;
};

struct DeleteShader {
    ShaderHandle handle;
};

struct CreateProgram {
    ProgramHandle handle;
};

struct AttachShader {
    ProgramHandle handle;
    ShaderHandle shader_handle;
};

struct LinkProgram {
    ProgramHandle handle;
};

struct DeleteProgram {
    ProgramHandle handle;
};

struct CreateTexture2D {
    TextureHandle handle;
    u16 width;
    u16 height;
    TextureFormat format;
    // TODO: Support custom mips.
    // TODO: Support different filtering.
    Memory data;
};

struct DeleteTexture {
    TextureHandle handle;
};

struct CreateFrameBuffer {
    FrameBufferHandle handle;
    u16 width;
    u16 height;
    // TODO: do we own textures or does the user own textures? add a flag?
    std::vector<TextureHandle> textures;
};

struct DeleteFrameBuffer {
    FrameBufferHandle handle;
};
}  // namespace cmd

// clang-format off
using RenderCommand =
    std::variant<cmd::CreateVertexBuffer,
            cmd::UpdateVertexBuffer,
            cmd::DeleteVertexBuffer,
            cmd::CreateIndexBuffer,
            cmd::UpdateIndexBuffer,
            cmd::DeleteIndexBuffer,
            cmd::CreateShader,
            cmd::DeleteShader,
            cmd::CreateProgram,
            cmd::AttachShader,
            cmd::LinkProgram,
            cmd::DeleteProgram,
            cmd::CreateTexture2D,
            cmd::DeleteTexture,
            cmd::CreateFrameBuffer,
            cmd::DeleteFrameBuffer>;
// clang-format on

using UniformData = std::variant<int, float, Vec2, Vec3, Vec4, Mat3, Mat4>;

// Current render state.
struct RenderItem {
    struct TextureBinding {
        TextureHandle handle;
    };

    // Vertices and indices.
    VertexBufferHandle vb;
    uint vb_offset = 0;  // Offset in bytes.
    VertexDecl vertex_decl_override;
    IndexBufferHandle ib;  // Offset in bytes.
    uint ib_offset = 0;
    uint primitive_count = 0;

    // Shader program and parameters.
    ProgramHandle program;
    std::unordered_map<std::string, UniformData> uniforms;
    std::array<TextureBinding, DW_MAX_TEXTURE_SAMPLERS> textures;

    // Scissor.
    bool scissor_enabled = false;
    u16 scissor_x = 0;
    u16 scissor_y = 0;
    u16 scissor_width = 0;
    u16 scissor_height = 0;

    // Render state.
    bool depth_enabled = true;
    bool cull_face_enabled = true;
    CullFrontFace cull_front_face = CullFrontFace::CCW;
    PolygonMode polygon_mode = PolygonMode::Fill;
    bool blend_enabled = false;
    BlendEquation blend_equation_rgb = BlendEquation::Add;
    BlendFunc blend_src_rgb = BlendFunc::One;
    BlendFunc blend_dest_rgb = BlendFunc::Zero;
    BlendEquation blend_equation_a = BlendEquation::Add;
    BlendFunc blend_src_a = BlendFunc::One;
    BlendFunc blend_dest_a = BlendFunc::Zero;
    bool colour_write = true;  // TODO: make component-wise
    bool depth_write = true;
    // TODO: Stencil write.
};

// View.
struct View {
    View();
    void clear();

    Colour clear_colour;
    FrameBufferHandle frame_buffer;
    std::vector<RenderItem> render_items;
};

// Frame.
class Renderer;
struct Frame {
    Frame();
    ~Frame() = default;

    View& view(uint view_index);

    RenderItem current_item;
    std::vector<View> views;
    std::vector<RenderCommand> commands_pre;
    std::vector<RenderCommand> commands_post;

    // Transient vertex/index buffer storage.
    struct {
        std::unique_ptr<std::byte[]> data;
        uint size;
        VertexBufferHandle handle;
    } transient_vb_storage;

    struct {
        std::unique_ptr<std::byte[]> data;
        uint size;
        IndexBufferHandle handle;
    } transient_ib_storage;

    // Transient vertex/index buffer data.
    struct TransientVertexBufferData {
        std::byte* data;
        uint size;
        VertexDecl decl;
    };
    std::unordered_map<TransientVertexBufferHandle, TransientVertexBufferData> transient_vertex_buffers_;
    TransientVertexBufferHandle next_transient_vertex_buffer_handle_;
    struct TransientIndexBufferData {
        std::byte* data;
        uint size;
    };
    std::unordered_map<TransientIndexBufferHandle, TransientIndexBufferData> transient_index_buffers_;
    TransientIndexBufferHandle next_transient_index_buffer_handle_;

#ifdef DW_DEBUG
    // Used for debugging to ensure that vertex buffers aren't updated multiple times a frame.
    HashSet<VertexBufferHandle> updated_vertex_buffers;
    HashSet<IndexBufferHandle> updated_index_buffers;
#endif
};

// Low level renderer.
class RenderContext;
class DW_API Renderer {
public:
    explicit Renderer(Logger& logger);
    ~Renderer();

    /// Initialise.
    tl::expected<void, std::string> init(RendererType type, u16 width, u16 height, const std::string& title,
                      bool use_render_thread);

    /// Create vertex buffer.
    VertexBufferHandle createVertexBuffer(Memory data, const VertexDecl& decl,
                                          BufferUsage usage = BufferUsage::Static);
    void setVertexBuffer(VertexBufferHandle handle);
    void updateVertexBuffer(VertexBufferHandle handle, Memory data, uint offset);
    void deleteVertexBuffer(VertexBufferHandle handle);

    /// Create index buffer.
    IndexBufferHandle createIndexBuffer(Memory data, IndexBufferType type,
                                        BufferUsage usage = BufferUsage::Static);
    void setIndexBuffer(IndexBufferHandle handle);
    void updateIndexBuffer(IndexBufferHandle handle, Memory data, uint offset);
    void deleteIndexBuffer(IndexBufferHandle handle);

    /// Transient vertex buffer.
    TransientVertexBufferHandle allocTransientVertexBuffer(uint vertex_count,
                                                           const VertexDecl& decl);
    std::byte* getTransientVertexBufferData(TransientVertexBufferHandle handle);
    void setVertexBuffer(TransientVertexBufferHandle handle);

    /// Transient index buffer.
    TransientIndexBufferHandle allocTransientIndexBuffer(uint index_count);
    std::byte* getTransientIndexBufferData(TransientIndexBufferHandle handle);
    void setIndexBuffer(TransientIndexBufferHandle handle);

    /// Create shader.
    ShaderHandle createShader(ShaderStage type, const std::string& entry_point, Memory data);
    void deleteShader(ShaderHandle handle);

    /// Create program.
    ProgramHandle createProgram();
    void attachShader(ProgramHandle program, ShaderHandle shader);
    void linkProgram(ProgramHandle program);
    void deleteProgram(ProgramHandle program);

    /// Uniforms.
    void setUniform(const std::string& uniform_name, int value);
    void setUniform(const std::string& uniform_name, float value);
    void setUniform(const std::string& uniform_name, const Vec2& value);
    void setUniform(const std::string& uniform_name, const Vec3& value);
    void setUniform(const std::string& uniform_name, const Vec4& value);
    void setUniform(const std::string& uniform_name, const Mat3& value);
    void setUniform(const std::string& uniform_name, const Mat4& value);
    void setUniform(const std::string& uniform_name, UniformData data);

    // Create texture.
    TextureHandle createTexture2D(u16 width, u16 height, TextureFormat format, Memory data);
    void setTexture(TextureHandle handle, uint sampler_unit);
    // get texture information.
    void deleteTexture(TextureHandle handle);

    // Framebuffer.
    FrameBufferHandle createFrameBuffer(u16 width, u16 height, TextureFormat format);
    FrameBufferHandle createFrameBuffer(std::vector<TextureHandle> textures);
    TextureHandle getFrameBufferTexture(FrameBufferHandle handle, uint index);
    void deleteFrameBuffer(FrameBufferHandle handle);

    /// View.
    void setViewClear(uint view, const Colour& colour);
    void setViewFrameBuffer(uint view, FrameBufferHandle handle);

    /// Update state.
    void setStateEnable(RenderState state);
    void setStateDisable(RenderState state);
    void setStateCullFrontFace(CullFrontFace front_face);
    void setStatePolygonMode(PolygonMode polygon_mode);
    void setStateBlendEquation(BlendEquation equation, BlendFunc src, BlendFunc dest);
    void setStateBlendEquation(BlendEquation equation_rgb, BlendFunc src_rgb, BlendFunc dest_rgb,
                               BlendEquation equation_a, BlendFunc src_a, BlendFunc dest_a);
    void setColourWrite(bool write_enabled);
    void setDepthWrite(bool write_enabled);

    /// Scissor.
    void setScissor(u16 x, u16 y, u16 width, u16 height);

    /// Update uniform and draw state, but submit no geometry.
    void submit(uint view, ProgramHandle program);

    /// Update uniform and draw state, then draw. Based off:
    /// https://github.com/bkaradzic/bgfx/blob/master/src/bgfx.cpp#L854
    void submit(uint view, ProgramHandle program, uint vertex_count);

    /// Update uniform and draw state, then draw. Based off:
    /// https://github.com/bkaradzic/bgfx/blob/master/src/bgfx.cpp#L854
    /// Offset is in vertices/indices depending on whether an index buffer is being used.
    void submit(uint view, ProgramHandle program, uint vertex_count, uint offset);

    /// Render a single frame.
    bool frame();

    /// Get the current window size.
    Vec2i windowSize() const;

    /// Get the current window scale.
    Vec2 windowScale() const;

    /// Get the current backbuffer size.
    Vec2i backbufferSize() const;

    /// Get the view which corresponds to the backbuffer.
    uint backbufferView() const;

private:
    Logger& logger_;

    u16 width_, height_;
    std::string window_title_;

    bool use_render_thread_;
    bool is_first_frame_;
    std::thread render_thread_;

    // Handles.
    HandleGenerator<VertexBufferHandle> vertex_buffer_handle_;
    HandleGenerator<IndexBufferHandle> index_buffer_handle_;
    HandleGenerator<ShaderHandle> shader_handle_;
    HandleGenerator<ProgramHandle> program_handle_;
    HandleGenerator<TextureHandle> texture_handle_;
    HandleGenerator<FrameBufferHandle> frame_buffer_handle_;

    // Vertex/index buffers.
    std::unordered_map<VertexBufferHandle, VertexDecl> vertex_buffer_decl_;
    std::unordered_map<IndexBufferHandle, IndexBufferType> index_buffer_types_;
    VertexBufferHandle transient_vb;
    uint transient_vb_max_size;
    IndexBufferHandle transient_ib;
    uint transient_ib_max_size;

    // Textures.
    struct TextureData {
        u16 width;
        u16 height;
        TextureFormat format;
    };
    std::unordered_map<TextureHandle, TextureData> texture_data_;

    // Framebuffers.
    std::unordered_map<FrameBufferHandle, std::vector<TextureHandle>> frame_buffer_textures_;

    // Shared.
    std::atomic<bool> shared_rt_should_exit_;
    bool shared_rt_finished_;
    dga::Barrier shared_frame_barrier_;
    std::mutex swap_mutex_;
    std::condition_variable swap_cv_;
    bool swapped_frames_;

    Frame frames_[2];
    Frame* submit_;
    Frame* render_;

    // Add a command to the submit thread.
    void submitPreFrameCommand(RenderCommand command);
    void submitPostFrameCommand(RenderCommand command);

    // Renderer.
    std::unique_ptr<RenderContext> shared_render_context_;

    // Render thread proc.
    void renderThread();
    bool renderFrame(Frame* frame);
};
}  // namespace dw
