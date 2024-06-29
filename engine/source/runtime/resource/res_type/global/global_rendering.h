#pragma once
#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
    struct DirLight
    {
        glm::float3 m_direction;
        glm::float3 m_color;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DirLight, m_direction, m_color)

    struct CameraPose
    {
        glm::float3 m_position;
        glm::float3 m_target;
        glm::float3 m_up;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CameraPose, m_position, m_target, m_up)

    struct CameraConfig
    {
        CameraPose m_pose;
        glm::float2    m_aspect;
        float      m_z_far;
        float      m_z_near;
        float      m_fovY;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CameraConfig, m_pose, m_aspect, m_z_far, m_z_near, m_fovY)

    struct CubeMap
    {
        std::string m_negative_x_map;
        std::string m_positive_x_map;
        std::string m_negative_y_map;
        std::string m_positive_y_map;
        std::string m_negative_z_map;
        std::string m_positive_z_map;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CubeMap,
                                       m_negative_x_map,
                                       m_positive_x_map,
                                       m_negative_y_map,
                                       m_positive_y_map,
                                       m_negative_z_map,
                                       m_positive_z_map)

    struct IBLTexs
    {
        std::string m_dfg_map;
        std::string m_ld_map;
        std::string m_irradians_map;
        std::string _PreIntegratedFGD_GGXDisneyDiffuse;
        std::string _PreIntegratedFGD_CharlieAndFabric;

        std::vector<glm::float4> m_SH;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(IBLTexs, m_dfg_map, m_ld_map, m_irradians_map,
        _PreIntegratedFGD_GGXDisneyDiffuse, _PreIntegratedFGD_CharlieAndFabric, m_SH)

    struct BlueNoiseTexs
    {
        std::string m_bluenoise_map;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BlueNoiseTexs, m_bluenoise_map)

    struct VolumeCloudTexs
    {
        std::string m_weather2d;
        std::string m_cloud3d;
        std::string m_worley3d;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VolumeCloudTexs, m_weather2d, m_cloud3d, m_worley3d)


    struct GlobalRenderingRes
    {
        bool          m_enable_fxaa {false};
        CubeMap       m_skybox_irradiance_map;
        CubeMap       m_skybox_specular_map;
        IBLTexs       m_ibl_map;
        std::string   m_brdf_map;
        std::string   m_color_grading_map;
        Color         m_sky_color;
        Color         m_ambient_light;
        CameraConfig  m_camera_config;
        DirLight      m_directional_light;
        BlueNoiseTexs m_bluenoises;
        VolumeCloudTexs m_volume_clouds;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GlobalRenderingRes,
                                       m_enable_fxaa,
                                       m_skybox_irradiance_map,
                                       m_skybox_specular_map,
                                       m_ibl_map,
                                       m_brdf_map,
                                       m_color_grading_map,
                                       m_sky_color,
                                       m_ambient_light,
                                       m_camera_config,
                                       m_directional_light,
                                       m_bluenoises,
                                       m_volume_clouds)
} // namespace MoYu