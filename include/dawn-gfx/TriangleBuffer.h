/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#pragma once

#include "MathDefs.h"
#include "Renderer.h"
#include <utility>
#include <vector>

namespace dw {
namespace gfx {
struct Mesh {
    VertexBufferHandle vb;
    IndexBufferHandle ib;
    uint vertex_count;
    uint index_count;
};

Vec3 calculateTangent(const Vec3& p1, const Vec3& p2, const Vec3& p3, const Vec2& tc1, const Vec2& tc2, const Vec2& tc3);

class DW_API TriangleBuffer {
public:
    TriangleBuffer();
    ~TriangleBuffer() = default;

    // Estimations.
    void estimateVertexCount(uint count);
    void estimateIndexCount(uint count);

    // Begin creating new geometry.
    void begin();

    // Compile the vertex and index arrays into GPU buffers.
    Mesh end(Renderer& r);

    // Add a vertex.
    void position(const Vec3& p);
    void normal(const Vec3& n);
    void texcoord(const Vec2& tc);
    void tangent(const Vec3& t);

    // Add a triangle.
    void triangle(u32 v0, u32 v1, u32 v2);

    // Calculate tangents.
    void calculateTangents();

private:
    struct Vertex {
        Vec3 position = {0.0f, 0.0f, 0.0f};
        Vec3 normal = {0.0f, 0.0f, 0.0f};
        Vec2 tex_coord = {0.0f, 0.0f};
        Vec3 tangent = {0.0f, 0.0f, 0.0f};
    };
    bool contains_normals_;
    bool contains_texcoords_;
    bool contains_tangents_;
    std::vector<Vertex> vertices_;
    std::vector<u32> indices_;
};
}  // namespace dw
}
