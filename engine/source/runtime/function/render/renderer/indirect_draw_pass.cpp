#include "runtime/function/render/renderer/indirect_draw_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void IndirectDrawPass::initialize(const DrawPassInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;
        depthTexDesc = init_info.depthTexDesc;
	}

    void IndirectDrawPass::update(RHI::D3D12CommandContext& context,
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

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectDrawOpaquePass");

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

            RHI::D3D12GraphicsContext& graphicContext = context.GetGraphicsContext();

            ID3D12CommandSignature* pCommandSignature = CommandSignatures::pIndirectDraw->GetApiHandle();

            std::shared_ptr<RHI::D3D12Texture> renderTargetColor = registry.GetD3D12Texture(drawPassOutput->renderTargetColorHandle);
            std::shared_ptr<RHI::D3D12Texture> renderTargetDepth = registry.GetD3D12Texture(drawPassOutput->renderTargetDepthHandle);

            RHI::D3D12RenderTargetView* renderTargetView = renderTargetColor->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = renderTargetDepth->GetDefaultDSV().get();

            graphicContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext.SetViewport(
                RHIViewport {0.0f, 0.0f, (float)colorTexDesc.Width, (float)colorTexDesc.Height, 0.0f, 1.0f});
            graphicContext.SetScissorRect(RHIRect {0, 0, (int)colorTexDesc.Width, (int)colorTexDesc.Height});

            if (needClearRenderTarget)
            {
                graphicContext.ClearRenderTarget(renderTargetView, depthStencilView);
            }
            graphicContext.SetRenderTarget(renderTargetView, depthStencilView);

            graphicContext.SetRootSignature(RootSignatures::pIndirectDraw.get());
            graphicContext.SetPipelineState(PipelineStates::pIndirectDraw.get());
            graphicContext->SetGraphicsRootConstantBufferView(1, pPerframeBuffer->GetGpuVirtualAddress());
            graphicContext->SetGraphicsRootShaderResourceView(2, pMeshBuffer->GetGpuVirtualAddress());
            graphicContext->SetGraphicsRootShaderResourceView(3, pMaterialBuffer->GetGpuVirtualAddress());

            graphicContext->ExecuteIndirect(pCommandSignature,
                                            HLSL::MeshLimit,
                                            pIndirectCommandBuffer->GetResource(),
                                            0,
                                            pIndirectCommandBuffer->GetCounterBuffer()->GetResource(),
                                            0);
        });
    }


    void IndirectDrawPass::destroy()
    {

    }

    bool IndirectDrawPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->renderTargetColorHandle.IsValid())
        {
            needClearRenderTarget = true;
            drawPassOutput->renderTargetColorHandle = graph.Create<RHI::D3D12Texture>(colorTexDesc);
        }
        if (!drawPassOutput->renderTargetDepthHandle.IsValid())
        {
            needClearRenderTarget = true;
            drawPassOutput->renderTargetDepthHandle = graph.Create<RHI::D3D12Texture>(depthTexDesc);
        }
        return needClearRenderTarget;
    }

}
