/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "vulkan/VulkanRenderContext.h"
#include <cstring>
#include <set>
#include <cstdint>

#include "dawn-gfx/Shader.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace dw {
namespace gfx {
namespace {
const std::vector<const char*> kValidationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> kRequiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
constexpr auto kMaxFramesInFlight = 2;

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                     VkDebugUtilsMessageTypeFlagsEXT message_types,
                     const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
    const auto& logger = *static_cast<const Logger*>(user_data);
    std::string message_str{callback_data->pMessage};
    logger.debug("Vulkan validation layer: type = {}, severity = {}, message = {}",
                 vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(message_types)),
                 vk::to_string(vk::DebugUtilsMessageSeverityFlagsEXT(message_severity)),
                 callback_data->pMessage);
    return VK_FALSE;
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    static QueueFamilyIndices fromPhysicalDevice(vk::PhysicalDevice device,
                                                 vk::SurfaceKHR surface) {
        QueueFamilyIndices indices;
        std::vector<vk::QueueFamilyProperties> queue_families = device.getQueueFamilyProperties();
        int i = 0;
        for (const auto& queue_family : queue_families) {
            if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphics_family = i;
            }

            if (device.getSurfaceSupportKHR(i, surface)) {
                indices.present_family = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    bool isComplete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;

    static SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device,
                                                         vk::SurfaceKHR surface) {
        SwapChainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats = device.getSurfaceFormatsKHR(surface);
        details.present_modes = device.getSurfacePresentModesKHR(surface);
        return details;
    }

    vk::SurfaceFormatKHR chooseSurfaceFormat() const {
        for (const auto& available_format : formats) {
            if (available_format.format == vk::Format::eB8G8R8A8Unorm &&
                available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return available_format;
            }
        }
        return formats[0];
    }

    vk::PresentModeKHR choosePresentMode() const {
        // TODO: Configure this.
        for (const auto& available_present_mode : present_modes) {
            if (available_present_mode == vk::PresentModeKHR::eMailbox) {
                return available_present_mode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(Vec2i window_size) const {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            vk::Extent2D actualExtent{static_cast<u32>(window_size.x),
                                      static_cast<u32>(window_size.y)};
            actualExtent.width =
                std::max(capabilities.minImageExtent.width,
                         std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height =
                std::max(capabilities.minImageExtent.height,
                         std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }
};
}  // namespace

VulkanRenderContext::VulkanRenderContext(Logger& logger)
    : RenderContext{logger}, current_frame_(0) {
}

tl::expected<void, std::string> VulkanRenderContext::createWindow(u16 width, u16 height,
                                                                  const std::string& title,
                                                                  InputCallbacks input_callbacks) {
    glfwInit();

    // Select monitor.
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();

    // Get DPI settings.
#ifndef DGA_EMSCRIPTEN
    glfwGetMonitorContentScale(monitor, &window_scale_.x, &window_scale_.y);
#else
    // Unsupported in emscripten.
    window_scale_ = {1.0f, 1.0f};
#endif

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // TODO: Support resizing.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(static_cast<int>(width * window_scale_.x),
                               static_cast<int>(height * window_scale_.y), title.c_str(), nullptr,
                               nullptr);

    createInstance(true);
    createDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
    createSyncObjects();

    return {};
}

void VulkanRenderContext::destroyWindow() {
    if (window_) {
        cleanup();

        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }
}

void VulkanRenderContext::processEvents() {
    glfwPollEvents();
}

bool VulkanRenderContext::isWindowClosed() const {
    return glfwWindowShouldClose(window_) != 0;
}

Vec2i VulkanRenderContext::windowSize() const {
    int window_width, window_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    return Vec2i{window_width, window_height};
}

Vec2 VulkanRenderContext::windowScale() const {
    return {1.0f, 1.0f};
}

Vec2i VulkanRenderContext::backbufferSize() const {
    return {0, 0};
}

void VulkanRenderContext::startRendering() {
}

void VulkanRenderContext::stopRendering() {
}

void VulkanRenderContext::processCommandList(std::vector<RenderCommand>&) {
}

bool VulkanRenderContext::frame(const Frame*) {
    drawFrame();
    return true;
}

bool VulkanRenderContext::checkValidationLayerSupport() {
    auto layer_properties_list = vk::enumerateInstanceLayerProperties();
    for (const char* layer_name : kValidationLayers) {
        bool layer_found = false;
        for (const vk::LayerProperties& layer_properties : layer_properties_list) {
            if (std::strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        if (!layer_found) {
            return false;
        }
    }
    return true;
}

std::vector<const char*> VulkanRenderContext::getRequiredExtensions(bool enable_validation_layers) {
    // Get required GLFW extensions.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // Add the debug utils extension if validation layers are enabled.
    if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanRenderContext::createInstance(bool enable_validation_layers) {
    vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    if (enable_validation_layers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Vulkan validation layers requested, but not available.");
    }

    auto all_extensions = vk::enumerateInstanceExtensionProperties();
    std::ostringstream extension_list;
    extension_list << "Vulkan extensions supported:";
    for (const auto& extension : all_extensions) {
        extension_list << " " << extension.extensionName;
    }
    extension_list << std::endl;
    logger_.info(extension_list.str());

    vk::ApplicationInfo app_info;
    app_info.pApplicationName = "VulkanRenderContext";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "dawn-gfx";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo create_info;
    create_info.pApplicationInfo = &app_info;

    // Enable required extensions.
    auto extensions = getRequiredExtensions(enable_validation_layers);
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    // Enable validation layers if requested.
    if (enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<u32>(kValidationLayers.size());
        create_info.ppEnabledLayerNames = kValidationLayers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    // Create instance. Will throw an std::runtime_error on failure.
    instance_ = vk::createInstance(create_info);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_);

    if (enable_validation_layers) {
        vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_info;
        debug_messenger_info
            .messageSeverity = /*vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |*/
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        debug_messenger_info.messageType = /*vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |*/
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        debug_messenger_info.pfnUserCallback = DebugMessageCallback;
        debug_messenger_info.pUserData = &logger_;
        debug_messenger_ = instance_.createDebugUtilsMessengerEXT(debug_messenger_info);
    }

    // Create surface.
    VkSurfaceKHR c_surface;
    if (glfwCreateWindowSurface(static_cast<VkInstance>(instance_), window_, nullptr, &c_surface) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    surface_ = c_surface;
}

void VulkanRenderContext::createDevice() {
    // Pick a physical device.
    auto is_device_suitable = [this](vk::PhysicalDevice device) -> bool {
        auto indices = QueueFamilyIndices::fromPhysicalDevice(device, surface_);
        if (!indices.isComplete()) {
            return false;
        }

        // Check for required extensions.
        std::vector<vk::ExtensionProperties> device_extensions =
            device.enumerateDeviceExtensionProperties();
        std::set<std::string> missing_extensions(kRequiredDeviceExtensions.begin(),
                                                 kRequiredDeviceExtensions.end());
        for (const auto& extension : device_extensions) {
            missing_extensions.erase(extension.extensionName);
        }
        if (!missing_extensions.empty()) {
            return false;
        }

        // Check that the swap chain is adequate.
        SwapChainSupportDetails swap_chain_support =
            SwapChainSupportDetails::querySwapChainSupport(device, surface_);
        if (swap_chain_support.formats.empty() || swap_chain_support.present_modes.empty()) {
            return false;
        }

        return true;
    };

    std::vector<vk::PhysicalDevice> physical_devices = instance_.enumeratePhysicalDevices();
    for (const auto& device : physical_devices) {
        if (is_device_suitable(device)) {
            physical_device_ = device;
            break;
        }
    }
    if (!physical_device_) {
        throw std::runtime_error("failed to find a suitable GPU.");
    }
    auto indices = QueueFamilyIndices::fromPhysicalDevice(physical_device_, surface_);

    // Create a logical device.
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queues_families = {indices.graphics_family.value(),
                                                 indices.present_family.value()};
    float queue_priority = 1.0f;
    for (uint32_t queue_family : unique_queues_families) {
        vk::DeviceQueueCreateInfo queue_create_info;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    vk::PhysicalDeviceFeatures device_features;

    vk::DeviceCreateInfo create_info;
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.queueCreateInfoCount = static_cast<u32>(queue_create_infos.size());
    create_info.pEnabledFeatures = &device_features;
    // No device specific extensions.c
    create_info.enabledExtensionCount = static_cast<uint32_t>(kRequiredDeviceExtensions.size());
    create_info.ppEnabledExtensionNames = kRequiredDeviceExtensions.data();
    // Technically unneeded, but worth doing anyway for old vulkan implementations.
    if (debug_messenger_) {
        create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
        create_info.ppEnabledLayerNames = kValidationLayers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }
    device_ = physical_device_.createDevice(create_info);

    // Get queue handles.
    graphics_queue_ = device_.getQueue(indices.graphics_family.value(), 0);
    present_queue_ = device_.getQueue(indices.present_family.value(), 0);
}

void VulkanRenderContext::createSwapChain() {
    SwapChainSupportDetails swap_chain_support =
        SwapChainSupportDetails::querySwapChainSupport(physical_device_, surface_);
    vk::SurfaceFormatKHR surface_format = swap_chain_support.chooseSurfaceFormat();
    vk::PresentModeKHR present_mode = swap_chain_support.choosePresentMode();
    vk::Extent2D extent = swap_chain_support.chooseSwapExtent(windowSize());

    std::uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR create_info;
    create_info.surface = surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    QueueFamilyIndices indices = QueueFamilyIndices::fromPhysicalDevice(physical_device_, surface_);
    uint32_t queueFamilyIndices[] = {indices.graphics_family.value(),
                                     indices.present_family.value()};
    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = vk::SwapchainKHR{};

    swap_chain_ = device_.createSwapchainKHR(create_info);
    swap_chain_images_ = device_.getSwapchainImagesKHR(swap_chain_);
    swap_chain_image_format_ = create_info.imageFormat;
    swap_chain_extent_ = create_info.imageExtent;
}

void VulkanRenderContext::createImageViews() {
    swap_chain_image_views_.reserve(swap_chain_images_.size());
    for (const auto& swap_chain_image : swap_chain_images_) {
        vk::ImageViewCreateInfo create_info;
        create_info.image = swap_chain_image;
        create_info.viewType = vk::ImageViewType::e2D;
        create_info.format = swap_chain_image_format_;
        create_info.components.r = vk::ComponentSwizzle::eIdentity;
        create_info.components.g = vk::ComponentSwizzle::eIdentity;
        create_info.components.b = vk::ComponentSwizzle::eIdentity;
        create_info.components.a = vk::ComponentSwizzle::eIdentity;
        create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        swap_chain_image_views_.push_back(device_.createImageView(create_info));
    }
}

void VulkanRenderContext::createRenderPass() {
    vk::AttachmentDescription colour_attachment;
    colour_attachment.format = swap_chain_image_format_;
    colour_attachment.samples = vk::SampleCountFlagBits::e1;
    colour_attachment.loadOp = vk::AttachmentLoadOp::eLoad;
    colour_attachment.storeOp = vk::AttachmentStoreOp::eStore;
    colour_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colour_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colour_attachment.initialLayout = vk::ImageLayout::eUndefined;
    colour_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colour_attachment_ref;
    colour_attachment_ref.attachment = 0;
    colour_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colour_attachment_ref;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo render_pass_info;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &colour_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    render_pass_ = device_.createRenderPass(render_pass_info);
}

void VulkanRenderContext::createGraphicsPipeline() {
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

    // Create shader modules
    vk::ShaderModuleCreateInfo vs_module_create_info;
    vs_module_create_info.pCode = reinterpret_cast<const u32*>(vertex_shader.spirv.data());
    vs_module_create_info.codeSize = vertex_shader.spirv.size() * 4;
    auto vs_module = device_.createShaderModule(vs_module_create_info);
    vk::ShaderModuleCreateInfo fs_module_create_info;
    fs_module_create_info.pCode = reinterpret_cast<const u32*>(fragment_shader.spirv.data());
    fs_module_create_info.codeSize = fragment_shader.spirv.size() * 4;
    auto fs_module = device_.createShaderModule(fs_module_create_info);

    // Create shader stages.
    vk::PipelineShaderStageCreateInfo vs_stage_info;
    vs_stage_info.stage = vk::ShaderStageFlagBits::eVertex;
    vs_stage_info.module = vs_module;
    vs_stage_info.pName = vertex_shader.entry_point.c_str();
    vk::PipelineShaderStageCreateInfo fs_stage_info;
    fs_stage_info.stage = vk::ShaderStageFlagBits::eFragment;
    fs_stage_info.module = fs_module;
    fs_stage_info.pName = fragment_shader.entry_point.c_str();
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vs_stage_info, fs_stage_info};

    // Create fixed function stages.
    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = nullptr;  // Optional
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr;  // Optional

    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swap_chain_extent_.width);
    viewport.height = static_cast<float>(swap_chain_extent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = swap_chain_extent_;

    vk::PipelineViewportStateCreateInfo viewport_state;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
    rasterizer.depthBiasClamp = 0.0f;           // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f;           // Optional
    multisampling.pSampleMask = nullptr;             // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    vk::PipelineColorBlendAttachmentState colour_blend_attachment;
    colour_blend_attachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colour_blend_attachment.blendEnable = VK_FALSE;
    colour_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eOne;   // Optional
    colour_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eZero;  // Optional
    colour_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;              // Optional
    colour_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;   // Optional
    colour_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;  // Optional
    colour_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;              // Optional

    /* Alpha blending:
        colour_blend_attachment.blendEnable = VK_TRUE;
        colour_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colour_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colour_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        colour_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colour_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colour_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
     */

    vk::PipelineColorBlendStateCreateInfo colour_blending;
    colour_blending.logicOpEnable = VK_FALSE;
    colour_blending.logicOp = vk::LogicOp::eCopy;  // Optional
    colour_blending.attachmentCount = 1;
    colour_blending.pAttachments = &colour_blend_attachment;
    colour_blending.blendConstants[0] = 0.0f;  // Optional
    colour_blending.blendConstants[1] = 0.0f;  // Optional
    colour_blending.blendConstants[2] = 0.0f;  // Optional
    colour_blending.blendConstants[3] = 0.0f;  // Optional

    vk::DynamicState dynamic_states[] = {vk::DynamicState::eViewport, vk::DynamicState::eLineWidth};

    vk::PipelineDynamicStateCreateInfo dynamic_state;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    vk::PipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.setLayoutCount = 0;             // Optional
    pipeline_layout_info.pSetLayouts = nullptr;          // Optional
    pipeline_layout_info.pushConstantRangeCount = 0;     // Optional
    pipeline_layout_info.pPushConstantRanges = nullptr;  // Optional
    pipeline_layout_ = device_.createPipelineLayout(pipeline_layout_info);

    vk::GraphicsPipelineCreateInfo pipeline_info;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shaderStages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = nullptr;  // Optional
    pipeline_info.pColorBlendState = &colour_blending;
    pipeline_info.pDynamicState = nullptr;  // Optional
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = vk::Pipeline{};  // Optional
    pipeline_info.basePipelineIndex = -1;               // Optional

    vk::ArrayProxy<const vk::GraphicsPipelineCreateInfo> pipeline_infos = pipeline_info;
    auto graphics_pipelines = device_.createGraphicsPipelines(vk::PipelineCache{}, pipeline_infos);
    graphics_pipeline_ = graphics_pipelines[0];
}

void VulkanRenderContext::createFramebuffers() {
    swap_chain_framebuffers_.reserve(swap_chain_image_views_.size());
    for (const auto& image_view : swap_chain_image_views_) {
        vk::ImageView attachments[] = {image_view};

        vk::FramebufferCreateInfo framebuffer_info;
        framebuffer_info.renderPass = render_pass_;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swap_chain_extent_.width;
        framebuffer_info.height = swap_chain_extent_.height;
        framebuffer_info.layers = 1;
        swap_chain_framebuffers_.push_back(device_.createFramebuffer(framebuffer_info));
    }
}

void VulkanRenderContext::createCommandBuffers() {
    QueueFamilyIndices queue_family_indices =
        QueueFamilyIndices::fromPhysicalDevice(physical_device_, surface_);

    vk::CommandPoolCreateInfo pool_info;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
    command_pool_ = device_.createCommandPool(pool_info);

    vk::CommandBufferAllocateInfo allocate_info;
    allocate_info.commandPool = command_pool_;
    allocate_info.level = vk::CommandBufferLevel::ePrimary;
    allocate_info.commandBufferCount = static_cast<u32>(swap_chain_framebuffers_.size());
    command_buffers_ = device_.allocateCommandBuffers(allocate_info);

    // Record the same commands to each command buffer (for each swap chain image), as we'll always
    // be drawing one thing.
    int i = 0;
    for (auto& command_buffer : command_buffers_) {
        vk::CommandBufferBeginInfo begin_info;
        begin_info.flags = {};
        command_buffer.begin(begin_info);

        vk::RenderPassBeginInfo render_pass_info;
        render_pass_info.renderPass = render_pass_;
        render_pass_info.framebuffer = swap_chain_framebuffers_[i];
        render_pass_info.renderArea.offset = vk::Offset2D{0, 0};
        render_pass_info.renderArea.extent = swap_chain_extent_;

        vk::ClearValue clearColor;
        clearColor.color = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clearColor;

        command_buffer.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline_);
        command_buffer.draw(3, 1, 0, 0);
        command_buffer.endRenderPass();
        command_buffer.end();

        i++;
    }
}

void VulkanRenderContext::createSyncObjects() {
    image_available_semaphores_.reserve(kMaxFramesInFlight);
    render_finished_semaphores_.reserve(kMaxFramesInFlight);
    in_flight_fences_.reserve(kMaxFramesInFlight);
    for (int i = 0; i < kMaxFramesInFlight; ++i) {
        image_available_semaphores_.push_back(device_.createSemaphore(vk::SemaphoreCreateInfo{}));
        render_finished_semaphores_.push_back(device_.createSemaphore(vk::SemaphoreCreateInfo{}));

        vk::FenceCreateInfo fence_create_info;
        fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;
        in_flight_fences_.push_back(device_.createFence(fence_create_info));
    }
}

void VulkanRenderContext::drawFrame() {
    device_.waitForFences(in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);
    device_.resetFences(in_flight_fences_[current_frame_]);

    u32 image_index;
    device_.acquireNextImageKHR(swap_chain_, UINT64_MAX,
                                image_available_semaphores_[current_frame_], vk::Fence{},
                                &image_index);

    // Submit command buffer.
    vk::SubmitInfo submit_info;
    vk::Semaphore wait_semaphores[] = {image_available_semaphores_[current_frame_]};
    vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers_[image_index];
    vk::Semaphore signal_semaphores[] = {render_finished_semaphores_[current_frame_]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    graphics_queue_.submit(submit_info, in_flight_fences_[current_frame_]);

    // Present.
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signal_semaphores;
    vk::SwapchainKHR swapChains[] = {swap_chain_};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &image_index;
    presentInfo.pResults = nullptr;  // Optional
    present_queue_.presentKHR(presentInfo);

    current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
}

void VulkanRenderContext::cleanup() {
    vkDeviceWaitIdle(device_);

    for (const auto& fence : in_flight_fences_) {
        device_.destroy(fence);
    }
    for (const auto& semaphore : render_finished_semaphores_) {
        device_.destroy(semaphore);
    }
    for (const auto& semaphore : image_available_semaphores_) {
        device_.destroy(semaphore);
    }

    device_.destroy(command_pool_);
    for (const auto& framebuffer : swap_chain_framebuffers_) {
        device_.destroy(framebuffer);
    }
    device_.destroy(graphics_pipeline_);
    device_.destroy(pipeline_layout_);
    device_.destroy(render_pass_);
    for (const auto& image_view : swap_chain_image_views_) {
        device_.destroy(image_view);
    }
    device_.destroy(swap_chain_);
    device_.destroy();
    instance_.destroy(surface_);
    if (debug_messenger_) {
        instance_.destroy(debug_messenger_);
    }
    instance_.destroy();
}
}  // namespace gfx
}  // namespace dw