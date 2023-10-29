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

    namespace Geometry
    {

        struct TriangleMesh : BasicMesh
        {
            TriangleMesh(float width);

            static BasicMesh ToBasicMesh(float width = 1.0f);
        };

        struct SquareMesh : BasicMesh
        {
            SquareMesh(float width);

            static BasicMesh ToBasicMesh(float width = 1.0f);
        };

    } // namespace Geometry

    namespace TerrainGeometry
    {
        // https://zhuanlan.zhihu.com/p/352850047
        // 地块的mesh是宽度为4的mesh，一边有5个顶点
        struct TerrainPatchMesh
        {
            TerrainPatchMesh(float scale);

            std::vector<D3D12TerrainPatch> vertices;
            std::vector<int> indices;
        };

        TerrainPatchMesh ToTerrainPatchMesh(float scale = 1.0);

        AxisAlignedBox ToAxisAlignedBox(const TerrainPatchMesh& basicMesh);
        StaticMeshData ToStaticMesh(const TerrainPatchMesh& basicMesh);


    }

}
#endif