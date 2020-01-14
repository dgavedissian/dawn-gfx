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

    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags properties, vk::Buffer& buffer,
                      vk::DeviceMemory& buffer_memory);
    void copyBuffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size);
    void copyBufferToImage(vk::Buffer buffer, vk::Image image, u32 width, u32 height);
    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout old_layout,
                               vk::ImageLayout new_layout);
    vk::ImageView createImageView(vk::Image image, vk::Format format);

    vk::CommandBuffer beginSingleUseCommands();
    void endSingleUseCommands(vk::CommandBuffer command_buffer);

private:
    vk::PhysicalDevice physical_device_;
    vk::Device device_;
    vk::CommandPool command_pool_;
    vk::Queue graphics_queue_;
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
    std::string entry_point;

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
    struct UniformLocation {
        // Empty optional indicates a push_constant buffer.
        std::optional<u32> ubo_index;
        usize offset = 0;
        usize size = 0;
    };
    std::unordered_map<std::string, UniformLocation> uniform_locations;

    // Uniform buffers.
    struct AutoUniformBuffer {
        std::vector<vk::Buffer> buffers;
        std::vector<vk::DeviceMemory> buffers_memory;
        usize size = 0;
    };
    std::vector<AutoUniformBuffer> uniform_buffers;
};

struct TextureVK {
    vk::Image image;
    vk::DeviceMemory image_memory;
    vk::ImageView image_view;
};

struct PipelineVK {
    vk::PipelineLayout layout;
    vk::Pipeline pipeline;

    struct Info {
        const RenderItem* render_item;
        const VertexBufferVK* vb;
        const VertexDeclVK* decl;
        const ProgramVK* program;

        bool operator==(const Info& other) const {
            return render_item->colour_write == other.render_item->colour_write && vb == other.vb &&
                   decl == other.decl && program == other.program;
        }
    };
};

struct DescriptorSetVK {
    // One descriptor set per swap chain image.
    std::vector<vk::DescriptorSet> descriptor_sets;

    void destroy(vk::Device device, vk::DescriptorPool pool) {
        device.freeDescriptorSets(pool, descriptor_sets);
    }

    struct Info {
        const ProgramVK* program;
        std::vector<RenderItem::TextureBinding> textures;

        bool operator==(const Info& other) const {
            return program == other.program && textures == other.textures;
        }
    };
};
}  // namespace gfx
}  // namespace dw

namespace std {
template <> struct hash<dw::gfx::PipelineVK::Info> {
    std::size_t operator()(const dw::gfx::PipelineVK::Info& i) const {
        std::size_t hash = 0;
        dga::hashCombine(hash, i.render_item->colour_write, i.vb, i.decl, i.program);
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
    ~RenderContextVK() override = default;

    // Window management. Executed on the main thread.
    tl::expected<void, std::string> createWindow(u16 width, u16 height, const std::string& title,
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

    vk::SwapchainKHR swap_chain_;
    vk::Format swap_chain_image_format_;
    vk::Extent2D swap_chain_extent_;
    std::vector<vk::Image> swap_chain_images_;
    std::vector<vk::ImageView> swap_chain_image_views_;

    vk::RenderPass render_pass_;

    std::vector<vk::Framebuffer> swap_chain_framebuffers_;

    std::vector<vk::CommandBuffer> command_buffers_;

    vk::DescriptorPool descriptor_pool_;

    std::vector<vk::Semaphore> image_available_semaphores_;
    std::vector<vk::Semaphore> render_finished_semaphores_;
    std::vector<vk::Fence> in_flight_fences_;
    std::vector<vk::Fence> images_in_flight_;
    std::size_t current_frame_;
    u32 next_frame_index_;

    // Resource maps.
    std::unordered_map<VertexBufferHandle, VertexBufferVK> vertex_buffer_map_;
    std::unordered_map<IndexBufferHandle, IndexBufferVK> index_buffer_map_;
    // Note: Pointers to ShaderVK objects should be stable.
    std::unordered_map<ShaderHandle, ShaderVK> shader_map_;
    std::unordered_map<ProgramHandle, ProgramVK> program_map_;
    std::unordered_map<TextureHandle, TextureVK> texture_map_;

    // Cached objects.
    std::unordered_map<VertexDecl, VertexDeclVK> vertex_decl_cache_;
    std::unordered_map<PipelineVK::Info, PipelineVK> graphics_pipeline_cache_;
    std::unordered_map<DescriptorSetVK::Info, DescriptorSetVK> descriptor_set_cache_;
    std::unordered_map<RenderItem::SamplerInfo, vk::Sampler> sampler_cache_;

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions(bool enable_validation_layers);

    void createInstance(bool enable_validation_layers);
    void createDevice();
    void createSwapChain();
    void createImageViews();
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
