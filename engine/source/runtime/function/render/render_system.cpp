#include "runtime/function/render/render_system.h"

#include "runtime/core/base/macro.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/render_resource.h"

#include "runtime/function/render/renderer_moyu.h"

namespace Pilot
{
    RenderSystem::~RenderSystem() 
    {
        m_render_resource  = nullptr;
        m_render_scene     = nullptr;
        m_render_camera    = nullptr;
        m_renderer_manager = nullptr;
    }

    void RenderSystem::initialize(RenderSystemInitInfo init_info)
    {
        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        ASSERT(config_manager);
        std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
        ASSERT(asset_manager);

        // global rendering resource
        GlobalRenderingRes global_rendering_res;
        const std::string& global_rendering_res_url = config_manager->getGlobalRenderingResUrl();
        asset_manager->loadAsset(global_rendering_res_url, global_rendering_res);

        // upload ibl, color grading textures
        LevelResourceDesc level_resource_desc;
        level_resource_desc.m_ibl_resource_desc.m_skybox_irradiance_map = global_rendering_res.m_skybox_irradiance_map;
        level_resource_desc.m_ibl_resource_desc.m_skybox_specular_map   = global_rendering_res.m_skybox_specular_map;
        level_resource_desc.m_ibl_resource_desc.m_brdf_map              = global_rendering_res.m_brdf_map;
        level_resource_desc.m_color_grading_resource_desc.m_color_grading_map =
            global_rendering_res.m_color_grading_map;

        // initialize render manager
        RHI::DeviceOptions deviceOptions              = {};
        deviceOptions.FeatureLevel                    = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
#ifdef _DEBUG
        deviceOptions.EnableDebugLayer                = true;
        deviceOptions.EnableAutoDebugName             = true;
#endif
        RendererManagerInitInfo renderManagerInitInfo = {};
        renderManagerInitInfo.Options                 = deviceOptions;
        renderManagerInitInfo.Window_system           = init_info.window_system;
        m_renderer_manager                            = std::make_shared<RendererManager>();
        m_renderer_manager->Initialize(renderManagerInitInfo);

        // initial global resources
        m_render_resource = std::make_shared<RenderResource>();
        m_render_resource->iniUploadBatch(m_renderer_manager->GetDevice());
        m_render_resource->uploadGlobalRenderResource(level_resource_desc);

        // setup render camera
        const CameraPose& camera_pose = global_rendering_res.m_camera_config.m_pose;
        m_render_camera               = std::make_shared<RenderCamera>();
        m_render_camera->lookAt(camera_pose.m_position, camera_pose.m_target, camera_pose.m_up);
        m_render_camera->m_zfar  = global_rendering_res.m_camera_config.m_z_far;
        m_render_camera->m_znear = global_rendering_res.m_camera_config.m_z_near;
        m_render_camera->setAspect(global_rendering_res.m_camera_config.m_aspect.x /
                                   global_rendering_res.m_camera_config.m_aspect.y);
        m_render_camera->setFOVy(90);

        // setup render scene
        m_render_scene = std::make_shared<RenderScene>();

        m_render_scene->m_ambient_light.m_is_active = true;
        m_render_scene->m_ambient_light.m_color     = global_rendering_res.m_ambient_light;

        m_render_scene->setVisibleNodesReference();

        // initialize renderer
        m_renderer_manager->InitRenderer();
        //m_renderer_manager->PreparePassData(m_render_resource);
    }

    void RenderSystem::tick()
    {
        // process swap data between logic and render contexts
        processSwapData();

        // update per-frame buffer
        m_render_resource->updatePerFrameBuffer(m_render_scene, m_render_camera);

        // update per-frame visible objects
        m_render_scene->updateAllObjects(std::static_pointer_cast<RenderResource>(m_render_resource), m_render_camera);

        //// update per-frame visible objects
        //m_render_scene->updateVisibleObjects(std::static_pointer_cast<RenderResource>(m_render_resource),
        //                                     m_render_camera);

        // prepare pipeline's render passes data
        m_renderer_manager->PreparePassData(m_render_resource);

        // finish static resource uploading
        m_render_resource->commitUploadBatch();

        // render one frame
        m_renderer_manager->Tick();
    }

    void RenderSystem::swapLogicRenderData() { m_swap_context.swapLogicRenderData(); }

    RenderSwapContext& RenderSystem::getSwapContext() { return m_swap_context; }

    std::shared_ptr<RenderCamera> RenderSystem::getRenderCamera() const { return m_render_camera; }

    void RenderSystem::initializeUIRenderBackend(WindowUI* window_ui)
    {
        // TODO:初始化UI相关的绘制
        m_renderer_manager->InitUIRenderer(window_ui);
    }

    /*
    void RenderSystem::updateEngineContentViewport(float offset_x, float offset_y, float width, float height)
    {
        m_renderer_manager->MoYuRenderer->SetViewPort(offset_x, offset_y, width, height);
        m_render_camera->setAspect(width / height);
    }
    */
    
    EngineContentViewport RenderSystem::getEngineContentViewport() const
    {
        return m_renderer_manager->MoYuRenderer->GetViewPort();
    }

    uint32_t RenderSystem::getGuidOfPickedMesh(const Vector2& picked_uv)
    {
        return 0;
        //return m_render_pipeline->getGuidOfPickedMesh(picked_uv);
    }

    GuidAllocator<MeshSourceDesc>& RenderSystem::getMeshAssetIdAllocator()
    {
        return m_render_scene->getMeshAssetIdAllocator();
    }

    void RenderSystem::clearForLevelReloading() { m_render_scene->clearForLevelReloading(); }

    void RenderSystem::processSwapData()
    {
        RenderSwapData& swap_data = m_swap_context.getRenderSwapData();

        std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
        ASSERT(asset_manager);

        // TODO: update global resources if needed
        if (swap_data.m_level_resource_desc.has_value())
        {
            m_render_resource->uploadGlobalRenderResource(*swap_data.m_level_resource_desc);

            // reset level resource swap data to a clean state
            m_swap_context.resetLevelRsourceSwapData();
        }

        // update game object if needed
        if (swap_data.m_game_object_resource_desc.has_value())
        {
            while (!swap_data.m_game_object_resource_desc->isEmpty())
            {
                GameObjectDesc gobject = swap_data.m_game_object_resource_desc->getNextProcessObject();

                for (size_t part_index = 0; part_index < gobject.getObjectParts().size(); part_index++)
                {
                    const auto& game_object_part = gobject.getObjectParts()[part_index];

                    // mesh properties
                    if (game_object_part.m_mesh_desc.m_is_active)
                    {
                        RenderEntity render_entity;
                        render_entity.m_gobject_id    = gobject.getId();
                        render_entity.m_gcomponent_id = game_object_part.m_component_id;

                        render_entity.m_model_matrix  = game_object_part.m_transform_desc.m_transform_matrix;

                        MeshSourceDesc mesh_source = {game_object_part.m_mesh_desc.m_mesh_file};
                        bool is_mesh_loaded        = m_render_scene->getMeshAssetIdAllocator().hasElement(mesh_source);

                        RenderMeshData mesh_data;
                        if (!is_mesh_loaded)
                        {
                            mesh_data = m_render_resource->loadMeshData(mesh_source, render_entity.m_bounding_box);
                        }
                        else
                        {
                            render_entity.m_bounding_box = m_render_resource->getCachedBoudingBox(mesh_source);
                        }

                        render_entity.m_mesh_asset_id = m_render_scene->getMeshAssetIdAllocator().allocGuid(mesh_source);

                        // create game object on the graphics api side
                        if (!is_mesh_loaded)
                        {
                            m_render_resource->uploadGameObjectRenderResource(render_entity, mesh_data);
                        }

                        // material properties
                        //if (game_object_part.m_material_desc.m_is_active)
                        {
                            MaterialSourceDesc material_source = {
                                game_object_part.m_mesh_desc.m_material_desc.m_blend,
                                game_object_part.m_mesh_desc.m_material_desc.m_double_sided,

                                game_object_part.m_mesh_desc.m_material_desc.m_base_color_factor,
                                game_object_part.m_mesh_desc.m_material_desc.m_metallic_factor,
                                game_object_part.m_mesh_desc.m_material_desc.m_roughness_factor,
                                game_object_part.m_mesh_desc.m_material_desc.m_normal_scale,
                                game_object_part.m_mesh_desc.m_material_desc.m_occlusion_strength,
                                game_object_part.m_mesh_desc.m_material_desc.m_emissive_factor,

                                game_object_part.m_mesh_desc.m_material_desc.m_base_color_texture_file,
                                game_object_part.m_mesh_desc.m_material_desc.m_metallic_roughness_texture_file,
                                game_object_part.m_mesh_desc.m_material_desc.m_normal_texture_file,
                                game_object_part.m_mesh_desc.m_material_desc.m_occlusion_texture_file,
                                game_object_part.m_mesh_desc.m_material_desc.m_emissive_texture_file};

                            render_entity.m_material_asset_id = m_render_scene->getMaterialAssetdAllocator().allocGuid(material_source);

                            RenderMaterialData material_data = m_render_resource->loadMaterialData(material_source);

                            m_render_resource->uploadGameObjectRenderResource(render_entity, material_data);
                        }

                        // add object to render scene if needed
                        bool is_entity_in_scene = false;
                        for (size_t i = 0; i < m_render_scene->m_render_entities.size(); i++)
                        {
                            RenderEntity& entity = m_render_scene->m_render_entities[i];
                            if (entity.m_gobject_id == render_entity.m_gobject_id &&
                                entity.m_gcomponent_id == render_entity.m_gcomponent_id &&
                                entity.m_mesh_asset_id == render_entity.m_mesh_asset_id)
                            {
                                is_entity_in_scene = true;
                                entity = render_entity;
                                break;
                            }
                        }
                        if (!is_entity_in_scene)
                        {
                            m_render_scene->m_render_entities.push_back(render_entity);
                        }

                    }

                    // process light properties
                    {
                        // ambient light
                        if (game_object_part.m_ambeint_light_desc.m_is_active)
                        {
                            m_render_scene->m_ambient_light = game_object_part.m_ambeint_light_desc;
                        }

                        // direction light
                        if (game_object_part.m_direction_light_desc.m_is_active)
                        {
                            m_render_scene->m_directional_light              = game_object_part.m_direction_light_desc;
                            m_render_scene->m_directional_light.m_gobject_id = gobject.getId();
                            m_render_scene->m_directional_light.m_gcomponent_id = game_object_part.m_component_id;
                        }
                        else
                        {
                            bool is_direction_light_dirty =
                                (m_render_scene->m_directional_light.m_gobject_id == gobject.getId()) &&
                                (m_render_scene->m_directional_light.m_gcomponent_id == game_object_part.m_component_id);

                            if (is_direction_light_dirty)
                            {
                                m_render_scene->m_directional_light.m_is_active = false;
                            }
                        }

                        // point light
                        {
                            int point_light_index_in_scene = -1;
                            for (size_t i = 0; i < m_render_scene->m_point_light_list.size(); i++)
                            {
                                auto& tmp_light = m_render_scene->m_point_light_list[i];

                                if ((tmp_light.m_gobject_id == gobject.getId()) &&
                                    (tmp_light.m_gcomponent_id == game_object_part.m_component_id))
                                {
                                    point_light_index_in_scene = i;
                                    break;
                                }
                            }

                            if (game_object_part.m_point_light_desc.m_is_active)
                            {
                                if (point_light_index_in_scene == -1)
                                {
                                    m_render_scene->m_point_light_list.push_back(game_object_part.m_point_light_desc);
                                    m_render_scene->m_point_light_list.back().m_gobject_id = gobject.getId();
                                    m_render_scene->m_point_light_list.back().m_gcomponent_id =
                                        game_object_part.m_component_id;
                                }
                                else
                                {
                                    m_render_scene->m_point_light_list[point_light_index_in_scene] =
                                        game_object_part.m_point_light_desc;
                                    m_render_scene->m_point_light_list[point_light_index_in_scene].m_gobject_id =
                                        gobject.getId();
                                    m_render_scene->m_point_light_list[point_light_index_in_scene].m_gcomponent_id =
                                        game_object_part.m_component_id;
                                }
                            }
                            else
                            {
                                if (point_light_index_in_scene != -1)
                                {
                                    m_render_scene->m_point_light_list.erase(
                                        m_render_scene->m_point_light_list.begin() + point_light_index_in_scene);
                                }
                            }
                        }

                        // spot light
                        {
                            int spot_light_index_in_scene = -1;
                            for (size_t i = 0; i < m_render_scene->m_spot_light_list.size(); i++)
                            {
                                auto& tmp_light = m_render_scene->m_spot_light_list[i];

                                if ((tmp_light.m_gobject_id == gobject.getId()) &&
                                    (tmp_light.m_gcomponent_id == game_object_part.m_component_id))
                                {
                                    spot_light_index_in_scene = i;
                                    break;
                                }
                            }

                            if (game_object_part.m_spot_light_desc.m_is_active)
                            {
                                if (spot_light_index_in_scene == -1)
                                {
                                    m_render_scene->m_spot_light_list.push_back(game_object_part.m_spot_light_desc);
                                    m_render_scene->m_spot_light_list.back().m_gobject_id = gobject.getId();
                                    m_render_scene->m_spot_light_list.back().m_gcomponent_id =
                                        game_object_part.m_component_id;
                                }
                                else
                                {
                                    m_render_scene->m_spot_light_list[spot_light_index_in_scene] =
                                        game_object_part.m_spot_light_desc;
                                    m_render_scene->m_spot_light_list[spot_light_index_in_scene].m_gobject_id =
                                        gobject.getId();
                                    m_render_scene->m_spot_light_list[spot_light_index_in_scene].m_gcomponent_id =
                                        game_object_part.m_component_id;
                                }
                            }
                            else
                            {
                                if (spot_light_index_in_scene != -1)
                                {
                                    m_render_scene->m_spot_light_list.erase(m_render_scene->m_spot_light_list.begin() +
                                                                            spot_light_index_in_scene);
                                }
                            }
                        }

                    }

                }
                // after finished processing, pop this game object
                swap_data.m_game_object_resource_desc->popProcessObject();
            }

            // reset game object swap data to a clean state
            m_swap_context.resetGameObjectResourceSwapData();
        }

        // remove deleted objects
        if (swap_data.m_game_object_to_delete.has_value())
        {
            while (!swap_data.m_game_object_to_delete->isEmpty())
            {
                GameObjectDesc gobject = swap_data.m_game_object_to_delete->getNextProcessObject();
                
                m_render_scene->deleteEntityByGObjectID(gobject.getId());

                m_render_scene->deleteDirectionLightByGObjectID(gobject.getId());
                m_render_scene->deletePointLightByGObjectID(gobject.getId());
                m_render_scene->deleteSpotLightByGObjectID(gobject.getId());

                swap_data.m_game_object_to_delete->popProcessObject();
            }

            m_swap_context.resetGameObjectToDelete();
        }

        // process camera swap data
        if (swap_data.m_camera_swap_data.has_value())
        {
            if (swap_data.m_camera_swap_data->m_fov_y.has_value())
            {
                m_render_camera->setFOVy(*swap_data.m_camera_swap_data->m_fov_y);
            }

            if (swap_data.m_camera_swap_data->m_view_matrix.has_value())
            {
                m_render_camera->setMainViewMatrix(*swap_data.m_camera_swap_data->m_view_matrix);
            }

            if (swap_data.m_camera_swap_data->m_camera_type.has_value())
            {
                m_render_camera->setCurrentCameraType(*swap_data.m_camera_swap_data->m_camera_type);
            }

            m_swap_context.resetCameraSwapData();
        }
    }


} // namespace Pilot
