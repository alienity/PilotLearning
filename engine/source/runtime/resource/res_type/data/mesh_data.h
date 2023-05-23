#pragma once
#include "runtime/core/math/moyu_math.h"

#include <string>
#include <vector>
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
        float u;
        float v;
    };

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

    struct MeshData
    {
        std::vector<Vertex>          vertex_buffer;
        std::vector<int>             index_buffer;
        std::vector<SkeletonBinding> bind;
    };

} // namespace MoYu