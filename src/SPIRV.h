/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

// Mark this header as a system header
#if defined(DGA_GCC) || defined(DGA_CLANG)
#pragma GCC system_header
#elif defined(DGA_MSVC)
#pragma warning(push, 0)
#endif

#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>

// Re-enable warnings
#if defined(DGA_MSVC)
#pragma warning(pop)
#endif
