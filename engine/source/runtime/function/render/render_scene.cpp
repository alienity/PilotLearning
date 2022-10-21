#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/glm_wrapper.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/render_pass.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/function/render/rhi/hlsl_data_types.h"

namespace Pilot
{
    void RenderScene::updateAllObjects(std::shared_ptr<RenderResource> render_resource,
                                       std::shared_ptr<RenderCamera>   camera)
    {
        m_all_mesh_nodes.clear();

        for (const RenderEntity& entity : m_render_entities)
        {
            BoundingBox mesh_asset_bounding_box {entity.m_bounding_box.getMinCorner(),
                                                 entity.m_bounding_box.getMaxCorner()};

            m_all_mesh_nodes.emplace_back();
            RenderMeshNode& temp_node = m_all_mesh_nodes.back();

            temp_node.model_matrix = GLMUtil::fromMat4x4(entity.m_model_matrix);
            temp_node.model_matrix_inverse = glm::inverse(temp_node.model_matrix);

            //assert(entity.m_joint_matrices.size() <= HLSL::m_mesh_vertex_blending_max_joint_count);
            //for (size_t joint_index = 0; joint_index < entity.m_joint_matrices.size(); joint_index++)
            //{
            //    temp_node.joint_matrices[joint_index] = GLMUtil::fromMat4x4(entity.m_joint_matrices[joint_index]);
            //}
            
            D3D12Mesh& mesh_asset            = render_resource->getEntityMesh(entity);
            temp_node.ref_mesh               = &mesh_asset;
            temp_node.enable_vertex_blending = entity.m_enable_vertex_blending;

            D3D12PBRMaterial& material_asset = render_resource->getEntityMaterial(entity);
            temp_node.ref_material           = &material_asset;

            Vector3 box_center = (mesh_asset_bounding_box.max_bound + mesh_asset_bounding_box.min_bound) * 0.5f;
            Vector3 box_extent = (mesh_asset_bounding_box.max_bound - mesh_asset_bounding_box.min_bound) * 0.5f;

            MeshBoundingBox mesh_bounding_box = {GLMUtil::fromVec3(box_center), GLMUtil::fromVec3(box_extent)};

            temp_node.bounding_box = mesh_bounding_box;
        }
        render_resource->m_mesh_perframe_storage_buffer_object.total_mesh_num = m_render_entities.size();
    }

    void RenderScene::setVisibleNodesReference()
    {
        RenderPass::m_visiable_nodes.p_all_mesh_nodes    = &m_all_mesh_nodes;
        RenderPass::m_visiable_nodes.p_ambient_light     = &m_ambient_light;
        RenderPass::m_visiable_nodes.p_directional_light = &m_directional_light;
        RenderPass::m_visiable_nodes.p_point_light_list  = &m_point_light_list;
        RenderPass::m_visiable_nodes.p_spot_light_list   = &m_spot_light_list;
    }

    void RenderScene::deleteEntityByGObjectID(GObjectID go_id)
    {
        for (auto it = m_render_entities.begin(); it != m_render_entities.end(); it++)
        {
            if (it->m_gobject_id == go_id)
            {
                m_render_entities.erase(it);
            }
        }
    }

    void RenderScene::deleteEntityByGObjectID(GObjectID go_id, GComponentID com_id)
    {
        for (auto it = m_render_entities.begin(); it != m_render_entities.end(); it++)
        {
            if (it->m_gobject_id == go_id && it->m_gcomponent_id == com_id)
            {
                m_render_entities.erase(it);
            }
        }
    }

    void RenderScene::deleteEntityByGObjectID(GObjectID go_id, GComponentID com_id, uint32_t mesh_asset_id)
    {
        for (auto it = m_render_entities.begin(); it != m_render_entities.end(); it++)
        {
            if (it->m_gobject_id == go_id && it->m_gcomponent_id == com_id && it->m_mesh_asset_id == mesh_asset_id)
            {
                m_render_entities.erase(it);
            }
        }
    }

    void RenderScene::deleteDirectionLightByGObjectID(GObjectID go_id)
    {
        if (m_directional_light.m_gobject_id == go_id)
        {
            m_directional_light.m_is_active = false;
        }
    }

    void RenderScene::deletePointLightByGObjectID(GObjectID go_id)
    {
        uint32_t point_light_index_to_del = -1;
        for (size_t i = 0; i < m_point_light_list.size(); i++)
        {
            if (m_point_light_list[i].m_gobject_id == go_id)
            {
                point_light_index_to_del = i;
                break;
            }
        }
        if (point_light_index_to_del != -1)
        {
            m_point_light_list.erase(m_point_light_list.begin() + point_light_index_to_del);
        }
    }

    void RenderScene::deleteSpotLightByGObjectID(GObjectID go_id)
    {
        uint32_t spot_light_index_to_del = -1;
        for (size_t i = 0; i < m_spot_light_list.size(); i++)
        {
            if (m_spot_light_list[i].m_gobject_id == go_id)
            {
                spot_light_index_to_del = i;
                break;
            }
        }
        if (spot_light_index_to_del != -1)
        {
            m_spot_light_list.erase(m_spot_light_list.begin() + spot_light_index_to_del);
        }
    }

    void RenderScene::clearForLevelReloading()
    {
        m_point_light_list.clear();
        m_spot_light_list.clear();
        m_render_entities.clear();
    }

}
