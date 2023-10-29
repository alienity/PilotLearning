#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"
#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_guid_allocator.h"
#include "runtime/resource/basic_geometry/mesh_tools.h"

#include <vector>

namespace MoYu
{
    class TerrainRenderer
    {
    public:
        void InitTerrainRenderer();


        float GetTerrainHeight(glm::float2 localXZ);
        glm::float3 GetTerrainNormal(glm::float2 localXZ);

        std::vector<TerrainGeometry::TerrainPatchMesh> _TerrainPatchQuadTree;
        uint32_t _TerrainQuadLevel;

        void GeneratePatch(uint32_t level, glm::int2 offset);

        glm::float4x4 local2WorldMatrix;
        glm::float4x4 world2LocalMatrix;

        glm::uint terrainSize;      // 1024*1024
        glm::uint terrainMaxHeight; // 1024

        TerrainGeometry::TerrainPatchMesh _BasicPatchMesh;

        std::shared_ptr<MoYu::MoYuScratchImage> _terrain_height_map;
        std::shared_ptr<MoYu::MoYuScratchImage> _terrain_normal_map;

        TerrainConfigs _terrainConfig;
    };
} // namespace MoYu
