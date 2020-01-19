/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Renderer.h"
#include "RenderContext.h"

namespace dw {
namespace gfx {
class RenderContextNull : public RenderContext {
public:
    explicit RenderContextNull(Logger& logger);
    ~RenderContextNull() override = default;

    RendererType type() const override {
        return RendererType::Null;
    }

    // Capabilities / customisations.
    Mat4 adjustProjectionMatrix(Mat4 projection_matrix) const override;
    bool hasFlippedViewport() const override;

    // Window management. Executed on the main thread.
    tl::expected<void, std::string> createWindow(u16 width, u16 height,
                                                 const std::string& title,
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
};
}
}  // namespace dw
