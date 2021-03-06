/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Renderer.h"
#include "RenderContext.h"

#include <dga/hash_combine.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <map>

/*
 * TODOs:
 * - Move all the helper classes / structs into separate files.
 * - Refactor TextureVK into a real fully contained class that handles a texture resource properly.
 * Similar for other types like ShaderVK and ProgramVK.
 * - Use a real Vulkan memory allocator rather than directly allocating from the device.
 * - Revisit the way uniforms are handled to avoid all the heap allocating hash maps.
 * - Support recreation of the swapchain (when the window is resized etc).
 * - Refactor GLFW into a separate abstraction (that can be shared with RenderContextGL).
 */

namespace dw {
namespace gfx {
// Device wrapper.
class DeviceVK {
public:
    DeviceVK(vk::PhysicalDevice physical_device, vk::Device device, vk::CommandPool command_pool,
             vk::Queue graphics_queue);
    ~DeviceVK();

    vk::PhysicalDevice getPhysicalDevice() const;
    vk::Device getDevice() const;
    vk::CommandPool getCommandPool() const;

    u32 findMemoryType(u32 type_filter, vk::MemoryPropertyFlags properties);

    // Returns the allocation size of the buffe rmemory.
    vk::DeviceSize createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties, vk::Buffer& buffer,
                                vk::DeviceMemory& buffer_memory);
    void copyBuffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size);
    void copyBufferToImage(vk::Buffer buffer, vk::Image image, u32 width, u32 height);
    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout old_layout,
                               vk::ImageLayout new_layout);

    void createImage(u32 width, u32 height, vk::Format format, vk::ImageTiling tiling,
                     vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                     vk::Image& image, vk::DeviceMemory& image_memory);
    vk::ImageView createImageView(vk::Image image, vk::Format format,
                                  vk::ImageAspectFlags aspect_flags);

    vk::CommandBuffer beginSingleUseCommands();
    void endSingleUseCommands(vk::CommandBuffer command_buffer);

    const vk::PhysicalDeviceProperties& properties();

private:
    vk::PhysicalDevice physical_device_;
    vk::Device device_;
    vk::CommandPool command_pool_;
    vk::Queue graphics_queue_;

    vk::PhysicalDeviceProperties properties_;
};

// A buffer of data used by vertex and index buffers (and possibly user managed uniform buffers in
// the future).
struct BufferVK {
    DeviceVK* device;
    vk::DeviceSize size;
    BufferUsage usage;

    BufferVK(DeviceVK* device, const byte* data, vk::DeviceSize size, BufferUsage usage,
             vk::BufferUsageFlags buffer_type, usize swap_chain_size);
    ~BufferVK();

    BufferVK(BufferVK&& other) noexcept;
    BufferVK(const BufferVK&) = delete;
    BufferVK& operator=(BufferVK&& other) noexcept;
    BufferVK& operator=(const BufferVK&) = delete;

    vk::Buffer get(u32 frame_index) const;

    bool update(u32 frame_index, const byte* data, vk::DeviceSize data_size, vk::DeviceSize offset);

private:
    u32 getIndex(u32 frame_index) const;

    std::vector<vk::Buffer> buffer;
    std::vector<vk::DeviceMemory> buffer_memory;
};

struct VertexDeclVK {
    vk::VertexInputBindingDescription binding_description;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;

    VertexDeclVK(const VertexDecl& decl);

    static vk::Format getVertexAttributeFormat(VertexDecl::AttributeType type, usize count,
                                               bool normalised);
};

struct VertexBufferVK {
    VertexDecl decl;
    BufferVK buffer;
};

struct IndexBufferVK {
    vk::IndexType type;
    BufferVK buffer;
};

struct ShaderVK {
    vk::ShaderModule module;
    ShaderStage stage;
    // We use a raw pointer here, because vk::PipelineShaderStageCreateInfo needs a stable pointer.
    std::unique_ptr<char[]> entry_point;

    // Reflection data.
    struct StructLayout {
        struct StructField {
            std::string name;
            usize offset;
            usize size;
        };

        std::string name;
        usize size;
        std::vector<StructField> fields;
    };
    std::optional<StructLayout> push_constants;
    std::map<u32, StructLayout> uniform_buffer_bindings;
    std::map<u32, vk::DescriptorType> descriptor_type_bindings;
};

struct ProgramVK {
    std::unordered_map<vk::ShaderStageFlagBits, ShaderVK> stages;
    std::vector<vk::PipelineShaderStageCreateInfo> pipeline_stages;
    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
    vk::DescriptorSetLayout descriptor_set_layout;

    // Uniforms.
    struct Uniform {
        // Empty optional indicates a push_constant buffer.
        std::optional<usize> binding_location;
        usize offset = 0;
        usize size = 0;
        std::optional<UniformData> data;
    };
    std::unordered_map<std::string, Uniform> uniform_locations;

    // Uniform buffers.
    struct UniformBuffer {
        usize binding = 0;
        usize size = 0;
    };
    std::vector<UniformBuffer> uniform_buffers;
};

class UniformScratchBuffer {
public:
    struct Allocation {
        byte* ptr;
        usize offset_from_base;
    };

    UniformScratchBuffer(DeviceVK* device, usize size);
    ~UniformScratchBuffer();

    UniformScratchBuffer(const UniformScratchBuffer&) = delete;
    UniformScratchBuffer(UniformScratchBuffer&&) = delete;
    UniformScratchBuffer& operator=(const UniformScratchBuffer&) = delete;
    UniformScratchBuffer& operator=(UniformScratchBuffer&&) = delete;

    Allocation alloc(usize size);
    void reset();

    vk::Buffer getBuffer() const;

private:
    DeviceVK* device_;
    vk::Buffer buffer_;
    vk::DeviceMemory buffer_memory_;
    byte* data_;
    usize current_size_;
    usize maximum_size_;
};

struct TextureVK {
    vk::Image image;
    vk::DeviceMemory image_memory;
    vk::ImageView image_view;
    vk::Format image_format;
    vk::ImageLayout image_layout;
    vk::ImageAspectFlags aspect_mask;

    void setImageBarrier(vk::CommandBuffer command_buffer, vk::ImageLayout new_layout);
};

struct DescriptorSetVK {
    // One descriptor set per swap chain image.
    std::vector<vk::DescriptorSet> descriptor_sets;

    void destroy(vk::Device device, vk::DescriptorPool pool) {
        device.freeDescriptorSets(pool, descriptor_sets);
    }

    struct Info {
        // TODO: Instead pass specific <binding index / binding type> pairs, so we can reuse
        // descriptor sets for multiple programs of the same "shape".
        const ProgramVK* program;
        std::vector<RenderItem::TextureBinding> textures;

        bool operator==(const Info& other) const {
            return program == other.program && textures == other.textures;
        }
    };
};

struct FramebufferVK {
    vk::RenderPass render_pass;
    TextureVK depth;
    vk::Framebuffer framebuffer;
    std::vector<TextureVK*> images;
    vk::Extent2D extent;

    FramebufferVK(DeviceVK* device, u16 width, u16 height, std::vector<TextureVK*> attachments);
};

struct PipelineVK {
    vk::PipelineLayout layout;
    vk::Pipeline pipeline;

    struct Info {
        // TODO: Replace RenderItem ptr with a copy of the render state used by the pipeline, which
        // we can hash separately.
        const RenderItem* render_item;
        const VertexBufferVK* vb;
        const VertexDeclVK* decl;
        const ProgramVK* program;
        const FramebufferVK* framebuffer;

        bool operator==(const Info& other) const {
            return render_item->colour_write == other.render_item->colour_write &&
                   render_item->blend_enabled == other.render_item->blend_enabled &&
                   render_item->blend_src_rgb == other.render_item->blend_src_rgb &&
                   render_item->blend_dest_rgb == other.render_item->blend_dest_rgb &&
                   render_item->blend_equation_rgb == other.render_item->blend_equation_rgb &&
                   render_item->blend_src_a == other.render_item->blend_src_a &&
                   render_item->blend_dest_a == other.render_item->blend_dest_a &&
                   render_item->blend_equation_a == other.render_item->blend_equation_a &&
                   render_item->cull_face_enabled == other.render_item->cull_face_enabled &&
                   render_item->cull_front_face == other.render_item->cull_front_face &&
                   vb == other.vb && decl == other.decl && program == other.program &&
                   framebuffer == other.framebuffer;
        }
    };
};
}  // namespace gfx
}  // namespace dw

namespace std {
template <> struct hash<dw::gfx::PipelineVK::Info> {
    std::size_t operator()(const dw::gfx::PipelineVK::Info& i) const {
        std::size_t hash = 0;
        dga::hashCombine(hash, i.render_item->colour_write, i.render_item->blend_enabled,
                         i.render_item->blend_src_rgb, i.render_item->blend_dest_rgb,
                         i.render_item->blend_equation_rgb, i.render_item->blend_src_a,
                         i.render_item->blend_dest_a, i.render_item->blend_equation_a,
                         i.render_item->depth_enabled, i.render_item->depth_write,
                         i.render_item->cull_face_enabled, i.render_item->cull_front_face);
        dga::hashCombine(hash, i.vb, i.decl, i.program, i.framebuffer);
        return hash;
    }
};

template <> struct hash<dw::gfx::DescriptorSetVK::Info> {
    std::size_t operator()(const dw::gfx::DescriptorSetVK::Info& i) const {
        std::size_t hash = 0;
        dga::hashCombine(hash, i.program);
        for (const auto& texture : i.textures) {
            dga::hashCombine(hash, texture.handle, texture.sampler_info);
        }
        return hash;
    }
};
}  // namespace std

namespace dw {
namespace gfx {
class RenderContextVK : public RenderContext {
public:
    explicit RenderContextVK(Logger& logger);
    ~RenderContextVK() override;

    RendererType type() const override {
        return RendererType::Vulkan;
    }

    // Capabilities / customisations.
    Mat4 adjustProjectionMatrix(Mat4 projection_matrix) const override;
    bool hasFlippedViewport() const override;

    // Window management. Executed on the main thread.
    Result<void, std::string> createWindow(u16 width, u16 height, const std::string& title,
                                           InputCallbacks input_callbacks) override;
    void destroyWindow() override;
    void processEvents() override;
    bool isWindowClosed() const override;
    Vec2i windowSize() const override;
    Vec2 windowScale() const override;
    Vec2i framebufferSize() const override;

    // Command buffer processing. Executed on the render thread.
    void startRendering() override;
    void stopRendering() override;
    void prepareFrame() override;
    void processCommandList(std::vector<RenderCommand>& command_list) override;
    bool frame(const Frame* frame) override;

    // Variant walker methods. Executed on the render thread.
    void operator()(const cmd::CreateVertexBuffer& c);
    void operator()(const cmd::UpdateVertexBuffer& c);
    void operator()(const cmd::DeleteVertexBuffer& c);
    void operator()(const cmd::CreateIndexBuffer& c);
    void operator()(const cmd::UpdateIndexBuffer& c);
    void operator()(const cmd::DeleteIndexBuffer& c);
    void operator()(const cmd::CreateProgram& c);
    void operator()(const cmd::DeleteProgram& c);
    void operator()(const cmd::CreateTexture2D& c);
    void operator()(const cmd::DeleteTexture& c);
    void operator()(const cmd::CreateFrameBuffer& c);
    void operator()(const cmd::DeleteFrameBuffer& c);
    template <typename T> void operator()(const T& c) {
        static_assert(!std::is_same<T, T>::value, "Unimplemented RenderCommand");
    }

private:
    GLFWwindow* window_;
    Vec2 window_scale_;

    vk::Instance instance_;
    vk::DebugUtilsMessengerEXT debug_messenger_;
    vk::SurfaceKHR surface_;
    std::unique_ptr<DeviceVK> device_;
    vk::Device vk_device_;

    vk::Queue graphics_queue_;
    vk::Queue present_queue_;
    u32 graphics_queue_family_index_;
    u32 present_queue_family_index_;

    // Swapchain
    // =========

    vk::SwapchainKHR swap_chain_;
    vk::Format swap_chain_image_format_;
    vk::Extent2D swap_chain_extent_;
    std::vector<vk::Image> swap_chain_images_;
    std::vector<vk::ImageView> swap_chain_image_views_;

    vk::Format depth_format_;
    vk::Image depth_image_;
    vk::DeviceMemory depth_image_memory_;
    vk::ImageView depth_image_view_;

    std::vector<vk::Framebuffer> swap_chain_framebuffers_;
    vk::RenderPass swapchain_render_pass_;

    std::vector<vk::CommandBuffer> command_buffers_;

    // Concurrency primitives for frame tracking.
    std::vector<vk::Semaphore> image_available_semaphores_;
    std::vector<vk::Semaphore> render_finished_semaphores_;
    std::vector<vk::Fence> in_flight_fences_;
    std::vector<vk::Fence> images_in_flight_;
    std::size_t current_frame_;
    u32 next_frame_index_;

    // Resources
    // =========

    vk::DescriptorPool descriptor_pool_;

    // Per frame uniform scratch buffers (one per swapchain image).
    std::vector<std::unique_ptr<UniformScratchBuffer>> uniform_scratch_buffers_;

    // Resource maps.
    std::unordered_map<VertexBufferHandle, VertexBufferVK> vertex_buffer_map_;
    std::unordered_map<IndexBufferHandle, IndexBufferVK> index_buffer_map_;
    std::unordered_map<ProgramHandle, ProgramVK> program_map_;
    std::unordered_map<TextureHandle, TextureVK> texture_map_;
    std::unordered_map<FrameBufferHandle, FramebufferVK> framebuffer_map_;

    // Cached objects.
    // TODO: Implement some form of cache eviction.
    std::unordered_map<VertexDecl, VertexDeclVK> vertex_decl_cache_;
    std::unordered_map<PipelineVK::Info, PipelineVK> graphics_pipeline_cache_;
    std::unordered_map<DescriptorSetVK::Info, DescriptorSetVK> descriptor_set_cache_;
    std::unordered_map<RenderItem::SamplerInfo, vk::Sampler> sampler_cache_;

    // Helper functions
    // ================

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions(bool enable_validation_layers);

    void createInstance(bool enable_validation_layers);
    void createDevice();
    void createSwapChain();
    void createRenderPass();
    void createFramebuffers();
    void createCommandBuffers();
    void createDescriptorPool();
    void createSyncObjects();

    PipelineVK findOrCreateGraphicsPipeline(PipelineVK::Info info);
    DescriptorSetVK findOrCreateDescriptorSet(DescriptorSetVK::Info info);
    vk::Sampler findOrCreateSampler(RenderItem::SamplerInfo info);

    void cleanup();
};
}  // namespace gfx
}  // namespace dw
