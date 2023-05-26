#pragma once
#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{

    struct FirstPersonCameraParameter
    {
        float m_fov {50.f};
        float m_vertical_offset {0.6f};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FirstPersonCameraParameter, m_fov, m_vertical_offset)

    struct ThirdPersonCameraParameter
    {
        float      m_fov {50.f};
        float      m_horizontal_offset {3.f};
        float      m_vertical_offset {2.5f};
        Quaternion m_cursor_pitch {Quaternion::Identity};
        Quaternion m_cursor_yaw {Quaternion::Identity};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ThirdPersonCameraParameter,
                                       m_fov,
                                       m_horizontal_offset,
                                       m_vertical_offset,
                                       m_cursor_pitch,
                                       m_cursor_yaw)

    struct FreeCameraParameter
    {
        float m_fov {50.f};
        float m_speed {1.f};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FreeCameraParameter, m_fov, m_speed)

    struct CameraComponentRes
    {
        std::string                m_CamParamName;
        FirstPersonCameraParameter m_FirstPersonCamParam;
        ThirdPersonCameraParameter m_ThirdPersonCamParam;
        FreeCameraParameter        m_FreeCamParam;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CameraComponentRes,
                                       m_CamParamName,
                                       m_FirstPersonCamParam,
                                       m_ThirdPersonCamParam,
                                       m_FreeCamParam)
} // namespace MoYu