#include "runtime/function/render/renderer/indirect_display_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void DisplayPass::initialize(const DisplayPassInitInfo& init_info) {}

    void DisplayPass::update(RHI::RenderGraph&         graph,
                             DisplayInputParameters&   passInput,
                             DisplayOutputParameters&  passOutput)
    {
        DisplayInputParameters*  drawPassInput  = &passInput;
        DisplayOutputParameters* drawPassOutput = &passOutput;

        graph.AddRenderPass("DisplayDrawPass")
            .Read(drawPassInput->inputRTColorHandle)
            .Write(&drawPassOutput->renderTargetColorHandle)
            .Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                
                RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

                RHI::D3D12Texture* pInputRTColor = registry->GetD3D12Texture(drawPassInput->inputRTColorHandle);
                RHI::D3D12ShaderResourceView* pInputRTColorSRV = pInputRTColor->GetDefaultSRV().get();

                RHI::D3D12Texture* pRTColorTexture = registry->GetD3D12Texture(drawPassOutput->renderTargetColorHandle);
                RHI::D3D12RenderTargetView* pRTColorRTV = pRTColorTexture->GetDefaultRTV().get();

                CD3DX12_RESOURCE_DESC rtColorTextureDesc = pRTColorTexture->GetDesc();

                int rtColorWidth  = rtColorTextureDesc.Width;
                int rtColorHeight = rtColorTextureDesc.Height;

                graphicContext->SetRootSignature(RootSignatures::pFullScreenPresent.get());
                graphicContext->SetPipelineState(PipelineStates::pFullScreenPresent.get());
                graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                graphicContext->SetViewport(RHIViewport {0, 0, (float)rtColorWidth, (float)rtColorHeight, 0, 1});
                graphicContext->SetScissorRect(RHIRect {0, 0, (long)rtColorWidth, (long)rtColorHeight});
                graphicContext->SetConstant(0, 0, pInputRTColorSRV->GetIndex());
                graphicContext->SetRenderTarget(pRTColorRTV, nullptr);
                graphicContext->DrawInstanced(3, 1, 0, 0);
            });
    }

    void DisplayPass::destroy() {}

}
