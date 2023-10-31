#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include <vector>

namespace MoYu
{
    struct TerrainComponentRes
    {
        glm::int2 terrain_size; // 1024*1024
        int terrain_max_height; // 1024
        
        std::string m_heightmap_file {""};
        std::string m_normalmap_file {""};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TerrainComponentRes, terrain_size, terrain_max_height, m_heightmap_file, m_normalmap_file)

} // namespace MoYu