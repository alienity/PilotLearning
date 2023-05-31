#include "runtime/function/framework/component/mesh/mesh_renderer_component.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/framework/material/material_manager.h"
#include "runtime/resource/res_type/components/material.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"

#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/glm_wrapper.h"

namespace MoYu
{
    std::string MeshRendererComponent::m_component_name = "MeshRendererComponent";

    void MeshRendererComponent::postLoadResource(std::weak_ptr<GObject> object, void* data)
    {
        m_object = object;

        MeshRendererComponentRes* mesh_renderer_res = (MeshRendererComponentRes*)data;

        m_mesh_component_res     = mesh_renderer_res->m_mesh_res;
        m_material_component_res = mesh_renderer_res->m_material_res;

        m_scene_mesh = {m_mesh_component_res.m_is_mesh_data,
                        m_mesh_component_res.m_sub_mesh_file,
                        m_mesh_component_res.m_mesh_data_path};

        MaterialManager* m_mat_manager_ptr = g_runtime_global_context.m_material_manager.get();

        MaterialRes m_mat_res = m_mat_manager_ptr->loadMaterialRes(m_material_component_res.m_material_file);

        if (m_material_component_res.m_is_material_init)
        {
            MaterialRes* mat_res_data = (MaterialRes*)m_material_component_res.m_material_serialized_data.data();
            memcpy(&m_mat_res, mat_res_data, sizeof(MaterialRes));
        }

        ScenePBRMaterial m_mat_data = {m_mat_res.m_blend,
                                       m_mat_res.m_double_sided,
                                       m_mat_res.m_base_color_factor,
                                       m_mat_res.m_metallic_factor,
                                       m_mat_res.m_roughness_factor,
                                       m_mat_res.m_normal_scale,
                                       m_mat_res.m_occlusion_strength,
                                       m_mat_res.m_emissive_factor,
                                       m_mat_res.m_base_color_texture_file,
                                       m_mat_res.m_metallic_roughness_texture_file,
                                       m_mat_res.m_normal_texture_file,
                                       m_mat_res.m_occlusion_texture_file,
                                       m_mat_res.m_emissive_texture_file};

        m_material.m_shader_name = m_mat_res.shader_name;
        m_material.m_mat_data = m_mat_data;

        m_is_dirty = true;
    }

    void MeshRendererComponent::reset()
    {
        m_mesh_component_res = {};
        m_material_component_res = {};

        m_scene_mesh = {};
        m_material = {};
    }

    /*
    GameObjectMaterialDesc SetMaterialDesc(std::string material_path)
    {
        std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
        ASSERT(asset_manager);

        MaterialRes new_material_res = g_runtime_global_context.m_material_manager->loadMaterialRes(material_path);

        GameObjectMaterialDesc m_material_desc = {};

        m_material_desc.m_blend        = new_material_res.m_blend;
        m_material_desc.m_double_sided = new_material_res.m_double_sided;

        #define DescFromResProp(descPropName, resPropName) m_material_desc.descPropName = new_material_res.resPropName;

        DescFromResProp(m_base_color_factor, m_base_color_factor)
        DescFromResProp(m_metallic_factor, m_metallic_factor)
        DescFromResProp(m_roughness_factor, m_roughness_factor)
        DescFromResProp(m_normal_scale, m_normal_scale)
        DescFromResProp(m_occlusion_strength, m_occlusion_strength)
        DescFromResProp(m_emissive_factor, m_emissive_factor)

        #define DescFromResFile(descFileName, resFileName) \
    if (!new_material_res.resFileName.empty()) \
        m_material_desc.descFileName = asset_manager->getFullPath(new_material_res.resFileName).generic_string();

        DescFromResFile(m_base_color_texture_file, m_base_color_texture_file)
        DescFromResFile(m_metallic_roughness_texture_file, m_metallic_roughness_texture_file)
        DescFromResFile(m_normal_texture_file, m_normal_texture_file)
        DescFromResFile(m_occlusion_texture_file, m_occlusion_texture_file)
        DescFromResFile(m_emissive_texture_file, m_emissive_texture_file)

        return m_material_desc;
    }
    */
    /*
    void MeshRendererComponent::postLoadResource(std::weak_ptr<GObject> parent_object)
    {
        m_parent_object = parent_object;

        std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
        ASSERT(asset_manager);

        m_raw_meshes.clear();
        m_raw_meshes.resize(m_mesh_res.m_sub_meshes.size());

        size_t raw_mesh_count = 0;
        for (const SubMeshRes& sub_mesh : m_mesh_res.m_sub_meshes)
        {
            GameObjectComponentDesc& mesh_component = m_raw_meshes[raw_mesh_count];

            mesh_component.m_component_id = m_id;

            mesh_component.m_mesh_desc.m_is_active = true;
            mesh_component.m_mesh_desc.m_mesh_file =
                asset_manager->getFullPath(sub_mesh.m_obj_file_ref).generic_string();

            g_runtime_global_context.m_material_manager->setMaterialDirty(sub_mesh.m_material);
       
            mesh_component.m_mesh_desc.m_material_desc = SetMaterialDesc(sub_mesh.m_material);
            
            //auto object_space_transform = sub_mesh.m_transform.getMatrix();
            
            mesh_component.m_transform_desc.m_is_active = true;
            
            ++raw_mesh_count;
        }
    }
    */

    GameObjectComponentDesc component2SwapData(MoYu::GObjectID     game_object_id,
                                               MoYu::GComponentID  transform_component_id,
                                               TransformComponent* m_transform_component_ptr,
                                               MoYu::GComponentID  mesh_renderer_component_id,
                                               SceneMesh*          m_scene_mesh_ptr,
                                               SceneMaterial*      m_scene_mat_ptr)
    {
        Matrix4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();

        std::tuple<Quaternion, Vector3, Vector3> rts = GLMUtil::decomposeMat4x4(transform_matrix);

        Quaternion m_rotation = std::get<0>(rts);
        Vector3    m_position = std::get<1>(rts);
        Vector3    m_scale    = std::get<2>(rts);

        SceneTransform scene_transform     = {};
        scene_transform.m_identifier       = SceneCommonIdentifier {game_object_id, transform_component_id};
        scene_transform.m_position         = m_position;
        scene_transform.m_rotation         = m_rotation;
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

    void MeshRendererComponent::tick(float delta_time)
    {
        if (!m_object.lock())
            return;

        RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
        RenderSwapData& logic_swap_data = render_swap_context.getLogicSwapData();

        TransformComponent* m_transform_component_ptr = m_object.lock()->getTransformComponent().lock().get();

        MoYu::GObjectID game_object_id = m_object.lock()->getID();
        MoYu::GComponentID transform_component_id = m_transform_component_ptr->getComponentId();
        MoYu::GComponentID mesh_renderer_component_id = this->getComponentId();

        if (m_is_ready_erase) // Editor±ê¼ÇÉ¾³ý¸ÃComponent
        {
            GameObjectComponentDesc mesh_renderer_desc = component2SwapData(game_object_id,
                                                                            transform_component_id,
                                                                            m_transform_component_ptr,
                                                                            mesh_renderer_component_id,
                                                                            &m_scene_mesh,
                                                                            &m_material);

            logic_swap_data.addDeleteGameObject({game_object_id, {mesh_renderer_desc}});
        }
        else if (m_transform_component_ptr->isDirty() || isDirty())
        {
            GameObjectComponentDesc mesh_renderer_desc = component2SwapData(game_object_id,
                                                                            transform_component_id,
                                                                            m_transform_component_ptr,
                                                                            mesh_renderer_component_id,
                                                                            &m_scene_mesh,
                                                                            &m_material);

            logic_swap_data.addDirtyGameObject({game_object_id, {mesh_renderer_desc}});
        }
    }

    void MeshRendererComponent::updateMeshRes(std::string mesh_file_path)
    {

    }

    void MeshRendererComponent::updateMaterial(std::string material_path)
    {

    }

    /*
    bool MeshRendererComponent::addNewMeshRes(std::string mesh_file_path)
    {
        SubMeshRes newSubMeshRes = {};
        newSubMeshRes.m_obj_file_ref = mesh_file_path;
        newSubMeshRes.m_transform    = Transform();
        newSubMeshRes.m_material     = _default_gold_material_path;

        m_mesh_res.m_sub_meshes.push_back(newSubMeshRes);

        g_runtime_global_context.m_material_manager->setMaterialDirty(_default_gold_material_path);

        postLoadResource(m_parent_object);

        return true;
    }

    void MeshRendererComponent::updateMaterial(std::string mesh_file_path, std::string material_path)
    {
        for (size_t i = 0; i < m_mesh_res.m_sub_meshes.size(); i++)
        {
            SubMeshRes& sub_mesh = m_mesh_res.m_sub_meshes[i];
            if (sub_mesh.m_obj_file_ref == mesh_file_path)
            {
                sub_mesh.m_material = material_path;
                g_runtime_global_context.m_material_manager->setMaterialDirty(material_path);
                break;
            }
        }
    }
    */
} // namespace MoYu
