#include "runtime/function/framework/component/terrain/terrain_component.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/framework/material/material_manager.h"
#include "runtime/resource/res_type/components/material.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"

#include "runtime/core/math/moyu_math2.h"

#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/render_common.h"

namespace MoYu
{
    //MaterialComponentRes _pbr_mat = {"asset/objects/environment/_material/temp.material.json", false, {}};

    //TerrainComponent _capsule_mesh_mat = {_capsule_mesh, _pbr_mat};

    void TerrainComponent::postLoadResource(std::weak_ptr<GObject> object, const std::string json_data)
    {
        m_object = object;

        MeshRendererComponentRes mesh_renderer_res = AssetManager::loadJson<MeshRendererComponentRes>(json_data);
        updateMeshRendererRes(mesh_renderer_res);

        markInit();
    }

    void TerrainComponent::save(ComponentDefinitionRes& out_component_res)
    {
        MeshComponentRes mesh_res {};
        (&mesh_res)->m_is_mesh_data   = m_scene_mesh.m_is_mesh_data;
        (&mesh_res)->m_mesh_data_path = m_scene_mesh.m_mesh_data_path;
        (&mesh_res)->m_sub_mesh_file  = m_scene_mesh.m_sub_mesh_file;

        MaterialComponentRes material_res {};
        (&material_res)->m_material_file    = m_mesh_renderer_res.m_material_res.m_material_file;
        (&material_res)->m_is_material_init = true;

        MaterialRes mat_res_data = ToMaterialRes(m_material.m_mat_data, m_material.m_shader_name);
        (&material_res)->m_material_serialized_json_data = AssetManager::saveJson(mat_res_data);

        MeshRendererComponentRes mesh_renderer_res {};
        (&mesh_renderer_res)->m_mesh_res     = mesh_res;
        (&mesh_renderer_res)->m_material_res = material_res;

        out_component_res.m_type_name           = "MeshRendererComponent";
        out_component_res.m_component_name      = this->m_component_name;
        out_component_res.m_component_json_data = AssetManager::saveJson(mesh_renderer_res);
    }

    void TerrainComponent::reset()
    {

        m_material = {};
    }

    GameObjectComponentDesc component2SwapData(MoYu::GObjectID     game_object_id,
                                               MoYu::GComponentID  transform_component_id,
                                               TransformComponent* m_transform_component_ptr,
                                               MoYu::GComponentID  mesh_renderer_component_id,
                                               SceneMesh*          m_scene_mesh_ptr,
                                               SceneMaterial*      m_scene_mat_ptr)
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

        SceneMeshRenderer scene_mesh_renderer = {};
        scene_mesh_renderer.m_identifier      = SceneCommonIdentifier {game_object_id, mesh_renderer_component_id};
        scene_mesh_renderer.m_scene_mesh      = *m_scene_mesh_ptr;
        scene_mesh_renderer.m_material        = *m_scene_mat_ptr;

        GameObjectComponentDesc light_component_desc = {};
        light_component_desc.m_component_type        = ComponentType::C_Transform | ComponentType::C_MeshRenderer;
        light_component_desc.m_transform_desc        = scene_transform;
        light_component_desc.m_mesh_renderer_desc    = scene_mesh_renderer;

        return light_component_desc;
    }

    void TerrainComponent::tick(float delta_time)
    {
        if (m_object.expired() || this->isNone())
            return;

        std::shared_ptr<MoYu::GObject> m_obj_ptr = m_object.lock();

        RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
        RenderSwapData& logic_swap_data = render_swap_context.getLogicSwapData();

        TransformComponent* m_transform_component_ptr = m_obj_ptr->getTransformComponent().lock().get();

        MoYu::GObjectID game_object_id = m_obj_ptr->getID();
        MoYu::GComponentID transform_component_id = m_transform_component_ptr->getComponentId();
        MoYu::GComponentID mesh_renderer_component_id = this->getComponentId();

        if (this->isToErase())
        {
            GameObjectComponentDesc mesh_renderer_desc = component2SwapData(game_object_id,
                                                                            transform_component_id,
                                                                            m_transform_component_ptr,
                                                                            mesh_renderer_component_id,
                                                                            &m_scene_mesh,
                                                                            &m_material);

            logic_swap_data.addDeleteGameObject({game_object_id, {mesh_renderer_desc}});

            this->markNone();
        }
        else if (m_transform_component_ptr->isMatrixDirty() || this->isDirty())
        {
            GameObjectComponentDesc mesh_renderer_desc = component2SwapData(game_object_id,
                                                                            transform_component_id,
                                                                            m_transform_component_ptr,
                                                                            mesh_renderer_component_id,
                                                                            &m_scene_mesh,
                                                                            &m_material);

            logic_swap_data.addDirtyGameObject({game_object_id, {mesh_renderer_desc}});

            this->markIdle();
        }
    }

} // namespace MoYu
