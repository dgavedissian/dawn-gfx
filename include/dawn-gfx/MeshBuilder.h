/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#pragma once

#include "Base.h"
#include "Renderer.h"
#include "TriangleBuffer.h"

namespace dw {
class DW_API MeshBuilder {
public:
    explicit MeshBuilder(Renderer& r);
    ~MeshBuilder() = default;

    MeshBuilder& normals(bool normals);
    MeshBuilder& texcoords(bool texcoords);

    Mesh createPlane(float width, float height);
    Mesh createBox(float half_size);
    Mesh createSphere(float radius, uint rings = 25, uint segments = 25);

private:
    Renderer& r_;
    bool with_normals_;
    bool with_texcoords_;
};
}  // namespace dw