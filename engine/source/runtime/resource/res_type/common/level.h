#pragma once

#include "runtime/resource/res_type/common/object.h"

namespace MoYu
{
    class LevelRes
    {
    public:
        float m_gravity {9.8f};

        std::vector<ObjectInstanceRes> m_objects;
    };
} // namespace MoYu