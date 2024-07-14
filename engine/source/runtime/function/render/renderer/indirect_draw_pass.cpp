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

	    ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
	    std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

	    indirectDrawVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LitForwardShader.hlsl", ShaderCompileOptions(L"Vert"));
	    indirectDrawPS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Pixel, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LitForwardShader.hlsl", ShaderCompileOptions(L"Frag"));

	    {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(32)
                    .AddConstantBufferView<1, 0>()
                    .AddStaticSampler<10, 0>(D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER_COMPARISON_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8, D3D12_COMPARISON_FUNC_GREATER_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();
            pIndirectDrawSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
	    }

	    //{
     //       RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;
     //       
     //       RHIDepthStencilState DepthStencilState;
     //       DepthStencilState.DepthEnable = true;
     //       DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

     //       RHIRenderTargetState RenderTargetState;
     //       RenderTargetState.RTFormats[0]     = DXGI_FORMAT_R32G32B32A32_FLOAT; // DXGI_FORMAT_R32G32B32A32_FLOAT;
     //       RenderTargetState.NumRenderTargets = 1;
     //       RenderTargetState.DSFormat         = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT;

     //       RHISampleState SampleState;
     //       SampleState.Count = EngineConfig::g_AntialiasingMode == EngineConfig::MSAA ? EngineConfig::g_MASSConfig.m_MSAASampleCount : 1;

     //       struct PsoStream
     //       {
     //           PipelineStateStreamRootSignature     RootSignature;
     //           PipelineStateStreamInputLayout       InputLayout;
     //           PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
     //           PipelineStateStreamRasterizerState   RasterrizerState;
     //           PipelineStateStreamVS                VS;
     //           PipelineStateStreamPS                PS;
     //           PipelineStateStreamDepthStencilState DepthStencilState;
     //           PipelineStateStreamRenderTargetState RenderTargetState;
     //           PipelineStateStreamSampleState       SampleState;
     //       } psoStream;
     //       psoStream.RootSignature         = PipelineStateStreamRootSignature(pIndirectDrawSignature.get());
     //       psoStream.InputLayout           = &InputLayout;
     //       psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
     //       psoStream.RasterrizerState      = RHIRasterizerState();
     //       psoStream.VS                    = &indirectDrawVS;
     //       psoStream.PS                    = &indirectDrawPS;
     //       psoStream.DepthStencilState     = DepthStencilState;
     //       psoStream.RenderTargetState     = RenderTargetState;
     //       psoStream.SampleState           = SampleState;

     //       PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
     //       pIndirectDrawPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectDraw", psoDesc);
     //   }

	    
	}

    void IndirectDrawPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {

        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        //DrawInputParameters  drawPassInput  = passInput;
        //DrawOutputParameters drawPassOutput = passOutput;

        RHI::RgResourceHandle meshBufferHandle     = passInput.meshBufferHandle;
        RHI::RgResourceHandle materialBufferHandle = passInput.materialBufferHandle;
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle opaqueDrawHandle     = passInput.opaqueDrawHandle;

        //std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = passInput.pPerframeBuffer;
        //std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer     = passInput.pMeshBuffer;
        //std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer = passInput.pMaterialBuffer;
        //std::shared_ptr<RHI::D3D12Buffer> pIndirectCommandBuffer = passInput.pIndirectCommandBuffer;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectDrawOpaquePass");

        if (passInput.directionalShadowmapTexHandle.IsValid())
        {
            drawpass.Read(passInput.directionalShadowmapTexHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
        for (size_t i = 0; i < passInput.spotShadowmapTexHandles.size(); i++)
        {
            drawpass.Read(passInput.spotShadowmapTexHandles[i], false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
        drawpass.Read(passInput.meshBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.materialBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.opaqueDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        drawpass.Write(passOutput.renderTargetColorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.renderTargetDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

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
            //graphicContext->SetPipelineState(PipelineStates::pIndirectDraw.get());
            graphicContext->SetPipelineState(PipelineStates::pIndirectDraw_BRDF.get());
            graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
            graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(meshBufferHandle));
            graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(materialBufferHandle));

            //graphicContext->SetConstantBuffer(1, pPerframeBuffer->GetGpuVirtualAddress());
            //graphicContext->SetBufferSRV(2, pMeshBuffer.get());
            //graphicContext->SetBufferSRV(3, pMaterialBuffer.get());

            auto pIndirectCommandBuffer = registry->GetD3D12Buffer(opaqueDrawHandle);

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
