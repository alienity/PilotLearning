#pragma once

#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
    struct WorldRes
    {
        // world name
        std::string m_name;

        // all level urls for this world
        std::vector<std::string> m_level_urls;

        // the default level for this world, which should be first loading level
        std::string m_default_level_url;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WorldRes, m_name, m_level_urls, m_default_level_url)
} // namespace MoYu
