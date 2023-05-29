#include "runtime/function/framework/component/light/light_component.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/math/moyu_math.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"

#include "runtime/function/render/render_system.h"
#include "runtime/function/render/glm_wrapper.h"

#include "runtime/resource/res_type/components/light.h"

namespace MoYu
{
    std::string LightComponent::m_component_name = "LightComponent";
    
    void LightComponent::reset()
    {
        
    }

    void LightComponent::postLoadResource(std::weak_ptr<GObject> object, void* data)
    {
        m_object = object;

        LightComponentRes* light_res = (LightComponentRes*)data;

        m_light_res = *light_res;

        m_light_res_buffer[m_current_index] = {};
        m_light_res_buffer[m_next_index] = m_light_res;

        m_is_dirty = true;
    }

    void LightComponent::tick(float delta_time)
    {
        if (!m_object.expired())
            return;

        m_light_res_buffer[m_next_index] = m_light_res;

        RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
        RenderSwapData& logic_swap_data = render_swap_context.getLogicSwapData();

        TransformComponent* m_transform_component_ptr = m_object.lock()->getTransformComponent().get();

        if (m_light_res_buffer[m_current_index].m_LightParamName == "" &&
            m_light_res_buffer[m_next_index].m_LightParamName != "")
        {
            MoYu::GObjectID game_object_id = m_object.lock()->getID();

            MoYu::GComponentID transform_component_id = m_transform_component_ptr->getComponentId();

            SceneTransform scene_transform = {};
            scene_transform.m_identifier = SceneCommonIdentifier {game_object_id, transform_component_id};
            scene_transform.m_position = m_transform_component_ptr->getPosition();
            scene_transform.m_rotation = m_transform_component_ptr->getRotation();
            scene_transform.m_scale = m_transform_component_ptr->getScale();
            scene_transform.m_transform_matrix = Math::makeViewMatrix(scene_transform.m_position, scene_transform.m_rotation);

            MoYu::GComponentID light_component_id = this->getComponentId();

            SceneLight scene_light = {};
            scene_light.m_identifier = SceneCommonIdentifier {game_object_id, light_component_id};

            if (m_light_res.m_LightParamName == "DirectionLightParameter")
            {
                Matrix3x3 rotation_matrix = Matrix3x3::fromQuaternion(scene_transform.m_rotation);

                MoYu::LightComponentRes& tmp_light_res = m_light_res_buffer[m_next_index];

                BaseDirectionLight direction_light = {};

                direction_light.m_color = tmp_light_res.m_DirectionLightParam.color;
                direction_light.m_intensity = tmp_light_res.m_DirectionLightParam.intensity;
                direction_light.m_direction = -rotation_matrix.getColumn(2);


                scene_light.m_light_type = LightType::DirectionLight;
                scene_light.direction_light = direction_light;

            }
            else if (m_light_res.m_LightParamName == "PointLightParameter")
            {
                scene_light.m_light_type = LightType::PointLight;
            }
            else if (m_light_res.m_LightParamName == "SpotLightParameter")
            {
                scene_light.m_light_type = LightType::SpotLight;
            }



            GameObjectComponentDesc light_component_desc = {};
            light_component_desc.m_component_type = ComponentType::C_Transform | ComponentType::C_Light;
            light_component_desc.m_transform_desc = scene_transform;
            light_component_desc.m_scene_light = scene_light;

            std::vector<GameObjectComponentDesc> dirty_light_part_descs = {light_component_desc};

            GameObjectDesc game_object_desc = {game_object_id, dirty_light_part_descs};

            logic_swap_data.addDirtyGameObject(game_object_desc);
        }
        else if (m_light_res_buffer[m_next_index].m_LightParamName == "" &&
                 m_light_res_buffer[m_current_index].m_LightParamName != "")
        {
            
        }
        else
        {

        }



        if (m_transform_component_ptr->isDirty() || isDirty())
        {

            Matrix4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();
            std::tuple<Quaternion, Vector3, Vector3> rts = GLMUtil::decomposeMat4x4(transform_matrix);

            Quaternion m_rotation = std::get<0>(rts);
            Vector3    m_position = std::get<1>(rts);
            Vector3    m_scale    = std::get<2>(rts);








            /*
            if (m_light_res.m_parameter.getTypeName() == "DirectionalLightParameter")
            {
                DirectionalLightParameter* m_directional_light_params = (DirectionalLightParameter*)m_light_res.m_parameter;

                Matrix3x3 rotation_matrix = Matrix3x3::fromQuaternion(std::get<0>(rts));

                m_light_part_desc.m_direction_light_desc.m_position       = std::get<1>(rts);
                m_light_part_desc.m_direction_light_desc.m_color          = m_directional_light_params->color;
                m_light_part_desc.m_direction_light_desc.m_intensity      = m_directional_light_params->intensity;

                m_light_part_desc.m_direction_light_desc.m_shadowmap      = m_directional_light_params->shadows;
                m_light_part_desc.m_direction_light_desc.m_shadow_bounds  = m_directional_light_params->shadow_bounds;
                m_light_part_desc.m_direction_light_desc.m_shadow_near_plane  = m_directional_light_params->shadow_near_plane;
                m_light_part_desc.m_direction_light_desc.m_shadow_far_plane  = m_directional_light_params->shadow_far_plane;
                m_light_part_desc.m_direction_light_desc.m_shadowmap_size = m_directional_light_params->shadowmap_size;
                m_light_part_desc.m_direction_light_desc.m_direction      = -rotation_matrix.getColumn(2);
                if (m_light_part_desc.m_direction_light_desc.m_shadowmap)
                {
                    //Matrix4x4 light_view_matrix = Math::makeViewMatrix(
                    //    m_light_part_desc.m_transform_desc.m_position, m_light_part_desc.m_transform_desc.m_rotation);
                    Vector3 eye_position = m_light_part_desc.m_transform_desc.m_position;
                    Vector3 target_position = eye_position + m_light_part_desc.m_direction_light_desc.m_direction;
                    Vector3 up_dir          = rotation_matrix.getColumn(1);

                    Matrix4x4 light_view_matrix = Math::makeLookAtMatrix(eye_position, target_position, up_dir);

                    Vector2 shadow_bounds = m_light_part_desc.m_direction_light_desc.m_shadow_bounds;
                    float shadow_near = m_light_part_desc.m_direction_light_desc.m_shadow_near_plane;
                    float shadow_far  = m_light_part_desc.m_direction_light_desc.m_shadow_far_plane;

                    Matrix4x4 light_proj_matrix = Math::makeOrthographicProjectionMatrix(-shadow_bounds.x * 0.5f,
                                                                                         shadow_bounds.x * 0.5f,
                                                                                         -shadow_bounds.y * 0.5f,
                                                                                         shadow_bounds.y * 0.5f,
                                                                                         shadow_near,
                                                                                         shadow_far);

                    m_light_part_desc.m_direction_light_desc.m_shadow_view_proj_mat = light_proj_matrix * light_view_matrix;
                }

            }
            else if (m_light_res.m_parameter.getTypeName() == "PointLightParameter")
            {
                PointLightParameter* m_point_light_params = (PointLightParameter*)m_light_res.m_parameter;

                m_light_part_desc.m_point_light_desc.m_position  = std::get<1>(rts);
                m_light_part_desc.m_point_light_desc.m_color     = m_point_light_params->color;
                m_light_part_desc.m_point_light_desc.m_intensity = m_point_light_params->intensity;
                m_light_part_desc.m_point_light_desc.m_radius    = m_point_light_params->falloff_radius;
            }
            else if (m_light_res.m_parameter.getTypeName() == "SpotLightParameter")
            {
                SpotLightParameter* m_spot_light_params = (SpotLightParameter*)m_light_res.m_parameter;

                DirectionalLightParameter* m_directional_light_params = (DirectionalLightParameter*)m_light_res.m_parameter;

                Matrix3x3 rotation_matrix = Matrix3x3::fromQuaternion(std::get<0>(rts));

                m_light_part_desc.m_spot_light_desc.m_position      = std::get<1>(rts);
                m_light_part_desc.m_spot_light_desc.m_direction     = -rotation_matrix.getColumn(2);
                m_light_part_desc.m_spot_light_desc.m_color         = m_spot_light_params->color;
                m_light_part_desc.m_spot_light_desc.m_intensity     = m_spot_light_params->intensity;
                m_light_part_desc.m_spot_light_desc.m_radius        = m_spot_light_params->falloff_radius;
                m_light_part_desc.m_spot_light_desc.m_inner_radians = Math::degreesToRadians(m_spot_light_params->inner_angle);
                m_light_part_desc.m_spot_light_desc.m_outer_radians = Math::degreesToRadians(m_spot_light_params->outer_angle);

                m_light_part_desc.m_spot_light_desc.m_shadowmap         = m_spot_light_params->shadows;
                m_light_part_desc.m_spot_light_desc.m_shadow_bounds     = m_spot_light_params->shadow_bounds;
                m_light_part_desc.m_spot_light_desc.m_shadow_near_plane = m_spot_light_params->shadow_near_plane;
                m_light_part_desc.m_spot_light_desc.m_shadow_far_plane  = m_spot_light_params->falloff_radius + 10.0f;
                m_light_part_desc.m_spot_light_desc.m_shadowmap_size    = m_spot_light_params->shadowmap_size;
                if (m_light_part_desc.m_spot_light_desc.m_shadowmap)
                {
                    Vector3 eye_position    = m_light_part_desc.m_transform_desc.m_position;
                    Vector3 target_position = eye_position + m_light_part_desc.m_spot_light_desc.m_direction;
                    Vector3 up_dir          = rotation_matrix.getColumn(1);

                    Matrix4x4 light_view_matrix = Math::makeLookAtMatrix(eye_position, target_position, up_dir);

                    float shadow_near = m_light_part_desc.m_spot_light_desc.m_shadow_near_plane;
                    float shadow_far  = m_light_part_desc.m_spot_light_desc.m_shadow_far_plane;

                    MoYu::Radian fovy = MoYu::Radian(m_light_part_desc.m_spot_light_desc.m_outer_radians);
                    float aspect = 1.0f;

                    Matrix4x4 light_proj_matrix = Math::makePerspectiveMatrix(fovy, aspect, shadow_near, shadow_far);

                    m_light_part_desc.m_spot_light_desc.m_shadow_view_proj_mat = light_proj_matrix * light_view_matrix;
                }
            }

            std::vector<GameObjectComponentDesc> dirty_light_part_descs = {m_light_part_desc};
            logic_swap_data.addDirtyGameObject(GameObjectDesc {m_parent_object.lock()->getID(), dirty_light_part_descs});
            */

        }

        std::swap(m_current_index, m_next_index);

        m_is_dirty = false;
    }

} // namespace MoYu
