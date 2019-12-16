/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Base.h"
#include <vector>
#include <utility>
#include <cstddef>

namespace dw {
// Vertex Declaration.
class DW_API VertexDecl {
public:
    enum class Attribute { Position, Normal, Colour, TexCoord0 };

    enum class AttributeType { Float, Uint8 };

    VertexDecl();
    ~VertexDecl() = default;

    VertexDecl& begin();
    VertexDecl& add(Attribute attribute, usize count, AttributeType type, bool normalised = false);
    VertexDecl& end();

    u16 stride() const;
    bool empty() const;

private:
    static u16 encodeAttributes(Attribute attribute, usize count, AttributeType type,
                                bool normalised);
    static void decodeAttributes(u16 encoded_attribute, Attribute& attribute, usize& count,
                                 AttributeType& type, bool& normalised);
    static u16 attributeTypeSize(AttributeType type);

    // Attribute: 7
    // Count: 3
    // AttributeType: 5
    // Normalised: 1
    std::vector<std::pair<u16, std::byte*>> attributes_;
    u16 stride_;

    friend class RHIRenderer;
    friend class GLRenderContext;
};
}  // namespace dw
