#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/core/color/color.h"

#include "runtime/function/render/render_type.h"

#include <vector>

namespace Pilot
{
    struct AmbientLight
    {
        Color m_color;
    };

    struct DirectionalLight
    {
        Vector3 m_position;
        Color   m_color;
        float   m_intensity;
        Vector3 m_direction;
        Vector3 m_up;

        bool    m_shadowmap;
        Vector3 m_shadow_bounds;
        Vector2 m_shadowmap_size;
    };

    struct PointLight
    {
        Vector3 m_position;
        Color   m_color;
        float   m_intensity;
        float   m_radius;
    };

    struct SpotLight
    {
        Vector3 m_position;
        Color   m_color;
        float   m_intensity;
        float   m_radius;
        float   m_inner_radius;
        float   m_outer_radius;
    };

    struct PointLightList
    {
        std::vector<PointLight> m_point_light_list;
    };

    struct SpotLightList
    {
        std::vector<SpotLight> m_spot_light_list;
    };

} // namespace Pilot