/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#include "TriangleBuffer.h"

namespace dw {
TriangleBuffer::TriangleBuffer()
    : contains_normals_(false), contains_texcoords_(false) {
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
}

Mesh TriangleBuffer::end(Renderer& r) {
    // Set up vertex data.
    Memory data;
    VertexDecl decl;
    if (contains_normals_ && contains_texcoords_) {
        data = Memory(vertices_);
        decl.begin()
            .add(VertexDecl::Attribute::Position, 3, VertexDecl::AttributeType::Float)
            .add(VertexDecl::Attribute::Normal, 3, VertexDecl::AttributeType::Float)
            .add(VertexDecl::Attribute::TexCoord0, 2, VertexDecl::AttributeType::Float)
            .end();
    } else {
        // Build VertexDecl.
        decl.begin();
        decl.add(VertexDecl::Attribute::Position, 3, VertexDecl::AttributeType::Float);
        if (contains_normals_) {
            decl.add(VertexDecl::Attribute::Normal, 3, VertexDecl::AttributeType::Float,
                     true);
        }
        if (contains_texcoords_) {
            decl.add(VertexDecl::Attribute::TexCoord0, 2,
                     VertexDecl::AttributeType::Float);
        }
        decl.end();

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
            assert(offset == stride);
        }
    }

    // Upload to GPU.
    Mesh result;
    result.vb = r.createVertexBuffer(std::move(data), decl);
    result.vertex_count = static_cast<uint>(vertices_.size());
    result.ib = r.createIndexBuffer(Memory(indices_), IndexBufferType::U32);
    result.index_count = static_cast<uint>(indices_.size());
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

void TriangleBuffer::triangle(u32 v0, u32 v1, u32 v2) {
    indices_.emplace_back(v0);
    indices_.emplace_back(v1);
    indices_.emplace_back(v2);
}
}  // namespace dw
