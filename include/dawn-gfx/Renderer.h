/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Base.h"
#include "detail/Handle.h"
#include "detail/Memory.h"
#include "MathDefs.h"
#include "Colour.h"
#include "Logger.h"
#include "VertexDecl.h"
#include "Input.h"

#include <dga/barrier.h>
#include <dga/hash_combine.h>
#include <vector>
#include <variant>
#include <unordered_map>
#include <string>
#include <array>
#include <atomic>
#include <thread>
#include <optional>

#define DW_MAX_TEXTURE_SAMPLERS 8
#define DW_MAX_TRANSIENT_VERTEX_BUFFER_SIZE (1 << 20)
#define DW_MAX_TRANSIENT_INDEX_BUFFER_SIZE (1 << 20)

// Handles.
#define DEFINE_HANDLE_TYPE(_Name)                                                                 \
    namespace dw {                                                                                \
    namespace gfx {                                                                               \
    struct _Name : BaseHandle<_Name> {                                                            \
        using BaseHandle::BaseHandle;                                                             \
    };                                                                                            \
    }                                                                                             \
    }                                                                                             \
    template <>                                                                                   \
    struct fmt::formatter<dw::gfx::_Name> : fmt::formatter<dw::gfx::_Name::base_type> {           \
        template <typename FormatCtx> auto format(const dw::gfx::_Name& handle, FormatCtx& ctx) { \
            return fmt::formatter<dw::gfx::_Name::base_type>::format(                             \
                static_cast<dw::gfx::_Name::base_type>(handle), ctx);                             \
        }                                                                                         \
    };                                                                                            \
    template <> struct std::hash<dw::gfx::_Name> {                                                \
        using argument_type = dw::gfx::_Name;                                                     \
        using result_type = std::size_t;                                                          \
        result_type operator()(const argument_type& k) const {                                    \
            std::hash<dw::gfx::BaseHandle<dw::gfx::_Name>> base_hash;                             \
            return base_hash(k);                                                                  \
        }                                                                                         \
    }

DEFINE_HANDLE_TYPE(VertexBufferHandle);
DEFINE_HANDLE_TYPE(TransientVertexBufferHandle);
DEFINE_HANDLE_TYPE(IndexBufferHandle);
DEFINE_HANDLE_TYPE(TransientIndexBufferHandle);
DEFINE_HANDLE_TYPE(ShaderHandle);
DEFINE_HANDLE_TYPE(ProgramHandle);
DEFINE_HANDLE_TYPE(UniformBufferHandle);
DEFINE_HANDLE_TYPE(TextureHandle);
DEFINE_HANDLE_TYPE(FrameBufferHandle);

namespace dw {
namespace gfx {

// Renderer type.
enum class RendererType { Null, OpenGL, Vulkan };

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

// Sampler flags.
namespace SamplerFlag {
enum Enum : std::uint32_t {
    // U wrapping mode.
    URepeat = 0x0001,  // repeating
    UMirror = 0x0002,  // mirrored
    UClamp = 0x0003,   // clamp

    // V wrapping mode.
    VRepeat = 0x0004,  // repeating
    VMirror = 0x0008,  // mirrored
    VClamp = 0x000c,   // clamp

    // W wrapping mode.
    WRepeat = 0x0010,  // repeating
    WMirror = 0x0020,  // mirrored
    WClamp = 0x0030,   // clamp

    // Combined wrapping modes.
    UVRepeat = URepeat | VRepeat,
    UVWRepeat = URepeat | VRepeat | WRepeat,
    UVMirror = UMirror | VMirror,
    UVWMirror = UMirror | VMirror | WMirror,
    UVClamp = UClamp | VClamp,
    UVWClamp = UClamp | VClamp | WClamp,

    // Min filter.
    MinPoint = 0x0040,   // nearest point filter
    MinLinear = 0x0080,  // linear (anisotropic) filter

    // Mag filter.
    MagPoint = 0x0100,   // nearest point filter
    MagLinear = 0x0200,  // linear (anisotropic) filter

    // Mip filter.
    MipPoint = 0x0400,   // nearest point filter
    MipLinear = 0x0800,  // linear (anisotropic) filter

    // Combined filters.
    MinMagPoint = MinPoint | MagPoint,
    MinMagLinear = MinLinear | MagLinear,

    // Default.
    Default = SamplerFlag::UVWRepeat | SamplerFlag::MinMagLinear | SamplerFlag::MipLinear
};

constexpr auto maskUWrappingMode = 0x0003;
constexpr auto shiftUWrappingMode = 0;
constexpr auto maskVWrappingMode = 0x000c;
constexpr auto shiftVWrappingMode = 2;
constexpr auto maskWWrappingMode = 0x0030;
constexpr auto shiftWWrappingMode = 4;
constexpr auto maskMinFilter = 0x00c0;
constexpr auto shiftMinFilter = 6;
constexpr auto maskMagFilter = 0x0300;
constexpr auto shiftMagFilter = 8;
constexpr auto maskMipFilter = 0x0c00;
constexpr auto shiftMipFilter = 10;
}  // namespace SamplerFlag

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

// Shader stage info.
struct ShaderStageInfo {
    ShaderStage stage;
    std::string entry_point;
    Memory spirv;
};

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

struct CreateProgram {
    ProgramHandle handle;
    std::vector<ShaderStageInfo> stages;
};

struct DeleteProgram {
    ProgramHandle handle;
};

struct CreateTexture2D {
    TextureHandle handle;
    u16 width;
    u16 height;
    TextureFormat format;
    Memory data;
    bool generate_mipmaps;
    bool framebuffer_usage;
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
            cmd::CreateProgram,
            cmd::DeleteProgram,
            cmd::CreateTexture2D,
            cmd::DeleteTexture,
            cmd::CreateFrameBuffer,
            cmd::DeleteFrameBuffer>;
// clang-format on

using UniformData = std::variant<int, float, Vec2, Vec3, Vec4, Mat3, Mat4>;

// Current render state.
struct RenderItem {
    struct SamplerInfo {
        u32 sampler_flags;
        float max_anisotropy;

        bool operator==(const SamplerInfo& other) const {
            return sampler_flags == other.sampler_flags && max_anisotropy == other.max_anisotropy;
        }
    };

    struct TextureBinding {
        uint binding_location;
        TextureHandle handle;
        SamplerInfo sampler_info;

        bool operator==(const TextureBinding& other) const {
            return binding_location == other.binding_location && handle == other.handle &&
                   sampler_info == other.sampler_info;
        }
    };

    // Vertices and indices.
    std::optional<VertexBufferHandle> vb;
    uint vb_offset = 0;  // Offset in bytes.
    VertexDecl vertex_decl_override;
    std::optional<IndexBufferHandle> ib;  // Offset in bytes.
    uint ib_offset = 0;
    uint primitive_count = 0;

    // Shader program and parameters.
    std::optional<ProgramHandle> program;
    std::unordered_map<std::string, UniformData> uniforms;
    std::vector<TextureBinding> textures;

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

// Render queue.
struct RenderQueue {
    struct ClearParameters {
        Colour colour;
        bool clear_colour;
        bool clear_depth;
    };
    std::optional<ClearParameters> clear_parameters;
    std::optional<FrameBufferHandle> frame_buffer;
    std::vector<RenderItem> render_items;
};

// Frame.
class Renderer;
struct Frame {
    Frame();
    void clear();

    RenderItem pending_item;
    std::vector<RenderQueue> render_queues;

    std::vector<RenderCommand> commands_pre;
    std::vector<RenderCommand> commands_post;

    // Transient vertex/index buffer storage.
    struct {
        std::array<byte, DW_MAX_TRANSIENT_VERTEX_BUFFER_SIZE> data;
        uint size;
        std::optional<VertexBufferHandle> handle;
    } transient_vb_storage;

    struct {
        std::array<byte, DW_MAX_TRANSIENT_INDEX_BUFFER_SIZE> data;
        uint size;
        std::optional<IndexBufferHandle> handle;
    } transient_ib_storage;

    // Transient vertex/index buffer data.
    struct TransientVertexBufferData {
        byte* data;
        uint size;
        VertexDecl decl;
    };
    std::unordered_map<TransientVertexBufferHandle, TransientVertexBufferData>
        transient_vertex_buffers_;
    HandleGenerator<TransientVertexBufferHandle> transient_vertex_buffer_handle_generator_;
    struct TransientIndexBufferData {
        byte* data;
        uint size;
    };
    std::unordered_map<TransientIndexBufferHandle, TransientIndexBufferData>
        transient_index_buffers_;
    HandleGenerator<TransientIndexBufferHandle> transient_index_buffer_handle_generator_;

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
    Result<void, std::string> init(RendererType type, u16 width, u16 height,
                                   const std::string& title, InputCallbacks input_callbacks,
                                   bool use_render_thread);

    /// Adjusts a RH D3D projection matrix to be compatible with the underlying renderer type.
    /// Does nothing unless init() has been called.
    Mat4 adjustProjectionMatrix(Mat4 projection_matrix) const;

    /// Returns true if viewport coordinates are from the top-left (Vulkan) rather than bottom-left
    /// (OpenGL, D3D).
    bool hasFlippedViewport() const;

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
    std::optional<TransientVertexBufferHandle> allocTransientVertexBuffer(uint vertex_count,
                                                                          const VertexDecl& decl);
    byte* getTransientVertexBufferData(TransientVertexBufferHandle handle);
    void setVertexBuffer(TransientVertexBufferHandle handle);

    /// Transient index buffer.
    std::optional<TransientIndexBufferHandle> allocTransientIndexBuffer(uint index_count);
    byte* getTransientIndexBufferData(TransientIndexBufferHandle handle);
    void setIndexBuffer(TransientIndexBufferHandle handle);

    /// Create program.
    ProgramHandle createProgram(std::vector<ShaderStageInfo> stages);
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
    TextureHandle createTexture2D(u16 width, u16 height, TextureFormat format, Memory data,
                                  bool generate_mipmaps = true, bool framebuffer_usage = false);
    // get texture information.
    void deleteTexture(TextureHandle handle);
    // Binds a texture to a binding location defined in the current shader program.
    bool setTexture(uint binding_location, TextureHandle handle,
                    u32 sampler_flags = SamplerFlag::Default, float max_anisotropy = 0.0f);

    // Framebuffer.
    FrameBufferHandle createFrameBuffer(u16 width, u16 height, TextureFormat format);
    FrameBufferHandle createFrameBuffer(std::vector<TextureHandle> textures);
    TextureHandle getFrameBufferTexture(FrameBufferHandle handle, uint index);
    void deleteFrameBuffer(FrameBufferHandle handle);

    /// Creates a new render queue that outputs to a specific frame buffer.
    /// @param frame_buffer Framebuffer to output to. To output to the backbuffer, set this to
    /// std::nullopt.
    /// @return The render queue ID which can be used in submit calls.
    uint startRenderQueue(std::optional<FrameBufferHandle> frame_buffer = std::nullopt);

    /// Returns the last created render queue.
    uint lastCreatedRenderQueue() const;

    /// Causes the last created render queue to clear the framebuffer to a specific colour when
    /// processed.
    void setRenderQueueClear(const Colour& colour, bool clear_colour = true,
                             bool clear_depth = true);

    /// Causes the current render queue to clear the framebuffer to a specific colour when
    /// processed.
    void setRenderQueueClear(uint render_queue, const Colour& colour, bool clear_colour = true,
                             bool clear_depth = true);

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

    /// Update uniform and draw state, but submit no geometry. Submits to the last created render
    /// queue.
    void submit(ProgramHandle program);

    /// Update uniform and draw state, but submit no geometry.
    void submit(uint render_queue, ProgramHandle program);

    /// Update uniform and draw state, then draw. Submits to the last created render queue.
    /// Offset is in vertices/indices depending on whether an index buffer is being used.
    void submit(ProgramHandle program, uint vertex_count, uint offset = 0);

    /// Update uniform and draw state, then draw.
    /// Offset is in vertices/indices depending on whether an index buffer is being used.
    void submit(uint render_queue, ProgramHandle program, uint vertex_count, uint offset = 0);

    /// Update uniform and draw state, then draws a full screen quad. Submits to the last created
    /// render queue.
    void submitFullscreenQuad(ProgramHandle program);

    /// Update uniform and draw state, then draws a full screen quad.
    void submitFullscreenQuad(uint render_queue, ProgramHandle program);

    /// Render a single frame.
    bool frame();

    /// Get the current window size.
    Vec2i windowSize() const;

    /// Get the current window scale.
    Vec2 windowScale() const;

    /// Get the current backbuffer size.
    Vec2i backbufferSize() const;

    /// Get the currently active renderer.
    RendererType rendererType() const;

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
    struct VertexBufferInfo {
        VertexDecl decl;
        BufferUsage usage;
    };
    std::unordered_map<VertexBufferHandle, VertexBufferInfo> vertex_buffer_info_;
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

    // Fullscreen quad.
    VertexBufferHandle fullscreen_quad_vb_;

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
}  // namespace gfx
}  // namespace dw

namespace std {
template <> struct hash<dw::gfx::RenderItem::SamplerInfo> {
    std::size_t operator()(const dw::gfx::RenderItem::SamplerInfo& i) const {
        std::size_t hash = 0;
        dga::hashCombine(hash, i.sampler_flags, i.max_anisotropy);
        return hash;
    }
};
}  // namespace std
