#include "runtime/function/render/render_pass.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/render/render_resource.h"

Pilot::VisiableNodes Pilot::RenderPass::m_visiable_nodes;

namespace Pilot
{
    void RenderPass::initialize(const RenderPassInitInfo* init_info)
    {
        m_global_render_resource =
            &(std::static_pointer_cast<RenderResource>(m_render_resource)->m_global_render_resource);
    }

    void RenderPass::postInitialize() {}

    void RenderPass::update(RHI::D3D12CommandContext& context,
                            RHI::RenderGraph&         graph,
                            PassInput&                passInput,
                            PassOutput&               passOutput)
    {}

    void RenderPass::destroy() {}

} // namespace Pilot
