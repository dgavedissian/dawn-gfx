/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "detail/MathGeoLib.h"
#include <cmath>
#ifdef M_PI
#undef M_PI
#endif

namespace dw {
struct Vec2i {
    int x;
    int y;
};

static const float M_PI = 3.14159265358979323846264338327950288f;
static const float M_HALF_PI = M_PI * 0.5f;
static const float M_EPSILON = std::numeric_limits<float>::epsilon();
static const float M_LARGE_EPSILON = 0.00005f;
static const float M_LARGE_VALUE = 100000000.0f;
static const float M_INFINITY = HUGE_VALF;
static const float M_DEGTORAD = M_PI / 180.0f;
static const float M_DEGTORAD_OVER_2 = M_PI / 360.0f;
static const float M_RADTODEG = 1.0f / M_DEGTORAD;

// Vectors.
using Vec2 = math::float2;
using Vec3 = math::float3;
using Vec4 = math::float4;

// Matrices, stored in row-major format (mat[r][c]).
using Mat3x3 = math::float3x3;
using Mat3x4 = math::float3x4;
using Mat4x4 = math::float4x4;
using Mat3 = Mat3x3;
using Mat4 = Mat4x4;

// Quaternion
using Quat = math::Quat;

// Plane
using Plane = math::Plane;
}