/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#pragma once

#include <vector>
#include "../Base.h"

// Mark this header as a system header
#if defined(DGA_GCC) || defined(DGA_CLANG)
#pragma GCC system_header
#elif defined(DGA_MSVC)
#pragma warning(push, 0)
#endif

// Fixed width types for MathGeoLib
#define u8 dw::gfx::u8
#define u16 dw::gfx::u16
#define u32 dw::gfx::u32
#define u64 dw::gfx::u64
#define s8 dw::gfx::i8
#define s16 dw::gfx::i16
#define s32 dw::gfx::i32
#define s64 dw::gfx::i64

// MathGeoLib
#define MATH_ENABLE_STL_SUPPORT
#define MATH_ENABLE_UNCOMMON_OPERATIONS
#define LOGGING_SUPPORT_DISABLED
#define KNET_NO_FIXEDWIDTH_TYPES
#include <MathGeoLib.h>
#ifdef LOG
#undef LOG
#endif

// Undef fixed width types
#undef u8
#undef u16
#undef u32
#undef u64
#undef s8
#undef s16
#undef s32
#undef s64

// Re-enable warnings
#if defined(DGA_MSVC)
#pragma warning(pop)
#endif
