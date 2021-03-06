/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include <dga/aliases.h>
#include <dga/platform.h>
#include <dga/result.h>

namespace dw {
namespace gfx {
using dga::u8;
using dga::u16;
using dga::u32;
using dga::u64;
using dga::i8;
using dga::i16;
using dga::i32;
using dga::i64;
using dga::usize;
using dga::uint;
using dga::byte;

using dga::Result;
using dga::Error;
}
}

#ifndef DW_API
#define DW_API DGA_API
#endif
