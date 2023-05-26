#pragma once
#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
    struct Box
    {
        Vector3 m_half_extents {0.5f, 0.5f, 0.5f};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Box, m_half_extents)

    struct Sphere
    {
        float m_radius {0.5f};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Sphere, m_radius)

    struct Capsule
    {
        float m_radius {0.3f};
        float m_half_height {0.7f};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Capsule, m_radius, m_half_height)

} // namespace MoYu