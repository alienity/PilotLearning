#include "runtime/function/render/renderer/smaa_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void SMAAPass::initialize(const SMAAInitInfo& init_info)
	{
        mTmpColorTexDesc = init_info.m_ColorTexDesc;
        mTmpColorTexDesc.SetAllowUnorderedAccess();
        mTmpColorTexDesc.SetAllowRenderTarget(false);

    }

    void SMAAPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        DrawInputParameters  drawPassInput  = passInput;
        DrawOutputParameters drawPassOutput = passOutput;

        
    }

    void SMAAPass::destroy()
    {

    }

}
