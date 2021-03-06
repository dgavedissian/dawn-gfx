/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Renderer.h"
#include "Input.h"
#include <functional>

namespace dw {
namespace gfx {
class RenderContext {
public:
    RenderContext(Logger& logger) : logger_{logger} {
    }
    virtual ~RenderContext() = default;

    virtual RendererType type() const = 0;

    // Capabilities / customisations.
    virtual Mat4 adjustProjectionMatrix(Mat4 projection_matrix) const = 0;
    virtual bool hasFlippedViewport() const = 0;

    // Window management. Executed on the main thread.
    virtual Result<void, std::string> createWindow(u16 width, u16 height,
                                                         const std::string& title,
                                                         InputCallbacks input_callbacks) = 0;
    virtual void destroyWindow() = 0;
    virtual void processEvents() = 0;
    virtual bool isWindowClosed() const = 0;
    virtual Vec2i windowSize() const = 0;
    virtual Vec2 windowScale() const = 0;
    virtual Vec2i framebufferSize() const = 0;

    // Command buffer processing. Executed on the render thread.
    virtual void startRendering() = 0;
    virtual void stopRendering() = 0;
    virtual void prepareFrame() = 0;
    virtual void processCommandList(std::vector<RenderCommand>& command_list) = 0;
    virtual bool frame(const Frame* frame) = 0;

protected:
    Logger& logger_;
};
}  // namespace gfx
}  // namespace dw
