/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "vulkan/RenderContextVK.h"
#include <cstring>
#include <set>
#include <cstdint>
#include <map>

#include <spirv_cross.hpp>
#include <dawn-gfx/Renderer.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace dw {
namespace gfx {
namespace {
struct TextureFormatVK {
    vk::Format format;
    vk::Format format_srgb;
};
// clang-format off
const std::array<TextureFormatVK, usize(TextureFormat::Count)> kTextureFormatMap = {{
    {vk::Format::eUndefined,          vk::Format::eUndefined   }, // A8
    {vk::Format::eR8Unorm,            vk::Format::eR8Srgb      }, // R8
    {vk::Format::eR8Sint,             vk::Format::eUndefined   }, // R8I
    {vk::Format::eR8Uint,             vk::Format::eUndefined   }, // R8U
    {vk::Format::eR8Snorm,            vk::Format::eUndefined   }, // R8S
    {vk::Format::eR16Unorm,           vk::Format::eUndefined   }, // R16
    {vk::Format::eR16Sint,            vk::Format::eUndefined   }, // R16I
    {vk::Format::eR16Unorm,           vk::Format::eUndefined   }, // R16U
    {vk::Format::eR16Sfloat,          vk::Format::eUndefined   }, // R16F
    {vk::Format::eR16Snorm,           vk::Format::eUndefined   }, // R16S
    {vk::Format::eR32Sint,            vk::Format::eUndefined   }, // R32I
    {vk::Format::eR32Uint,            vk::Format::eUndefined   }, // R32U
    {vk::Format::eR32Sfloat,          vk::Format::eUndefined   }, // R32F
    {vk::Format::eR8G8Unorm,          vk::Format::eR8G8Srgb    }, // RG8
    {vk::Format::eR8G8Sint,           vk::Format::eUndefined   }, // RG8I
    {vk::Format::eR8G8Uint,           vk::Format::eUndefined   }, // RG8U
    {vk::Format::eR8G8Snorm,          vk::Format::eUndefined   }, // RG8S
    {vk::Format::eR16G16Unorm,        vk::Format::eUndefined   }, // RG16
    {vk::Format::eR16G16Sint,         vk::Format::eUndefined   }, // RG16I
    {vk::Format::eR16G16Uint,         vk::Format::eUndefined   }, // RG16U
    {vk::Format::eR16G16Sfloat,       vk::Format::eUndefined   }, // RG16F
    {vk::Format::eR16G16Snorm,        vk::Format::eUndefined   }, // RG16S
    {vk::Format::eR32G32Sint,         vk::Format::eUndefined   }, // RG32I
    {vk::Format::eR32G32Uint,         vk::Format::eUndefined   }, // RG32U
    {vk::Format::eR32G32Sfloat,       vk::Format::eUndefined   }, // RG32F
    {vk::Format::eR8G8B8Unorm,        vk::Format::eR8G8B8Srgb  }, // RGB8
    {vk::Format::eR8G8B8Sint,         vk::Format::eR8G8B8Srgb  }, // RGB8I
    {vk::Format::eR8G8B8Uint,         vk::Format::eR8G8B8Srgb  }, // RGB8U
    {vk::Format::eR8G8B8Snorm,        vk::Format::eUndefined   }, // RGB8S
    {vk::Format::eB8G8R8A8Unorm,      vk::Format::eB8G8R8A8Srgb}, // BGRA8
    {vk::Format::eR8G8B8A8Unorm,      vk::Format::eR8G8B8A8Srgb}, // RGBA8
    {vk::Format::eR8G8B8A8Sint,       vk::Format::eR8G8B8A8Srgb}, // RGBA8I
    {vk::Format::eR8G8B8A8Uint,       vk::Format::eR8G8B8A8Srgb}, // RGBA8U
    {vk::Format::eR8G8B8A8Snorm,      vk::Format::eUndefined   }, // RGBA8S
    {vk::Format::eR16G16B16A16Unorm,  vk::Format::eUndefined   }, // RGBA16
    {vk::Format::eR16G16B16A16Sint,   vk::Format::eUndefined   }, // RGBA16I
    {vk::Format::eR16G16B16A16Uint,   vk::Format::eUndefined   }, // RGBA16U
    {vk::Format::eR16G16B16A16Sfloat, vk::Format::eUndefined   }, // RGBA16F
    {vk::Format::eR16G16B16A16Snorm,  vk::Format::eUndefined   }, // RGBA16S
    {vk::Format::eR32G32B32A32Sint,   vk::Format::eUndefined   }, // RGBA32I
    {vk::Format::eR32G32B32A32Uint,   vk::Format::eUndefined   }, // RGBA32U
    {vk::Format::eR32G32B32A32Sfloat, vk::Format::eUndefined   }, // RGBA32F
    {vk::Format::eD16Unorm,           vk::Format::eUndefined   }, // D16
    {vk::Format::eD24UnormS8Uint,     vk::Format::eUndefined   }, // D24
    {vk::Format::eD24UnormS8Uint,     vk::Format::eUndefined   }, // D24S8
    {vk::Format::eD32Sfloat,          vk::Format::eUndefined   }, // D32
    {vk::Format::eD32Sfloat,          vk::Format::eUndefined   }, // D16F
    {vk::Format::eD32Sfloat,          vk::Format::eUndefined   }, // D24F
    {vk::Format::eD32Sfloat,          vk::Format::eUndefined   }, // D32F
    {vk::Format::eD24UnormS8Uint,     vk::Format::eUndefined   }, // D0S8
}};

const std::unordered_map<BlendEquation, vk::BlendOp> kBlendEquationMap = {
    {BlendEquation::Add, vk::BlendOp::eAdd},
    {BlendEquation::Subtract, vk::BlendOp::eSubtract},
    {BlendEquation::ReverseSubtract, vk::BlendOp::eReverseSubtract},
    {BlendEquation::Min, vk::BlendOp::eMin},
    {BlendEquation::Max, vk::BlendOp::eMax}
};
const std::unordered_map<BlendFunc, vk::BlendFactor> kBlendFuncMap = {
    {BlendFunc::Zero, vk::BlendFactor::eZero},
    {BlendFunc::One, vk::BlendFactor::eOne},
    {BlendFunc::SrcColor, vk::BlendFactor::eSrcColor},
    {BlendFunc::OneMinusSrcColor, vk::BlendFactor::eOneMinusSrcColor},
    {BlendFunc::DstColor, vk::BlendFactor::eDstColor},
    {BlendFunc::OneMinusDstColor, vk::BlendFactor::eOneMinusDstColor},
    {BlendFunc::SrcAlpha, vk::BlendFactor::eSrcAlpha},
    {BlendFunc::OneMinusSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha},
    {BlendFunc::DstAlpha, vk::BlendFactor::eDstAlpha},
    {BlendFunc::OneMinusDstAlpha, vk::BlendFactor::eOneMinusDstAlpha},
    {BlendFunc::ConstantColor, vk::BlendFactor::eConstantColor},
    {BlendFunc::OneMinusConstantColor, vk::BlendFactor::eOneMinusConstantColor},
    {BlendFunc::ConstantAlpha, vk::BlendFactor::eConstantAlpha},
    {BlendFunc::OneMinusConstantAlpha, vk::BlendFactor::eOneMinusConstantAlpha},
    {BlendFunc::SrcAlphaSaturate, vk::BlendFactor::eSrcAlphaSaturate},
};
// clang-format on

const std::array<const char*, 1> kValidationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::array<const char*, 1> kRequiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
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

vk::ShaderStageFlagBits convertShaderStage(ShaderStage stage) {
    static const std::unordered_map<ShaderStage, vk::ShaderStageFlagBits> shader_stage_map = {
        {ShaderStage::Vertex, vk::ShaderStageFlagBits::eVertex},
        {ShaderStage::Geometry, vk::ShaderStageFlagBits::eGeometry},
        {ShaderStage::Fragment, vk::ShaderStageFlagBits::eFragment},
    };
    return shader_stage_map.at(stage);
}
}  // namespace

DeviceVK::DeviceVK(vk::PhysicalDevice physical_device, vk::Device device,
                   vk::CommandPool command_pool, vk::Queue graphics_queue)
    : physical_device_(physical_device),
      device_(device),
      command_pool_(command_pool),
      graphics_queue_(graphics_queue) {
    properties_ = physical_device_.getProperties();
}

DeviceVK::~DeviceVK() {
    device_.destroy(command_pool_);
    device_.destroy();
}

vk::PhysicalDevice DeviceVK::getPhysicalDevice() const {
    return physical_device_;
}

vk::Device DeviceVK::getDevice() const {
    return device_;
}

vk::CommandPool DeviceVK::getCommandPool() const {
    return command_pool_;
}

u32 DeviceVK::findMemoryType(u32 type_filter, vk::MemoryPropertyFlags properties) {
    auto mem_properties = physical_device_.getMemoryProperties();
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1u << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find a suitable memory type.");
}

vk::DeviceSize DeviceVK::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
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

    return mem_requirements.size;
}

void DeviceVK::copyBuffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size) {
    vk::CommandBuffer command_buffer = beginSingleUseCommands();

    vk::BufferCopy copyRegion;
    copyRegion.size = size;
    command_buffer.copyBuffer(src_buffer, dst_buffer, copyRegion);

    endSingleUseCommands(command_buffer);
}

void DeviceVK::copyBufferToImage(vk::Buffer buffer, vk::Image image, u32 width, u32 height) {
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

void DeviceVK::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout old_layout,
                                     vk::ImageLayout new_layout) {
    vk::CommandBuffer command_buffer = beginSingleUseCommands();

    // TODO: Merge this with TextureVK::setImageBarrier

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

void DeviceVK::createImage(u32 width, u32 height, vk::Format format, vk::ImageTiling tiling,
                           vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                           vk::Image& image, vk::DeviceMemory& image_memory) {
    vk::ImageCreateInfo image_info;
    image_info.imageType = vk::ImageType::e2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = vk::ImageLayout::eUndefined;
    image_info.usage = usage;
    image_info.sharingMode = vk::SharingMode::eExclusive;
    image_info.samples = vk::SampleCountFlagBits::e1;
    image = device_.createImage(image_info);

    vk::MemoryRequirements mem_requirements = device_.getImageMemoryRequirements(image);
    vk::MemoryAllocateInfo alloc_info;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = findMemoryType(mem_requirements.memoryTypeBits, properties);
    image_memory = device_.allocateMemory(alloc_info);

    device_.bindImageMemory(image, image_memory, 0);
}

vk::ImageView DeviceVK::createImageView(vk::Image image, vk::Format format,
                                        vk::ImageAspectFlags aspect_flags) {
    vk::ImageViewCreateInfo image_view_info;
    image_view_info.image = image;
    image_view_info.viewType = vk::ImageViewType::e2D;
    image_view_info.format = format;
    image_view_info.components.r = vk::ComponentSwizzle::eIdentity;
    image_view_info.components.g = vk::ComponentSwizzle::eIdentity;
    image_view_info.components.b = vk::ComponentSwizzle::eIdentity;
    image_view_info.components.a = vk::ComponentSwizzle::eIdentity;
    image_view_info.subresourceRange.aspectMask = aspect_flags;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    return device_.createImageView(image_view_info);
}

vk::CommandBuffer DeviceVK::beginSingleUseCommands() {
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

void DeviceVK::endSingleUseCommands(vk::CommandBuffer command_buffer) {
    command_buffer.end();

    vk::SubmitInfo submit_info;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    graphics_queue_.submit(submit_info, vk::Fence{});
    graphics_queue_.waitIdle();

    device_.freeCommandBuffers(command_pool_, command_buffer);
}

const vk::PhysicalDeviceProperties& DeviceVK::properties() {
    return properties_;
}

BufferVK::BufferVK(DeviceVK* device, const byte* data, vk::DeviceSize size, BufferUsage usage,
                   vk::BufferUsageFlags buffer_type, usize swap_chain_size)
    : device(device), size(size), usage(usage) {
    if (usage == BufferUsage::Static) {
        // Static memory uses a staging buffer to upload static vertex data to device local memory.
        vk::Buffer staging_buffer;
        vk::DeviceMemory staging_buffer_memory;
        device->createBuffer(
            size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            staging_buffer, staging_buffer_memory);

        void* mapped_data = device->getDevice().mapMemory(staging_buffer_memory, 0, size);
        memcpy(mapped_data, data, static_cast<std::size_t>(size));
        device->getDevice().unmapMemory(staging_buffer_memory);

        // Copy staging data into buffers.
        buffer.resize(1);
        buffer_memory.resize(1);
        device->createBuffer(size, vk::BufferUsageFlagBits::eTransferDst | buffer_type,
                             vk::MemoryPropertyFlagBits::eDeviceLocal, buffer[0], buffer_memory[0]);
        device->copyBuffer(staging_buffer, buffer[0], size);

        device->getDevice().destroy(staging_buffer);
        device->getDevice().free(staging_buffer_memory);
    } else if (usage == BufferUsage::Stream) {
        // Streaming buffers are stored as host coherent buffers.
        buffer.resize(swap_chain_size);
        buffer_memory.resize(swap_chain_size);
        for (usize i = 0; i < swap_chain_size; ++i) {
            device->createBuffer(size, buffer_type,
                                 vk::MemoryPropertyFlagBits::eHostVisible |
                                     vk::MemoryPropertyFlagBits::eHostCoherent,
                                 buffer[i], buffer_memory[i]);

            void* mapped_data = device->getDevice().mapMemory(buffer_memory[i], 0, size);
            memcpy(mapped_data, data, usize(size));
            device->getDevice().unmapMemory(buffer_memory[i]);
        }
    }
}

BufferVK::~BufferVK() {
    assert(buffer.size() == buffer_memory.size());
    for (usize i = 0; i < buffer.size(); ++i) {
        device->getDevice().free(buffer_memory[i]);
        device->getDevice().destroy(buffer[i]);
    }
}

BufferVK::BufferVK(BufferVK&& other) noexcept
    : device(other.device), size(other.size), usage(other.usage) {
    std::swap(buffer, other.buffer);
    std::swap(buffer_memory, other.buffer_memory);
}

BufferVK& BufferVK::operator=(BufferVK&& other) noexcept {
    device = other.device;
    size = other.size;
    std::swap(buffer, other.buffer);
    std::swap(buffer_memory, other.buffer_memory);
    usage = other.usage;
    return *this;
}

vk::Buffer BufferVK::get(u32 frame_index) const {
    return buffer[getIndex(frame_index)];
}

bool BufferVK::update(u32 frame_index, const byte* data, vk::DeviceSize data_size,
                      vk::DeviceSize offset) {
    switch (usage) {
        case BufferUsage::Static:
            return false;

        case BufferUsage::Dynamic:
            // TODO: Not implemented.
            return true;

        case BufferUsage::Stream: {
            const auto& current_buffer_memory = buffer_memory[getIndex(frame_index)];
            void* mapped_data = device->getDevice().mapMemory(current_buffer_memory, usize(offset),
                                                              usize(data_size));
            memcpy(mapped_data, data + usize(offset), usize(data_size));
            device->getDevice().unmapMemory(current_buffer_memory);
            return true;
        }
    }
    return false;
}

u32 BufferVK::getIndex(u32 frame_index) const {
    if (buffer.size() == 1) {
        return 0;
    } else {
        assert(frame_index < buffer.size());
        return frame_index;
    }
}

VertexDeclVK::VertexDeclVK(const VertexDecl& decl) {
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

vk::Format VertexDeclVK::getVertexAttributeFormat(VertexDecl::AttributeType type, usize count,
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

UniformScratchBuffer::UniformScratchBuffer(
    DeviceVK* device /*, vk::DescriptorPool descriptor_pool*/, usize size)
    : device_(device), current_size_(0) {
    maximum_size_ = device_->createBuffer(
        size, vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        buffer_, buffer_memory_);
    data_ =
        reinterpret_cast<byte*>(device_->getDevice().mapMemory(buffer_memory_, 0, maximum_size_));
}

UniformScratchBuffer::~UniformScratchBuffer() {
    device_->getDevice().unmapMemory(buffer_memory_);
    device_->getDevice().destroy(buffer_);
    device_->getDevice().free(buffer_memory_);
}

UniformScratchBuffer::Allocation UniformScratchBuffer::alloc(usize size) {
    if (current_size_ + size > maximum_size_) {
        throw std::runtime_error(
            "Ran out of uniform scratch space this frame. Dynamic resizing not yet implemented.");
    }
    UniformScratchBuffer::Allocation allocation{data_ + current_size_, current_size_};
    current_size_ += size;
    return allocation;
}

void UniformScratchBuffer::reset() {
    current_size_ = 0;
}

vk::Buffer UniformScratchBuffer::getBuffer() const {
    return buffer_;
}

void TextureVK::setImageBarrier(vk::CommandBuffer command_buffer, vk::ImageLayout new_layout) {
    if (new_layout == image_layout) {
        return;
    }

    vk::AccessFlags src_access_mask = {};
    vk::AccessFlags dst_access_mask = {};

    switch (image_layout) {
        case vk::ImageLayout::eUndefined:
            break;
        case vk::ImageLayout::eGeneral:
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            src_access_mask |= vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            src_access_mask |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;
        case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            src_access_mask |= vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            src_access_mask |= vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            break;
        case vk::ImageLayout::ePreinitialized:
            src_access_mask |= vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            src_access_mask |= vk::AccessFlagBits::eMemoryRead;
            break;
        default:
            break;
    }

    switch (new_layout) {
        case vk::ImageLayout::eUndefined:
            break;
        case vk::ImageLayout::eGeneral:
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            dst_access_mask |= vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            dst_access_mask |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            // aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            dst_access_mask |=
                vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            dst_access_mask |= vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            dst_access_mask |= vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::ePreinitialized:
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            dst_access_mask |= vk::AccessFlagBits::eMemoryRead;
            break;
        default:
            break;
    }

    vk::ImageMemoryBarrier imb;
    imb.srcAccessMask = src_access_mask;
    imb.dstAccessMask = dst_access_mask;
    imb.oldLayout = image_layout;
    imb.newLayout = new_layout;
    imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imb.image = image;
    imb.subresourceRange.aspectMask = aspect_mask;
    imb.subresourceRange.baseMipLevel = 0;
    imb.subresourceRange.levelCount = 1;
    imb.subresourceRange.baseArrayLayer = 0;
    imb.subresourceRange.layerCount = 1;
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                   vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, imb);

    image_layout = new_layout;
}

FramebufferVK::FramebufferVK(DeviceVK* device, u16 width, u16 height,
                             std::vector<TextureVK*> attachments)
    : extent(width, height), images(std::move(attachments)) {
    // Create depth image.
    auto depth_image_format = vk::Format::eD32Sfloat;
    device->createImage(width, height, depth_image_format, vk::ImageTiling::eOptimal,
                        vk::ImageUsageFlagBits::eDepthStencilAttachment,
                        vk::MemoryPropertyFlagBits::eDeviceLocal, depth.image, depth.image_memory);
    depth.image_view =
        device->createImageView(depth.image, depth_image_format, vk::ImageAspectFlagBits::eDepth);

    // Create render pass.
    std::vector<vk::ImageView> image_views;

    // Setup colour attachments.
    std::vector<vk::AttachmentDescription> attachment_descriptions;
    std::vector<vk::AttachmentReference> colour_attachment_refs;
    for (TextureVK* attachment : images) {
        vk::AttachmentDescription colour_attachment;
        colour_attachment.format = attachment->image_format;
        colour_attachment.samples = vk::SampleCountFlagBits::e1;
        colour_attachment.loadOp = vk::AttachmentLoadOp::eClear;
        colour_attachment.storeOp = vk::AttachmentStoreOp::eStore;
        colour_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colour_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colour_attachment.initialLayout = vk::ImageLayout::eUndefined;
        colour_attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
        attachment_descriptions.emplace_back(colour_attachment);

        vk::AttachmentReference colour_attachment_ref;
        colour_attachment_ref.attachment = attachment_descriptions.size() - 1;
        colour_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        colour_attachment_refs.emplace_back(colour_attachment_ref);

        image_views.emplace_back(attachment->image_view);
    }

    // Depth attachment.
    vk::AttachmentDescription depth_attachment;
    depth_attachment.format = depth_image_format;
    depth_attachment.samples = vk::SampleCountFlagBits::e1;
    depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
    depth_attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    attachment_descriptions.emplace_back(depth_attachment);

    vk::AttachmentReference depth_attachment_ref;
    depth_attachment_ref.attachment = attachment_descriptions.size() - 1;
    depth_attachment_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    image_views.emplace_back(depth.image_view);

    // Subpass.
    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = colour_attachment_refs.size();
    subpass.pColorAttachments = colour_attachment_refs.data();
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo render_pass_info;
    render_pass_info.attachmentCount = attachment_descriptions.size();
    render_pass_info.pAttachments = attachment_descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    render_pass = device->getDevice().createRenderPass(render_pass_info);

    // Create framebuffer.
    vk::FramebufferCreateInfo framebuffer_info;
    framebuffer_info.renderPass = render_pass;
    framebuffer_info.attachmentCount = image_views.size();
    framebuffer_info.pAttachments = image_views.data();
    framebuffer_info.width = extent.width;
    framebuffer_info.height = extent.height;
    framebuffer_info.layers = 1;
    framebuffer = device->getDevice().createFramebuffer(framebuffer_info);
}

RenderContextVK::RenderContextVK(Logger& logger) : RenderContext{logger}, current_frame_(0) {
}

RenderContextVK::~RenderContextVK() {
    cleanup();
}

Mat4 RenderContextVK::adjustProjectionMatrix(Mat4 projection_matrix) const {
    return projection_matrix;
}

bool RenderContextVK::hasFlippedViewport() const {
    return true;
}

Result<void, std::string> RenderContextVK::createWindow(u16 width, u16 height,
                                                        const std::string& title,
                                                        InputCallbacks input_callbacks) {
    glfwInit();

    // Select monitor.
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();

    // Get DPI settings.
    glfwGetMonitorContentScale(monitor, &window_scale_.x, &window_scale_.y);

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
    createRenderPass();
    createFramebuffers();
    createCommandBuffers();
    createDescriptorPool();
    createSyncObjects();

    uniform_scratch_buffers_.reserve(swap_chain_image_views_.size());
    for (usize i = 0; i < swap_chain_image_views_.size(); ++i) {
        // We estimate that there will be a maximum of 65535 draw calls, with an average of 128
        // bytes of uniforms each.
        uniform_scratch_buffers_.emplace_back(
            std::make_unique<UniformScratchBuffer>(device_.get(), 65535 * 128));
    }

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
    return window_scale_;
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

void RenderContextVK::prepareFrame() {
    // Wait for in-flight fence.
    vk_device_.waitForFences(in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);

    // Acquire next image.
    vk_device_.acquireNextImageKHR(swap_chain_, UINT64_MAX,
                                   image_available_semaphores_[current_frame_], vk::Fence{},
                                   &next_frame_index_);

    // Check if a previous frame is using this image (i.e. there is a fence to wait on).
    if (images_in_flight_[next_frame_index_]) {
        vk_device_.waitForFences(images_in_flight_[next_frame_index_], VK_TRUE, UINT64_MAX);
    }

    // Mark this image as now being in use by this frame.
    images_in_flight_[next_frame_index_] = in_flight_fences_[current_frame_];
}

void RenderContextVK::processCommandList(std::vector<RenderCommand>& command_list) {
    assert(window_);
    for (auto& command : command_list) {
        visit(*this, command);
    }
}

bool RenderContextVK::frame(const Frame* frame) {
    // Update transient vertex and index buffers.
    auto& tvb = frame->transient_vb_storage;
    if (tvb.handle && tvb.size > 0) {
        auto& tvb_data = vertex_buffer_map_.at(*frame->transient_vb_storage.handle);
        tvb_data.buffer.update(next_frame_index_, frame->transient_vb_storage.data.data(),
                               frame->transient_vb_storage.size, 0);
    }
    auto& tib = frame->transient_ib_storage;
    if (tib.handle && tib.size > 0) {
        auto& tib_data = index_buffer_map_.at(*frame->transient_ib_storage.handle);
        tib_data.buffer.update(next_frame_index_, frame->transient_ib_storage.data.data(),
                               frame->transient_ib_storage.size, 0);
    }

    uniform_scratch_buffers_[next_frame_index_]->reset();

    auto command_buffer = command_buffers_[next_frame_index_];
    vk::CommandBufferBeginInfo begin_info;
    begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    command_buffer.begin(begin_info);

    // Write render queues to command buffer.
    const FramebufferVK* previous_frame_buffer = nullptr;
    bool in_render_pass = false;
    for (const auto& q : frame->render_queues) {
        // Get framebuffer.
        const FramebufferVK* current_frame_buffer = nullptr;
        vk::Framebuffer target_framebuffer;
        vk::RenderPass target_render_pass;
        vk::Extent2D target_extent;
        if (q.frame_buffer) {
            const auto& fb = framebuffer_map_.at(*q.frame_buffer);
            target_framebuffer = fb.framebuffer;
            target_render_pass = fb.render_pass;
            target_extent = fb.extent;
            current_frame_buffer = &fb;
        } else {
            // Bind to backbuffer.
            target_framebuffer = swap_chain_framebuffers_[next_frame_index_];
            target_render_pass = swapchain_render_pass_;
            target_extent = swap_chain_extent_;
        }

        // If the framebuffer has changed, start a new render pass.
        if (!in_render_pass || current_frame_buffer != previous_frame_buffer) {
            // End the previous render pass.
            if (in_render_pass) {
                command_buffer.endRenderPass();
                in_render_pass = false;
            }

            // Transition framebuffer state.
            if (previous_frame_buffer && current_frame_buffer != previous_frame_buffer) {
                for (TextureVK* image : previous_frame_buffer->images) {
                    image->setImageBarrier(command_buffer, vk::ImageLayout::eShaderReadOnlyOptimal);
                }
                // TODO: Depth.
            }
            if (current_frame_buffer) {
                for (TextureVK* image : current_frame_buffer->images) {
                    image->setImageBarrier(command_buffer,
                                           vk::ImageLayout::eColorAttachmentOptimal);
                }
                // TODO: Depth.
            }

            // Begin render pass.
            vk::RenderPassBeginInfo render_pass_info;
            render_pass_info.renderPass = target_render_pass;
            render_pass_info.framebuffer = target_framebuffer;
            render_pass_info.renderArea.offset = vk::Offset2D{0, 0};
            render_pass_info.renderArea.extent = target_extent;

            // Set clear parameters.
            std::vector<vk::ClearValue> clear_values;
            int colour_attachment_count = 1;
            if (current_frame_buffer) {
                colour_attachment_count = current_frame_buffer->images.size();
            }
            vk::ClearColorValue clear_colour;
            if (q.clear_parameters.has_value()) {
                const auto& colour = q.clear_parameters.value().colour;
                clear_colour = {
                    std::array<float, 4>{colour.r(), colour.g(), colour.b(), colour.a()}};
            } else {
                clear_colour = {std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}};
            }
            for (int i = 0; i < colour_attachment_count; ++i) {
                clear_values.emplace_back(clear_colour);
            }
            clear_values.emplace_back(vk::ClearDepthStencilValue{1.0f, 0});
            render_pass_info.clearValueCount = clear_values.size();
            render_pass_info.pClearValues = clear_values.data();

            // Begin render pass.
            command_buffer.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
            in_render_pass = true;
        }
        previous_frame_buffer = current_frame_buffer;

        for (auto& ri : q.render_items) {
            auto& program = program_map_.at(*ri.program);

            // Apply uniforms.
            for (auto& entry : ri.uniforms) {
                auto uniform = program.uniform_locations.find(entry.first);
                if (uniform == program.uniform_locations.end()) {
                    logger_.warn("Unknown uniform '{}' in program {}", entry.first, *ri.program);
                    continue;
                }
                uniform->second.data = std::move(entry.second);
            }

            // If there are no vertices to render, we are done.
            if (!ri.vb) {
                continue;
            }

            // Upload uniforms to uniform buffer.
            std::map<usize, UniformScratchBuffer::Allocation> ubo_data;
            for (const auto& ubo : program.uniform_buffers) {
                const u32 alignment = device_->properties().limits.minUniformBufferOffsetAlignment;
                const u32 vsize = strideAlign(ubo.size, alignment);
                ubo_data[ubo.binding] = uniform_scratch_buffers_[next_frame_index_]->alloc(vsize);
            }
            for (const auto& uniform_entry : program.uniform_locations) {
                const auto& uniform = uniform_entry.second;
                if (!uniform.binding_location.has_value()) {
                    logger_.warn("Push constants not implemented yet.");
                    continue;
                }

                // Write to memory.
                if (uniform.data) {
                    auto variant_bytes = std::visit(VariantToBytesHelper{}, *uniform.data);
                    byte* data_dst = ubo_data.at(*uniform.binding_location).ptr + uniform.offset;
                    std::memcpy(data_dst, variant_bytes.data, variant_bytes.size);
                    /*
                    logger_.info("Writing {} (size {}) to offset {} in UBO {}", uniform_pair.first,
                                 variant_bytes.size, uniform_location.offset,
                                 *uniform_location.ubo_location);
                    */
                }
#ifndef NDEBUG
                if (!uniform.data) {
                    logger_.warn("Uniform {} is uninitialised.", uniform_entry.first);
                }
#endif
            }

            const auto& vb = vertex_buffer_map_.at(*ri.vb);

            // Get (or create) vertex decl.
            const auto& current_vertex_decl =
                ri.vertex_decl_override.empty() ? vb.decl : ri.vertex_decl_override;
            auto decl_it = vertex_decl_cache_.find(current_vertex_decl);
            if (decl_it == vertex_decl_cache_.end()) {
                decl_it = vertex_decl_cache_
                              .emplace(current_vertex_decl, VertexDeclVK{current_vertex_decl})
                              .first;
            }

            // Bind (and create) graphics pipeline.
            auto graphics_pipeline = findOrCreateGraphicsPipeline(
                PipelineVK::Info{&ri, &vb, &decl_it->second, &program, current_frame_buffer});
            command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                        graphics_pipeline.pipeline);
            if (ri.scissor_enabled) {
                command_buffer.setScissor(
                    0, vk::Rect2D{vk::Offset2D{ri.scissor_x, ri.scissor_y},
                                  vk::Extent2D{ri.scissor_width, ri.scissor_height}});
            } else {
                command_buffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, swap_chain_extent_});
            }

            // Calculate dynamic offsets for the UBOs.
            std::vector<u32> dynamic_offsets;
            for (const auto& ubo_entry : ubo_data) {
                dynamic_offsets.emplace_back(ubo_entry.second.offset_from_base);
            }

            // Bind descriptor set.
            auto descriptor_set =
                findOrCreateDescriptorSet(DescriptorSetVK::Info{&program, std::move(ri.textures)});
            command_buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, graphics_pipeline.layout, 0,
                descriptor_set.descriptor_sets[next_frame_index_], dynamic_offsets);

            // Bind vertex/index buffers and draw.
            command_buffer.bindVertexBuffers(0, vb.buffer.get(next_frame_index_), ri.vb_offset);
            if (ri.ib) {
                const auto& ib = index_buffer_map_.at(*ri.ib);
                command_buffer.bindIndexBuffer(ib.buffer.get(next_frame_index_), ri.ib_offset,
                                               ib.type);
                command_buffer.drawIndexed(ri.primitive_count * 3, 1, 0, 0, 0);
            } else {
                command_buffer.draw(ri.primitive_count * 3, 1, 0, 0);
            }
        }
    }
    if (in_render_pass) {
        command_buffer.endRenderPass();
        in_render_pass = false;
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
    submit_info.pCommandBuffers = &command_buffers_[next_frame_index_];
    vk::Semaphore signal_semaphores[] = {render_finished_semaphores_[current_frame_]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    vk_device_.resetFences(in_flight_fences_[current_frame_]);
    graphics_queue_.submit(submit_info, in_flight_fences_[current_frame_]);

    // Present.
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signal_semaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swap_chain_;
    presentInfo.pImageIndices = &next_frame_index_;
    present_queue_.presentKHR(presentInfo);

    current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
    return true;
}

void RenderContextVK::operator()(const cmd::CreateVertexBuffer& c) {
    if (c.usage == BufferUsage::Dynamic) {
        throw std::invalid_argument(
            "Dynamic vertex buffers are unimplemented in the Vulkan backend.");
    }

    VertexBufferVK vb{c.decl,
                      BufferVK{device_.get(), c.data.data(), c.data.size(), c.usage,
                               vk::BufferUsageFlagBits::eVertexBuffer, swap_chain_images_.size()}};
    vertex_buffer_map_.emplace(c.handle, std::move(vb));
}

void RenderContextVK::operator()(const cmd::UpdateVertexBuffer& c) {
    assert(vertex_buffer_map_.count(c.handle) > 0);
    auto& vb = vertex_buffer_map_.at(c.handle);
    if (!vb.buffer.update(next_frame_index_, c.data.data(), c.data.size(), c.offset)) {
        logger_.warn("Unable to update vertex buffer {}", c.handle);
    }
}

void RenderContextVK::operator()(const cmd::DeleteVertexBuffer& c) {
    assert(vertex_buffer_map_.count(c.handle) > 0);
    vertex_buffer_map_.erase(vertex_buffer_map_.find(c.handle));
}

void RenderContextVK::operator()(const cmd::CreateIndexBuffer& c) {
    vk::IndexType type =
        c.type == IndexBufferType::U16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
    IndexBufferVK ib{type,
                     BufferVK{device_.get(), c.data.data(), c.data.size(), c.usage,
                              vk::BufferUsageFlagBits::eIndexBuffer, swap_chain_images_.size()}};
    index_buffer_map_.emplace(c.handle, std::move(ib));
}

void RenderContextVK::operator()(const cmd::UpdateIndexBuffer& c) {
    assert(index_buffer_map_.count(c.handle) > 0);
    auto& ib = index_buffer_map_.at(c.handle);
    if (!ib.buffer.update(next_frame_index_, c.data.data(), c.data.size(), c.offset)) {
        logger_.warn("Unable to update index buffer {}", c.handle);
    }
}

void RenderContextVK::operator()(const cmd::DeleteIndexBuffer& c) {
    assert(index_buffer_map_.count(c.handle) > 0);
    index_buffer_map_.erase(index_buffer_map_.find(c.handle));
}

void RenderContextVK::operator()(const cmd::CreateProgram& c) {
    ProgramVK program;

    for (const auto& stage : c.stages) {
        ShaderVK shader;
        shader.stage = stage.stage;
        shader.entry_point = std::make_unique<char[]>(stage.entry_point.size() + 1);
        std::memcpy(shader.entry_point.get(), stage.entry_point.c_str(),
                    stage.entry_point.size() + 1);

        // Create shader module.
        vk::ShaderModuleCreateInfo create_info;
        create_info.pCode = reinterpret_cast<const u32*>(stage.spirv.data());
        create_info.codeSize = stage.spirv.size();
        shader.module = vk_device_.createShaderModule(create_info);

        // Generate reflection data and create UBOs.
        spirv_cross::Compiler comp(reinterpret_cast<const u32*>(stage.spirv.data()),
                                   stage.spirv.size() / sizeof(u32));
        spirv_cross::ShaderResources res = comp.get_shader_resources();
        for (const auto& resource : res.uniform_buffers) {
            const spirv_cross::SPIRType& type = comp.get_type(resource.base_type_id);

            ShaderVK::StructLayout struct_layout;
            struct_layout.name = comp.get_name(resource.id);
            struct_layout.size = comp.get_declared_struct_size(type);

            usize member_count = type.member_types.size();
            struct_layout.fields.reserve(member_count);
            for (usize i = 0; i < member_count; ++i) {
                struct_layout.fields.emplace_back(ShaderVK::StructLayout::StructField{
                    comp.get_member_name(type.self, i), comp.type_struct_member_offset(type, i),
                    comp.get_declared_struct_member_size(type, i)});
            }
            shader.uniform_buffer_bindings.emplace(
                comp.get_decoration(resource.id, spv::Decoration::DecorationBinding),
                std::move(struct_layout));
        }

        // Find bindings.
        for (const auto& resource : res.uniform_buffers) {
            shader.descriptor_type_bindings.emplace(
                comp.get_decoration(resource.id, spv::Decoration::DecorationBinding),
                vk::DescriptorType::eUniformBufferDynamic);
        }
        for (const auto& resource : res.sampled_images) {
            shader.descriptor_type_bindings.emplace(
                comp.get_decoration(resource.id, spv::Decoration::DecorationBinding),
                vk::DescriptorType::eCombinedImageSampler);
        }
        for (const auto& resource : res.separate_images) {
            shader.descriptor_type_bindings.emplace(
                comp.get_decoration(resource.id, spv::Decoration::DecorationBinding),
                vk::DescriptorType::eSampledImage);
        }
        for (const auto& resource : res.separate_samplers) {
            shader.descriptor_type_bindings.emplace(
                comp.get_decoration(resource.id, spv::Decoration::DecorationBinding),
                vk::DescriptorType::eSampler);
        }

        vk::PipelineShaderStageCreateInfo stage_info;
        stage_info.stage = convertShaderStage(shader.stage);
        stage_info.module = shader.module;
        stage_info.pName = shader.entry_point.get();

        program.stages[stage_info.stage] = std::move(shader);
        program.pipeline_stages.push_back(stage_info);
    }

    // Create descriptor set layout.
    std::map<u32, vk::DescriptorSetLayoutBinding> descriptor_bindings_map;
    for (const auto& stage : program.stages) {
        for (const auto& b : stage.second.descriptor_type_bindings) {
            auto it = descriptor_bindings_map.find(b.first);
            if (it != descriptor_bindings_map.end()) {
                if (it->second.descriptorType != b.second) {
                    logger_.error(
                        "Attempting to bind a descriptor of type {} to binding {} which is "
                        "already bound to descriptor type {}, ignoring.",
                        vk::to_string(b.second), b.first, vk::to_string(it->second.descriptorType));
                    continue;
                }
                // Binding already exists, append this shader stage type.
                it->second.stageFlags |= stage.first;
            } else {
                vk::DescriptorSetLayoutBinding layout_binding;
                layout_binding.binding = b.first;
                layout_binding.descriptorType = b.second;
                layout_binding.descriptorCount = 1;
                layout_binding.stageFlags = stage.first;
                layout_binding.pImmutableSamplers = nullptr;
                descriptor_bindings_map.emplace(b.first, layout_binding);
            }
        }
    }

    program.layout_bindings.reserve(descriptor_bindings_map.size());
    for (const auto& binding_entry : descriptor_bindings_map) {
        program.layout_bindings.push_back(binding_entry.second);
    }

    // Create descriptor set layout.
    vk::DescriptorSetLayoutCreateInfo layout_info;
    layout_info.bindingCount = program.layout_bindings.size();
    layout_info.pBindings = program.layout_bindings.data();
    program.descriptor_set_layout = vk_device_.createDescriptorSetLayout(layout_info);

    // Merge resources from each shader stage. We've already done error checking above.
    std::map<u32, ShaderVK::StructLayout> uniform_buffer_bindings;
    for (const auto& stage : program.stages) {
        uniform_buffer_bindings.insert(stage.second.uniform_buffer_bindings.begin(),
                                       stage.second.uniform_buffer_bindings.end());
    }

    // Loop through uniform buffer bindings and discover uniforms.
    for (const auto& binding : uniform_buffer_bindings) {
        // logger_.info("Uniform buffer binding {} {} is {} bytes", binding.first,
        // binding.second.name, binding.second.size);

        ProgramVK::UniformBuffer ubo;
        ubo.binding = binding.first;
        ubo.size = binding.second.size;
        program.uniform_buffers.push_back(ubo);

        // Find uniform locations.
        for (const auto& field : binding.second.fields) {
            // logger_.info("- member {} is {} bytes and has an offset of {}", field.name,
            // field.size, field.offset);
            std::string qualified_name =
                (binding.second.name.empty() ? "" : binding.second.name + ".") + field.name;
            program.uniform_locations.emplace(
                std::move(qualified_name),
                ProgramVK::Uniform{binding.first, field.offset, field.size, {}});
        }
    }

    program_map_.emplace(c.handle, std::move(program));
}

void RenderContextVK::operator()(const cmd::DeleteProgram& c) {
    auto& program = program_map_.at(c.handle);
    vk_device_.destroy(program.descriptor_set_layout);
    for (const auto& stage : program.stages) {
        vk_device_.destroy(stage.second.module);
    }
    program_map_.erase(c.handle);
}

void RenderContextVK::operator()(const cmd::CreateTexture2D& c) {
    TextureVK texture;

    texture.image_format = kTextureFormatMap.at(usize(c.format)).format;

    vk::DeviceSize buffer_size = c.width * c.height * 4;
    assert(c.data.size() <= buffer_size);

    texture.aspect_mask = vk::ImageAspectFlagBits::eColor;

    if (c.framebuffer_usage) {
        device_->createImage(
            static_cast<u32>(c.width), static_cast<u32>(c.height), texture.image_format,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal, texture.image, texture.image_memory);
        texture.image_layout = vk::ImageLayout::eUndefined;
    } else {
        // Create image by copying to a staging buffer.
        vk::Buffer staging_buffer;
        vk::DeviceMemory staging_buffer_memory;
        device_->createBuffer(
            buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            staging_buffer, staging_buffer_memory);

        void* data = vk_device_.mapMemory(staging_buffer_memory, 0, buffer_size);
        memcpy(data, c.data.data(), static_cast<std::size_t>(c.data.size()));
        vk_device_.unmapMemory(staging_buffer_memory);

        // Create image.
        device_->createImage(
            static_cast<u32>(c.width), static_cast<u32>(c.height), texture.image_format,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal, texture.image, texture.image_memory);

        device_->transitionImageLayout(texture.image, texture.image_format,
                                       vk::ImageLayout::eUndefined,
                                       vk::ImageLayout::eTransferDstOptimal);
        device_->copyBufferToImage(staging_buffer, texture.image, static_cast<u32>(c.width),
                                   static_cast<u32>(c.height));
        device_->transitionImageLayout(texture.image, texture.image_format,
                                       vk::ImageLayout::eTransferDstOptimal,
                                       vk::ImageLayout::eShaderReadOnlyOptimal);
        texture.image_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

        vk_device_.destroy(staging_buffer);
        vk_device_.free(staging_buffer_memory);
    }

    // Create image view.
    texture.image_view = device_->createImageView(texture.image, texture.image_format,
                                                  vk::ImageAspectFlagBits::eColor);

    texture_map_.emplace(c.handle, std::move(texture));
}

void RenderContextVK::operator()(const cmd::DeleteTexture& c) {
}

void RenderContextVK::operator()(const cmd::CreateFrameBuffer& c) {
    std::vector<TextureVK*> textures;
    textures.reserve(c.textures.size());
    for (const auto& texture_handle : c.textures) {
        textures.push_back(&texture_map_.at(texture_handle));
    }
    FramebufferVK framebuffer{device_.get(), c.width, c.height, std::move(textures)};
    framebuffer_map_.emplace(c.handle, std::move(framebuffer));
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
    auto all_layers = vk::enumerateInstanceLayerProperties();
    std::ostringstream extension_list;
    extension_list << "Vulkan extensions supported:";
    for (const auto& extension : all_extensions) {
        extension_list << " " << extension.extensionName;
    }
    logger_.info(extension_list.str());
    std::ostringstream layer_list;
    layer_list << "Vulkan layers available:";
    for (const auto& layer : all_layers) {
        layer_list << " " << layer.layerName;
    }
    logger_.info(layer_list.str());

    vk::ApplicationInfo app_info;
    app_info.pApplicationName = "RenderContextVK";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "dawn-gfx";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_1;

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

    vk::PhysicalDevice physical_device;
    std::vector<vk::PhysicalDevice> physical_devices = instance_.enumeratePhysicalDevices();
    for (const auto& device : physical_devices) {
        auto properties = device.getProperties();
        logger_.info("Considering GPU: {}", properties.deviceName);
    }
    for (const auto& device : physical_devices) {
        if (is_device_suitable(device)) {
            physical_device = device;
            break;
        }
    }
    if (!physical_device) {
        throw std::runtime_error("failed to find a suitable GPU.");
    }
    logger_.info("Selected GPU: {}", physical_device.getProperties().deviceName);
    auto indices = QueueFamilyIndices::fromPhysicalDevice(physical_device, surface_);
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
    vk_device_ = physical_device.createDevice(create_info);

    // Get queue handles.
    graphics_queue_ = vk_device_.getQueue(indices.graphics_family.value(), 0);
    present_queue_ = vk_device_.getQueue(indices.present_family.value(), 0);

    // Create command pool.
    vk::CommandPoolCreateInfo pool_info;
    pool_info.queueFamilyIndex = graphics_queue_family_index_;
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    vk::CommandPool command_pool = vk_device_.createCommandPool(pool_info);

    // Create device wrapper.
    device_ =
        std::make_unique<DeviceVK>(physical_device, vk_device_, command_pool, graphics_queue_);
}

void RenderContextVK::createSwapChain() {
    SwapChainSupportDetails swap_chain_support =
        SwapChainSupportDetails::querySupport(device_->getPhysicalDevice(), surface_);
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

    u32 queue_family_indices[] = {graphics_queue_family_index_, present_queue_family_index_};
    if (graphics_queue_family_index_ != present_queue_family_index_) {
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
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

    swap_chain_ = vk_device_.createSwapchainKHR(create_info);
    swap_chain_images_ = vk_device_.getSwapchainImagesKHR(swap_chain_);
    swap_chain_image_format_ = create_info.imageFormat;
    swap_chain_extent_ = create_info.imageExtent;

    // Image views.
    swap_chain_image_views_.reserve(swap_chain_images_.size());
    for (const auto& swap_chain_image : swap_chain_images_) {
        swap_chain_image_views_.push_back(device_->createImageView(
            swap_chain_image, swap_chain_image_format_, vk::ImageAspectFlagBits::eColor));
    }

    // Depth.
    depth_format_ = vk::Format::eD32Sfloat;
    device_->createImage(swap_chain_extent_.width, swap_chain_extent_.height, depth_format_,
                         vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                         vk::MemoryPropertyFlagBits::eDeviceLocal, depth_image_,
                         depth_image_memory_);
    depth_image_view_ =
        device_->createImageView(depth_image_, depth_format_, vk::ImageAspectFlagBits::eDepth);
}

void RenderContextVK::createRenderPass() {
    // Colour attachment.
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

    // Depth attachment.
    vk::AttachmentDescription depth_attachment;
    depth_attachment.format = depth_format_;
    depth_attachment.samples = vk::SampleCountFlagBits::e1;
    depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
    depth_attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    vk::AttachmentReference depth_attachment_ref;
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    // Subpass.
    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colour_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

    std::array<vk::AttachmentDescription, 2> attachments = {colour_attachment, depth_attachment};
    vk::RenderPassCreateInfo render_pass_info;
    render_pass_info.attachmentCount = attachments.size();
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    swapchain_render_pass_ = vk_device_.createRenderPass(render_pass_info);
}

void RenderContextVK::createFramebuffers() {
    swap_chain_framebuffers_.reserve(swap_chain_image_views_.size());
    for (const auto& image_view : swap_chain_image_views_) {
        std::array<vk::ImageView, 2> attachments = {image_view, depth_image_view_};

        vk::FramebufferCreateInfo framebuffer_info;
        framebuffer_info.renderPass = swapchain_render_pass_;
        framebuffer_info.attachmentCount = attachments.size();
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = swap_chain_extent_.width;
        framebuffer_info.height = swap_chain_extent_.height;
        framebuffer_info.layers = 1;
        swap_chain_framebuffers_.push_back(vk_device_.createFramebuffer(framebuffer_info));
    }
}

void RenderContextVK::createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocate_info;
    allocate_info.commandPool = device_->getCommandPool();
    allocate_info.level = vk::CommandBufferLevel::ePrimary;
    allocate_info.commandBufferCount = static_cast<u32>(swap_chain_framebuffers_.size());
    command_buffers_ = vk_device_.allocateCommandBuffers(allocate_info);
}

void RenderContextVK::createDescriptorPool() {
    vk::DescriptorPoolSize dps[] = {
        {vk::DescriptorType::eCombinedImageSampler, (10 * DW_MAX_TEXTURE_SAMPLERS) << 10},
        {vk::DescriptorType::eSampledImage, (10 * DW_MAX_TEXTURE_SAMPLERS) << 10},
        {vk::DescriptorType::eSampler, (10 * DW_MAX_TEXTURE_SAMPLERS) << 10},
        {vk::DescriptorType::eUniformBufferDynamic, 10 << 10},
        {vk::DescriptorType::eStorageBuffer, DW_MAX_TEXTURE_SAMPLERS << 10},
        {vk::DescriptorType::eStorageImage, DW_MAX_TEXTURE_SAMPLERS << 10},
    };

    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.poolSizeCount = sizeof(dps) / sizeof(dps[0]);
    poolInfo.pPoolSizes = dps;
    poolInfo.maxSets = 10 << 10;

    descriptor_pool_ = vk_device_.createDescriptorPool(poolInfo);
}

void RenderContextVK::createSyncObjects() {
    image_available_semaphores_.reserve(kMaxFramesInFlight);
    render_finished_semaphores_.reserve(kMaxFramesInFlight);
    in_flight_fences_.reserve(kMaxFramesInFlight);
    for (int i = 0; i < kMaxFramesInFlight; ++i) {
        image_available_semaphores_.push_back(
            vk_device_.createSemaphore(vk::SemaphoreCreateInfo{}));
        render_finished_semaphores_.push_back(
            vk_device_.createSemaphore(vk::SemaphoreCreateInfo{}));

        vk::FenceCreateInfo fence_create_info;
        fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;
        in_flight_fences_.push_back(vk_device_.createFence(fence_create_info));
    }

    images_in_flight_.resize(swap_chain_images_.size());
}

PipelineVK RenderContextVK::findOrCreateGraphicsPipeline(PipelineVK::Info info) {
    auto cached_pipeline = graphics_pipeline_cache_.find(info);
    if (cached_pipeline != graphics_pipeline_cache_.end()) {
        return cached_pipeline->second;
    }

    // Cache miss. Create a new graphics pipeline.
    PipelineVK graphics_pipeline;

    // Create fixed function pipeline stages.
    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<u32>(info.decl->attribute_descriptions.size());
    vertex_input_info.pVertexBindingDescriptions = &info.decl->binding_description;
    vertex_input_info.pVertexAttributeDescriptions = info.decl->attribute_descriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport;
    vk::Extent2D viewport_extent = info.framebuffer ? info.framebuffer->extent : swap_chain_extent_;
    viewport.width = static_cast<float>(viewport_extent.width);
    viewport.height = -static_cast<float>(viewport_extent.height);
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(viewport_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = viewport_extent;

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
    rasterizer.cullMode = info.render_item->cull_face_enabled ? vk::CullModeFlagBits::eNone
                                                              : vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = info.render_item->cull_front_face == CullFrontFace::CW
                               ? vk::FrontFace::eClockwise
                               : vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    std::vector<vk::PipelineColorBlendAttachmentState> colour_blend_attachments;
    usize colour_attachment_count = info.framebuffer ? info.framebuffer->images.size() : 1;
    for (usize i = 0; i < colour_attachment_count; ++i) {
        vk::PipelineColorBlendAttachmentState colour_blend_attachment;
        if (info.render_item->colour_write) {
            colour_blend_attachment.colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        }
        colour_blend_attachment.blendEnable = info.render_item->blend_enabled;
        colour_blend_attachment.srcColorBlendFactor =
            kBlendFuncMap.at(info.render_item->blend_src_rgb);
        colour_blend_attachment.dstColorBlendFactor =
            kBlendFuncMap.at(info.render_item->blend_dest_rgb);
        colour_blend_attachment.colorBlendOp =
            kBlendEquationMap.at(info.render_item->blend_equation_rgb);
        colour_blend_attachment.srcAlphaBlendFactor =
            kBlendFuncMap.at(info.render_item->blend_src_a);
        colour_blend_attachment.dstAlphaBlendFactor =
            kBlendFuncMap.at(info.render_item->blend_dest_a);
        colour_blend_attachment.alphaBlendOp =
            kBlendEquationMap.at(info.render_item->blend_equation_a);
        colour_blend_attachments.push_back(colour_blend_attachment);
    }

    vk::PipelineColorBlendStateCreateInfo colour_blending;
    colour_blending.logicOpEnable = VK_FALSE;
    colour_blending.logicOp = vk::LogicOp::eCopy;
    colour_blending.attachmentCount = colour_blend_attachments.size();
    colour_blending.pAttachments = colour_blend_attachments.data();
    colour_blending.blendConstants[0] = 0.0f;
    colour_blending.blendConstants[1] = 0.0f;
    colour_blending.blendConstants[2] = 0.0f;
    colour_blending.blendConstants[3] = 0.0f;

    vk::DynamicState dynamic_states[] = {vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamic_state;
    dynamic_state.dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]);
    dynamic_state.pDynamicStates = dynamic_states;

    vk::PipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &info.program->descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    graphics_pipeline.layout = vk_device_.createPipelineLayout(pipeline_layout_info);

    // Depth / Stencil.
    vk::PipelineDepthStencilStateCreateInfo depth_stencil;
    depth_stencil.depthTestEnable = info.render_item->depth_enabled ? VK_TRUE : VK_FALSE;
    depth_stencil.depthWriteEnable = info.render_item->depth_write ? VK_TRUE : VK_FALSE;
    depth_stencil.depthCompareOp = vk::CompareOp::eLess;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = vk::StencilOpState{};
    depth_stencil.back = vk::StencilOpState{};

    // Create pipeline.
    vk::GraphicsPipelineCreateInfo pipeline_info;
    pipeline_info.stageCount = info.program->pipeline_stages.size();
    pipeline_info.pStages = info.program->pipeline_stages.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &colour_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = graphics_pipeline.layout;
    pipeline_info.renderPass =
        info.framebuffer ? info.framebuffer->render_pass : swapchain_render_pass_;
    pipeline_info.subpass = 0;

    graphics_pipeline.pipeline =
        vk_device_.createGraphicsPipelines(vk::PipelineCache{}, pipeline_info)[0];

    graphics_pipeline_cache_.emplace(info, graphics_pipeline);
    return graphics_pipeline;
}

DescriptorSetVK RenderContextVK::findOrCreateDescriptorSet(DescriptorSetVK::Info info) {
    auto cached_descriptor_set = descriptor_set_cache_.find(info);
    if (cached_descriptor_set != descriptor_set_cache_.end()) {
        return cached_descriptor_set->second;
    }

    // Cache miss. Create a new descriptor set.
    std::vector<vk::DescriptorSetLayout> layouts(swap_chain_images_.size(),
                                                 info.program->descriptor_set_layout);
    vk::DescriptorSetAllocateInfo alloc_info;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = layouts.size();
    alloc_info.pSetLayouts = layouts.data();
    std::vector<vk::DescriptorSet> descriptor_sets = vk_device_.allocateDescriptorSets(alloc_info);

    // Write to them.
    for (usize i = 0; i < swap_chain_images_.size(); ++i) {
        std::vector<vk::WriteDescriptorSet> descriptor_writes;
        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> buffer_info_storage;
        std::vector<std::unique_ptr<vk::DescriptorImageInfo>> image_info_storage;

        for (const auto& binding : info.program->layout_bindings) {
            vk::WriteDescriptorSet descriptor_write;
            descriptor_write.dstSet = descriptor_sets[i];
            descriptor_write.dstBinding = binding.binding;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = binding.descriptorType;
            descriptor_write.descriptorCount = 1;
            switch (binding.descriptorType) {
                case vk::DescriptorType::eUniformBufferDynamic: {
                    buffer_info_storage.emplace_back(std::make_unique<vk::DescriptorBufferInfo>());
                    auto& buffer_info = *buffer_info_storage.back();

                    // TODO: Consider storing the uniform buffers in a more convenient way that
                    // avoids the O(n) search, although for almost all cases linear search shouldn't
                    // matter as we won't really ever have more than 10 uniform buffers.
                    auto uniform_buffer_it = std::find_if(
                        info.program->uniform_buffers.begin(), info.program->uniform_buffers.end(),
                        [binding = binding.binding](const ProgramVK::UniformBuffer& buffer) {
                            return buffer.binding == binding;
                        });
                    assert(uniform_buffer_it != info.program->uniform_buffers.end());

                    buffer_info.buffer = uniform_scratch_buffers_[i]->getBuffer();
                    buffer_info.offset = 0;
                    buffer_info.range = uniform_buffer_it->size;
                    descriptor_write.pBufferInfo = &buffer_info;
                } break;
                case vk::DescriptorType::eCombinedImageSampler: {
                    // TODO: Consider storing resource bindings in a more convenient way that
                    // avoids the O(n) search, although for almost all cases linear search shouldn't
                    // matter as we won't really ever have more than 10 uniform buffers.
                    auto texture_binding_it = std::find_if(
                        info.textures.begin(), info.textures.end(),
                        [binding = binding.binding](const RenderItem::TextureBinding& texture) {
                            return texture.binding_location == binding;
                        });
                    if (texture_binding_it == info.textures.end()) {
                        logger_.error("Binding location {} requires a texture to be bound.",
                                      binding.binding);
                        continue;
                    }

                    const auto& texture_info = *texture_binding_it;
                    const auto& texture = texture_map_.at(texture_info.handle);

                    image_info_storage.emplace_back(std::make_unique<vk::DescriptorImageInfo>());
                    auto& image_info = *image_info_storage.back();

                    image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                    image_info.imageView = texture.image_view;
                    image_info.sampler = findOrCreateSampler(texture_info.sampler_info);
                    descriptor_write.pImageInfo = &image_info;
                } break;
                default:
                    logger_.error("Unhandled descriptor type {}",
                                  vk::to_string(binding.descriptorType));
            }
            descriptor_writes.push_back(descriptor_write);
        }

        vk_device_.updateDescriptorSets(descriptor_writes, {});
    }

    DescriptorSetVK descriptor_set{std::move(descriptor_sets)};
    descriptor_set_cache_.emplace(info, descriptor_set);
    return descriptor_set;
}

vk::Sampler RenderContextVK::findOrCreateSampler(RenderItem::SamplerInfo info) {
    auto cached_sampler = sampler_cache_.find(info);
    if (cached_sampler != sampler_cache_.end()) {
        return cached_sampler->second;
    }

    // Cache miss. Create a new sampler.

    // Parse sampler flags.
    const std::unordered_map<u32, vk::SamplerAddressMode> wrapping_mode_map = {
        {0b01, vk::SamplerAddressMode::eRepeat},
        {0b10, vk::SamplerAddressMode::eMirroredRepeat},
        {0b11, vk::SamplerAddressMode::eClampToEdge}};
    const std::unordered_map<u32, vk::Filter> filter_map = {{0b01, vk::Filter::eNearest},
                                                            {0b10, vk::Filter::eLinear}};
    const std::unordered_map<u32, vk::SamplerMipmapMode> mipmap_mode = {
        {0b00, vk::SamplerMipmapMode::eNearest},
        {0b01, vk::SamplerMipmapMode::eNearest},
        {0b10, vk::SamplerMipmapMode::eLinear}};
    auto flags = info.sampler_flags;
    auto u_wrapping_mode =
        (flags & SamplerFlag::maskUWrappingMode) >> SamplerFlag::shiftUWrappingMode;
    auto v_wrapping_mode =
        (flags & SamplerFlag::maskVWrappingMode) >> SamplerFlag::shiftVWrappingMode;
    auto w_wrapping_mode =
        (flags & SamplerFlag::maskWWrappingMode) >> SamplerFlag::shiftWWrappingMode;
    auto min_filter = (flags & SamplerFlag::maskMinFilter) >> SamplerFlag::shiftMinFilter;
    auto mag_filter = (flags & SamplerFlag::maskMagFilter) >> SamplerFlag::shiftMagFilter;
    auto mip_filter = (flags & SamplerFlag::maskMipFilter) >> SamplerFlag::shiftMipFilter;

    // Create sampler object.
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = filter_map.at(mag_filter);
    samplerInfo.minFilter = filter_map.at(min_filter);
    samplerInfo.addressModeU = wrapping_mode_map.at(u_wrapping_mode);
    samplerInfo.addressModeV = wrapping_mode_map.at(v_wrapping_mode);
    samplerInfo.addressModeW = wrapping_mode_map.at(w_wrapping_mode);
    // TODO: Enable anisotropy
    // samplerInfo.anisotropyEnable = VK_TRUE;
    // samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = mipmap_mode.at(mip_filter);
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vk::Sampler sampler = vk_device_.createSampler(samplerInfo);
    sampler_cache_.emplace(info, sampler);
    return sampler;
}

void RenderContextVK::cleanup() {
    vk_device_.waitIdle();

    // Clear cached objects.
    for (const auto& entry : sampler_cache_) {
        vk_device_.destroy(entry.second);
    }
    sampler_cache_.clear();
    descriptor_set_cache_.clear();
    for (const auto& entry : graphics_pipeline_cache_) {
        vk_device_.destroy(entry.second.layout);
        vk_device_.destroy(entry.second.pipeline);
    }
    graphics_pipeline_cache_.clear();
    vertex_decl_cache_.clear();

    // Free resources.
    for (const auto& entry : framebuffer_map_) {
        vk_device_.destroy(entry.second.render_pass);
        vk_device_.destroy(entry.second.framebuffer);
        vk_device_.destroy(entry.second.depth.image_view);
        vk_device_.destroy(entry.second.depth.image);
        vk_device_.free(entry.second.depth.image_memory);
    }
    framebuffer_map_.clear();
    for (const auto& entry : texture_map_) {
        vk_device_.destroy(entry.second.image_view);
        vk_device_.destroy(entry.second.image);
        vk_device_.free(entry.second.image_memory);
    }
    texture_map_.clear();
    for (const auto& entry : program_map_) {
        vk_device_.destroy(entry.second.descriptor_set_layout);
        for (const auto& stage : entry.second.stages) {
            vk_device_.destroy(stage.second.module);
        }
    }
    program_map_.clear();
    index_buffer_map_.clear();
    vertex_buffer_map_.clear();

    uniform_scratch_buffers_.clear();
    vk_device_.destroy(descriptor_pool_);

    // Destroy swapchain.
    for (const auto& fence : in_flight_fences_) {
        vk_device_.destroy(fence);
    }
    images_in_flight_.clear();
    in_flight_fences_.clear();
    for (const auto& semaphore : render_finished_semaphores_) {
        vk_device_.destroy(semaphore);
    }
    render_finished_semaphores_.clear();
    for (const auto& semaphore : image_available_semaphores_) {
        vk_device_.destroy(semaphore);
    }
    image_available_semaphores_.clear();

    vk_device_.destroy(swapchain_render_pass_);
    for (const auto& framebuffer : swap_chain_framebuffers_) {
        vk_device_.destroy(framebuffer);
    }
    swap_chain_framebuffers_.clear();

    vk_device_.destroy(depth_image_view_);
    vk_device_.destroy(depth_image_);
    vk_device_.free(depth_image_memory_);

    for (const auto& image_view : swap_chain_image_views_) {
        vk_device_.destroy(image_view);
    }
    swap_chain_image_views_.clear();
    swap_chain_images_.clear();
    vk_device_.destroy(swap_chain_);

    // Destroy device and instance.
    device_.reset();
    instance_.destroy(surface_);
    if (debug_messenger_) {
        instance_.destroy(debug_messenger_);
    }
    instance_.destroy();
}
}  // namespace gfx
}  // namespace dw
