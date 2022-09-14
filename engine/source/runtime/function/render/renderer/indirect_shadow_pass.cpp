#include "runtime/function/render/renderer/indirect_shadow_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

namespace Pilot
{

    void IndirectShadowPass::initialize(const ShadowPassInitInfo& init_info)
    {
        depthBufferTexDesc = init_info.depthBufferTexDesc;
    }

    void IndirectShadowPass::update(RHI::D3D12CommandContext& context,
                                    RHI::RenderGraph&         graph,
                                    ShadowInputParameters&    passInput,
                                    ShadowOutputParameters&   passOutput)
    {
        ShadowInputParameters*  drawPassInput  = &passInput;
        ShadowOutputParameters* drawPassOutput = &passOutput;

        int commandBufferCounterOffset = drawPassInput->commandBufferCounterOffset;

        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = drawPassInput->pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer     = drawPassInput->pMeshBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer = drawPassInput->pMaterialBuffer;

        std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer = drawPassInput->p_IndirectCommandBuffer;

        drawPassOutput->backBufDepth = graph.Create<RHI::D3D12Texture>(depthBufferTexDesc);
        drawPassOutput->backBufDsv   = graph.Create<RHI::D3D12DepthStencilView>(
            RHI::RgViewDesc().SetResource(drawPassOutput->backBufDepth).AsDsv());

        graph.AddRenderPass("IndirectShadowPass")
            .Write(&drawPassOutput->backBufDepth)
            .Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {

                /*
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

                context.ClearRenderTarget(nullptr, depthStencilView);
                context.SetRenderTarget(nullptr, depthStencilView);

                context->ExecuteIndirect(pCommandSignature,
                                         HLSL::MeshLimit,
                                         p_IndirectCommandBuffer->GetResource(),
                                         0,
                                         p_IndirectCommandBuffer->GetResource(),
                                         commandBufferCounterOffset);
                */
            });
    }

    void IndirectShadowPass::destroy() {}

}



