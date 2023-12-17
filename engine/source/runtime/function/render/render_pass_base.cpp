#include "runtime/function/render/render_pass_base.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_system.h"

#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphRegistry.h"

namespace MoYu
{
    void RenderPassBase::setCommonInfo(RenderPassCommonInfo common_info)
    {
        m_RenderGraphAllocator = common_info.renderGraphAllocator;
        m_RenderGraphRegistry  = common_info.renderGraphRegistry;
        m_Device               = common_info.device;
        m_WindowSystem         = common_info.windowsSystem;
    }
} // namespace MoYu
