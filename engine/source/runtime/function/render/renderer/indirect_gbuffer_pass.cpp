#include "runtime/function/render/renderer/indirect_gbuffer_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void IndirectGBufferPass::initialize(const DrawPassInitInfo& init_info)
	{
        gbufferDesc = init_info.colorTexDesc;
        depthDesc = init_info.depthTexDesc;

        gbufferDesc.SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT);

        gbuffer0Desc = gbufferDesc;
        gbuffer0Desc.Name = "GBuffer0";
        gbuffer1Desc = gbufferDesc;
        gbuffer1Desc.Name = "GBuffer1";
        gbuffer2Desc = gbufferDesc;
        gbuffer2Desc.Name = "GBuffer2";
        gbuffer3Desc = gbufferDesc;
        gbuffer3Desc.Name = "GBuffer3";

        ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        drawGBufferVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LightGBufferShader.hlsl", ShaderCompileOptions(L"Vert"));
        drawGBufferPS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Pixel, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LightGBufferShader.hlsl", ShaderCompileOptions(L"Frag"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(4)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AddStaticSampler<14, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AddStaticSampler<15, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                             8,
                                             D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                                             D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pDrawGBufferSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::CommandSignatureDesc Builder(4);
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pGBufferCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, Builder, pDrawGBufferSignature->GetApiHandle());
        }

        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexStandard::InputLayout;

            RHIRasterizerState rasterizerState = RHIRasterizerState();
            rasterizerState.CullMode = RHI_CULL_MODE::Back;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0] = gbuffer0Desc.Format;
            RenderTargetState.RTFormats[1] = gbuffer1Desc.Format;
            RenderTargetState.RTFormats[2] = gbuffer2Desc.Format;
            RenderTargetState.RTFormats[3] = gbuffer3Desc.Format;
            RenderTargetState.NumRenderTargets = 4;
            RenderTargetState.DSFormat         = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT;

            RHISampleState SampleState;
            SampleState.Count = 1;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamRasterizerState   RasterrizerState;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pDrawGBufferSignature.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState      = rasterizerState;
            psoStream.VS                    = &drawGBufferVS;
            psoStream.PS                    = &drawGBufferPS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pDrawGBufferPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectGBuffer", psoDesc);
        }


	}

    void IndirectGBufferPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, GBufferOutput& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        RHI::RgResourceHandle renderDataPerDrawHandle = passInput.renderDataPerDrawHandle;
        RHI::RgResourceHandle propertiesPerMaterialHandle = passInput.propertiesPerMaterialHandle;
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle opaqueDrawHandle = passInput.opaqueDrawHandle;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectGBufferPass");


        drawpass.Read(passInput.renderDataPerDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.propertiesPerMaterialHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.opaqueDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        drawpass.Write(passOutput.gbuffer0Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.gbuffer1Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.gbuffer2Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.gbuffer3Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        RHI::RgResourceHandle gbuffer0Handle = passOutput.gbuffer0Handle;
        RHI::RgResourceHandle gbuffer1Handle = passOutput.gbuffer1Handle;
        RHI::RgResourceHandle gbuffer2Handle = passOutput.gbuffer2Handle;
        RHI::RgResourceHandle gbuffer3Handle = passOutput.gbuffer3Handle;
        RHI::RgResourceHandle depthHandle = passOutput.depthHandle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12RenderTargetView* gbuffer0View = registry->GetD3D12Texture(gbuffer0Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* gbuffer1View = registry->GetD3D12Texture(gbuffer1Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* gbuffer2View = registry->GetD3D12Texture(gbuffer2Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* gbuffer3View = registry->GetD3D12Texture(gbuffer3Handle)->GetDefaultRTV().get();

            RHI::D3D12DepthStencilView* depthStencilView = registry->GetD3D12Texture(depthHandle)->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)gbufferDesc.Width, (float)gbufferDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)gbufferDesc.Width, (int)gbufferDesc.Height});

            std::vector<RHI::D3D12RenderTargetView*> _rtviews = { gbuffer0View, gbuffer1View, gbuffer2View, gbuffer3View};

            graphicContext->SetRenderTargets(_rtviews, depthStencilView);
            graphicContext->ClearRenderTarget(_rtviews, depthStencilView);

            graphicContext->SetRootSignature(pDrawGBufferSignature.get());
            graphicContext->SetPipelineState(pDrawGBufferPSO.get());
            //graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
            graphicContext->SetConstant(0, 1, registry->GetD3D12Buffer(renderDataPerDrawHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 2, registry->GetD3D12Buffer(propertiesPerMaterialHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 3, registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV()->GetIndex());


            auto pIndirectCommandBuffer = registry->GetD3D12Buffer(opaqueDrawHandle);

            graphicContext->ExecuteIndirect(pGBufferCommandSignature.get(),
                                            pIndirectCommandBuffer,
                                            0,
                                            HLSL::MeshLimit,
                                            pIndirectCommandBuffer->GetCounterBuffer().get(),
                                            0);
        });
    }


    void IndirectGBufferPass::destroy()
    {
        pDrawGBufferSignature = nullptr;
        pDrawGBufferPSO = nullptr;
        pGBufferCommandSignature = nullptr;
    }

    bool IndirectGBufferPass::initializeRenderTarget(RHI::RenderGraph& graph, GBufferOutput* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->gbuffer0Handle.IsValid())
        {
            needClearRenderTarget = true;
            drawPassOutput->gbuffer0Handle = graph.Create<RHI::D3D12Texture>(gbuffer0Desc);
        }
        if (!drawPassOutput->gbuffer1Handle.IsValid())
        {
            needClearRenderTarget = true;
            drawPassOutput->gbuffer1Handle = graph.Create<RHI::D3D12Texture>(gbuffer1Desc);
        }
        if (!drawPassOutput->gbuffer2Handle.IsValid())
        {
            needClearRenderTarget = true;
            drawPassOutput->gbuffer2Handle = graph.Create<RHI::D3D12Texture>(gbuffer2Desc);
        }
        if (!drawPassOutput->gbuffer3Handle.IsValid())
        {
            needClearRenderTarget = true;
            drawPassOutput->gbuffer3Handle = graph.Create<RHI::D3D12Texture>(gbuffer3Desc);
        }
        if (!drawPassOutput->depthHandle.IsValid())
        {
            needClearRenderTarget = true;
            drawPassOutput->depthHandle = graph.Create<RHI::D3D12Texture>(depthDesc);
        }


        return needClearRenderTarget;
    }

}
