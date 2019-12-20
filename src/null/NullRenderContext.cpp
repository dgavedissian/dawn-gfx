/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2018 (git@dga.me.uk)
 */
#include "null/NullRenderContext.h"

namespace dw {
namespace gfx {
NullRenderContext::NullRenderContext(Logger& logger) : RenderContext{logger} {
}

tl::expected<void, std::string> NullRenderContext::createWindow(u16, u16, const std::string&,
                                                                InputCallbacks) {
    return {};
}

void NullRenderContext::destroyWindow() {
}

void NullRenderContext::processEvents() {
}

bool NullRenderContext::isWindowClosed() const {
    return false;
}

Vec2i NullRenderContext::windowSize() const {
    return {0, 0};
}

Vec2 NullRenderContext::windowScale() const {
    return {1.0f, 1.0f};
}

Vec2i NullRenderContext::backbufferSize() const {
    return {0, 0};
}

void NullRenderContext::startRendering() {
}

void NullRenderContext::stopRendering() {
}

void NullRenderContext::processCommandList(std::vector<RenderCommand>&) {
}

bool NullRenderContext::frame(const Frame*) {
    return true;
}
}  // namespace gfx
}  // namespace dw