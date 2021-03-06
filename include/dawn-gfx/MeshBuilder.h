/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "Base.h"
#include "Renderer.h"
#include "TriangleBuffer.h"

namespace dw {
namespace gfx {
class DW_API MeshBuilder {
public:
    explicit MeshBuilder(Renderer& r);
    ~MeshBuilder() = default;

    MeshBuilder& normals(bool normals);
    MeshBuilder& texcoords(bool texcoords);
    MeshBuilder& tangents(bool tangents);

    Mesh createPlane(float width, float height);
    Mesh createBox(float half_size);
    Mesh createSphere(float radius, uint rings = 25, uint segments = 25);

private:
    Renderer& r_;
    bool with_normals_;
    bool with_texcoords_;
    bool with_tangents_;
};
}
}  // namespace dw
