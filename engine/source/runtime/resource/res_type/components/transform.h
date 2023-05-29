#pragma once
#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
    struct TransformRes
    {
        Vector3    m_position {Vector3::Zero};
        Vector3    m_scale {Vector3::One};
        Quaternion m_rotation {Quaternion::Identity};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TransformRes, m_position, m_scale, m_rotation)
} // namespace MoYu