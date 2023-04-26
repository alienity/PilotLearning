#include "runtime/function/render/renderer/skybox_pass.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void SkyBoxPass::initialize(const SkyBoxInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;
        depthTexDesc = init_info.depthTexDesc;
	}

    void SkyBoxPass::prepareMeshData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        RenderResource* real_resource = (RenderResource*)render_resource.get();

        std::shared_ptr<RHI::D3D12Texture> specularTex =
            real_resource->m_global_render_resource._ibl_resource._specular_texture_image;
        specularIBLTexIndex = specularTex->GetDefaultSRV()->GetIndex();
        specularIBLTexLevel = 0.0f;
    }

    void SkyBoxPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {

        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        //DrawInputParameters*  drawPassInput  = &passInput;
        //DrawOutputParameters* drawPassOutput = &passOutput;

        RHI::RgResourceHandle perframeBufferHandle = RHI::ToRgResourceHandle(passInput.perframeBufferHandle, RHI::RgResourceSubType::VertexAndConstantBuffer);

        //std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = passInput.pPerframeBuffer;

        RHI::RenderPass& drawpass = graph.AddRenderPass("SkyboxPass");

        RHI::RgResourceHandle localRenderTargetColorHandle = passOutput.renderTargetColorHandle;
        RHI::RgResourceHandle localRenderTargetDepthHandle = passOutput.renderTargetDepthHandle;

        drawpass.Write(passOutput.renderTargetColorHandle);
        drawpass.Write(passOutput.renderTargetDepthHandle);

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12Texture* pRenderTargetColor = registry->GetD3D12Texture(localRenderTargetColorHandle);
            RHI::D3D12Texture* pRenderTargetDepth = registry->GetD3D12Texture(localRenderTargetDepthHandle);

            RHI::D3D12RenderTargetView* renderTargetView = pRenderTargetColor->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = pRenderTargetDepth->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)colorTexDesc.Width, (float)colorTexDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)colorTexDesc.Width, (int)colorTexDesc.Height});

            if (needClearRenderTarget)
            {
                graphicContext->ClearRenderTarget(renderTargetView, depthStencilView);
            }
            graphicContext->SetRenderTarget(renderTargetView, depthStencilView);

            graphicContext->SetRootSignature(RootSignatures::pSkyBoxRootSignature.get());
            graphicContext->SetPipelineState(PipelineStates::pSkyBoxPSO.get());
            graphicContext->SetConstants(0, specularIBLTexIndex, specularIBLTexLevel);
            //graphicContext->SetConstantBuffer(1, pPerframeBuffer->GetGpuVirtualAddress());
            graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
            
            graphicContext->Draw(3);
        });
    }


    void SkyBoxPass::destroy()
    {

    }

    bool SkyBoxPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->renderTargetColorHandle.IsValid())
        {
            needClearRenderTarget                   = true;
            drawPassOutput->renderTargetColorHandle = graph.Create<RHI::D3D12Texture>(colorTexDesc);
        }
        if (!drawPassOutput->renderTargetDepthHandle.IsValid())
        {
            needClearRenderTarget                   = true;
            drawPassOutput->renderTargetDepthHandle = graph.Create<RHI::D3D12Texture>(depthTexDesc);
        }
        return needClearRenderTarget;
    }

}
