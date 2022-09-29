#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/glm_wrapper.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/render_pass.h"
#include "runtime/function/render/render_resource.h"

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

            assert(entity.m_joint_matrices.size() <= m_mesh_vertex_blending_max_joint_count);
            for (size_t joint_index = 0; joint_index < entity.m_joint_matrices.size(); joint_index++)
            {
                temp_node.joint_matrices[joint_index] = GLMUtil::fromMat4x4(entity.m_joint_matrices[joint_index]);
            }
            temp_node.node_id = entity.m_instance_id;

            D3D12Mesh& mesh_asset            = render_resource->getEntityMesh(entity);
            temp_node.ref_mesh               = &mesh_asset;
            temp_node.enable_vertex_blending = entity.m_enable_vertex_blending;

            D3D12PBRMaterial& material_asset = render_resource->getEntityMaterial(entity);
            temp_node.ref_material           = &material_asset;

            MeshBoundingBox mesh_bounding_box = {};
            mesh_bounding_box.center =
                GLMUtil::fromVec3((mesh_asset_bounding_box.max_bound + mesh_asset_bounding_box.min_bound) * 0.5f);
            mesh_bounding_box.extent =
                GLMUtil::fromVec3((mesh_asset_bounding_box.max_bound - mesh_asset_bounding_box.min_bound) * 0.5f);

            temp_node.bounding_box     = mesh_bounding_box;
        }
        render_resource->m_mesh_perframe_storage_buffer_object.total_mesh_num = m_render_entities.size();
    }

    void RenderScene::setVisibleNodesReference()
    {
        RenderPass::m_visiable_nodes.p_all_mesh_nodes = &m_all_mesh_nodes;
        RenderPass::m_visiable_nodes.p_axis_node      = &m_axis_node;

        //RenderPass::m_visiable_nodes.p_main_camera_visible_particlebillboard_nodes =
        //    &m_main_camera_visible_particlebillboard_nodes;
    }

    GuidAllocator<GameObjectComponentId>& RenderScene::getInstanceIdAllocator() { return m_instance_id_allocator; }

    GuidAllocator<MeshSourceDesc>& RenderScene::getMeshAssetIdAllocator() { return m_mesh_asset_id_allocator; }

    GuidAllocator<MaterialSourceDesc>& RenderScene::getMaterialAssetdAllocator() { return m_material_asset_id_allocator; }

    void RenderScene::addInstanceIdToMap(uint32_t instance_id, GObjectID go_id)
    {
        m_mesh_object_id_map[instance_id] = go_id;
    }

    GObjectID RenderScene::getGObjectIDByMeshID(uint32_t mesh_id) const
    {
        auto find_it = m_mesh_object_id_map.find(mesh_id);
        if (find_it != m_mesh_object_id_map.end())
        {
            return find_it->second;
        }
        return GObjectID();
    }

    void RenderScene::deleteEntityByGObjectID(GObjectID go_id)
    {
        for (auto it = m_mesh_object_id_map.begin(); it != m_mesh_object_id_map.end(); it++)
        {
            if (it->second == go_id)
            {
                m_mesh_object_id_map.erase(it);
                break;
            }
        }

        GameObjectComponentId part_id = {go_id, 0};
        size_t                find_guid;
        if (m_instance_id_allocator.getElementGuid(part_id, find_guid))
        {
            for (auto it = m_render_entities.begin(); it != m_render_entities.end(); it++)
            {
                if (it->m_instance_id == find_guid)
                {
                    m_render_entities.erase(it);
                    break;
                }
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
        m_instance_id_allocator.clear();
        m_mesh_object_id_map.clear();
        m_render_entities.clear();
    }

    void RenderScene::updateVisibleObjectsAxis(std::shared_ptr<RenderResource> render_resource)
    {
        if (m_render_axis.has_value())
        {
            RenderEntity& axis = *m_render_axis;

            m_axis_node.model_matrix = GLMUtil::fromMat4x4(axis.m_model_matrix);
            m_axis_node.node_id      = axis.m_instance_id;

            D3D12Mesh& mesh_asset              = render_resource->getEntityMesh(axis);
            m_axis_node.ref_mesh               = &mesh_asset;
            m_axis_node.enable_vertex_blending = axis.m_enable_vertex_blending;
        }
    }

    void RenderScene::updateVisibleObjectsParticle(std::shared_ptr<RenderResource> render_resource)
    {
        // TODO
        m_main_camera_visible_particlebillboard_nodes.clear();
    }




















}
