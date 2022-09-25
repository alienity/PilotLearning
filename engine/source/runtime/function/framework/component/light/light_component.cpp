#include "runtime/function/framework/component/light/light_component.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/math/moyu_math.h"
#include "runtime/core/color/color.h"

#include "runtime/function/render/render_system.h"

namespace Pilot
{
    void LightComponent::reset()
    {
        if (!m_light_res.m_parameter)
        {
            PILOT_REFLECTION_DELETE(m_light_res.m_parameter)
        }
        m_light_res.m_parameter = PILOT_REFLECTION_NEW(PointLightParameter)
    }

    void LightComponent::postLoadResource(std::weak_ptr<GObject> parent_object)
    {
        m_parent_object = parent_object;

        const std::string& light_type_name = m_light_res.m_parameter.getTypeName();
        if (light_type_name == "DirectionalLightParameter")
        {
            m_light_type = LightType::directional;
        }
        else if (light_type_name == "PointLightParameter")
        {
            m_light_type = LightType::point;
        }
        else if (light_type_name == "SpotLightParameter")
        {
            m_light_type = LightType::spot;
        }
        else
        {
            LOG_ERROR("invalid light type");
        }

    }

} // namespace Pilot
