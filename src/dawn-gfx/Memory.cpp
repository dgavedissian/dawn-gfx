/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#include "dawn-gfx/detail/Memory.h"

namespace dw {
Memory::Memory() : data_{nullptr}, size_{0} {
}

Memory::Memory(usize size) : size_{size} {
    if (size > 0) {
        data_.reset(new std::byte[size], std::default_delete<std::byte[]>());
    }
}

std::byte& Memory::operator[](size_t index) const {
    return data_.get()[index];
}

std::byte* Memory::data() const {
    return data_.get();
}

usize Memory::size() const {
    return size_;
}
}  // namespace dw
