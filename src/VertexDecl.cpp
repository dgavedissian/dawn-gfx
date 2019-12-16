/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#include "VertexDecl.h"
#include <cassert>

namespace dw {
VertexDecl::VertexDecl() : stride_(0) {
}

VertexDecl& VertexDecl::begin() {
    return *this;
}

VertexDecl& VertexDecl::add(VertexDecl::Attribute attribute, usize count,
                            VertexDecl::AttributeType type, bool normalised) {
    attributes_.emplace_back(
        std::make_pair(encodeAttributes(attribute, count, type, normalised),
                       reinterpret_cast<std::byte*>(static_cast<std::uintptr_t>(stride_))));
    stride_ += static_cast<u16>(count) * attributeTypeSize(type);
    return *this;
}

VertexDecl& VertexDecl::end() {
    return *this;
}

u16 VertexDecl::stride() const {
    return stride_;
}

bool VertexDecl::empty() const {
    return stride_ == 0;
}

u16 VertexDecl::encodeAttributes(VertexDecl::Attribute attribute, usize count,
                                 VertexDecl::AttributeType type, bool normalised) {
    // Attribute: 7
    // Count: 3
    // AttributeType: 5
    // Normalised: 1
    return static_cast<u16>((static_cast<u16>(attribute) << 9) |
                            ((static_cast<u16>(count) & 0x7) << 6) |
                            ((static_cast<u16>(type) & 0x1F) << 1) | (normalised ? 1 : 0));
}

void VertexDecl::decodeAttributes(u16 encoded_attribute, Attribute& attribute, usize& count,
                                  AttributeType& type, bool& normalised) {
    // Attribute: 7
    // Count: 3
    // AttributeType: 5
    // Normalised: 1
    attribute = static_cast<Attribute>(encoded_attribute >> 9);
    count = static_cast<usize>((encoded_attribute >> 6) & 0x7);
    type = static_cast<AttributeType>((encoded_attribute >> 1) & 0x1F);
    normalised = (encoded_attribute & 0x1) == 1;
}

u16 VertexDecl::attributeTypeSize(AttributeType type) {
    switch (type) {
        case AttributeType::Uint8:
            return sizeof(u8);
        case AttributeType::Float:
            return sizeof(float);
        default:
            assert(false);
            return 0;
    }
}
}  // namespace dw
