#include "runtime/function/framework/component/light/light_component.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/math/moyu_math.h"
#include "runtime/core/color/color.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"

#include "runtime/function/render/render_system.h"
#include "runtime/function/render/glm_wrapper.h"
#include <glm/gtx/quaternion.hpp>

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

        m_light_part_desc.m_component_id               = m_id;
        m_light_part_desc.m_transform_desc.m_is_active = true;
    }

    void LightComponent::tick(float delta_time)
    {
        if (!m_parent_object.lock())
            return;

        TransformComponent* m_transform_component_ptr = m_parent_object.lock()->getTransformComponent();

        if (m_transform_component_ptr->isDirty())
        {
            Matrix4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();
            std::tuple<Quaternion, Vector3, Vector3> rts = GLMUtil::decomposeMat4x4(transform_matrix);

            m_light_part_desc.m_transform_desc.m_is_active = true;
            m_light_part_desc.m_transform_desc.m_transform_matrix = transform_matrix;
            m_light_part_desc.m_transform_desc.m_rotation         = std::get<0>(rts);
            m_light_part_desc.m_transform_desc.m_position         = std::get<1>(rts);
            m_light_part_desc.m_transform_desc.m_scale            = std::get<2>(rts);

            if (m_light_res.m_parameter.getTypeName() == "DirectionalLightParameter")
            {
                DirectionalLightParameter* m_directional_light_params = (DirectionalLightParameter*)m_light_res.m_parameter;

                Matrix3x3 rotation_matrix = Matrix3x3::fromQuaternion(std::get<0>(rts));

                m_light_part_desc.m_direction_light_desc.m_is_active      = true;
                m_light_part_desc.m_direction_light_desc.m_position       = std::get<1>(rts);
                m_light_part_desc.m_direction_light_desc.m_color          = m_directional_light_params->color;
                m_light_part_desc.m_direction_light_desc.m_intensity      = m_directional_light_params->intensity;
                m_light_part_desc.m_direction_light_desc.m_shadowmap      = m_directional_light_params->shadows;
                m_light_part_desc.m_direction_light_desc.m_shadow_bounds  = m_directional_light_params->shadow_bounds;
                m_light_part_desc.m_direction_light_desc.m_shadowmap_size = m_directional_light_params->shadowmap_size;
                m_light_part_desc.m_direction_light_desc.m_direction      = -rotation_matrix.getColumn(2);
                m_light_part_desc.m_direction_light_desc.m_up             = rotation_matrix.getColumn(1);
            }
            else if (m_light_res.m_parameter.getTypeName() == "PointLightParameter")
            {
                PointLightParameter* m_point_light_params = (PointLightParameter*)m_light_res.m_parameter;

                m_light_part_desc.m_point_light_desc.m_is_active = true;
                m_light_part_desc.m_point_light_desc.m_position  = std::get<1>(rts);
                m_light_part_desc.m_point_light_desc.m_color     = m_point_light_params->color;
                m_light_part_desc.m_point_light_desc.m_intensity = m_point_light_params->intensity;
                m_light_part_desc.m_point_light_desc.m_radius    = m_point_light_params->falloff_radius;
            }
            else if (m_light_res.m_parameter.getTypeName() == "SpotLightParameter")
            {
                SpotLightParameter* m_spot_light_params = (SpotLightParameter*)m_light_res.m_parameter;

                m_light_part_desc.m_spot_light_desc.m_is_active     = true;
                m_light_part_desc.m_spot_light_desc.m_position      = std::get<1>(rts);
                m_light_part_desc.m_spot_light_desc.m_color         = m_spot_light_params->color;
                m_light_part_desc.m_spot_light_desc.m_intensity     = m_spot_light_params->intensity;
                m_light_part_desc.m_spot_light_desc.m_radius        = m_spot_light_params->falloff_radius;
                m_light_part_desc.m_spot_light_desc.m_inner_radians = Math::degreesToRadians(m_spot_light_params->inner_angle);
                m_light_part_desc.m_spot_light_desc.m_outer_radians = Math::degreesToRadians(m_spot_light_params->outer_angle);
            }

            RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
            RenderSwapData&    logic_swap_data     = render_swap_context.getLogicSwapData();

            std::vector<GameObjectComponentDesc> dirty_light_part_descs = {m_light_part_desc};
            logic_swap_data.addDirtyGameObject(GameObjectDesc {m_parent_object.lock()->getID(), dirty_light_part_descs});
        }
    }

} // namespace Pilot