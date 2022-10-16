#include "runtime/function/framework/component/mesh/mesh_component.h"

#include "runtime/resource/asset_manager/asset_manager.h"
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

    void MeshComponent::postLoadResource(std::weak_ptr<GObject> parent_object)
    {
        m_parent_object = parent_object;

        std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
        ASSERT(asset_manager);

        m_raw_meshes.clear();
        m_raw_meshes.resize(m_mesh_res.m_sub_meshes.size());

        m_material_res_map.clear();

        size_t raw_mesh_count = 0;
        for (const SubMeshRes& sub_mesh : m_mesh_res.m_sub_meshes)
        {
            GameObjectComponentDesc& meshComponent = m_raw_meshes[raw_mesh_count];

            meshComponent.m_component_id = m_id;

            meshComponent.m_mesh_desc.m_is_active = true;
            meshComponent.m_mesh_desc.m_mesh_file = asset_manager->getFullPath(sub_mesh.m_obj_file_ref).generic_string();
            
            meshComponent.m_mesh_desc.m_material_desc.m_with_texture = sub_mesh.m_material.empty() == false;

            if (meshComponent.m_mesh_desc.m_material_desc.m_with_texture)
            {
                MaterialRes material_res;
                asset_manager->loadAsset(sub_mesh.m_material, material_res);

                meshComponent.m_mesh_desc.m_material_desc.m_blend        = material_res.m_blend;
                meshComponent.m_mesh_desc.m_material_desc.m_double_sided = material_res.m_double_sided;

                meshComponent.m_mesh_desc.m_material_desc.m_base_color_factor  = material_res.m_base_color_factor;
                meshComponent.m_mesh_desc.m_material_desc.m_metallic_factor    = material_res.m_metallic_factor;
                meshComponent.m_mesh_desc.m_material_desc.m_roughness_factor   = material_res.m_roughness_factor;
                meshComponent.m_mesh_desc.m_material_desc.m_normal_scale       = material_res.m_normal_scale;
                meshComponent.m_mesh_desc.m_material_desc.m_occlusion_strength = material_res.m_occlusion_strength;
                meshComponent.m_mesh_desc.m_material_desc.m_emissive_factor    = material_res.m_emissive_factor;

                meshComponent.m_mesh_desc.m_material_desc.m_base_color_texture_file = asset_manager->getFullPath(material_res.m_base_colour_texture_file).generic_string();
                meshComponent.m_mesh_desc.m_material_desc.m_metallic_roughness_texture_file = asset_manager->getFullPath(material_res.m_metallic_roughness_texture_file).generic_string();
                meshComponent.m_mesh_desc.m_material_desc.m_normal_texture_file = asset_manager->getFullPath(material_res.m_normal_texture_file).generic_string();
                meshComponent.m_mesh_desc.m_material_desc.m_occlusion_texture_file = asset_manager->getFullPath(material_res.m_occlusion_texture_file).generic_string();
                meshComponent.m_mesh_desc.m_material_desc.m_emissive_texture_file = asset_manager->getFullPath(material_res.m_emissive_texture_file).generic_string();

                m_material_res_map[sub_mesh.m_material] = material_res;
                m_material_dirty_map[sub_mesh.m_material] = false;
            }

            //auto object_space_transform = sub_mesh.m_transform.getMatrix();
            
            meshComponent.m_transform_desc.m_is_active = true;
            
            ++raw_mesh_count;
        }
    }

    void MeshComponent::tick(float delta_time)
    {
        if (!m_parent_object.lock())
            return;



        TransformComponent* m_transform_component_ptr = m_parent_object.lock()->getTransformComponent();

        if (m_transform_component_ptr->isDirty())
        {
            Matrix4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();

            std::vector<GameObjectComponentDesc> dirty_mesh_parts;
            
            for (GameObjectComponentDesc& mesh_part : m_raw_meshes)
            {
                std::tuple<Quaternion, Vector3, Vector3> rts = GLMUtil::decomposeMat4x4(transform_matrix);

                mesh_part.m_transform_desc.m_is_active        = true;
                mesh_part.m_transform_desc.m_transform_matrix = transform_matrix;
                mesh_part.m_transform_desc.m_rotation         = std::get<0>(rts);
                mesh_part.m_transform_desc.m_position         = std::get<1>(rts);
                mesh_part.m_transform_desc.m_scale            = std::get<2>(rts);

                dirty_mesh_parts.push_back(mesh_part);
            }

            if (!dirty_mesh_parts.empty())
            {
                RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
                RenderSwapData&    logic_swap_data     = render_swap_context.getLogicSwapData();

                logic_swap_data.addDirtyGameObject(GameObjectDesc {m_parent_object.lock()->getID(), dirty_mesh_parts});
            }
        }

    }

    bool MeshComponent::addNewMeshRes(std::string mesh_file_path)
    {
        SubMeshRes newSubMeshRes = {};
        newSubMeshRes.m_obj_file_ref = mesh_file_path;
        newSubMeshRes.m_transform    = Transform();
        newSubMeshRes.m_material     = "asset/objects/environment/_material/gold.material.json";

        m_mesh_res.m_sub_meshes.push_back(newSubMeshRes);

        postLoadResource(m_parent_object);

        return true;
    }

    MaterialRes& MeshComponent::getMaterialRes(std::string material_path)
    {
        return m_material_res_map[material_path];
    }

    void MeshComponent::createMaterial(std::string mesh_file_path, std::string material_path)
    {
        std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
        ASSERT(asset_manager);

        for (size_t i = 0; i < m_mesh_res.m_sub_meshes.size(); i++)
        {
            SubMeshRes& sub_mesh = m_mesh_res.m_sub_meshes[i];
            if (sub_mesh.m_obj_file_ref == mesh_file_path)
            {
                sub_mesh.m_material = material_path;

                MaterialRes default_material_res;
                asset_manager->loadAsset("asset/objects/environment/_material/gold.material.json", default_material_res);
                asset_manager->saveAsset(default_material_res, material_path);

                m_material_res_map[sub_mesh.m_material] = default_material_res;

                break;
            }
        }

        updateMaterial(mesh_file_path, material_path);
    }

    void MeshComponent::updateMaterial(std::string mesh_file_path, std::string material_path)
    {
        std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
        ASSERT(asset_manager);

        for (size_t i = 0; i < m_mesh_res.m_sub_meshes.size(); i++)
        {
            SubMeshRes& sub_mesh = m_mesh_res.m_sub_meshes[i];
            if (sub_mesh.m_obj_file_ref == mesh_file_path)
            {
                MaterialRes material_res = m_material_res_map[material_path];

                GameObjectComponentDesc& meshComponent = m_raw_meshes[i];

                meshComponent.m_mesh_desc.m_material_desc.m_blend              = material_res.m_blend;
                meshComponent.m_mesh_desc.m_material_desc.m_double_sided       = material_res.m_double_sided;

                meshComponent.m_mesh_desc.m_material_desc.m_base_color_factor  = material_res.m_base_color_factor;
                meshComponent.m_mesh_desc.m_material_desc.m_metallic_factor    = material_res.m_metallic_factor;
                meshComponent.m_mesh_desc.m_material_desc.m_roughness_factor   = material_res.m_roughness_factor;
                meshComponent.m_mesh_desc.m_material_desc.m_normal_scale       = material_res.m_normal_scale;
                meshComponent.m_mesh_desc.m_material_desc.m_occlusion_strength = material_res.m_occlusion_strength;
                meshComponent.m_mesh_desc.m_material_desc.m_emissive_factor    = material_res.m_emissive_factor;

                meshComponent.m_mesh_desc.m_material_desc.m_base_color_texture_file = asset_manager->getFullPath(material_res.m_base_colour_texture_file).generic_string();
                meshComponent.m_mesh_desc.m_material_desc.m_metallic_roughness_texture_file = asset_manager->getFullPath(material_res.m_metallic_roughness_texture_file).generic_string();
                meshComponent.m_mesh_desc.m_material_desc.m_normal_texture_file = asset_manager->getFullPath(material_res.m_normal_texture_file).generic_string();
                meshComponent.m_mesh_desc.m_material_desc.m_occlusion_texture_file = asset_manager->getFullPath(material_res.m_occlusion_texture_file).generic_string();
                meshComponent.m_mesh_desc.m_material_desc.m_emissive_texture_file = asset_manager->getFullPath(material_res.m_emissive_texture_file).generic_string();

                break;
            }
        }
    }

} // namespace Piccolo
