#include "runtime/function/render/render_system.h"

#include "runtime/core/base/macro.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    RenderSystem::~RenderSystem() {}

    void RenderSystem::initialize(RenderSystemInitInfo init_info)
    {
        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        ASSERT(config_manager);
        std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
        ASSERT(asset_manager);


    }

    void RenderSystem::tick()
    {

    }

    void RenderSystem::swapLogicRenderData()
    {

    }

    RenderSwapContext& RenderSystem::getSwapContext() 
    { 
        return m_swap_context;
    }

    std::shared_ptr<RenderCamera> RenderSystem::getRenderCamera() const
    {
        return nullptr;
    }


} // namespace Pilot
