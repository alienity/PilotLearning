#include "runtime/function/render/renderer/indirect_draw_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void IndirectDrawPass::initialize(const RenderPassInitInfo& init_info)
	{

	}

    void IndirectDrawPass::update(RHI::D3D12CommandContext& context,
                                  RHI::RenderGraph&         graph,
                                  PassInput&                passInput,
                                  PassOutput&               passOutput)
    {

    }

    void IndirectDrawPass::draw(RHI::D3D12CommandContext&   context,
                                RHI::D3D12RenderTargetView* pRTV,
                                int                         backBufWidth,
                                int                         backBufHeight)
    {

    }

    void IndirectDrawPass::destroy()
    {

    }

}
