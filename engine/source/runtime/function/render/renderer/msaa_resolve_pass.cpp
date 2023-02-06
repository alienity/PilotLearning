#include "runtime/function/render/renderer/msaa_resolve_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void MSAAResolvePass::initialize(const MSAAResolveInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;
        depthTexDesc = init_info.depthTexDesc;
	}

    void MSAAResolvePass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        DrawInputParameters*  drawPassInput  = &passInput;
        DrawOutputParameters* drawPassOutput = &passOutput;

        initializeResolveTarget(graph, drawPassOutput);

        RHI::RenderPass& resolvepass = graph.AddRenderPass("ResolveMSAA");

        resolvepass.Resolve(drawPassInput->renderTargetColorHandle, drawPassOutput->resolveTargetColorHandle);
        resolvepass.Resolve(drawPassInput->renderTargetDepthHandle, drawPassOutput->resolveTargetDepthHandle);

        resolvepass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12Texture* pRenderTargetColor = registry->GetD3D12Texture(drawPassInput->renderTargetColorHandle);
            RHI::D3D12Texture* pRenderTargetDepth = registry->GetD3D12Texture(drawPassInput->renderTargetDepthHandle);

            RHI::D3D12Texture* pResolveTargetColor = registry->GetD3D12Texture(drawPassOutput->resolveTargetColorHandle);
            RHI::D3D12Texture* pResolveTargetDepth = registry->GetD3D12Texture(drawPassOutput->resolveTargetDepthHandle);

            context->TransitionBarrier(pRenderTargetColor, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            context->TransitionBarrier(pRenderTargetDepth, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            context->TransitionBarrier(pResolveTargetColor, D3D12_RESOURCE_STATE_RESOLVE_DEST);
            context->TransitionBarrier(pResolveTargetDepth, D3D12_RESOURCE_STATE_RESOLVE_DEST);
            context->FlushResourceBarriers();

            context->GetGraphicsCommandList()->ResolveSubresource(pRenderTargetColor->GetResource(),
                                                                  D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                                                  pResolveTargetColor->GetResource(),
                                                                  D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                                                  colorTexDesc.Format);

            context->GetGraphicsCommandList()->ResolveSubresource(pRenderTargetDepth->GetResource(),
                                                                  D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                                                  pResolveTargetDepth->GetResource(),
                                                                  D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                                                  depthTexDesc.Format);
        });
    }


    void MSAAResolvePass::destroy()
    {

    }

    bool MSAAResolvePass::initializeResolveTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        if (!drawPassOutput->resolveTargetColorHandle.IsValid())
        {
            drawPassOutput->resolveTargetColorHandle = graph.Create<RHI::D3D12Texture>(colorTexDesc);
        }
        if (!drawPassOutput->resolveTargetDepthHandle.IsValid())
        {
            drawPassOutput->resolveTargetDepthHandle = graph.Create<RHI::D3D12Texture>(depthTexDesc);
        }
        return true;
    }

}
