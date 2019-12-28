/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#include "TriangleBuffer.h"

namespace dw {
namespace gfx {
Vec3 calculateTangent(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec2& tc1,
                      const Vec2& tc2, const Vec2& tc3) {
    Vec3 edge1 = p2 - p1;
    Vec3 edge2 = p3 - p1;
    Vec2 deltaUV1 = tc2 - tc1;
    Vec2 deltaUV2 = tc3 - tc2;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
    Vec3 tangent;
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    tangent.Normalize();
    return tangent;
}

TriangleBuffer::TriangleBuffer() : contains_normals_(false), contains_texcoords_(false) {
}

void TriangleBuffer::estimateVertexCount(uint count) {
    vertices_.reserve(count);
}

void TriangleBuffer::estimateIndexCount(uint count) {
    indices_.reserve(count);
}

void TriangleBuffer::begin() {
    vertices_.clear();
    indices_.clear();
    contains_normals_ = false;
    contains_texcoords_ = false;
    contains_tangents_ = false;
}

Mesh TriangleBuffer::end(Renderer& r) {
    // Set up vertex data.
    Memory data;
    VertexDecl decl;
    decl.begin();
    decl.add(VertexDecl::Attribute::Position, 3, VertexDecl::AttributeType::Float);
    if (contains_normals_) {
        decl.add(VertexDecl::Attribute::Normal, 3, VertexDecl::AttributeType::Float, true);
    }
    if (contains_texcoords_) {
        decl.add(VertexDecl::Attribute::TexCoord0, 2, VertexDecl::AttributeType::Float);
    }
    if (contains_tangents_) {
        decl.add(VertexDecl::Attribute::Tangent, 3, VertexDecl::AttributeType::Float, true);
    }
    decl.end();
    if (contains_normals_ && contains_texcoords_ && contains_tangents_) {
        // We can just copy over the vertices directly.
        data = Memory(vertices_);
    } else {
        // Build packed buffer based on parameters.
        data = Memory(vertices_.size() * decl.stride());
        const uint stride =
            decl.stride() / sizeof(float);  // convert stride in bytes to stride in floats.
        auto* packed_data = reinterpret_cast<float*>(data.data());
        for (size_t i = 0; i < vertices_.size(); ++i) {
            uint offset = 0;
            Vertex& source_vertex = vertices_[i];
            float* vertex = &packed_data[i * stride];
            // Copy data.
            vertex[offset++] = source_vertex.position.x;
            vertex[offset++] = source_vertex.position.y;
            vertex[offset++] = source_vertex.position.z;
            if (contains_normals_) {
                vertex[offset++] = source_vertex.normal.x;
                vertex[offset++] = source_vertex.normal.y;
                vertex[offset++] = source_vertex.normal.z;
            }
            if (contains_texcoords_) {
                vertex[offset++] = source_vertex.tex_coord.x;
                vertex[offset++] = source_vertex.tex_coord.y;
            }
            if (contains_tangents_) {
                vertex[offset++] = source_vertex.tangent.x;
                vertex[offset++] = source_vertex.tangent.y;
                vertex[offset++] = source_vertex.tangent.z;
            }
            assert(offset == stride);
        }
    }

    // Upload to GPU.
    Mesh result{r.createVertexBuffer(std::move(data), decl),
                r.createIndexBuffer(Memory(indices_), IndexBufferType::U32),
                static_cast<uint>(vertices_.size()), static_cast<uint>(indices_.size())};
    return result;
}

void TriangleBuffer::position(const Vec3& p) {
    vertices_.emplace_back();
    vertices_.back().position = p;
}

void TriangleBuffer::normal(const Vec3& n) {
    vertices_.back().normal = n;
    contains_normals_ = true;
}

void TriangleBuffer::texcoord(const Vec2& tc) {
    vertices_.back().tex_coord = tc;
    contains_texcoords_ = true;
}

void TriangleBuffer::tangent(const Vec3& t) {
    vertices_.back().tangent = t;
    contains_tangents_ = true;
}

void TriangleBuffer::triangle(u32 v0, u32 v1, u32 v2) {
    indices_.emplace_back(v0);
    indices_.emplace_back(v1);
    indices_.emplace_back(v2);
}

void TriangleBuffer::calculateTangents() {
    assert(indices_.size() % 3 == 0);
    // Calculating tangents relies on texcoords.
    if (!contains_texcoords_) {
        return;
    }
    for (usize i = 0; i < indices_.size(); i += 3) {
        auto& v1 = vertices_[indices_[i]];
        auto& v2 = vertices_[indices_[i + 1]];
        auto& v3 = vertices_[indices_[i + 2]];
        v1.tangent = v2.tangent = v3.tangent = calculateTangent(
            v1.position, v2.position, v3.position, v1.tex_coord, v2.tex_coord, v3.tex_coord);
    }
    contains_tangents_ = true;
}
}  // namespace gfx
}  // namespace dw
