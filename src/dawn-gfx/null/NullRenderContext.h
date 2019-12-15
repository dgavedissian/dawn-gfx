/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "RHIRenderer.h"

namespace dw {
class DW_API NullRenderContext : public RenderContext {
public:
    NullRenderContext();
    virtual ~NullRenderContext();

    // Window management. Executed on the main thread.
    tl::expected<void, std::string> createWindow(u16 width, u16 height, const std::string& title) override;
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
};
}  // namespace dw
