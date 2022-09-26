#include "runtime/function/framework/component/light/light_component.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/math/moyu_math.h"
#include "runtime/core/color/color.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"

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

    void LightComponent::tick(float delta_time)
    {
        if (!m_parent_object.lock())
            return;

        if (m_parent_object.lock()->isDirty())
        {
            

            //RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
            //RenderSwapData&    logic_swap_data     = render_swap_context.getLogicSwapData();

            //logic_swap_data.addDirtyGameObject(GameObjectDesc {m_parent_object.lock()->getID(), dirty_mesh_parts});
        }
    }

} // namespace Pilot
