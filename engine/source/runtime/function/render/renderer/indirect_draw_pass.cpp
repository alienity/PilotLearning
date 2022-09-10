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

        int numMeshes = drawPassInput->numMeshes;
        int commandBufferCounterOffset = drawPassInput->commandBufferCounterOffset;

        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = drawPassInput->pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer     = drawPassInput->pMeshBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer = drawPassInput->pMaterialBuffer;

        std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer = drawPassInput->p_IndirectCommandBuffer;

        RHI::RgResourceHandle       backBufColor     = drawPassOutput->backBufColor;
        RHI::D3D12RenderTargetView* renderTargetView = drawPassOutput->backBufRtv;

        drawPassOutput->backBufDepth = graph.Create<RHI::D3D12Texture>(depthBufferTexDesc);
        drawPassOutput->backBufDsv   = graph.Create<RHI::D3D12DepthStencilView>(
            RHI::RgViewDesc().SetResource(drawPassOutput->backBufDepth).AsDsv());

        graph.AddRenderPass("IndirectDrawPass")
            .Write(&backBufColor)
            .Write(&drawPassOutput->backBufDepth)
            .Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {
                RHI::D3D12Texture*  backBufTex    = registry.GetImportedResource(backBufColor);
                D3D12_RESOURCE_DESC backBufDesc   = backBufTex->GetDesc();
                int                 backBufWidth  = backBufDesc.Width;
                int                 backBufHeight = backBufDesc.Height;

                ID3D12CommandSignature* pCommandSignature =
                    registry.GetCommandSignature(CommandSignatures::IndirectDraw)->GetApiHandle();

                context.SetViewport(RHIViewport {0.0f, 0.0f, (float)backBufWidth, (float)backBufHeight, 0.0f, 1.0f});
                context.SetScissorRect(RHIRect {0, 0, backBufWidth, backBufHeight});
                
                context.SetGraphicsRootSignature(registry.GetRootSignature(RootSignatures::IndirectDraw));
                context.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectDraw));
                context->SetGraphicsRootConstantBufferView(1, pPerframeBuffer->GetGpuVirtualAddress());
                context->SetGraphicsRootShaderResourceView(2, pMeshBuffer->GetGpuVirtualAddress());
                context->SetGraphicsRootShaderResourceView(3, pMaterialBuffer->GetGpuVirtualAddress());

                RHI::D3D12DepthStencilView* depthStencilView =
                    registry.Get<RHI::D3D12DepthStencilView>(drawPassOutput->backBufDsv);

				context.ClearRenderTarget(renderTargetView, depthStencilView);
                context.SetRenderTarget(renderTargetView, depthStencilView);

                context->ExecuteIndirect(pCommandSignature,
                                         HLSL::MeshLimit,
                                         p_IndirectCommandBuffer->GetResource(),
                                         0,
                                         p_IndirectCommandBuffer->GetResource(),
                                         commandBufferCounterOffset);
            });
    }


    void IndirectDrawPass::destroy()
    {

    }

}
