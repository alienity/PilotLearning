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
#include <function/framework/component/mesh/mesh_renderer_component.h>

namespace MoYu
{
    SceneImage _defaultTerrainHeightmap {false, false, 1, "asset/texture/terrain/TerrainHeightMap.png"};
    SceneImage _defaultTerrainNormalmap {false, false, 1, "asset/texture/terrain/TerrainNormalMap.png"};
    TerrainComponentRes _defaultTerrainComponentRes { glm::int3(10240, 2048, 10240), 5, 5, 16, 8, 8, _defaultTerrainHeightmap, _defaultTerrainNormalmap };

    void TerrainComponent::postLoadResource(std::weak_ptr<GObject> object, const std::string json_data)
    {
        m_object = object;

        TerrainComponentRes terrain_res = AssetManager::loadJson<TerrainComponentRes>(json_data);
        updateTerrainRes(terrain_res);

        markInit();
    }

    void TerrainComponent::updateTerrainRes(const TerrainComponentRes& res)
    {
        m_terrain_res = res;

        m_terrain.terrain_size = m_terrain_res.terrain_size;
        m_terrain.m_terrain_height_map = m_terrain_res.m_heightmap_file.m_image_file == "" ? _defaultTerrainHeightmap : m_terrain_res.m_heightmap_file;
        m_terrain.m_terrain_normal_map = m_terrain_res.m_normalmap_file.m_image_file == "" ? _defaultTerrainNormalmap : m_terrain_res.m_normalmap_file;

        MaterialManager* m_mat_manager_ptr = g_runtime_global_context.m_material_manager.get();

        MaterialRes m_mat_res = m_mat_manager_ptr->loadMaterialRes(m_terrain_res.m_material_res.m_material_file);

        if (m_terrain_res.m_material_res.m_is_material_init)
        {
            std::string m_material_serialized_json = m_terrain_res.m_material_res.m_material_serialized_json_data;
            MaterialRes mat_res_data = AssetManager::loadJson<MaterialRes>(m_material_serialized_json);
            m_mat_res = mat_res_data;
        }

        StandardLightMaterial m_mat_data = ToStandardMaterial(m_mat_res);

        m_material.m_shader_name = m_mat_res._ShaderName;
        m_material.m_mat_data    = m_mat_data;

        markDirty();
    }

    void TerrainComponent::save(ComponentDefinitionRes& out_component_res)
    {
        TerrainComponentRes terrain_res{};
        (&terrain_res)->terrain_size = m_terrain.terrain_size;
        (&terrain_res)->max_terrain_lod = m_terrain.max_terrain_lod;
        (&terrain_res)->max_lod_node_count = m_terrain.max_lod_node_count;
        (&terrain_res)->patch_mesh_grid_count = m_terrain.patch_mesh_grid_count;
        (&terrain_res)->patch_mesh_size = m_terrain.patch_mesh_size;
        (&terrain_res)->patch_count_per_node = m_terrain.patch_count_per_node;
        (&terrain_res)->m_heightmap_file = m_terrain.m_terrain_height_map;
        (&terrain_res)->m_normalmap_file = m_terrain.m_terrain_normal_map;

        MaterialComponentRes material_res{};
        (&material_res)->m_material_file = terrain_res.m_material_res.m_material_file;
        (&material_res)->m_is_material_init = true;

        MaterialRes mat_res_data = ToMaterialRes(m_material.m_mat_data, m_material.m_shader_name);
        (&material_res)->m_material_serialized_json_data = AssetManager::saveJson(mat_res_data);

        (&terrain_res)->m_material_res = material_res;

        out_component_res.m_type_name = "TerrainComponent";
        out_component_res.m_component_name = this->m_component_name;
        out_component_res.m_component_json_data = AssetManager::saveJson(terrain_res);
    }

    void TerrainComponent::reset()
    {
        m_terrain_res = _defaultTerrainComponentRes;

        m_terrain  = SceneTerrainMesh{_defaultTerrainComponentRes.terrain_size,
                                      _defaultTerrainComponentRes.max_terrain_lod,
                                      _defaultTerrainComponentRes.max_lod_node_count,
                                      _defaultTerrainComponentRes.patch_mesh_grid_count,
                                      _defaultTerrainComponentRes.patch_mesh_size,
                                      _defaultTerrainComponentRes.patch_count_per_node,
                                      _defaultTerrainComponentRes.m_heightmap_file,
                                      _defaultTerrainComponentRes.m_normalmap_file};
        m_material = {};
    }

    GameObjectComponentDesc component2SwapData(MoYu::GObjectID     game_object_id,
                                               MoYu::GComponentID  transform_component_id,
                                               TransformComponent* m_transform_component_ptr,
                                               MoYu::GComponentID  terrain_component_id,
                                               SceneTerrainMesh*   m_terrain_mesh_ptr,
                                               SceneMaterial*      m_terrain_mat_ptr)
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

        SceneTerrainRenderer scene_terrain_renderer = {};
        scene_terrain_renderer.m_identifier         = SceneCommonIdentifier {game_object_id, terrain_component_id};
        scene_terrain_renderer.m_scene_terrain_mesh = *m_terrain_mesh_ptr;
        scene_terrain_renderer.m_terrain_material   = *m_terrain_mat_ptr;

        GameObjectComponentDesc current_component_desc = {};
        current_component_desc.m_component_type        = ComponentType::C_Transform | ComponentType::C_Terrain;
        current_component_desc.m_transform_desc        = scene_transform;
        current_component_desc.m_terrain_mesh_renderer_desc = scene_terrain_renderer;

        return current_component_desc;
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
        MoYu::GComponentID terrain_component_id = this->getComponentId();

        if (this->isToErase())
        {
            GameObjectComponentDesc mesh_renderer_desc = component2SwapData(game_object_id,
                                                                            transform_component_id,
                                                                            m_transform_component_ptr,
                                                                            terrain_component_id,
                                                                            &m_terrain,
                                                                            &m_material);

            logic_swap_data.addDeleteGameObject({game_object_id, {mesh_renderer_desc}});

            this->markNone();
        }
        else if (m_transform_component_ptr->isMatrixDirty() || this->isDirty())
        {
            GameObjectComponentDesc mesh_renderer_desc = component2SwapData(game_object_id,
                                                                            transform_component_id,
                                                                            m_transform_component_ptr,
                                                                            terrain_component_id,
                                                                            &m_terrain,
                                                                            &m_material);

            logic_swap_data.addDirtyGameObject({game_object_id, {mesh_renderer_desc}});

            this->markIdle();
        }
    }

} // namespace MoYu
