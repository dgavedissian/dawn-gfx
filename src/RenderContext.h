/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#pragma once

#include "Renderer.h"

namespace dw {
namespace gfx {
class DW_API RenderContext {
public:
    virtual ~RenderContext() = default;

    // Window management. Executed on the main thread.
    // TODO: Make this return a Window object instead.
    virtual tl::expected<void, std::string> createWindow(u16 width, u16 height,
                                                         const std::string& title) = 0;
    virtual void destroyWindow() = 0;
    virtual void processEvents() = 0;
    virtual bool isWindowClosed() const = 0;
    virtual Vec2i windowSize() const = 0;
    virtual Vec2 windowScale() const = 0;
    virtual Vec2i backbufferSize() const = 0;

    // Command buffer processing. Executed on the render thread.
    virtual void startRendering() = 0;
    virtual void stopRendering() = 0;
    virtual void processCommandList(std::vector<RenderCommand>& command_list) = 0;
    virtual bool frame(const Frame* frame) = 0;
};
}
}  // namespace dw
