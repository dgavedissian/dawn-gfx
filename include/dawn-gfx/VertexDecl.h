/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Base.h"
#include <vector>
#include <utility>
#include <cstddef>

#include <dga/hash_combine.h>

namespace dw {
namespace gfx {
// Vertex Declaration.
class DW_API VertexDecl {
public:
    enum class Attribute { Position, Normal, Colour, TexCoord0, Tangent };

    enum class AttributeType { Float, Uint8 };

    VertexDecl();
    ~VertexDecl() = default;

    VertexDecl& begin();
    // TODO: Combine type, count and normalised, so it's a compiler error if the user gets these wrong.
    // TODO: Alternatively, combine type and normalised, and make count an enum.
    VertexDecl& add(Attribute attribute, usize count, AttributeType type, bool normalised = false);
    VertexDecl& end();

    bool operator==(const VertexDecl& other) const;
    bool operator!=(const VertexDecl& other) const;

    u16 stride() const;
    bool empty() const;

    // TODO: Remove this weird encoding magic. Not sure why it's like this.
    static u16 encodeAttributes(Attribute attribute, usize count, AttributeType type,
                                bool normalised);
    static void decodeAttributes(u16 encoded_attribute, Attribute& attribute, usize& count,
                                 AttributeType& type, bool& normalised);
    static u16 attributeTypeSize(AttributeType type);

    // Attribute: 7
    // Count: 3
    // AttributeType: 5
    // Normalised: 1
    std::vector<std::pair<u16, byte*>> attributes_;
    u16 stride_;
};
}  // namespace gfx
}  // namespace dw

template <> struct std::hash<dw::gfx::VertexDecl> {
    std::size_t operator()(const dw::gfx::VertexDecl& decl) const {
        std::size_t hash = 0;
        dga::hashCombine(hash, decl.stride_);
        for (const auto& entry : decl.attributes_) {
            dga::hashCombine(hash, entry.first, entry.second);
        }
        return hash;
    }
};
