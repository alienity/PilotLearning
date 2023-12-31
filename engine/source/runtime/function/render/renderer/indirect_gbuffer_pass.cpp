#include "runtime/function/render/renderer/indirect_gbuffer_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void IndirectGBufferPass::initialize(const DrawPassInitInfo& init_info)
	{
        albedoDesc = init_info.albedoTexDesc;
        depthDesc = init_info.depthTexDesc;

        worldNormalDesc =
            RHI::RgTextureDesc("WorldNormal")
                .SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT)
                .SetExtent(albedoDesc.Width, albedoDesc.Height)
                .SetSampleCount(albedoDesc.SampleCount)
                .SetAllowRenderTarget(true)
                .SetClearValue(RHI::RgClearValue(0.0f, 0.0f, 0.0f, 0.0f, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT));

        motionVectorDesc  = worldNormalDesc;
        motionVectorDesc.Name = "MotionVector";
        motionVectorDesc.SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT);
        motionVectorDesc.SetClearValue(
            RHI::RgClearValue(0.0f, 0.0f, 0.0f, 0.0f, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT));

        metallic_Roughness_Reflectance_AO_Desc      = worldNormalDesc;
        metallic_Roughness_Reflectance_AO_Desc.Name = "Metallic_Roughness_Reflectance_AO";
        metallic_Roughness_Reflectance_AO_Desc.SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT);
        metallic_Roughness_Reflectance_AO_Desc.SetClearValue(
            RHI::RgClearValue(0.0f, 0.0f, 0.0f, 0.0f, metallic_Roughness_Reflectance_AO_Desc.Format));

        clearCoat_ClearCoatRoughness_AnisotropyDesc      = worldNormalDesc;
        clearCoat_ClearCoatRoughness_AnisotropyDesc.Name = "ClearCoat_ClearCoatRoughness_Anisotropy";
        clearCoat_ClearCoatRoughness_AnisotropyDesc.SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT);
        clearCoat_ClearCoatRoughness_AnisotropyDesc.SetClearValue(
            RHI::RgClearValue(0.0f, 0.0f, 0.0f, 0.0f, clearCoat_ClearCoatRoughness_AnisotropyDesc.Format));

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        indirectGBufferVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "hlsl/IndirectGBufferPS.hlsl", ShaderCompileOptions(L"VSMain"));
        indirectGBufferPS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Pixel, m_ShaderRootPath / "hlsl/IndirectGBufferPS.hlsl", ShaderCompileOptions(L"PSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(3)
                    .AddConstantBufferView<1, 0>()
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                             8,
                                             D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                                             D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pIndirectGBufferSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::CommandSignatureDesc Builder(4);
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pIndirectGBufferCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, Builder, pIndirectGBufferSignature->GetApiHandle());
        }

        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;

            RHIRasterizerState rasterizerState = RHIRasterizerState();
            rasterizerState.CullMode = RHI_CULL_MODE::Back;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0] = albedoDesc.Format;   // albedoTexDesc;
            RenderTargetState.RTFormats[1] = worldNormalDesc.Format;  // worldNormal;
            RenderTargetState.RTFormats[2] = metallic_Roughness_Reflectance_AO_Desc.Format; // metallic_Roughness_Reflectance_AO_Desc;
            RenderTargetState.RTFormats[3] = clearCoat_ClearCoatRoughness_AnisotropyDesc.Format; // clearCoat_ClearCoatRoughness_AnisotropyDesc;
            RenderTargetState.RTFormats[4] = motionVectorDesc.Format; // motionVectorDesc;
            RenderTargetState.NumRenderTargets = 5;
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
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pIndirectGBufferSignature.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState      = rasterizerState;
            psoStream.VS                    = &indirectGBufferVS;
            psoStream.PS                    = &indirectGBufferPS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pIndirectGBufferPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectGBuffer", psoDesc);
        }

	}

    void IndirectGBufferPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, GBufferOutput& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        RHI::RgResourceHandle meshBufferHandle     = passInput.meshBufferHandle;
        RHI::RgResourceHandle materialBufferHandle = passInput.materialBufferHandle;
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle opaqueDrawHandle     = passInput.opaqueDrawHandle;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectGBufferPass");


        drawpass.Read(passInput.meshBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.materialBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.opaqueDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        drawpass.Write(passOutput.albedoHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);
        drawpass.Write(passOutput.worldNormalHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.metallic_Roughness_Reflectance_AO_Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.clearCoat_ClearCoatRoughness_Anisotropy_Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.motionVectorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);

        RHI::RgResourceHandle albedoHandle       = passOutput.albedoHandle;
        RHI::RgResourceHandle depthHandle        = passOutput.depthHandle;
        RHI::RgResourceHandle worldNormalHandle  = passOutput.worldNormalHandle;
        RHI::RgResourceHandle metallic_Roughness_Reflectance_AO_Handle = passOutput.metallic_Roughness_Reflectance_AO_Handle;
        RHI::RgResourceHandle clearCoat_ClearCoatRoughness_Anisotropy_Handle = passOutput.clearCoat_ClearCoatRoughness_Anisotropy_Handle;
        RHI::RgResourceHandle motionVectorHandle = passOutput.motionVectorHandle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12RenderTargetView* renderTargetView = registry->GetD3D12Texture(albedoHandle)->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = registry->GetD3D12Texture(depthHandle)->GetDefaultDSV().get();
            
            RHI::D3D12RenderTargetView* worldNormalRTView = registry->GetD3D12Texture(worldNormalHandle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* metallic_Roughness_Reflectance_AO_RTView = registry->GetD3D12Texture(metallic_Roughness_Reflectance_AO_Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* clearCoat_ClearCoatRoughness_Anisotropy_RTView = registry->GetD3D12Texture(clearCoat_ClearCoatRoughness_Anisotropy_Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* motionVectorRTView = registry->GetD3D12Texture(motionVectorHandle)->GetDefaultRTV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)albedoDesc.Width, (float)albedoDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)albedoDesc.Width, (int)albedoDesc.Height});

            std::vector<RHI::D3D12RenderTargetView*> _rtviews = {renderTargetView,
                                                                 worldNormalRTView,
                                                                 metallic_Roughness_Reflectance_AO_RTView,
                                                                 clearCoat_ClearCoatRoughness_Anisotropy_RTView,
                                                                 motionVectorRTView};

            graphicContext->SetRenderTargets(_rtviews, depthStencilView);
            graphicContext->ClearRenderTarget(_rtviews, depthStencilView);

            graphicContext->SetRootSignature(pIndirectGBufferSignature.get());
            graphicContext->SetPipelineState(pIndirectGBufferPSO.get());
            graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
            graphicContext->SetConstant(0, 1, registry->GetD3D12Buffer(meshBufferHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 2, registry->GetD3D12Buffer(materialBufferHandle)->GetDefaultSRV()->GetIndex());
            //graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(meshBufferHandle));
            //graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(materialBufferHandle));

            auto pIndirectCommandBuffer = registry->GetD3D12Buffer(opaqueDrawHandle);

            graphicContext->ExecuteIndirect(pIndirectGBufferCommandSignature.get(),
                                            pIndirectCommandBuffer,
                                            0,
                                            HLSL::MeshLimit,
                                            pIndirectCommandBuffer->GetCounterBuffer().get(),
                                            0);
        });
    }


    void IndirectGBufferPass::destroy()
    {
        pIndirectGBufferSignature = nullptr;
        pIndirectGBufferPSO       = nullptr;
        pIndirectGBufferCommandSignature = nullptr;
    }

    bool IndirectGBufferPass::initializeRenderTarget(RHI::RenderGraph& graph, GBufferOutput* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->albedoHandle.IsValid())
        {
            needClearRenderTarget        = true;
            drawPassOutput->albedoHandle = graph.Create<RHI::D3D12Texture>(albedoDesc);
        }
        if (!drawPassOutput->depthHandle.IsValid())
        {
            needClearRenderTarget       = true;
            drawPassOutput->depthHandle = graph.Create<RHI::D3D12Texture>(depthDesc);
        }
        if (!drawPassOutput->worldNormalHandle.IsValid())
        {
            needClearRenderTarget        = true;
            drawPassOutput->worldNormalHandle = graph.Create<RHI::D3D12Texture>(worldNormalDesc);
        }
        if (!drawPassOutput->metallic_Roughness_Reflectance_AO_Handle.IsValid())
        {
            needClearRenderTarget       = true;
            drawPassOutput->metallic_Roughness_Reflectance_AO_Handle =
                graph.Create<RHI::D3D12Texture>(metallic_Roughness_Reflectance_AO_Desc);
        }
        if (!drawPassOutput->clearCoat_ClearCoatRoughness_Anisotropy_Handle.IsValid())
        {
            needClearRenderTarget       = true;
            drawPassOutput->clearCoat_ClearCoatRoughness_Anisotropy_Handle =
                graph.Create<RHI::D3D12Texture>(clearCoat_ClearCoatRoughness_AnisotropyDesc);
        }

        if (!drawPassOutput->motionVectorHandle.IsValid())
        {
            drawPassOutput->motionVectorHandle = graph.Create<RHI::D3D12Texture>(motionVectorDesc);
        }

        return needClearRenderTarget;
    }

}
