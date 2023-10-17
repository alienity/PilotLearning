#pragma once
#ifndef GEOMETRY_MESH_TOOL_H
#define GEOMETRY_MESH_TOOL_H
#include "runtime/function/render/render_common.h"
#include "runtime/core/math/moyu_math2.h"

#include <vector>

namespace MoYu
{
    namespace Geometry
    {
        typedef D3D12MeshVertexPositionNormalTangentTexture Vertex;

        Vertex VertexInterpolate(Vertex& v0, Vertex& v1, float t);
        Vertex VertexVectorBuild(Vertex& init, Vertex& temp);
        Vertex VertexTransform(Vertex& v0, glm::mat4x4& matrix);

        struct BasicMesh
        {
            std::vector<Vertex> vertices;
            std::vector<int>    indices;
        };

        AxisAlignedBox ToAxisAlignedBox(const BasicMesh& basicMesh);
        StaticMeshData ToStaticMesh(const BasicMesh& basicMesh);

    }

}
#endif