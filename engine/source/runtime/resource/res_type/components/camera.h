#pragma once
#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
#define FirstPersonCameraParameterName "FirstPersonCameraParameter"
#define ThirdPersonCameraParameterName "ThirdPersonCameraParameter"
#define FreeCameraParameterName "FreeCameraParameter"

    struct FirstPersonCameraParameter
    {
        float m_fovY {100.0f};
        float m_vertical_offset {0.6f};
        int   m_width {1366};
        int   m_height {768};
        float m_nearZ {0.1f};
        float m_farZ {100.0f};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FirstPersonCameraParameter,
                                       m_fovY,
                                       m_vertical_offset,
                                       m_width,
                                       m_height,
                                       m_nearZ,
                                       m_farZ)

    struct ThirdPersonCameraParameter
    {
        float      m_fovY {100.f};
        float      m_horizontal_offset {3.f};
        float      m_vertical_offset {2.5f};
        glm::quat m_cursor_pitch {MYQuaternion::Identity};
        glm::quat m_cursor_yaw {MYQuaternion::Identity};
        int        m_width {1366};
        int        m_height {768};
        float      m_nearZ {0.1f};
        float      m_farZ {100.0f};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThirdPersonCameraParameter,
                                       m_fovY,
                                       m_horizontal_offset,
                                       m_vertical_offset,
                                       m_cursor_pitch,
                                       m_cursor_yaw,
                                       m_width,
                                       m_height,
                                       m_nearZ,
                                       m_farZ)

    struct FreeCameraParameter
    {
        bool  m_perspective {true};
        float m_fovY {100.f};
        float m_speed {1.f};
        int   m_width {1366};
        int   m_height {768};
        float m_nearZ {0.1f};
        float m_farZ {100.0f};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FreeCameraParameter, m_perspective, m_fovY, m_speed, m_width, m_height, m_nearZ, m_farZ)

    struct CameraComponentRes
    {
        std::string                m_CamParamName {FreeCameraParameterName};
        FirstPersonCameraParameter m_FirstPersonCamParam {};
        ThirdPersonCameraParameter m_ThirdPersonCamParam {};
        FreeCameraParameter        m_FreeCamParam {};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CameraComponentRes,
                                       m_CamParamName,
                                       m_FirstPersonCamParam,
                                       m_ThirdPersonCamParam,
                                       m_FreeCamParam)
} // namespace MoYu