/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "Colour.h"

namespace dw {
namespace gfx {
Colour::Colour() : components_{0.0f, 0.0f, 0.0f, 1.0f} {
}

Colour::Colour(float r, float g, float b, float a) : components_{r, g, b, a} {
}

float Colour::r() const {
    return components_.x;
}

float Colour::g() const {
    return components_.y;
}

float Colour::b() const {
    return components_.z;
}

float Colour::a() const {
    return components_.w;
}

Vec3 Colour::rgb() const {
    return Vec3{components_.x, components_.y, components_.z};
}

Vec4 Colour::rgba() const {
    return components_;
}

float& Colour::r() {
    return components_.x;
}

float& Colour::g() {
    return components_.y;
}

float& Colour::b() {
    return components_.z;
}

float& Colour::a() {
    return components_.w;
}

Vec4& Colour::rgba() {
    return components_;
}
}  // namespace gfx
}  // namespace dw
