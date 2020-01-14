/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2018 (git@dga.me.uk)
 */
#include "null/RenderContextNull.h"

namespace dw {
namespace gfx {
RenderContextNull::RenderContextNull(Logger& logger) : RenderContext{logger} {
}

tl::expected<void, std::string> RenderContextNull::createWindow(u16, u16, const std::string&,
                                                                InputCallbacks) {
    return {};
}

void RenderContextNull::destroyWindow() {
}

void RenderContextNull::processEvents() {
}

bool RenderContextNull::isWindowClosed() const {
    return false;
}

Vec2i RenderContextNull::windowSize() const {
    return {0, 0};
}

Vec2 RenderContextNull::windowScale() const {
    return {1.0f, 1.0f};
}

Vec2i RenderContextNull::framebufferSize() const {
    return {0, 0};
}

void RenderContextNull::startRendering() {
}

void RenderContextNull::stopRendering() {
}

void RenderContextNull::prepareFrame() {
}

void RenderContextNull::processCommandList(std::vector<RenderCommand>&) {
}

bool RenderContextNull::frame(const Frame*) {
    return true;
}
}  // namespace gfx
}  // namespace dw