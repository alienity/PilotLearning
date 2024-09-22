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
        typedef D3D12MeshVertexStandard Vertex;

        Vertex VertexInterpolate(Vertex& v0, Vertex& v1, float t);
        Vertex VertexVectorBuild(Vertex& init, Vertex& temp);
        Vertex VertexTransform(Vertex& v0, glm::mat4x4& matrix);

        struct BasicMesh
        {
            std::vector<Vertex> vertices;
            std::vector<int>    indices;
        };

        AABB ToAxisAlignedBox(const BasicMesh& basicMesh);
        StaticMeshData ToStaticMesh(const BasicMesh& basicMesh);

    }

    namespace Geometry
    {

        struct TriangleMesh : BasicMesh
        {
            TriangleMesh(float width = 1.0f);

            static BasicMesh ToBasicMesh(float width = 1.0f);
        };

        struct SquareMesh : BasicMesh
        {
            SquareMesh(float width = 1.0f);

            static BasicMesh ToBasicMesh(float width = 1.0f);
        };

        struct TerrainPlane : BasicMesh
        {
            TerrainPlane(int gridCount = 16, float meshSize = 8);

            static TerrainPlane ToBasicMesh(int meshGridCount = 16, float meshSize = 8);
        };


    } // namespace Geometry

    /*
    namespace TerrainGeometry
    {
        // https://zhuanlan.zhihu.com/p/352850047
        // 地块的mesh是宽度为4的mesh，一边有5个顶点
        struct TerrainPatchMesh
        {
            TerrainPatchMesh(uint32_t totallevel, float scale, uint32_t treeIndex, glm::u64vec2 bitOffset, glm::float2 localOffset);

            uint32_t     totalLevel; // tree的总共级数
            float        localScale; // 当前patch的缩放
            float        localMaxHeight; // 当前patch的最大高度
            uint32_t     linearTreeIndex; // 在展开的tree中的位置
            glm::u64vec2 localTreePosBit; // 标识在tree中的具体位置，按位标识位置
            glm::float2  localOffset; // 当前patch在大地图中的世界坐标偏移

            std::vector<D3D12TerrainPatch> vertices;
            std::vector<int> indices;
        };

        AABB ToAxisAlignedBox(const TerrainPatchMesh& basicMesh);
        StaticMeshData ToStaticMesh(const TerrainPatchMesh& basicMesh);
    }
    */
}
#endif