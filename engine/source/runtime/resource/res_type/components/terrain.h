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
        glm::int2 terrain_size {glm::int2(1024)}; // 1024*1024
        int terrain_max_height {1024}; // 1024
        
        SceneImage m_heightmap_file {};
        SceneImage m_normalmap_file {};

        TerrainMaterialRes m_terrainMaterial {};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TerrainComponentRes,
                                                    terrain_size,
                                                    terrain_max_height,
                                                    m_heightmap_file,
                                                    m_normalmap_file,
                                                    m_terrainMaterial)

} // namespace MoYu