#pragma once

#include "runtime/resource/res_type/common_serializer.h"
#include "runtime/resource/res_type/common/object.h"

namespace MoYu
{
    struct LevelRes
    {
        float m_gravity {9.8f};

        std::vector<ObjectInstanceRes> m_objects;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LevelRes, m_gravity, m_objects)
} // namespace MoYu