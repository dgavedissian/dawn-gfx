/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "dawn-gfx/detail/Memory.h"

namespace dw {
namespace gfx {
Memory::Memory() : data_{nullptr}, size_{0} {
}

Memory::Memory(usize size) : size_{size} {
    if (size > 0) {
        data_.reset(new byte[size], std::default_delete<byte[]>());
    }
}

byte& Memory::operator[](size_t index) const {
    return data_.get()[index];
}

byte* Memory::data() const {
    return data_.get();
}

usize Memory::size() const {
    return size_;
}
}  // namespace gfx
}  // namespace dw
