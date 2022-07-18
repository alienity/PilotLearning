#include "runtime/function/render/render_system.h"

#include "runtime/core/base/macro.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/function/render/window_system.h"

#include "runtime/function/render/renderer_moyu.h"

namespace Pilot
{
    RenderSystem::~RenderSystem() {}

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

        m_render_resource = std::make_shared<RenderResource>();
        m_render_resource->uploadGlobalRenderResource(level_resource_desc);

        // setup render camera
        const CameraPose& camera_pose = global_rendering_res.m_camera_config.m_pose;
        m_render_camera               = std::make_shared<RenderCamera>();
        m_render_camera->lookAt(camera_pose.m_position, camera_pose.m_target, camera_pose.m_up);
        m_render_camera->m_zfar  = global_rendering_res.m_camera_config.m_z_far;
        m_render_camera->m_znear = global_rendering_res.m_camera_config.m_z_near;
        m_render_camera->setAspect(global_rendering_res.m_camera_config.m_aspect.x /
                                   global_rendering_res.m_camera_config.m_aspect.y);

        // setup render scene
        m_render_scene                  = std::make_shared<RenderScene>();
        m_render_scene->m_ambient_light = {global_rendering_res.m_ambient_light.toVector3()};
        m_render_scene->m_directional_light.m_direction =
            Vector3::normalize(global_rendering_res.m_directional_light.m_direction);
        m_render_scene->m_directional_light.m_color = global_rendering_res.m_directional_light.m_color.toVector3();
        m_render_scene->setVisibleNodesReference();

        // initialize render manager
        RHI::DeviceOptions depthOptions = {};
        depthOptions.FeatureLevel       = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
        RendererInitInfo renderInitInfo = {};
        renderInitInfo.Options          = depthOptions;
        renderInitInfo.Window_system    = init_info.window_system;
        m_renderer_manager = std::make_shared<RendererManager>();
        m_renderer_manager->Initialize(renderInitInfo);
    }

    void RenderSystem::tick()
    {
        // process swap data between logic and render contexts
        processSwapData();

        // update per-frame buffer
        m_render_resource->updatePerFrameBuffer(m_render_scene, m_render_camera);

        // update per-frame visible objects
        m_render_scene->updateVisibleObjects(std::static_pointer_cast<RenderResource>(m_render_resource),
                                             m_render_camera);

        // prepare pipeline's render passes data
        m_renderer_manager->PreparePassData(m_render_resource);

        // render one frame
        m_renderer_manager->Tick();
    }

    void RenderSystem::swapLogicRenderData()
    {

    }

    RenderSwapContext& RenderSystem::getSwapContext() 
    { 
        return m_swap_context;
    }

    std::shared_ptr<RenderCamera> RenderSystem::getRenderCamera() const { return m_render_camera; }

    void RenderSystem::initializeUIRenderBackend(WindowUI* window_ui)
    {

    }

    void RenderSystem::updateEngineContentViewport(float offset_x, float offset_y, float width, float height)
    {

    }

    uint32_t RenderSystem::getGuidOfPickedMesh(const Vector2& picked_uv)
    {

    }

    GObjectID RenderSystem::getGObjectIDByMeshID(uint32_t mesh_id) const
    {

    }

    EngineContentViewport RenderSystem::getEngineContentViewport() const
    {

    }

    void RenderSystem::createAxis(std::array<RenderEntity, 3> axis_entities, std::array<RenderMeshData, 3> mesh_datas)
    {

    }

    void RenderSystem::setVisibleAxis(std::optional<RenderEntity> axis)
    {

    }

    void RenderSystem::setSelectedAxis(size_t selected_axis)
    {

    }

    GuidAllocator<GameObjectPartId>& RenderSystem::getGOInstanceIdAllocator()
    {

    }

    GuidAllocator<MeshSourceDesc>& RenderSystem::getMeshAssetIdAllocator()
    {

    }

    void RenderSystem::clearForLevelReloading()
    {

    }

    void RenderSystem::processSwapData()
    {

    }


} // namespace Pilot
