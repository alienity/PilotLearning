#include "runtime/function/render/renderer/indirect_draw_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void IndirectDrawPass::initialize(const DrawPassInitInfo& init_info)
	{
        depthBufferTexDesc = init_info.depthBufferTexDesc;
	}

    void IndirectDrawPass::update(RHI::D3D12CommandContext& context,
                                  RHI::RenderGraph&         graph,
                                  DrawInputParameters&      passInput,
                                  DrawOutputParameters&     passOutput)
    {
        DrawInputParameters*  drawPassInput  = &passInput;
        DrawOutputParameters* drawPassOutput = &passOutput;

        int commandBufferCounterOffset = drawPassInput->commandBufferCounterOffset;

        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = drawPassInput->pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer     = drawPassInput->pMeshBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer = drawPassInput->pMaterialBuffer;

        std::shared_ptr<RHI::D3D12Buffer> pIndirectCommandBuffer = drawPassInput->pIndirectCommandBuffer;

        drawPassOutput->renderTargetDepthHandle    = graph.Create<RHI::D3D12Texture>(depthBufferTexDesc);
        drawPassOutput->renderTargetDepthDSVHandle = graph.Create<RHI::D3D12DepthStencilView>(
            RHI::RgViewDesc().SetResource(drawPassOutput->renderTargetDepthHandle).AsDsv());

        graph.AddRenderPass("IndirectDrawPass")
            .Write(&drawPassOutput->renderTargetColorHandle)
            .Write(&drawPassOutput->renderTargetDepthHandle)
            .Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {

                RHI::D3D12Texture* rtColorTexture =
                    registry.GetD3D12Texture(drawPassOutput->renderTargetColorHandle);
                D3D12_RESOURCE_DESC rtColorTextureDesc = rtColorTexture->GetDesc();

                int rtColorWidth  = rtColorTextureDesc.Width;
                int rtColorHeight = rtColorTextureDesc.Height;

                ID3D12CommandSignature* pCommandSignature =
                    registry.GetCommandSignature(CommandSignatures::IndirectDraw)->GetApiHandle();

                RHI::D3D12RenderTargetView* renderTargetView =
                    registry.GetD3D12RenderTargetView(drawPassOutput->renderTargetColorRTVHandle);
                RHI::D3D12DepthStencilView* depthStencilView =
                    registry.GetD3D12DepthStencilView(drawPassOutput->renderTargetDepthDSVHandle);

                context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                context.SetViewport(RHIViewport {0.0f, 0.0f, (float)rtColorWidth, (float)rtColorHeight, 0.0f, 1.0f});
                context.SetScissorRect(RHIRect {0, 0, rtColorWidth, rtColorHeight});
                
                context.ClearRenderTarget(renderTargetView, depthStencilView);
                context.SetRenderTarget(renderTargetView, depthStencilView);

                context.SetGraphicsRootSignature(registry.GetRootSignature(RootSignatures::IndirectDraw));
                context.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectDraw));
                context->SetGraphicsRootConstantBufferView(1, pPerframeBuffer->GetGpuVirtualAddress());
                context->SetGraphicsRootShaderResourceView(2, pMeshBuffer->GetGpuVirtualAddress());
                context->SetGraphicsRootShaderResourceView(3, pMaterialBuffer->GetGpuVirtualAddress());

                context->ExecuteIndirect(pCommandSignature,
                                         HLSL::MeshLimit,
                                         pIndirectCommandBuffer->GetResource(),
                                         0,
                                         pIndirectCommandBuffer->GetResource(),
                                         commandBufferCounterOffset);
            });
    }


    void IndirectDrawPass::destroy()
    {

    }

}
