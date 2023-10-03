#pragma once
#ifndef GEOMETRY_MESH_TOOL_H
#define GEOMETRY_MESH_TOOL_H
#include "runtime/function/render/render_common.h"
#include "runtime/function/render/glm_wrapper.h"

#include <vector>

namespace MoYu
{
    namespace Geometry
    {
        typedef D3D12MeshVertexPositionNormalTangentTexture Vertex;

        static Vertex VertexInterpolate(Vertex& v0, Vertex& v1, float t);
        static Vertex VertexVectorBuild(Vertex& init, Vertex& temp);
        static Vertex VertexTransform(Vertex& v0, glm::mat4x4& matrix);

        struct BasicMesh
        {
            std::vector<Vertex> vertices;
            std::vector<int>    indices;
        };

    }

}
#endif