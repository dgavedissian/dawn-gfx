/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#pragma once

// Mark this header as a system header
#if defined(DGA_GCC) || defined(DGA_CLANG)
#pragma GCC system_header
#elif defined(DGA_MSVC)
#pragma warning(push, 0)
#endif

#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>

// Re-enable warnings
#if defined(DGA_MSVC)
#pragma warning(pop)
#endif
