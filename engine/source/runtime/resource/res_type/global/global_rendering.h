#pragma once
#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
    struct CameraPose
    {
        Vector3 m_position;
        Vector3 m_target;
        Vector3 m_up;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CameraPose, m_position, m_target, m_up)

    struct CameraConfig
    {
        CameraPose m_pose;
        Vector2    m_aspect;
        float      m_z_far;
        float      m_z_near;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CameraConfig, m_pose, m_aspect, m_z_far, m_z_near)

    struct CubeMap
    {
        std::string m_negative_x_map;
        std::string m_positive_x_map;
        std::string m_negative_y_map;
        std::string m_positive_y_map;
        std::string m_negative_z_map;
        std::string m_positive_z_map;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CubeMap,
                                       m_negative_x_map,
                                       m_positive_x_map,
                                       m_negative_y_map,
                                       m_positive_y_map,
                                       m_negative_z_map,
                                       m_positive_z_map)

    struct GlobalRenderingRes
    {
        bool         m_enable_fxaa {false};
        CubeMap      m_skybox_irradiance_map;
        CubeMap      m_skybox_specular_map;
        std::string  m_brdf_map;
        std::string  m_color_grading_map;
        Color        m_sky_color;
        Color        m_ambient_light;
        CameraConfig m_camera_config;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GlobalRenderingRes,
                                       m_enable_fxaa,
                                       m_skybox_irradiance_map,
                                       m_skybox_specular_map,
                                       m_brdf_map,
                                       m_color_grading_map,
                                       m_sky_color,
                                       m_ambient_light,
                                       m_camera_config)
} // namespace MoYu