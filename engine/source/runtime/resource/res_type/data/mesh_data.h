#pragma once
#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
    struct Vertex
    {
        float px;
        float py;
        float pz;

        float nx;
        float ny;
        float nz;
        
        float tx;
        float ty;
        float tz;
        float tw;

        float u;
        float v;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vertex, px, py, pz, nx, ny, nz, tx, ty, tz, tw, u, v)

    struct SkeletonBinding
    {
        int   index0;
        int   index1;
        int   index2;
        int   index3;

        float weight0;
        float weight1;
        float weight2;
        float weight3;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SkeletonBinding, index0, index1, index2, index3, weight0, weight1, weight2, weight3)

    struct MeshData
    {
        std::vector<Vertex>          vertex_buffer;
        std::vector<uint32_t>        index_buffer;
        std::vector<SkeletonBinding> skeleton_bind;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MeshData, vertex_buffer, index_buffer, skeleton_bind)

} // namespace MoYu