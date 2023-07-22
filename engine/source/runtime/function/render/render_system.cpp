#include "runtime/function/render/render_system.h"

#include "runtime/core/base/macro.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/function/render/renderer_moyu.h"
#include "runtime/function/render/glm_wrapper.h"
#include "runtime/function/render/render_pass.h"

namespace MoYu
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

        //***********************
        /*
        global_rendering_res.m_enable_fxaa = false;
        
        global_rendering_res.m_skybox_irradiance_map = {"asset/texture/sky/skybox_irradiance_X-.hdr",
                                                        "asset/texture/sky/skybox_irradiance_X+.hdr",
                                                        "asset/texture/sky/skybox_irradiance_Y-.hdr",
                                                        "asset/texture/sky/skybox_irradiance_Y+.hdr",
                                                        "asset/texture/sky/skybox_irradiance_Z-.hdr",
                                                        "asset/texture/sky/skybox_irradiance_Z+.hdr"};

        global_rendering_res.m_skybox_specular_map = {"asset/texture/sky/skybox_specular_X-.hdr",
                                                      "asset/texture/sky/skybox_specular_X+.hdr",
                                                      "asset/texture/sky/skybox_specular_Y-.hdr",
                                                      "asset/texture/sky/skybox_specular_Y+.hdr",
                                                      "asset/texture/sky/skybox_specular_Z-.hdr",
                                                      "asset/texture/sky/skybox_specular_Z+.hdr"};

        global_rendering_res.m_brdf_map = "asset/texture/global/brdf_schilk.hdr";

        global_rendering_res.m_color_grading_map = "asset/texture/lut/color_grading_lut_01.png";

        global_rendering_res.m_sky_color     = Color(0.53, 0.81, 0.98);
        global_rendering_res.m_ambient_light = Color(0.03, 0.03, 0.03);

        global_rendering_res.m_camera_config = {
            CameraPose {Vector3(0.0, 1.0, 3.0), Vector3(0.0, 1.0, 3.0), Vector3(0.0, 1.0, 3.0)},
            Vector2(1280.0, 768.0),
            1000.0,
            0.1,
            0.53};

        global_rendering_res.m_directional_light = {Vector3(0.5, -0.5, 0.5), Vector3(1.0, 1.0, 1.0)};

        asset_manager->saveAsset(global_rendering_res, global_rendering_res_url);
        */
        //***********************

        /*
        // upload ibl, color grading textures
        LevelResourceDesc level_resource_desc;
        level_resource_desc.m_ibl_resource_desc.m_skybox_irradiance_map = global_rendering_res.m_skybox_irradiance_map;
        level_resource_desc.m_ibl_resource_desc.m_skybox_specular_map   = global_rendering_res.m_skybox_specular_map;
        level_resource_desc.m_ibl_resource_desc.m_brdf_map              = global_rendering_res.m_brdf_map;
        level_resource_desc.m_color_grading_resource_desc.m_color_grading_map = global_rendering_res.m_color_grading_map;
        */

        // initialize render manager
        RHI::DeviceOptions deviceOptions              = {};
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
        
        // setup render camera
        const CameraPose& camera_pose = global_rendering_res.m_camera_config.m_pose;
        m_render_camera               = std::make_shared<RenderCamera>();
        m_render_camera->lookAt(camera_pose.m_position, camera_pose.m_target, camera_pose.m_up);
        m_render_camera->m_zfar  = global_rendering_res.m_camera_config.m_z_far;
        m_render_camera->m_znear = global_rendering_res.m_camera_config.m_z_near;
        m_render_camera->setAspect(global_rendering_res.m_camera_config.m_aspect.x /
                                   global_rendering_res.m_camera_config.m_aspect.y);
        m_render_camera->setFOVy(global_rendering_res.m_camera_config.m_fovY);

        // setup render scene
        m_render_scene = std::make_shared<RenderScene>();
        
        // update global resources
        m_render_resource->updateGlobalRenderResource(m_render_scene.get(), global_rendering_res);

        // set pass data
        RenderPass::m_render_scene = m_render_scene.get();
        RenderPass::m_render_camera = m_render_camera.get();

        // initialize renderer
        m_renderer_manager->InitRenderer();
        //m_renderer_manager->PreparePassData(m_render_resource);
    }

    void RenderSystem::tick()
    {
        // process swap data between logic and render contexts
        this->processSwapData();

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
        m_renderer_manager->InitUIRenderer(window_ui);
    }

    EngineContentViewport RenderSystem::getEngineContentViewport() const
    {
        return m_renderer_manager->p_MoYuRenderer->GetViewPort();
    }

    void RenderSystem::clearForLevelReloading()
    {
        //m_render_scene->clearForLevelReloading();
    }

    void RenderSystem::processSwapData()
    {
        RenderSwapData& swap_data = m_swap_context.getRenderSwapData();

        //std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
        //ASSERT(asset_manager);

        // update game object if needed
        if (swap_data.m_game_object_resource_desc.has_value())
        {
            while (!swap_data.m_game_object_resource_desc->m_game_object_descs.empty())
            {
                GameObjectDesc gameObjDesc = swap_data.m_game_object_resource_desc->m_game_object_descs.front();
                swap_data.m_game_object_resource_desc->m_game_object_descs.pop_front();

                auto& objParts = gameObjDesc.getObjectParts();
                for (int i = 0; i < objParts.size(); i++)
                {
                    MoYu::SceneTransform meshTransform = objParts[i].m_transform_desc;

                    if (objParts[i].m_component_type & ComponentType::C_MeshRenderer)
                    {
                        MoYu::SceneMeshRenderer meshRenderer = objParts[i].m_mesh_renderer_desc;
                        m_render_scene->updateMeshRenderer(meshRenderer, meshTransform, m_render_resource);
                    }
                    else if (objParts[i].m_component_type & ComponentType::C_Light)
                    {
                        MoYu::SceneLight sceneLight = objParts[i].m_scene_light;
                        m_render_scene->updateLight(sceneLight, meshTransform);
                    }
                    else if (objParts[i].m_component_type & ComponentType::C_Camera)
                    {
                        MoYu::SceneCamera sceneCamera = objParts[i].m_scene_camera;
                        m_render_scene->updateCamera(sceneCamera, meshTransform);
                    }
                }
            }
            swap_data.m_game_object_resource_desc.reset();
        }

        // remove game object
        if (swap_data.m_game_object_to_delete.has_value())
        {
            while (!swap_data.m_game_object_to_delete->m_game_object_descs.empty())
            {
                GameObjectDesc gameObjDesc = swap_data.m_game_object_to_delete->m_game_object_descs.front();
                swap_data.m_game_object_to_delete->m_game_object_descs.pop_front();

                auto& objParts = gameObjDesc.getObjectParts();
                for (int i = 0; i < objParts.size(); i++)
                {
                    if (objParts[i].m_component_type & ComponentType::C_MeshRenderer)
                    {
                        MoYu::SceneMeshRenderer meshRenderer = objParts[i].m_mesh_renderer_desc;
                        m_render_scene->removeMeshRenderer(meshRenderer);
                    }
                    else if (objParts[i].m_component_type & ComponentType::C_Light)
                    {
                        MoYu::SceneLight sceneLight = objParts[i].m_scene_light;
                        m_render_scene->removeLight(sceneLight);
                    }
                    else if (objParts[i].m_component_type & ComponentType::C_Camera)
                    {
                        MoYu::SceneCamera sceneCamera = objParts[i].m_scene_camera;
                        m_render_scene->removeCamera(sceneCamera);
                    }
                }
            }
            swap_data.m_game_object_to_delete.reset();
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
            swap_data.m_camera_swap_data.reset();
        }
    }


} // namespace MoYu
