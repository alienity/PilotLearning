#include "runtime/function/render/renderer/indirect_display_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void DisplayPass::initialize(const DisplayPassInitInfo& init_info) {}

    void DisplayPass::update(RHI::D3D12CommandContext& context,
                             RHI::RenderGraph&         graph,
                             DisplayInputParameters&   passInput,
                             DisplayOutputParameters&  passOutput)
    {
        DisplayInputParameters*  drawPassInput  = &passInput;
        DisplayOutputParameters* drawPassOutput = &passOutput;

        graph.AddRenderPass("DisplayDrawPass")
            .Read(drawPassInput->inputRTColorHandle)
            .Write(&drawPassOutput->renderTargetColorHandle)
            .Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {

                RHI::D3D12ShaderResourceView* inputRTColorSRV = registry.GetD3D12ShaderResourceView(drawPassInput->inputRTColorSRVHandle);

                RHI::D3D12Texture* rtColorTexture = registry.GetD3D12Texture(drawPassOutput->renderTargetColorHandle);
                D3D12_RESOURCE_DESC rtColorTextureDesc = rtColorTexture->GetDesc();

                int rtColorWidth  = rtColorTextureDesc.Width;
                int rtColorHeight = rtColorTextureDesc.Height;

                context.SetGraphicsRootSignature(registry.GetRootSignature(RootSignatures::FullScreenPresent));
                context.SetPipelineState(registry.GetPipelineState(PipelineStates::FullScreenPresent));
                context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                context.SetViewport(RHIViewport {0, 0, (float)rtColorWidth, (float)rtColorHeight, 0, 1});
                context.SetScissorRect(RHIRect {0, 0, (long)rtColorWidth, (long)rtColorHeight});
                context->SetGraphicsRoot32BitConstant(0, inputRTColorSRV->GetIndex(), 0);
                context.SetRenderTarget(registry.GetD3D12RenderTargetView(drawPassOutput->renderTargetColorRTVHandle), nullptr);
                context->DrawInstanced(3, 1, 0, 0);
            });
    }

    void DisplayPass::destroy() {}

}
