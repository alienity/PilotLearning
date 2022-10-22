#include "runtime/function/render/renderer/indirect_draw_transparent_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void IndirectDrawTransparentPass::initialize(const DrawPassInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;
        depthTexDesc = init_info.depthTexDesc;
	}

    void IndirectDrawTransparentPass::update(RHI::D3D12CommandContext& context,
                                             RHI::RenderGraph&         graph,
                                             DrawInputParameters&      passInput,
                                             DrawOutputParameters&     passOutput)
    {
        DrawInputParameters*  drawPassInput  = &passInput;
        DrawOutputParameters* drawPassOutput = &passOutput;

        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = drawPassInput->pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer     = drawPassInput->pMeshBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer = drawPassInput->pMaterialBuffer;

        std::shared_ptr<RHI::D3D12Buffer> pIndirectCommandBuffer = drawPassInput->pIndirectCommandBuffer;

        bool needClearRenderTarget = initializeRenderTarget(graph, drawPassOutput);

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectDrawTransparentPass");

        if (drawPassInput->directionalShadowmapTexHandle.IsValid())
        {
            drawpass.Read(drawPassInput->directionalShadowmapTexHandle);
        }
        for (size_t i = 0; i < drawPassInput->spotShadowmapTexHandles.size(); i++)
        {
            drawpass.Read(drawPassInput->spotShadowmapTexHandles[i]);
        }
        drawpass.Write(&drawPassOutput->renderTargetColorHandle);
        drawpass.Write(&drawPassOutput->renderTargetDepthHandle);

        drawpass.Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {
            ID3D12CommandSignature* pCommandSignature =
                registry.GetCommandSignature(CommandSignatures::IndirectDraw)->GetApiHandle();

            RHI::D3D12RenderTargetView* renderTargetView =
                registry.GetD3D12RenderTargetView(drawPassOutput->renderTargetColorRTVHandle);
            RHI::D3D12DepthStencilView* depthStencilView =
                registry.GetD3D12DepthStencilView(drawPassOutput->renderTargetDepthDSVHandle);

            context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context.SetViewport(
                RHIViewport {0.0f, 0.0f, (float)colorTexDesc.Width, (float)colorTexDesc.Height, 0.0f, 1.0f});
            context.SetScissorRect(RHIRect {0, 0, (int)colorTexDesc.Width, (int)colorTexDesc.Height});

            if (needClearRenderTarget)
            {
                context.ClearRenderTarget(renderTargetView, depthStencilView);
            }
            context.SetRenderTarget(renderTargetView, depthStencilView);

            context.SetGraphicsRootSignature(registry.GetRootSignature(RootSignatures::IndirectDraw));
            context.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectDrawTransparent));
            context->SetGraphicsRootConstantBufferView(1, pPerframeBuffer->GetGpuVirtualAddress());
            context->SetGraphicsRootShaderResourceView(2, pMeshBuffer->GetGpuVirtualAddress());
            context->SetGraphicsRootShaderResourceView(3, pMaterialBuffer->GetGpuVirtualAddress());

            context->ExecuteIndirect(pCommandSignature,
                                     HLSL::MeshLimit,
                                     pIndirectCommandBuffer->GetResource(),
                                     0,
                                     pIndirectCommandBuffer->GetResource(),
                                     HLSL::commandBufferCounterOffset);
        });
    }

    void IndirectDrawTransparentPass::destroy() {}

    bool IndirectDrawTransparentPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->renderTargetColorHandle.IsValid())
        {
            needClearRenderTarget = true;
            drawPassOutput->renderTargetColorHandle = graph.Create<RHI::D3D12Texture>(colorTexDesc);
        }
        if (drawPassOutput->renderTargetColorHandle.IsValid())
        {
            if (!drawPassOutput->renderTargetColorSRVHandle.IsValid())
            {
                drawPassOutput->renderTargetColorSRVHandle = graph.Create<RHI::D3D12ShaderResourceView>(
                    RHI::RgViewDesc().SetResource(drawPassOutput->renderTargetColorHandle).AsTextureSrv());
            }
            if (!drawPassOutput->renderTargetColorRTVHandle.IsValid())
            {
                drawPassOutput->renderTargetColorRTVHandle = graph.Create<RHI::D3D12RenderTargetView>(
                    RHI::RgViewDesc().SetResource(drawPassOutput->renderTargetColorHandle).AsRtv(false));
            }
        }
        if (!drawPassOutput->renderTargetDepthHandle.IsValid())
        {
            needClearRenderTarget = true;
            drawPassOutput->renderTargetDepthHandle = graph.Create<RHI::D3D12Texture>(depthTexDesc);
        }
        if (drawPassOutput->renderTargetDepthHandle.IsValid())
        {
            if (!drawPassOutput->renderTargetDepthDSVHandle.IsValid())
            {
                drawPassOutput->renderTargetDepthDSVHandle = graph.Create<RHI::D3D12DepthStencilView>(
                    RHI::RgViewDesc().SetResource(drawPassOutput->renderTargetDepthHandle).AsDsv());
            }
        }
        return needClearRenderTarget;
    }

}
