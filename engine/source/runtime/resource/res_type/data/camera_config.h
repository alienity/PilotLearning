#pragma once
#include "runtime/core/meta/reflection/reflection.h"

#include "runtime/core/math/moyu_math.h"

namespace MoYu
{
    struct CameraPose
    {
        Vector3 m_position;
        Vector3 m_target;
        Vector3 m_up;
    };

    struct CameraConfig
    {
        CameraPose m_pose;
        Vector2    m_aspect;
        float      m_z_far;
        float      m_z_near;
    };
} // namespace MoYu