#pragma once
#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
    struct TransformRes
    {
        glm::float3     m_position {MYFloat3::Zero};
        glm::float3     m_scale {MYFloat3::One};
        glm::quat m_rotation {MYQuaternion::Identity};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TransformRes, m_position, m_scale, m_rotation)
} // namespace MoYu