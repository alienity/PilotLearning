#include "runtime/function/render/render_system.h"

#include "runtime/core/base/macro.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/render_camera.h"
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

        // setup render camera
        const CameraPose& camera_pose = global_rendering_res.m_camera_config.m_pose;
        m_render_camera               = std::make_shared<RenderCamera>();
        m_render_camera->lookAt(camera_pose.m_position, camera_pose.m_target, camera_pose.m_up);
        m_render_camera->m_zfar  = global_rendering_res.m_camera_config.m_z_far;
        m_render_camera->m_znear = global_rendering_res.m_camera_config.m_z_near;
        m_render_camera->setAspect(global_rendering_res.m_camera_config.m_aspect.x /
                                   global_rendering_res.m_camera_config.m_aspect.y);

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


} // namespace Pilot
