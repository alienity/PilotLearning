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

    void IndirectDrawTransparentPass::update(RHI::RenderGraph&         graph,
                                             DrawInputParameters&      passInput,
                                             DrawOutputParameters&     passOutput)
    {

        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        //DrawInputParameters  drawPassInput  = passInput;
        //DrawOutputParameters drawPassOutput = passOutput;

        RHI::RgResourceHandle perframeBufferHandle = RHI::ToRgResourceHandle(passInput.perframeBufferHandle, RHI::RgResourceType::VertexAndConstantBuffer);

        RHI::RgResourceHandle meshBufferHandle      = passInput.meshBufferHandle;
        RHI::RgResourceHandle materialBufferHandle  = passInput.materialBufferHandle;

        RHI::RgResourceHandle transparentDrawHandle = RHI::ToRgResourceHandle(passInput.transparentDrawHandle, RHI::RgResourceType::IndirectArgBuffer);

        //std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = passInput.pPerframeBuffer;
        //std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer     = passInput.pMeshBuffer;
        //std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer = passInput.pMaterialBuffer;
        //std::shared_ptr<RHI::D3D12Buffer> pIndirectCommandBuffer = passInput.pIndirectCommandBuffer;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectDrawTransparentPass");

        if (passInput.directionalShadowmapTexHandle.IsValid())
        {
            drawpass.Read(passInput.directionalShadowmapTexHandle);
        }
        for (size_t i = 0; i < passInput.spotShadowmapTexHandles.size(); i++)
        {
            drawpass.Read(passInput.spotShadowmapTexHandles[i]);
        }

        RHI::RgResourceHandle renderTargetColorHandle      = passOutput.renderTargetColorHandle;
        RHI::RgResourceHandle renderTargetDepthHandle = passOutput.renderTargetDepthHandle;

        drawpass.Write(renderTargetColorHandle);
        drawpass.Write(renderTargetDepthHandle);

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12Texture* pRenderTargetColorTex = registry->GetD3D12Texture(renderTargetColorHandle);
            RHI::D3D12RenderTargetView* renderTargetView = pRenderTargetColorTex->GetDefaultRTV().get();

            RHI::D3D12Texture* pDepthStencilTex = registry->GetD3D12Texture(renderTargetDepthHandle);
            RHI::D3D12DepthStencilView* depthStencilView = pDepthStencilTex->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)colorTexDesc.Width, (float)colorTexDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)colorTexDesc.Width, (int)colorTexDesc.Height});

            if (needClearRenderTarget)
            {
                graphicContext->ClearRenderTarget(renderTargetView, depthStencilView);
            }
            graphicContext->SetRenderTarget(renderTargetView, depthStencilView);

            graphicContext->SetRootSignature(RootSignatures::pIndirectDraw.get());
            graphicContext->SetPipelineState(PipelineStates::pIndirectDrawTransparent.get());

            graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
            graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(meshBufferHandle));
            graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(materialBufferHandle));

            //graphicContext->SetConstantBuffer(1, pPerframeBuffer->GetGpuVirtualAddress());
            //graphicContext->SetBufferSRV(2, pMeshBuffer.get());
            //graphicContext->SetBufferSRV(3, pMaterialBuffer.get());

            auto pIndirectCommandBuffer = registry->GetD3D12Buffer(transparentDrawHandle);

            graphicContext->ExecuteIndirect(CommandSignatures::pIndirectDraw.get(),
                                            pIndirectCommandBuffer,
                                            0,
                                            HLSL::MeshLimit,
                                            pIndirectCommandBuffer->GetCounterBuffer().get(),
                                            0);
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
        if (!drawPassOutput->renderTargetDepthHandle.IsValid())
        {
            needClearRenderTarget = true;
            drawPassOutput->renderTargetDepthHandle = graph.Create<RHI::D3D12Texture>(depthTexDesc);
        }
        return needClearRenderTarget;
    }

}
