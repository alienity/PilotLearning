#pragma once

#include "runtime/core/meta/json.h"

#include <string>
#include <vector>

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
