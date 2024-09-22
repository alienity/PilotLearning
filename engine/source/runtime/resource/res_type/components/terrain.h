#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include <vector>

namespace MoYu
{
    struct TerrainMetrialBaseTexture
    {
        MaterialImage m_albedo_texture_file {};
        MaterialImage m_ao_roughness_metal_file {};
        MaterialImage m_displacement_file {};
        MaterialImage m_normal_file {};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TerrainMetrialBaseTexture,
                                                    m_albedo_texture_file,
                                                    m_ao_roughness_metal_file,
                                                    m_normal_file,
                                                    m_displacement_file)

    struct TerrainMaterialRes
    {
        std::vector<TerrainMetrialBaseTexture> m_terrain_base_textures;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TerrainMaterialRes, m_terrain_base_textures)

    struct TerrainComponentRes
    {
        glm::int3 terrain_size {glm::int3(10240, 2048, 10240)}; // 10240*2048*10240

        //int max_node_id = 34124; //5x5+10x10+20x20+40x40+80x80+160x160 - 1
        int max_terrain_lod = 5; // 最大的lod等级
        int max_lod_node_count = 5; // 每级lod上拥有的node数

        int patch_mesh_grid_count = 16; // PatchMesh由16x16网格组成
        int patch_mesh_size = 8; // PatchMesh边长8米
        int patch_count_per_node = 8; // Node拆成8x8个Patch

        //float patch_mesh_grid_size = 0.5f; // patch_mesh_size / patch_mesh_grid_count
        //int sector_count_terrain = 160; // terrain_size.x / (patch_mesh_size * patch_count_per_node);

        SceneImage m_heightmap_file {};
        SceneImage m_normalmap_file {};

        TerrainMaterialRes m_terrainMaterial {};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TerrainComponentRes,
                                                    terrain_size,
                                                    max_terrain_lod,
                                                    max_lod_node_count,
                                                    patch_mesh_grid_count,
                                                    patch_mesh_size,
                                                    patch_count_per_node,
                                                    m_heightmap_file,
                                                    m_normalmap_file,
                                                    m_terrainMaterial)

} // namespace MoYu