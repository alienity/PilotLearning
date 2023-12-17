#include "runtime/function/framework/component/light/light_component.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/core/math/moyu_math2.h"

#include "runtime/resource/asset_manager/asset_manager.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"

#include "runtime/function/render/render_system.h"

#include "runtime/resource/res_type/components/light.h"

namespace MoYu
{
    LightComponentRes _empty_res = {"", {}, {}, {}};

    void LightComponent::reset()
    {
        
    }

    void LightComponent::postLoadResource(std::weak_ptr<GObject> object, const std::string json_data)
    {
        m_object = object;

        LightComponentRes light_res = AssetManager::loadJson<LightComponentRes>(json_data);

        m_light_res_buffer[m_current_index] = {};
        m_light_res_buffer[m_next_index]    = light_res;

        markInit();
    }

    void LightComponent::save(ComponentDefinitionRes& out_component_res)
    {
        LightComponentRes light_res {};
        (&light_res)->m_LightParamName = m_light_res_buffer[m_next_index].m_LightParamName;
        (&light_res)->m_DirectionLightParam = m_light_res_buffer[m_next_index].m_DirectionLightParam;
        (&light_res)->m_PointLightParam     = m_light_res_buffer[m_next_index].m_PointLightParam;
        (&light_res)->m_SpotLightParam      = m_light_res_buffer[m_next_index].m_SpotLightParam;

        out_component_res.m_type_name           = "LightComponent";
        out_component_res.m_component_name      = this->m_component_name;
        out_component_res.m_component_json_data = AssetManager::saveJson(light_res);
    }

    GameObjectComponentDesc component2SwapData(MoYu::GObjectID     game_object_id,
                                               MoYu::GComponentID  transform_component_id,
                                               TransformComponent* m_transform_component_ptr,
                                               MoYu::GComponentID  light_component_id,
                                               LightComponentRes*  m_light_res_ptr)
    {
        glm::float4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();

        glm::float3     m_scale;
        glm::quat m_orientation;
        glm::float3     m_translation;
        glm::float3     m_skew;
        glm::float4     m_perspective;
        glm::decompose(transform_matrix, m_scale, m_orientation, m_translation, m_skew, m_perspective);

        SceneTransform scene_transform     = {};
        scene_transform.m_identifier       = SceneCommonIdentifier {game_object_id, transform_component_id};
        scene_transform.m_position         = m_translation;
        scene_transform.m_rotation         = m_orientation;
        scene_transform.m_scale            = m_scale;
        scene_transform.m_transform_matrix = transform_matrix;

        SceneLight scene_light   = {};
        scene_light.m_identifier = SceneCommonIdentifier {game_object_id, light_component_id};

        if (m_light_res_ptr->m_LightParamName == DirectionLightParameterName)
        {
            //Matrix3x3 rotation_matrix = Matrix3x3::fromQuaternion(scene_transform.m_rotation);

            BaseDirectionLight direction_light = {};

            direction_light.m_color             = m_light_res_ptr->m_DirectionLightParam.color;
            direction_light.m_intensity         = m_light_res_ptr->m_DirectionLightParam.intensity;
            direction_light.m_shadowmap         = m_light_res_ptr->m_DirectionLightParam.shadows;
            direction_light.m_shadow_bounds     = m_light_res_ptr->m_DirectionLightParam.shadow_bounds;
            direction_light.m_shadow_near_plane = m_light_res_ptr->m_DirectionLightParam.shadow_near_plane;
            direction_light.m_shadow_far_plane  = m_light_res_ptr->m_DirectionLightParam.shadow_far_plane;
            direction_light.m_shadowmap_size    = m_light_res_ptr->m_DirectionLightParam.shadowmap_size;

            scene_light.direction_light = direction_light;
            scene_light.m_light_type    = LightType::DirectionLight;
        }
        else if (m_light_res_ptr->m_LightParamName == PointLightParameterName)
        {
            BasePointLight point_light = {};

            point_light.m_color     = m_light_res_ptr->m_PointLightParam.color;
            point_light.m_intensity = m_light_res_ptr->m_PointLightParam.intensity;
            point_light.m_radius    = m_light_res_ptr->m_PointLightParam.falloff_radius;

            scene_light.point_light  = point_light;
            scene_light.m_light_type = LightType::PointLight;
        }
        else if (m_light_res_ptr->m_LightParamName == SpotLightParameterName)
        {
            BaseSpotLight spot_light = {};

            spot_light.m_color             = m_light_res_ptr->m_SpotLightParam.color;
            spot_light.m_intensity         = m_light_res_ptr->m_SpotLightParam.intensity;
            spot_light.m_radius            = m_light_res_ptr->m_SpotLightParam.falloff_radius;
            spot_light.m_inner_degree      = m_light_res_ptr->m_SpotLightParam.inner_angle;
            spot_light.m_outer_degree      = m_light_res_ptr->m_SpotLightParam.outer_angle;
            spot_light.m_shadowmap         = m_light_res_ptr->m_SpotLightParam.shadows;
            spot_light.m_shadow_bounds     = m_light_res_ptr->m_SpotLightParam.shadow_bounds;
            spot_light.m_shadow_near_plane = m_light_res_ptr->m_SpotLightParam.shadow_near_plane;
            spot_light.m_shadow_far_plane  = m_light_res_ptr->m_SpotLightParam.shadow_far_plane;
            spot_light.m_shadowmap_size    = m_light_res_ptr->m_SpotLightParam.shadowmap_size;

            scene_light.spot_light   = spot_light;
            scene_light.m_light_type = LightType::SpotLight;
        }

        GameObjectComponentDesc light_component_desc = {};
        light_component_desc.m_component_type        = ComponentType::C_Transform | ComponentType::C_Light;
        light_component_desc.m_transform_desc        = scene_transform;
        light_component_desc.m_scene_light           = scene_light;

        return light_component_desc;
    }

    void LightComponent::tick(float delta_time)
    {
        if (m_object.expired() || this->isNone())
            return;

        std::shared_ptr<MoYu::GObject> m_obj_ptr = m_object.lock();

        RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
        RenderSwapData& logic_swap_data = render_swap_context.getLogicSwapData();

        TransformComponent* m_transform_component_ptr = m_obj_ptr->getTransformComponent().lock().get();

        MoYu::GObjectID    game_object_id         = m_obj_ptr->getID();
        MoYu::GComponentID transform_component_id = m_transform_component_ptr->getComponentId();
        MoYu::GComponentID light_component_id     = this->getComponentId();

        if (this->isToErase())
        {
            GameObjectComponentDesc light_component_desc = component2SwapData(game_object_id,
                                                                              transform_component_id,
                                                                              m_transform_component_ptr,
                                                                              light_component_id,
                                                                              &m_light_res_buffer[m_next_index]);

            logic_swap_data.addDeleteGameObject({game_object_id, {light_component_desc}});

            markNone();
        }
        else if (m_transform_component_ptr->isMatrixDirty() || this->isDirty())
        {
            //if (!this->isLightTypeInit())
            //{
            //    GameObjectComponentDesc _light_to_erase = component2SwapData(game_object_id,
            //                                                                 transform_component_id,
            //                                                                 m_transform_component_ptr,
            //                                                                 light_component_id,
            //                                                                 &m_light_res_buffer[m_current_index]);
            //    logic_swap_data.addDeleteGameObject({game_object_id, {_light_to_erase}});
            //}
            {
                GameObjectComponentDesc light_component_desc = component2SwapData(game_object_id,
                                                                                  transform_component_id,
                                                                                  m_transform_component_ptr,
                                                                                  light_component_id,
                                                                                  &m_light_res_buffer[m_next_index]);
                logic_swap_data.addDirtyGameObject({game_object_id, {light_component_desc}});
            }
            markIdle();
        }

        m_light_res_buffer[m_current_index] = m_light_res_buffer[m_next_index];

        std::swap(m_current_index, m_next_index);
    }

    bool LightComponent::isLightTypeInit()
    {
        return m_light_res_buffer[m_current_index].m_LightParamName == _empty_res.m_LightParamName;
    }

} // namespace MoYu
