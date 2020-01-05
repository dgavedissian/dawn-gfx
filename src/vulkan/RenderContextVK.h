/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Renderer.h"
#include "RenderContext.h"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace dw {
namespace gfx {
struct VertexBufferVK {
    vk::VertexInputBindingDescription binding_description;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
    vk::Buffer buffer;
    vk::DeviceMemory buffer_memory;

    void initVertexInputDescriptions(const VertexDecl& decl);

    static vk::Format getVertexAttributeFormat(VertexDecl::AttributeType type, usize count,
                                               bool normalised);
};

struct IndexBufferVK {
    vk::IndexType type;
    vk::Buffer buffer;
    vk::DeviceMemory buffer_memory;
};

struct ShaderVK {
    vk::ShaderModule module;
    ShaderStage stage;
    std::string entry_point;
};

struct ProgramVK {
    std::vector<vk::PipelineShaderStageCreateInfo> pipeline_stages;
};

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
    vk::PhysicalDevice physical_device_;
    vk::Device device_;

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
    vk::PipelineLayout pipeline_layout_;
    vk::Pipeline graphics_pipeline_;

    std::vector<vk::Framebuffer> swap_chain_framebuffers_;

    vk::CommandPool command_pool_;
    std::vector<vk::CommandBuffer> command_buffers_;

    std::vector<vk::Semaphore> image_available_semaphores_;
    std::vector<vk::Semaphore> render_finished_semaphores_;
    std::vector<vk::Fence> in_flight_fences_;
    std::vector<vk::Fence> images_in_flight_;
    std::size_t current_frame_;

    // Resource maps.
    std::unordered_map<VertexBufferHandle, VertexBufferVK> vertex_buffer_map_;
    std::unordered_map<IndexBufferHandle, IndexBufferVK> index_buffer_map_;
    // Note: Pointers to ShaderVK objects should be stable.
    std::unordered_map<ShaderHandle, ShaderVK> shader_map_;
    std::unordered_map<ProgramHandle, ProgramVK> program_map_;

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions(bool enable_validation_layers);

    void createInstance(bool enable_validation_layers);
    void createDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    vk::Pipeline createGraphicsPipeline(const RenderItem& render_item);
    void createFramebuffers();
    void createCommandBuffers();
    void createSyncObjects();

    u32 findMemoryType(u32 type_filter, vk::MemoryPropertyFlags properties);
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags properties, vk::Buffer& buffer,
                      vk::DeviceMemory& buffer_memory);
    void copyBuffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size);

    void cleanup();
};
}  // namespace gfx
}  // namespace dw
