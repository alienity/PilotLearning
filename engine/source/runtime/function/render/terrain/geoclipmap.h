// Copyright © 2023 Cory Petkovsek, Roope Palmroos, and Contributors.

#ifndef GEOCLIPMAP_CLASS_H
#define GEOCLIPMAP_CLASS_H

#include "runtime/function/render/render_common.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/core/log/log_system.h"

namespace MoYu
{
    struct GeoClipPatch
    {
        AxisAlignedBox aabb;
        std::vector<glm::float3> vertices;
        std::vector<uint32_t> indices;
    };

    class GeoClipMap
    {
    private:
        static inline int _patch_2d(int x, int y, int res);

    public:
        enum MeshType
        {
            TILE,
            FILLER,
            TRIM,
            CROSS,
            SEAM,
        };

        static std::vector<GeoClipPatch> generate(int p_resolution, int p_clipmap_levels);
    };

    // Inline Functions

    inline int GeoClipMap::_patch_2d(int x, int y, int res) { return y * res + x; }

}

#endif // GEOCLIPMAP_CLASS_H