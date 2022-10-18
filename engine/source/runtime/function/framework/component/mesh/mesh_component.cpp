#include "runtime/function/framework/component/mesh/mesh_component.h"

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

namespace Pilot
{
    void MeshComponent::reset()
    {
        m_raw_meshes.clear();
        m_mesh_res = {};
    }

    GameObjectMaterialDesc SetMaterialDesc(std::string material_path)
    {
        std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
        ASSERT(asset_manager);

        MaterialRes new_material_res = g_runtime_global_context.m_material_manager->loadMaterialRes(material_path);

        GameObjectMaterialDesc m_material_desc = {};

        m_material_desc.m_blend        = new_material_res.m_blend;
        m_material_desc.m_double_sided = new_material_res.m_double_sided;

        m_material_desc.m_with_texture = !new_material_res.m_base_colour_texture_file.empty();

        m_material_desc.m_base_color_factor  = new_material_res.m_base_color_factor;
        m_material_desc.m_metallic_factor    = new_material_res.m_metallic_factor;
        m_material_desc.m_roughness_factor   = new_material_res.m_roughness_factor;
        m_material_desc.m_normal_scale       = new_material_res.m_normal_scale;
        m_material_desc.m_occlusion_strength = new_material_res.m_occlusion_strength;
        m_material_desc.m_emissive_factor    = new_material_res.m_emissive_factor;

        if (m_material_desc.m_with_texture)
        {
            m_material_desc.m_base_color_texture_file =
                asset_manager->getFullPath(new_material_res.m_base_colour_texture_file).generic_string();
            m_material_desc.m_metallic_roughness_texture_file =
                asset_manager->getFullPath(new_material_res.m_metallic_roughness_texture_file).generic_string();
            m_material_desc.m_normal_texture_file =
                asset_manager->getFullPath(new_material_res.m_normal_texture_file).generic_string();
            m_material_desc.m_occlusion_texture_file =
                asset_manager->getFullPath(new_material_res.m_occlusion_texture_file).generic_string();
            m_material_desc.m_emissive_texture_file =
                asset_manager->getFullPath(new_material_res.m_emissive_texture_file).generic_string();
        }

        return m_material_desc;
    }

    void MeshComponent::postLoadResource(std::weak_ptr<GObject> parent_object)
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

    void MeshComponent::tick(float delta_time)
    {
        if (!m_parent_object.lock())
            return;

        std::vector<GameObjectComponentDesc> dirty_mesh_parts;

        TransformComponent* m_transform_component_ptr = m_parent_object.lock()->getTransformComponent();

        bool is_transform_dirty = m_transform_component_ptr->isDirty();

        for (size_t i = 0; i < m_mesh_res.m_sub_meshes.size(); i++)
        {
            GameObjectComponentDesc& mesh_component = m_raw_meshes[i];

            if (is_transform_dirty)
            {
                Matrix4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();

                std::tuple<Quaternion, Vector3, Vector3> rts = GLMUtil::decomposeMat4x4(transform_matrix);

                mesh_component.m_transform_desc.m_is_active   = true;
                mesh_component.m_transform_desc.m_transform_matrix = transform_matrix;
                mesh_component.m_transform_desc.m_rotation         = std::get<0>(rts);
                mesh_component.m_transform_desc.m_position         = std::get<1>(rts);
                mesh_component.m_transform_desc.m_scale            = std::get<2>(rts);
            }

            SubMeshRes& sub_mesh = m_mesh_res.m_sub_meshes[i];

            bool is_material_dirty = g_runtime_global_context.m_material_manager->isMaterialDirty(sub_mesh.m_material);

            if (is_material_dirty)
            {
                mesh_component.m_mesh_desc.m_material_desc = SetMaterialDesc(sub_mesh.m_material);
            }

            if (is_transform_dirty || is_material_dirty)
            {
                dirty_mesh_parts.push_back(mesh_component);
            }
        }

        if (!dirty_mesh_parts.empty())
        {
            RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
            RenderSwapData&    logic_swap_data     = render_swap_context.getLogicSwapData();

            logic_swap_data.addDirtyGameObject(GameObjectDesc {m_parent_object.lock()->getID(), dirty_mesh_parts});
        }

    }

    bool MeshComponent::addNewMeshRes(std::string mesh_file_path)
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

    void MeshComponent::updateMaterial(std::string mesh_file_path, std::string material_path)
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

} // namespace Piccolo
