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
                    .Add32BitConstants<0, 0>(16)
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

            pIndirectDrawSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
	    }
        {
            RHI::CommandSignatureDesc Builder(4);
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pIndirectDrawCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, Builder, pIndirectDrawSignature->GetApiHandle());
        }
	    {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexStandard::InputLayout;
            
            RHIRasterizerState rasterizerState = RHIRasterizerState();
            rasterizerState.CullMode = RHI_CULL_MODE::Back;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0]     = DXGI_FORMAT_R32G32B32A32_FLOAT; // DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat         = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT;

            RHISampleState SampleState;
            SampleState.Count = EngineConfig::g_AntialiasingMode == EngineConfig::AntialiasingMode::MSAAMode ? EngineConfig::g_MASSConfig.m_MSAASampleCount : 1;

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
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pIndirectDrawSignature.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState      = rasterizerState;
            psoStream.VS                    = &indirectDrawVS;
            psoStream.PS                    = &indirectDrawPS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pIndirectDrawPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectDraw", psoDesc);
        }
	}

    void IndirectDrawPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        RHI::RgResourceHandle renderDataPerDrawHandle = passInput.renderDataPerDrawHandle;
        RHI::RgResourceHandle propertiesPerMaterialHandle = passInput.propertiesPerMaterialHandle;
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle opaqueDrawHandle = passInput.opaqueDrawHandle;

        RHI::RgResourceHandle renderTargetColorHandle = passOutput.renderTargetColorHandle;
        RHI::RgResourceHandle renderTargetDepthHandle = passOutput.renderTargetDepthHandle;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectDrawOpaquePass");

        for (int i = 0; i < passInput.directionalShadowmapTexHandles.size(); i++)
        {
            drawpass.Read(passInput.directionalShadowmapTexHandles[i], false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
        for (size_t i = 0; i < passInput.spotShadowmapTexHandles.size(); i++)
        {
            drawpass.Read(passInput.spotShadowmapTexHandles[i], false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
        drawpass.Read(passInput.renderDataPerDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.propertiesPerMaterialHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.opaqueDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        drawpass.Write(passOutput.renderTargetColorHandle, true/*, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET*/);
        drawpass.Write(passOutput.renderTargetDepthHandle, true/*, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE */);

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
             
            graphicContext->SetRootSignature(pIndirectDrawSignature.get());
            graphicContext->SetPipelineState(pIndirectDrawPSO.get());

            graphicContext->SetConstant(0, 1, registry->GetD3D12Buffer(renderDataPerDrawHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 2, registry->GetD3D12Buffer(propertiesPerMaterialHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 3, registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV()->GetIndex());

            RHI::D3D12Buffer* pIndirectCommandBuffer = registry->GetD3D12Buffer(opaqueDrawHandle);

            graphicContext->ExecuteIndirect(pIndirectDrawCommandSignature.get(),
                                            pIndirectCommandBuffer,
                                            0,
                                            HLSL::MeshLimit,
                                            pIndirectCommandBuffer->GetCounterBuffer().get(),
                                            0);
        });

        passOutput.renderTargetColorHandle = renderTargetColorHandle;
        passOutput.renderTargetDepthHandle = renderTargetDepthHandle;
    }


    void IndirectDrawPass::destroy()
    {
        pIndirectDrawPSO = nullptr;
        pIndirectDrawCommandSignature = nullptr;
        pIndirectDrawSignature = nullptr;
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
