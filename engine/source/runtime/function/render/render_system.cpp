#include "runtime/function/render/render_system.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/core/math/moyu_math2.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/function/render/renderer_moyu.h"
#include "runtime/function/render/render_pass.h"

#include "runtime/platform/system/systemtime.h"

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

        //********************************************************************
        /*
        //global_rendering_res.m_ibl_map.m_dfg_map = "asset/texture/global/DFG.dds";
        //global_rendering_res.m_ibl_map.m_ld_map = "asset/texture/global/LD.dds";

        //global_rendering_res.m_ibl_map.m_SH.clear();
        //global_rendering_res.m_ibl_map.m_SH.push_back({0.175984, 0.254935, -0.499536, 0.406282});
        //global_rendering_res.m_ibl_map.m_SH.push_back({0.171224, 0.319489, -0.519344, 0.444903});
        //global_rendering_res.m_ibl_map.m_SH.push_back({0.145627, 0.383557, -0.483396, 0.455199});
        //global_rendering_res.m_ibl_map.m_SH.push_back({0.129544, -0.269722, 0.386505, -0.307530});
        //global_rendering_res.m_ibl_map.m_SH.push_back({0.123614, -0.271124, 0.373376, -0.304015});
        //global_rendering_res.m_ibl_map.m_SH.push_back({0.105162, -0.248446, 0.305565, -0.275927});
        //global_rendering_res.m_ibl_map.m_SH.push_back({0.034980, 0.041000, 0.041401, 1.000000});

        global_rendering_res.m_terrain_map.m_height_map = "asset/texture/terrain/HeightMap.png";
        global_rendering_res.m_terrain_map.m_normal_map = "asset/texture/terrain/NormalMap.png";
        
        asset_manager->saveAsset(global_rendering_res, global_rendering_res_url);
        */
        //********************************************************************

        // initialize render manager
        RHI::DeviceOptions deviceOptions              = {};
#ifdef _DEBUG
        deviceOptions.EnableDebugLayer                = true;
        deviceOptions.EnableAutoDebugName             = true;
        //deviceOptions.EnableGpuBasedValidation        = true;
        deviceOptions.WaveIntrinsics                  = true;
        deviceOptions.DynamicResources                = true;
#endif
        RendererManagerInitInfo renderManagerInitInfo = {};
        renderManagerInitInfo.Options                 = deviceOptions;
        renderManagerInitInfo.Window_system           = init_info.window_system;
        m_renderer_manager                            = std::make_shared<RendererManager>();
        m_renderer_manager->Initialize(renderManagerInitInfo);

        // initial global resources
        m_render_resource = std::make_shared<RenderResource>();
        m_render_resource->iniUploadBatch(m_renderer_manager->GetDevice());
        m_render_resource->InitDefaultTextures();
        
        // setup render camera
        const CameraPose& camera_pose = global_rendering_res.m_camera_config.m_pose;
        m_render_camera = std::make_shared<RenderCamera>();
        m_render_camera->lookAt(camera_pose.m_position, camera_pose.m_target, camera_pose.m_up);
        m_render_camera->perspectiveProjection(global_rendering_res.m_camera_config.m_aspect.x,
                                               global_rendering_res.m_camera_config.m_aspect.y,
                                               global_rendering_res.m_camera_config.m_z_near,
                                               global_rendering_res.m_camera_config.m_z_far,
                                               global_rendering_res.m_camera_config.m_fovY);
        m_render_camera->m_isPerspective = true;

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
        // tick time
        double deltaTimeMilisec = g_SystemTime.Tick();

        // process swap data between logic and render contexts
        this->processSwapData(deltaTimeMilisec);

        // prepare pipeline's render passes data
        m_renderer_manager->PreparePassData(m_render_resource);

        // update terrain mesh data;
        m_render_scene->updateTerrainClipmap(m_render_camera->position(), m_render_resource.get());

        // finish static resource uploading
        m_render_resource->commitUploadBatch();

        // render one frame
        m_renderer_manager->Tick(deltaTimeMilisec);
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

    void RenderSystem::processSwapData(float deltaTimeMs)
    {
        RenderSwapData& swap_data = m_swap_context.getRenderSwapData();

        // update common data
        m_render_camera->updatePerFrame();

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
                    else if (objParts[i].m_component_type & ComponentType::C_Terrain)
                    {
                        MoYu::SceneTerrainRenderer terrainRenderer = objParts[i].m_terrain_mesh_renderer_desc;
                        m_render_scene->updateTerrainRenderer(terrainRenderer, meshTransform, m_render_resource);
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
                    else if (objParts[i].m_component_type & ComponentType::C_Terrain)
                    {

                    }
                }
            }
            swap_data.m_game_object_to_delete.reset();
        }

        // process camera swap data
        if (swap_data.m_camera_swap_data.has_value())
        {
            m_render_camera->m_isPerspective = swap_data.m_camera_swap_data->m_is_perspective;
            m_render_camera->setMainViewMatrix(swap_data.m_camera_swap_data->m_view_matrix,
                                               swap_data.m_camera_swap_data->m_camera_type);
            m_render_camera->perspectiveProjection(swap_data.m_camera_swap_data->m_width,
                                                   swap_data.m_camera_swap_data->m_height,
                                                   swap_data.m_camera_swap_data->m_nearZ,
                                                   swap_data.m_camera_swap_data->m_farZ,
                                                   swap_data.m_camera_swap_data->m_fov_y);

            swap_data.m_camera_swap_data.reset();
        }
    }


} // namespace MoYu
