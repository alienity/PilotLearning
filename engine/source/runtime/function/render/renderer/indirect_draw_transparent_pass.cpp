#include "runtime/function/render/renderer/indirect_draw_transparent_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void IndirectDrawTransparentPass::initialize(const DrawPassInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;
        depthTexDesc = init_info.depthTexDesc;

        ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        indirectDrawTransparentVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LitForwardTransparentShader.hlsl", ShaderCompileOptions(L"Vert"));
        indirectDrawTransparentPS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Pixel, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LitForwardTransparentShader.hlsl", ShaderCompileOptions(L"Frag"));

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

            pIndirectDrawTransparentSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::CommandSignatureDesc Builder(4);
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pIndirectDrawTransparentCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, Builder, pIndirectDrawTransparentSignature->GetApiHandle());
        }
        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexStandard::InputLayout;

            RHIRasterizerState rasterizerState = RHIRasterizerState();
            rasterizerState.CullMode = RHI_CULL_MODE::Back;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthWrite = false;
            DepthStencilState.DepthFunc = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT; // DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT;

            RHIRenderTargetBlendDesc BlendDesc0;
            BlendDesc0.BlendEnable = true;
            BlendDesc0.SrcBlendRgb = RHI_FACTOR::One;
            BlendDesc0.DstBlendRgb = RHI_FACTOR::OneMinusSrcAlpha;
            BlendDesc0.BlendOpRgb  = RHI_BLEND_OP::Add;
            BlendDesc0.SrcBlendAlpha = RHI_FACTOR::One;
            BlendDesc0.DstBlendAlpha = RHI_FACTOR::OneMinusSrcAlpha;
            BlendDesc0.BlendOpAlpha = RHI_BLEND_OP::Add;


            RHIBlendState BlendState;
            BlendState.RenderTargets[0] = BlendDesc0;

            RHISampleState SampleState;
            SampleState.Count = EngineConfig::g_AntialiasingMode == EngineConfig::MSAA ? EngineConfig::g_MASSConfig.m_MSAASampleCount : 1;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamBlendState        BlendState;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pIndirectDrawTransparentSignature.get());
            psoStream.InputLayout = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.VS = &indirectDrawTransparentVS;
            psoStream.PS = &indirectDrawTransparentPS;
            psoStream.BlendState = BlendState;
            psoStream.DepthStencilState = DepthStencilState;
            psoStream.RenderTargetState = RenderTargetState;
            psoStream.SampleState = SampleState;

            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };
            pIndirectDrawTransparentPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectDrawTransparent", psoDesc);
        }
	}

    void IndirectDrawTransparentPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {

        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        RHI::RgResourceHandle renderDataPerDrawHandle = passInput.renderDataPerDrawHandle;
        RHI::RgResourceHandle propertiesPerMaterialHandle = passInput.propertiesPerMaterialHandle;
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle transparentDrawHandle = passInput.transparentDrawHandle;

        RHI::RgResourceHandle renderTargetColorHandle = passOutput.renderTargetColorHandle;
        RHI::RgResourceHandle renderTargetDepthHandle = passOutput.renderTargetDepthHandle;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectDrawTransparentPass");

        for (int i = 0; i < passInput.directionalShadowmapTexHandles.size(); i++)
        {
            drawpass.Read(passInput.directionalShadowmapTexHandles[i], false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
        for (int i = 0; i < passInput.spotShadowmapTexHandles.size(); i++)
        {
            drawpass.Read(passInput.spotShadowmapTexHandles[i], false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
        drawpass.Read(passInput.renderDataPerDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.propertiesPerMaterialHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.transparentDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        drawpass.Read(passOutput.renderTargetColorHandle, true);
        drawpass.Read(passOutput.renderTargetDepthHandle, true);

        drawpass.Write(passOutput.renderTargetColorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.renderTargetDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12Texture* pRenderTargetColor = registry->GetD3D12Texture(renderTargetColorHandle);
            RHI::D3D12Texture* pRenderTargetDepth = registry->GetD3D12Texture(renderTargetDepthHandle);

            RHI::D3D12RenderTargetView* renderTargetView = pRenderTargetColor->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = pRenderTargetDepth->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport{ 0.0f, 0.0f, (float)colorTexDesc.Width, (float)colorTexDesc.Height, 0.0f, 1.0f });
            graphicContext->SetScissorRect(RHIRect{ 0, 0, (int)colorTexDesc.Width, (int)colorTexDesc.Height });

            graphicContext->SetRenderTarget(renderTargetView, depthStencilView);
            if (needClearRenderTarget)
            {
                graphicContext->ClearRenderTarget(renderTargetView, depthStencilView);
            }

            graphicContext->SetRootSignature(pIndirectDrawTransparentSignature.get());
            graphicContext->SetPipelineState(pIndirectDrawTransparentPSO.get());

            graphicContext->SetConstant(0, 1, registry->GetD3D12Buffer(renderDataPerDrawHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 2, registry->GetD3D12Buffer(propertiesPerMaterialHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 3, registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV()->GetIndex());

            RHI::D3D12Buffer* pIndirectCommandBuffer = registry->GetD3D12Buffer(transparentDrawHandle);

            graphicContext->ExecuteIndirect(pIndirectDrawTransparentCommandSignature.get(),
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
