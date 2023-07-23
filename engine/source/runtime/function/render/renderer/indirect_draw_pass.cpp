#include "runtime/function/render/renderer/indirect_draw_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void IndirectDrawPass::initialize(const DrawPassInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;
        depthTexDesc = init_info.depthTexDesc;
	}

    void IndirectDrawPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {

        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        //DrawInputParameters  drawPassInput  = passInput;
        //DrawOutputParameters drawPassOutput = passOutput;

        RHI::RgResourceHandle meshBufferHandle     = passInput.meshBufferHandle;
        RHI::RgResourceHandle materialBufferHandle = passInput.materialBufferHandle;

        RHI::RgResourceHandleExt perframeBufferHandle = RHI::ToRgResourceHandle(passInput.perframeBufferHandle, RHI::RgResourceSubType::VertexAndConstantBuffer);
        RHI::RgResourceHandleExt opaqueDrawHandle = RHI::ToRgResourceHandle(passInput.opaqueDrawHandle, RHI::RgResourceSubType::IndirectArgBuffer);

        //std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = passInput.pPerframeBuffer;
        //std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer     = passInput.pMeshBuffer;
        //std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer = passInput.pMaterialBuffer;
        //std::shared_ptr<RHI::D3D12Buffer> pIndirectCommandBuffer = passInput.pIndirectCommandBuffer;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectDrawOpaquePass");

        if (passInput.directionalShadowmapTexHandle.IsValid())
        {
            drawpass.Read(passInput.directionalShadowmapTexHandle);
        }
        for (size_t i = 0; i < passInput.spotShadowmapTexHandles.size(); i++)
        {
            drawpass.Read(passInput.spotShadowmapTexHandles[i]);
        }
        drawpass.Read(passInput.meshBufferHandle);
        drawpass.Read(passInput.materialBufferHandle);
        drawpass.Read(perframeBufferHandle);
        drawpass.Read(opaqueDrawHandle);
        drawpass.Write(passOutput.renderTargetColorHandle);
        drawpass.Write(passOutput.renderTargetDepthHandle);

        RHI::RgResourceHandle renderTargetColorHandle = passOutput.renderTargetColorHandle;
        RHI::RgResourceHandle renderTargetDepthHandle = passOutput.renderTargetDepthHandle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12Texture* pRenderTargetColor = registry->GetD3D12Texture(renderTargetColorHandle);
            RHI::D3D12Texture* pRenderTargetDepth = registry->GetD3D12Texture(renderTargetDepthHandle);

            RHI::D3D12RenderTargetView* renderTargetView = pRenderTargetColor->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = pRenderTargetDepth->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)colorTexDesc.Width, (float)colorTexDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)colorTexDesc.Width, (int)colorTexDesc.Height});

            graphicContext->SetRenderTarget(renderTargetView, depthStencilView);
            if (needClearRenderTarget)
            {
                graphicContext->ClearRenderTarget(renderTargetView, depthStencilView);
            }

            graphicContext->SetRootSignature(RootSignatures::pIndirectDraw.get());
            graphicContext->SetPipelineState(PipelineStates::pIndirectDraw.get());
            graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle.rgHandle)->GetGpuVirtualAddress());
            graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(meshBufferHandle));
            graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(materialBufferHandle));

            //graphicContext->SetConstantBuffer(1, pPerframeBuffer->GetGpuVirtualAddress());
            //graphicContext->SetBufferSRV(2, pMeshBuffer.get());
            //graphicContext->SetBufferSRV(3, pMaterialBuffer.get());

            auto pIndirectCommandBuffer = registry->GetD3D12Buffer(opaqueDrawHandle.rgHandle);

            graphicContext->ExecuteIndirect(CommandSignatures::pIndirectDraw.get(),
                                            pIndirectCommandBuffer,
                                            0,
                                            HLSL::MeshLimit,
                                            pIndirectCommandBuffer->GetCounterBuffer().get(),
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
