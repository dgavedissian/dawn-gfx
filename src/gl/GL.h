/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

// Mark this header as a system header
#if defined(DW_GCC) || defined(DW_CLANG)
#pragma GCC system_header
#elif defined(DW_MSVC)
#pragma warning(push, 0)
#endif

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Re-enable warnings
#if defined(DW_MSVC)
#pragma warning(pop)
#endif
