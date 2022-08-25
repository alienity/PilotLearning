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
        DrawInputParameters*  drawPassInput  = (DrawInputParameters*)(&passInput);
        DrawOutputParameters* drawPassOutput = (DrawOutputParameters*)(&passOutput);

        int numMeshes = drawPassInput->numMeshes;
        int commandBufferCounterOffset = drawPassInput->commandBufferCounterOffset;

        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = drawPassInput->pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer     = drawPassInput->pMeshBuffer;

        std::shared_ptr<RHI::D3D12Buffer>              p_IndirectCommandBuffer = drawPassInput->p_IndirectCommandBuffer;

        RHI::RgResourceHandle       backBufColor     = drawPassOutput->backBufColor;
        RHI::D3D12RenderTargetView* renderTargetView = drawPassOutput->backBufRtv;
        RHI::D3D12DepthStencilView* depthStencilView = drawPassOutput->backBufDsv;

        graph.AddRenderPass("IndirectDrawPass")
            .Write(&backBufColor)
            .Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {
                RHI::D3D12Texture*  backBufTex    = registry.GetImportedResource(backBufColor);
                D3D12_RESOURCE_DESC backBufDesc   = backBufTex->GetDesc();
                int                 backBufWidth  = backBufDesc.Width;
                int                 backBufHeight = backBufDesc.Height;

                context.SetViewport(RHIViewport {0.0f, 0.0f, (float)backBufWidth, (float)backBufHeight, 0.0f, 1.0f});
                context.SetScissorRect(RHIRect {0, 0, backBufWidth, backBufHeight});
                
                context.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectDraw));
                context.SetGraphicsRootSignature(registry.GetRootSignature(RootSignatures::IndirectDraw));
                context->SetComputeRootConstantBufferView(1, pPerframeBuffer->GetGpuVirtualAddress());
                context->SetGraphicsRootShaderResourceView(2, pMeshBuffer->GetGpuVirtualAddress());

				context.ClearRenderTarget(renderTargetView, depthStencilView);
                context.SetRenderTarget(renderTargetView, depthStencilView);

                context->ExecuteIndirect(CommandSignatures::IndirectDraw,
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
