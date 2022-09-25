#pragma once
#include "runtime/core/math/moyu_math.h"
#include "runtime/core/color/color.h"
#include "runtime/core/meta/reflection/reflection.h"

namespace Pilot
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
        Vector3 forward = Vector3::Forward;
        Vector3 up      = Vector3::Up;
        bool    shadows = false;
        Vector3 shadow_bounds {128, 128, 512};
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
} // namespace Pilot