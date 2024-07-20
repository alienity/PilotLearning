#pragma once
#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
#define DirectionLightParameterName "DirectionLightParameter"
#define PointLightParameterName "PointLightParameter"
#define SpotLightParameterName "SpotLightParameter"

    struct DirectionLightParameter
    {
        Color color = Color::White;
        float intensity {1.0f};
        
        float maxShadowDistance{ 500.0f };
        float cascadeShadowSplit0{ 0.05f };
        float cascadeShadowSplit1{ 0.15f };
        float cascadeShadowSplit2{ 0.3f };
        float cascadeShadowBorder0{ 0.2f };
        float cascadeShadowBorder1{ 0.2f };
        float cascadeShadowBorder2{ 0.2f };
        float cascadeShadowBorder3{ 0.2f };

        bool    shadows {false};
        int     cascade {4};
        glm::float2 shadow_bounds {512, 512};
        float   shadow_near_plane {0.1f};
        float   shadow_far_plane {200.0f};
        glm::float2 shadowmap_size {2048, 2048};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DirectionLightParameter,
                                                    color,
                                                    intensity,
                                                    maxShadowDistance,
                                                    cascadeShadowSplit0,
                                                    cascadeShadowSplit1,
                                                    cascadeShadowSplit2,
                                                    cascadeShadowBorder0,
                                                    cascadeShadowBorder1,
                                                    cascadeShadowBorder2,
                                                    cascadeShadowBorder3,
                                                    shadows,
                                                    cascade,
                                                    shadow_bounds,
                                                    shadow_near_plane,
                                                    shadow_far_plane,
                                                    shadowmap_size)

    struct PointLightParameter
    {
        Color color = Color::White;
        float intensity {1.0f};

        float falloff_radius {5.0f};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PointLightParameter, color, intensity, falloff_radius)

    struct SpotLightParameter
    {
        Color color = Color::White;
        float intensity {1.0f};

        float falloff_radius {5.0f};

        float spotAngle {120.0f};
        float innerSpotPercent{0.9f};

        bool    shadows {false};
        glm::float2 shadow_bounds {32, 32};
        float   shadow_near_plane {0.1f};
        float   shadow_far_plane {32.0f};
        glm::float2 shadowmap_size {256, 256};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpotLightParameter,
                                       color,
                                       intensity,
                                       falloff_radius,
                                       spotAngle,
                                       innerSpotPercent,
                                       shadows,
                                       shadow_bounds,
                                       shadow_near_plane,
                                       shadow_far_plane,
                                       shadowmap_size)

    struct LightComponentRes
    {
        std::string             m_LightParamName {};
        DirectionLightParameter m_DirectionLightParam {};
        PointLightParameter     m_PointLightParam {};
        SpotLightParameter      m_SpotLightParam {};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LightComponentRes,
                                       m_LightParamName,
                                       m_DirectionLightParam,
                                       m_PointLightParam,
                                       m_SpotLightParam)

} // namespace MoYu