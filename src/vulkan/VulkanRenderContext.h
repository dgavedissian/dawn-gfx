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
class VulkanRenderContext : public RenderContext {
public:
    explicit VulkanRenderContext(Logger& logger);
    ~VulkanRenderContext() override = default;

    // Window management. Executed on the main thread.
    tl::expected<void, std::string> createWindow(u16 width, u16 height, const std::string& title,
                                                 InputCallbacks input_callbacks) override;
    void destroyWindow() override;
    void processEvents() override;
    bool isWindowClosed() const override;
    Vec2i windowSize() const override;
    Vec2 windowScale() const override;
    Vec2i backbufferSize() const override;

    // Command buffer processing. Executed on the render thread.
    void startRendering() override;
    void stopRendering() override;
    void processCommandList(std::vector<RenderCommand>& command_list) override;
    bool frame(const Frame* frame) override;

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

    vk::SwapchainKHR swap_chain_;
    vk::Format swap_chain_image_format_;
    vk::Extent2D swap_chain_extent_;
    std::vector<vk::Image> swap_chain_images_;
    std::vector<vk::ImageView> swap_chain_image_views_;

    vk::PipelineLayout pipeline_layout_;

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions(bool enable_validation_layers);

    void createInstance(bool enable_validation_layers);
    void createDevice();
    void createSwapChain();
    void createImageViews();
    void createGraphicsPipeline();

    void cleanup();
};
}  // namespace gfx
}  // namespace dw
