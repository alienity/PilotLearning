#include "runtime/function/render/render_pass_base.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/render/render_system.h"

namespace Pilot
{
    void RenderPassBase::postInitialize() {}
    void RenderPassBase::setCommonInfo(RenderPassCommonInfo common_info)
    {
        m_render_resource = common_info.render_resource;
    }
    void RenderPassBase::preparePassData(std::shared_ptr<RenderResourceBase> render_resource) {}
    void RenderPassBase::initializeUIRenderBackend(WindowUI* window_ui) {}
} // namespace Pilot
