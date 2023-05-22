#pragma once
#include "runtime/core/math/moyu_math.h"
#include "runtime/core/color/color.h"
#include "runtime/core/meta/reflection/reflection.h"

namespace MoYu
{
    REFLECTION_TYPE(LightParameter)
    CLASS(LightParameter, Fields)
    {
        REFLECTION_BODY(LightParameter);

    public:
        Color color = Color::White;
        float intensity {1.0f};

        virtual ~LightParameter() {}
    };

    REFLECTION_TYPE(DirectionalLightParameter)
    CLASS(DirectionalLightParameter : public LightParameter, Fields)
    {
        REFLECTION_BODY(DirectionalLightParameter);

    public:
        bool    shadows = false;
        Vector2 shadow_bounds {512, 512};
        float   shadow_near_plane {0.1f};
        float   shadow_far_plane {500.0f};
        Vector2 shadowmap_size {512, 512};
    };

    REFLECTION_TYPE(PointLightParameter)
    CLASS(PointLightParameter : public LightParameter, Fields)
    {
        REFLECTION_BODY(PointLightParameter);

    public:
        float falloff_radius {5.0f};
    };

    REFLECTION_TYPE(SpotLightParameter)
    CLASS(SpotLightParameter : public LightParameter, Fields)
    {
        REFLECTION_BODY(SpotLightParameter);

    public:
        float falloff_radius {5.0f};
        float inner_angle {45.0f};
        float outer_angle {60.0f};

        bool    shadows = false;
        Vector2 shadow_bounds {32, 32};
        float   shadow_near_plane {0.1f};
        float   shadow_far_plane {32.0f};
        Vector2 shadowmap_size {256, 256};
    };

    REFLECTION_TYPE(LightComponentRes)
    CLASS(LightComponentRes, Fields)
    {
        REFLECTION_BODY(LightComponentRes);

    public:
        Reflection::ReflectionPtr<LightParameter> m_parameter;

        LightComponentRes() = default;
        LightComponentRes(const LightComponentRes& res);

        ~LightComponentRes();
    };
} // namespace MoYu