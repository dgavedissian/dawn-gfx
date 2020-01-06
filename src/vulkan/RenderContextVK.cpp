/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "vulkan/RenderContextVK.h"
#include <cstring>
#include <set>
#include <cstdint>

#include "dawn-gfx/Shader.h"

#include <spirv_cross.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace dw {
namespace gfx {
namespace {
const std::vector<const char*> kValidationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> kRequiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
constexpr auto kMaxFramesInFlight = 2;

VKAPI_ATTR VkBool32 VKAPI_CALL
debugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
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
    std::optional<u32> graphics_family;
    std::optional<u32> present_family;

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

    static SwapChainSupportDetails querySupport(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
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

struct VariantSpan {
    const byte* data;
    usize size;
};

struct VariantToBytesHelper {
    template <typename T> VariantSpan operator()(const T& data) {
        return VariantSpan{reinterpret_cast<const byte*>(&data), sizeof(T)};
    }
};
}  // namespace

void VertexBufferVK::initVertexInputDescriptions(const VertexDecl& decl) {
    binding_description.binding = 0;
    binding_description.stride = decl.stride();
    binding_description.inputRate = vk::VertexInputRate::eVertex;

    // Create vertex attribute description from VertexDecl.
    attribute_descriptions.reserve(decl.attributes_.size());
    for (usize i = 0; i < decl.attributes_.size(); ++i) {
        const auto& attrib = decl.attributes_[i];

        // Decode attribute.
        VertexDecl::Attribute attribute;
        usize count;
        VertexDecl::AttributeType type;
        bool normalised;
        VertexDecl::decodeAttributes(attrib.first, attribute, count, type, normalised);

        // Setup attribute description
        vk::VertexInputAttributeDescription attribute_description;
        attribute_description.binding = 0;
        attribute_description.location = i;
        attribute_description.format = getVertexAttributeFormat(type, count, normalised);
        attribute_description.offset =
            static_cast<u32>(reinterpret_cast<std::uintptr_t>(attrib.second));
        attribute_descriptions.push_back(attribute_description);
    }
}

vk::Format VertexBufferVK::getVertexAttributeFormat(VertexDecl::AttributeType type, usize count,
                                                    bool normalised) {
    switch (type) {
        case VertexDecl::AttributeType::Float:
            switch (count) {
                case 1:
                    return vk::Format::eR32Sfloat;
                case 2:
                    return vk::Format::eR32G32Sfloat;
                case 3:
                    return vk::Format::eR32G32B32Sfloat;
                case 4:
                    return vk::Format::eR32G32B32A32Sfloat;
                default:
                    break;
            }
            break;
        case VertexDecl::AttributeType::Uint8:
            switch (count) {
                case 1:
                    return normalised ? vk::Format::eR8Unorm : vk::Format::eR8Uint;
                case 2:
                    return normalised ? vk::Format::eR8G8Unorm : vk::Format::eR8G8Uint;
                case 3:
                    return normalised ? vk::Format::eR8G8B8Unorm : vk::Format::eR8G8B8Uint;
                case 4:
                    return normalised ? vk::Format::eR8G8B8A8Unorm : vk::Format::eR8G8B8A8Uint;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    throw std::runtime_error(
        fmt::format("Unknown vertex attribute type {} with {} elements (normalised: {})",
                    static_cast<int>(type), count, normalised));
}

RenderContextVK::RenderContextVK(Logger& logger) : RenderContext{logger}, current_frame_(0) {
}

tl::expected<void, std::string> RenderContextVK::createWindow(u16 width, u16 height,
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

#ifdef NDEBUG
    createInstance(false);
#else
    createInstance(true);
#endif

    createDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createFramebuffers();
    createCommandBuffers();
    createDescriptorPool();
    createSyncObjects();

    return {};
}

void RenderContextVK::destroyWindow() {
    if (window_) {
        cleanup();

        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }
}

void RenderContextVK::processEvents() {
    glfwPollEvents();
}

bool RenderContextVK::isWindowClosed() const {
    return glfwWindowShouldClose(window_) != 0;
}

Vec2i RenderContextVK::windowSize() const {
    int window_width, window_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    return Vec2i{window_width, window_height};
}

Vec2 RenderContextVK::windowScale() const {
    return {1.0f, 1.0f};
}

Vec2i RenderContextVK::framebufferSize() const {
    int window_width, window_height;
    glfwGetFramebufferSize(window_, &window_width, &window_height);
    return Vec2i{window_width, window_height};
}

void RenderContextVK::startRendering() {
}

void RenderContextVK::stopRendering() {
}

void RenderContextVK::processCommandList(std::vector<RenderCommand>& command_list) {
    assert(window_);
    for (auto& command : command_list) {
        visit(*this, command);
    }
}

bool RenderContextVK::frame(const Frame* frame) {
    device_.waitForFences(in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);

    // Acquire next image.
    u32 next_index;
    device_.acquireNextImageKHR(swap_chain_, UINT64_MAX,
                                image_available_semaphores_[current_frame_], vk::Fence{},
                                &next_index);

    // Check if a previous frame is using this image (i.e. there is a fence to wait on).
    if (images_in_flight_[next_index]) {
        device_.waitForFences(images_in_flight_[next_index], VK_TRUE, UINT64_MAX);
    }
    // Mark this image as now being in use by this frame.
    images_in_flight_[next_index] = in_flight_fences_[current_frame_];

    auto command_buffer = command_buffers_[next_index];

    // Write render queues to command buffer.
    vk::CommandBufferBeginInfo begin_info;
    begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    command_buffer.begin(begin_info);

    for (const auto& q : frame->render_queues) {
        vk::Framebuffer target_framebuffer;
        if (q.frame_buffer) {
            throw std::runtime_error("unimplemented");
        } else {
            // Bind to backbuffer.
            target_framebuffer = swap_chain_framebuffers_[next_index];
        }

        // Begin render pass.
        vk::RenderPassBeginInfo render_pass_info;
        render_pass_info.renderPass = render_pass_;
        render_pass_info.framebuffer = target_framebuffer;
        render_pass_info.renderArea.offset = vk::Offset2D{0, 0};
        render_pass_info.renderArea.extent = swap_chain_extent_;

        // If this render queue has clear parameters, start a new render pass.
        if (q.clear_parameters.has_value()) {
            // Note: using an array here to avoid the dynamic allocation overhead of std::vector.
            vk::ClearValue clear_values[2];
            int clear_value_count = 0;

            if (q.clear_parameters->clear_colour) {
                const auto& colour = q.clear_parameters.value().colour;
                clear_values[clear_value_count++].color = {
                    std::array<float, 4>{colour.r(), colour.g(), colour.b(), colour.a()}};
            }

            if (q.clear_parameters->clear_depth) {
                clear_values[clear_value_count++].depthStencil = vk::ClearDepthStencilValue{};
            }

            render_pass_info.clearValueCount = clear_value_count;
            render_pass_info.pClearValues = clear_values;
        }

        command_buffer.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
        for (const auto& ri : q.render_items) {
            const auto& program = program_map_.at(*ri.program);

            // Update uniforms.
            for (const auto& shader_stage : program.stages) {
                std::vector<byte*> ubo_data;
                std::vector<bool> ubo_updated(shader_stage.second.uniform_buffers.size(), false);
                ubo_data.reserve(shader_stage.second.uniform_buffers.size());
                for (const auto& ubo : shader_stage.second.uniform_buffers) {
                    void* mapped_memory = device_.mapMemory(ubo.buffers_memory[0], 0, ubo.size);
                    ubo_data.push_back(reinterpret_cast<byte*>(mapped_memory));
                }
                for (const auto& uniform_pair : ri.uniforms) {
                    // Find uniform binding
                    auto uniform_it =
                        shader_stage.second.uniform_locations.find(uniform_pair.first);
                    if (uniform_it == shader_stage.second.uniform_locations.end()) {
                        continue;
                    }
                    auto& uniform_location = uniform_it->second;
                    if (!uniform_location.ubo_location.has_value()) {
                        logger_.warn("Push constants not implemented yet.");
                        continue;
                    }

                    // Write to memory.
                    auto variant_bytes = std::visit(VariantToBytesHelper{}, uniform_pair.second);
                    byte* data_dst =
                        ubo_data[*uniform_location.ubo_location] + uniform_location.offset;
                    ubo_updated[*uniform_location.ubo_location] = true;
                    std::memcpy(data_dst, variant_bytes.data, variant_bytes.size);

                    /*
                    logger_.info("Writing {} (size {}) to offset {} in UBO {}", uniform_pair.first,
                                 variant_bytes.size, uniform_location.offset,
                                 *uniform_location.ubo_location);
                    */
                }
                for (usize i = 0; i < shader_stage.second.uniform_buffers.size(); ++i) {
                    const auto& ubo = shader_stage.second.uniform_buffers[i];

                    device_.unmapMemory(ubo.buffers_memory[0]);

                    // Copy buffer to the "real" buffer.
                    // TODO: Implement dirty flags.
                    copyBuffer(ubo.buffers[0], ubo.buffers[1 + next_index], ubo.size);
                }
            }

            // If there are no vertices to render, we are done.
            if (!ri.vb) {
                continue;
            }

            const auto& vb = vertex_buffer_map_.at(*ri.vb);

            // Bind (and create) graphics pipeline.
            auto graphics_pipeline = createGraphicsPipeline(ri, vb, program);
            command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                        graphics_pipeline.pipeline);

            // Bind descriptor set.
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                              graphics_pipeline.layout, 0,
                                              program.descriptor_sets[next_index], nullptr);

            // Bind vertex/index buffers and draw.
            command_buffer.bindVertexBuffers(0, vb.buffer, ri.vb_offset);
            if (ri.ib) {
                const auto& ib = index_buffer_map_.at(*ri.ib);
                command_buffer.bindIndexBuffer(ib.buffer, ri.ib_offset, ib.type);
                command_buffer.drawIndexed(ri.primitive_count * 3, 1, 0, 0, 0);
            } else {
                command_buffer.draw(ri.primitive_count * 3, 1, 0, 0);
            }
        }
        command_buffer.endRenderPass();
    }

    command_buffer.end();

    // Submit command buffer.
    vk::SubmitInfo submit_info;
    vk::Semaphore wait_semaphores[] = {image_available_semaphores_[current_frame_]};
    vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers_[next_index];
    vk::Semaphore signal_semaphores[] = {render_finished_semaphores_[current_frame_]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    device_.resetFences(in_flight_fences_[current_frame_]);
    graphics_queue_.submit(submit_info, in_flight_fences_[current_frame_]);

    // Present.
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signal_semaphores;
    vk::SwapchainKHR swapChains[] = {swap_chain_};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &next_index;
    presentInfo.pResults = nullptr;  // Optional
    present_queue_.presentKHR(presentInfo);

    current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
    return true;
}

void RenderContextVK::operator()(const cmd::CreateVertexBuffer& c) {
    VertexBufferVK vb;
    vb.initVertexInputDescriptions(c.decl);

    vk::DeviceSize buffer_size = c.data.size();

    vk::Buffer staging_buffer;
    vk::DeviceMemory staging_buffer_memory;
    createBuffer(
        buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_buffer, staging_buffer_memory);

    void* data = device_.mapMemory(staging_buffer_memory, 0, buffer_size);
    memcpy(data, c.data.data(), static_cast<std::size_t>(buffer_size));
    device_.unmapMemory(staging_buffer_memory);

    createBuffer(buffer_size,
                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, vb.buffer, vb.buffer_memory);

    copyBuffer(staging_buffer, vb.buffer, buffer_size);

    device_.destroy(staging_buffer);
    device_.free(staging_buffer_memory);

    vertex_buffer_map_.emplace(c.handle, std::move(vb));
}

void RenderContextVK::operator()(const cmd::UpdateVertexBuffer& c) {
}

void RenderContextVK::operator()(const cmd::DeleteVertexBuffer& c) {
    assert(vertex_buffer_map_.count(c.handle) > 0);
    auto it = vertex_buffer_map_.find(c.handle);
    device_.free(it->second.buffer_memory);
    device_.destroy(it->second.buffer);
    vertex_buffer_map_.erase(it);
}

void RenderContextVK::operator()(const cmd::CreateIndexBuffer& c) {
    IndexBufferVK ib;
    ib.type = c.type == IndexBufferType::U16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;

    vk::DeviceSize buffer_size = c.data.size();

    vk::Buffer staging_buffer;
    vk::DeviceMemory staging_buffer_memory;
    createBuffer(
        buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_buffer, staging_buffer_memory);

    void* data = device_.mapMemory(staging_buffer_memory, 0, buffer_size);
    memcpy(data, c.data.data(), static_cast<std::size_t>(buffer_size));
    device_.unmapMemory(staging_buffer_memory);

    createBuffer(buffer_size,
                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, ib.buffer, ib.buffer_memory);

    copyBuffer(staging_buffer, ib.buffer, buffer_size);

    device_.destroy(staging_buffer);
    device_.free(staging_buffer_memory);

    index_buffer_map_.emplace(c.handle, std::move(ib));
}

void RenderContextVK::operator()(const cmd::UpdateIndexBuffer& c) {
}

void RenderContextVK::operator()(const cmd::DeleteIndexBuffer& c) {
    assert(index_buffer_map_.count(c.handle) > 0);
    auto it = index_buffer_map_.find(c.handle);
    device_.free(it->second.buffer_memory);
    device_.destroy(it->second.buffer);
    index_buffer_map_.erase(it);
}

void RenderContextVK::operator()(const cmd::CreateShader& c) {
    ShaderVK shader;
    shader.stage = c.stage;
    shader.entry_point = c.entry_point;

    // Create shader module.
    vk::ShaderModuleCreateInfo create_info;
    create_info.pCode = reinterpret_cast<const u32*>(c.data.data());
    create_info.codeSize = c.data.size();
    shader.module = device_.createShaderModule(create_info);

    // Generate reflection data and create UBOs.
    spirv_cross::Compiler comp(reinterpret_cast<const u32*>(c.data.data()),
                               c.data.size() / sizeof(u32));
    spirv_cross::ShaderResources res = comp.get_shader_resources();
    for (const auto& ubo_resource : res.uniform_buffers) {
        const spirv_cross::SPIRType& type = comp.get_type(ubo_resource.base_type_id);
        usize size = comp.get_declared_struct_size(type);
        const auto& name = comp.get_name(ubo_resource.id);
        logger_.info("UBO resource {} (named {}) is {} bytes", ubo_resource.name, name, size);
        usize member_count = type.member_types.size();
        for (usize i = 0; i < member_count; ++i) {
            ShaderVK::UniformLocation location;
            location.size = comp.get_declared_struct_member_size(type, i);
            location.offset = comp.type_struct_member_offset(type, i);

            const auto& member_type = comp.get_type(type.member_types[i]);
            const auto& member_name = comp.get_member_name(type.self, i);

            logger_.info("- member {} is {} bytes and has an offset of {}", member_name,
                         location.size, location.offset);

            location.ubo_location = shader.uniform_buffers.size();
            shader.uniform_locations.emplace(name + "." + member_name, location);
        }

        // Allocate a UBO and memory for each swap chain image, plus a "staging" buffer at index 0.
        ShaderVK::AutoUniformBuffer ubo;
        ubo.buffers.resize(swap_chain_images_.size() + 1);
        ubo.buffers_memory.resize(swap_chain_images_.size() + 1);
        createBuffer(
            size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            ubo.buffers[0], ubo.buffers_memory[0]);
        for (usize i = 0; i < swap_chain_images_.size(); ++i) {
            createBuffer(
                size,
                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal, ubo.buffers[i + 1],
                ubo.buffers_memory[i + 1]);
        }
        ubo.size = size;
        shader.uniform_buffers.push_back(std::move(ubo));
    }

    shader_map_.emplace(c.handle, std::move(shader));
}

void RenderContextVK::operator()(const cmd::DeleteShader& c) {
    assert(shader_map_.count(c.handle) > 0);
    device_.destroy(shader_map_.at(c.handle).module);
    shader_map_.erase(c.handle);
}

void RenderContextVK::operator()(const cmd::CreateProgram& c) {
    program_map_.emplace(c.handle, ProgramVK{});
}

void RenderContextVK::operator()(const cmd::AttachShader& c) {
    assert(program_map_.count(c.handle) > 0);
    assert(shader_map_.count(c.shader_handle) > 0);

    const auto& shader = shader_map_.at(c.shader_handle);

    static const std::unordered_map<ShaderStage, vk::ShaderStageFlagBits> shader_stage_map = {
        {ShaderStage::Vertex, vk::ShaderStageFlagBits::eVertex},
        {ShaderStage::Geometry, vk::ShaderStageFlagBits::eGeometry},
        {ShaderStage::Fragment, vk::ShaderStageFlagBits::eFragment},
    };

    vk::PipelineShaderStageCreateInfo stage_info;
    stage_info.stage = shader_stage_map.at(shader.stage);
    stage_info.module = shader.module;
    stage_info.pName = shader.entry_point.c_str();

    auto& program = program_map_.at(c.handle);
    program.stages[shader.stage] = shader;
    program.pipeline_stages.push_back(stage_info);
}

void RenderContextVK::operator()(const cmd::LinkProgram& c) {
    auto& program = program_map_.at(c.handle);

    // TODO: Figure this out from the shader.
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.resize(3);
    bindings[0].binding = 0;
    bindings[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
    bindings[0].pImmutableSamplers = nullptr;  // Optional
    bindings[1].binding = 1;
    bindings[1].descriptorType = vk::DescriptorType::eUniformBuffer;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = vk::ShaderStageFlagBits::eFragment;
    bindings[1].pImmutableSamplers = nullptr;  // Optional
    bindings[2].binding = 2;
    bindings[2].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = vk::ShaderStageFlagBits::eFragment;
    bindings[2].pImmutableSamplers = nullptr;  // Optional

    vk::DescriptorSetLayoutCreateInfo layout_info;
    layout_info.bindingCount = bindings.size();
    layout_info.pBindings = bindings.data();
    program.descriptor_set_layout = device_.createDescriptorSetLayout(layout_info);

    // Allocate descriptor sets.
    // TODO: Move this to renderFrame like here: https://github.com/bkaradzic/bgfx/blob/master/src/renderer_vk.cpp#L3638
    std::vector<vk::DescriptorSetLayout> layouts(swap_chain_images_.size(),
                                                 program.descriptor_set_layout);
    vk::DescriptorSetAllocateInfo alloc_info;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = layouts.size();
    alloc_info.pSetLayouts = layouts.data();
    program.descriptor_sets = device_.allocateDescriptorSets(alloc_info);

    // Write to them.
    for (usize i = 0; i < swap_chain_images_.size(); ++i) {
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = program.stages.at(ShaderStage::Vertex).uniform_buffers[0].buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        vk::DescriptorImageInfo imageInfo;
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        // imageInfo.imageView = textureImageView;
        // imageInfo.sampler = textureSampler;

        std::array<vk::WriteDescriptorSet, 2> descriptorWrite;
        descriptorWrite[0].dstSet = program.descriptor_sets[i];
        descriptorWrite[0].dstBinding = 0;  // layout(binding = 0)
        descriptorWrite[0].dstArrayElement = 0;
        descriptorWrite[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite[0].descriptorCount = 1;
        descriptorWrite[0].pBufferInfo = &bufferInfo;
        descriptorWrite[0].pImageInfo = nullptr;
        descriptorWrite[0].pTexelBufferView = nullptr;

        descriptorWrite[1].dstSet = program.descriptor_sets[i];
        descriptorWrite[1].dstBinding = 2;
        descriptorWrite[1].dstArrayElement = 0;
        descriptorWrite[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrite[1].descriptorCount = 1;
        descriptorWrite[1].pBufferInfo = nullptr;
        descriptorWrite[1].pImageInfo = &imageInfo;
        descriptorWrite[1].pTexelBufferView = nullptr;

        device_.updateDescriptorSets(descriptorWrite, {});
    }
}

void RenderContextVK::operator()(const cmd::DeleteProgram& c) {
    program_map_.erase(c.handle);
}

void RenderContextVK::operator()(const cmd::CreateTexture2D& c) {
    TextureVK texture;

    vk::DeviceSize buffer_size = c.data.size();

    vk::Buffer staging_buffer;
    vk::DeviceMemory staging_buffer_memory;
    createBuffer(
        buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_buffer, staging_buffer_memory);

    void* data = device_.mapMemory(staging_buffer_memory, 0, buffer_size);
    memcpy(data, c.data.data(), static_cast<std::size_t>(buffer_size));
    device_.unmapMemory(staging_buffer_memory);

    vk::ImageCreateInfo image_info;
    image_info.imageType = vk::ImageType::e2D;
    image_info.extent.width = static_cast<u32>(c.width);
    image_info.extent.height = static_cast<u32>(c.height);
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = vk::Format::eR8G8B8A8Unorm;
    image_info.tiling = vk::ImageTiling::eOptimal;
    image_info.initialLayout = vk::ImageLayout::eUndefined;
    image_info.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    image_info.sharingMode = vk::SharingMode::eExclusive;
    image_info.samples = vk::SampleCountFlagBits::e1;
    image_info.flags = {};  // Optional
    texture.image = device_.createImage(image_info);

    vk::MemoryRequirements memRequirements = device_.getImageMemoryRequirements(texture.image);

    vk::MemoryAllocateInfo alloc_info;
    alloc_info.allocationSize = memRequirements.size;
    alloc_info.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    texture.image_memory = device_.allocateMemory(alloc_info);

    device_.bindImageMemory(texture.image, texture.image_memory, 0);

    transitionImageLayout(texture.image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(staging_buffer, texture.image, static_cast<u32>(c.width),
                      static_cast<u32>(c.height));
    transitionImageLayout(texture.image, vk::Format::eR8G8B8A8Unorm,
                          vk::ImageLayout::eTransferDstOptimal,
                          vk::ImageLayout::eShaderReadOnlyOptimal);

    device_.destroy(staging_buffer);
    device_.free(staging_buffer_memory);

    texture_map_.emplace(c.handle, std::move(texture));
}

void RenderContextVK::operator()(const cmd::DeleteTexture& c) {
}

void RenderContextVK::operator()(const cmd::CreateFrameBuffer& c) {
}

void RenderContextVK::operator()(const cmd::DeleteFrameBuffer& c) {
}

bool RenderContextVK::checkValidationLayerSupport() {
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

std::vector<const char*> RenderContextVK::getRequiredExtensions(bool enable_validation_layers) {
    // Get required GLFW extensions.
    u32 glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // Add the debug utils extension if validation layers are enabled.
    if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void RenderContextVK::createInstance(bool enable_validation_layers) {
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
    logger_.info(extension_list.str());

    vk::ApplicationInfo app_info;
    app_info.pApplicationName = "RenderContextVK";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "dawn-gfx";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo create_info;
    create_info.pApplicationInfo = &app_info;

    // Enable required extensions.
    auto extensions = getRequiredExtensions(enable_validation_layers);
    create_info.enabledExtensionCount = static_cast<u32>(extensions.size());
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
        debug_messenger_info.pfnUserCallback = debugMessageCallback;
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

void RenderContextVK::createDevice() {
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
            SwapChainSupportDetails::querySupport(device, surface_);
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
    graphics_queue_family_index_ = indices.graphics_family.value();
    present_queue_family_index_ = indices.present_family.value();

    // Create a logical device.
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    std::set<u32> unique_queues_families = {graphics_queue_family_index_,
                                            present_queue_family_index_};
    float queue_priority = 1.0f;
    for (u32 queue_family : unique_queues_families) {
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
    // No device specific extensions.
    create_info.enabledExtensionCount = static_cast<u32>(kRequiredDeviceExtensions.size());
    create_info.ppEnabledExtensionNames = kRequiredDeviceExtensions.data();
    // Technically unneeded, but worth doing anyway for old vulkan implementations.
    if (debug_messenger_) {
        create_info.enabledLayerCount = static_cast<u32>(kValidationLayers.size());
        create_info.ppEnabledLayerNames = kValidationLayers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }
    device_ = physical_device_.createDevice(create_info);

    // Get queue handles.
    graphics_queue_ = device_.getQueue(indices.graphics_family.value(), 0);
    present_queue_ = device_.getQueue(indices.present_family.value(), 0);
}

void RenderContextVK::createSwapChain() {
    SwapChainSupportDetails swap_chain_support =
        SwapChainSupportDetails::querySupport(physical_device_, surface_);
    vk::SurfaceFormatKHR surface_format = swap_chain_support.chooseSurfaceFormat();
    vk::PresentModeKHR present_mode = swap_chain_support.choosePresentMode();
    vk::Extent2D extent = swap_chain_support.chooseSwapExtent(windowSize());

    u32 image_count = swap_chain_support.capabilities.minImageCount + 1;
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

    u32 queueFamilyIndices[] = {graphics_queue_family_index_, present_queue_family_index_};
    if (graphics_queue_family_index_ != present_queue_family_index_) {
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

void RenderContextVK::createImageViews() {
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

void RenderContextVK::createRenderPass() {
    vk::AttachmentDescription colour_attachment;
    colour_attachment.format = swap_chain_image_format_;
    colour_attachment.samples = vk::SampleCountFlagBits::e1;
    colour_attachment.loadOp = vk::AttachmentLoadOp::eClear;
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

PipelineVK RenderContextVK::createGraphicsPipeline(const RenderItem& render_item,
                                                   const VertexBufferVK& vb,
                                                   const ProgramVK& program) {
    PipelineVK graphics_pipeline;

    // Create fixed function pipeline stages.
    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<u32>(vb.attribute_descriptions.size());
    vertex_input_info.pVertexBindingDescriptions = &vb.binding_description;
    vertex_input_info.pVertexAttributeDescriptions = vb.attribute_descriptions.data();

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
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
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
    if (render_item.colour_write) {
        colour_blend_attachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }
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

    //    vk::PipelineDynamicStateCreateInfo dynamic_state;
    //    dynamic_state.dynamicStateCount = 0;
    //    dynamic_state.pDynamicStates = dynamic_states;

    vk::PipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &program.descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    graphics_pipeline.layout = device_.createPipelineLayout(pipeline_layout_info);

    // Look up shader program.
    const auto& shader_stages = program_map_.at(*render_item.program);

    vk::GraphicsPipelineCreateInfo pipeline_info;
    pipeline_info.stageCount = shader_stages.pipeline_stages.size();
    pipeline_info.pStages = shader_stages.pipeline_stages.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = nullptr;  // Optional
    pipeline_info.pColorBlendState = &colour_blending;
    pipeline_info.pDynamicState = nullptr;  // Optional
    pipeline_info.layout = graphics_pipeline.layout;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = vk::Pipeline{};  // Optional
    pipeline_info.basePipelineIndex = -1;               // Optional

    graphics_pipeline.pipeline =
        device_.createGraphicsPipelines(vk::PipelineCache{}, pipeline_info)[0];

    return graphics_pipeline;
}

void RenderContextVK::createFramebuffers() {
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

void RenderContextVK::createCommandBuffers() {
    vk::CommandPoolCreateInfo pool_info;
    pool_info.queueFamilyIndex = graphics_queue_family_index_;
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    command_pool_ = device_.createCommandPool(pool_info);

    vk::CommandBufferAllocateInfo allocate_info;
    allocate_info.commandPool = command_pool_;
    allocate_info.level = vk::CommandBufferLevel::ePrimary;
    allocate_info.commandBufferCount = static_cast<u32>(swap_chain_framebuffers_.size());
    command_buffers_ = device_.allocateCommandBuffers(allocate_info);
}

void RenderContextVK::createDescriptorPool() {
    vk::DescriptorPoolSize dps[] = {
        {vk::DescriptorType::eCombinedImageSampler, (10 * DW_MAX_TEXTURE_SAMPLERS) << 10},
        {vk::DescriptorType::eSampledImage, (10 * DW_MAX_TEXTURE_SAMPLERS) << 10},
        {vk::DescriptorType::eSampler, (10 * DW_MAX_TEXTURE_SAMPLERS) << 10},
        {vk::DescriptorType::eUniformBuffer, 10 << 10},
        {vk::DescriptorType::eStorageBuffer, DW_MAX_TEXTURE_SAMPLERS << 10},
        {vk::DescriptorType::eStorageImage, DW_MAX_TEXTURE_SAMPLERS << 10},
    };

    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.poolSizeCount = sizeof(dps) / sizeof(dps[0]);
    poolInfo.pPoolSizes = dps;
    poolInfo.maxSets = 10 << 10;

    descriptor_pool_ = device_.createDescriptorPool(poolInfo);
}

void RenderContextVK::createSyncObjects() {
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

    images_in_flight_.resize(swap_chain_images_.size());
}

u32 RenderContextVK::findMemoryType(u32 type_filter, vk::MemoryPropertyFlags properties) {
    auto mem_properties = physical_device_.getMemoryProperties();
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1u << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find a suitable memory type.");
}

void RenderContextVK::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                   vk::MemoryPropertyFlags properties, vk::Buffer& buffer,
                                   vk::DeviceMemory& buffer_memory) {
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    buffer = device_.createBuffer(bufferInfo);

    vk::MemoryRequirements mem_requirements = device_.getBufferMemoryRequirements(buffer);
    vk::MemoryAllocateInfo alloc_info;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = findMemoryType(mem_requirements.memoryTypeBits, properties);
    buffer_memory = device_.allocateMemory(alloc_info);

    device_.bindBufferMemory(buffer, buffer_memory, 0);
}

void RenderContextVK::copyBuffer(vk::Buffer src_buffer, vk::Buffer dst_buffer,
                                 vk::DeviceSize size) {
    vk::CommandBuffer command_buffer = beginSingleUseCommands();

    vk::BufferCopy copyRegion;
    copyRegion.size = size;
    command_buffer.copyBuffer(src_buffer, dst_buffer, copyRegion);

    endSingleUseCommands(command_buffer);
}

void RenderContextVK::copyBufferToImage(vk::Buffer buffer, vk::Image image, u32 width, u32 height) {
    vk::CommandBuffer command_buffer = beginSingleUseCommands();

    vk::BufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{width, height, 1};
    command_buffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

    endSingleUseCommands(command_buffer);
}

void RenderContextVK::transitionImageLayout(vk::Image image, vk::Format format,
                                            vk::ImageLayout old_layout,
                                            vk::ImageLayout new_layout) {
    vk::CommandBuffer command_buffer = beginSingleUseCommands();

    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;
    if (old_layout == vk::ImageLayout::eUndefined &&
        new_layout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (old_layout == vk::ImageLayout::eTransferDstOptimal &&
               new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("unsupported layout transition.");
    }

    command_buffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);

    endSingleUseCommands(command_buffer);
}

vk::CommandBuffer RenderContextVK::beginSingleUseCommands() {
    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.level = vk::CommandBufferLevel::ePrimary;
    alloc_info.commandPool = command_pool_;
    alloc_info.commandBufferCount = 1;

    vk::CommandBuffer command_buffer;
    command_buffer = device_.allocateCommandBuffers(alloc_info)[0];

    vk::CommandBufferBeginInfo begin_info;
    begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    command_buffer.begin(begin_info);

    return command_buffer;
}

void RenderContextVK::endSingleUseCommands(vk::CommandBuffer command_buffer) {
    command_buffer.end();

    vk::SubmitInfo submit_info;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    graphics_queue_.submit(submit_info, vk::Fence{});
    graphics_queue_.waitIdle();

    device_.freeCommandBuffers(command_pool_, command_buffer);
}

void RenderContextVK::cleanup() {
    device_.waitIdle();

    for (const auto& fence : in_flight_fences_) {
        device_.destroy(fence);
    }
    for (const auto& semaphore : render_finished_semaphores_) {
        device_.destroy(semaphore);
    }
    for (const auto& semaphore : image_available_semaphores_) {
        device_.destroy(semaphore);
    }

    device_.destroy(descriptor_pool_);

    device_.destroy(command_pool_);
    for (const auto& framebuffer : swap_chain_framebuffers_) {
        device_.destroy(framebuffer);
    }
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