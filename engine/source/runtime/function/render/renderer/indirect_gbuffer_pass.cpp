#include "runtime/function/render/renderer/indirect_gbuffer_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void IndirectGBufferPass::initialize(const DrawPassInitInfo& init_info)
	{
        albedoTexDesc = init_info.albedoTexDesc;
        depthTexDesc = init_info.depthTexDesc;

        normalTexDesc = RHI::RgTextureDesc("Normal")
                            .SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT)
                            .SetExtent(albedoTexDesc.Width, albedoTexDesc.Height)
                            .SetSampleCount(albedoTexDesc.SampleCount)
                            .SetAllowRenderTarget(true)
                            .SetClearValue(RHI::RgClearValue(0.0f, 0xff));

        clearCoatNormalTexDesc      = normalTexDesc;
        clearCoatNormalTexDesc.Name = "ClearCoatNormal";

        emissiveDesc      = normalTexDesc;
        emissiveDesc.SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT);
        emissiveDesc.Name = "Emissive";

        metallic_Roughness_Reflectance_AO_Desc      = normalTexDesc;
        metallic_Roughness_Reflectance_AO_Desc.SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT);
        metallic_Roughness_Reflectance_AO_Desc.Name = "Metallic_Roughness_Reflectance_AO";

        clearCoat_ClearCoatRoughness_AnisotropyDesc      = normalTexDesc;
        clearCoat_ClearCoatRoughness_AnisotropyDesc.SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT);
        clearCoat_ClearCoatRoughness_AnisotropyDesc.Name = "ClearCoat_ClearCoatRoughness_Anisotropy";

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        indirectGBufferVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "hlsl/IndirectGBufferPS.hlsl", ShaderCompileOptions(L"VSMain"));
        indirectGBufferPS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Pixel, m_ShaderRootPath / "hlsl/IndirectGBufferPS.hlsl", ShaderCompileOptions(L"PSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(1)
                    .AddConstantBufferView<1, 0>()
                    .AddShaderResourceView<0, 0>()
                    .AddShaderResourceView<1, 0>()
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
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0] = albedoTexDesc.Format;          // albedoTexDesc;
            RenderTargetState.RTFormats[1] = normalTexDesc.Format;          // normalDesc;
            RenderTargetState.RTFormats[2] = clearCoatNormalTexDesc.Format; // clearCoatNormalTexDesc;
            RenderTargetState.RTFormats[3] = emissiveDesc.Format;           // emissiveDesc;
            RenderTargetState.RTFormats[4] = metallic_Roughness_Reflectance_AO_Desc.Format; // metallic_Roughness_Reflectance_AO_Desc;
            RenderTargetState.RTFormats[5] = clearCoat_ClearCoatRoughness_AnisotropyDesc.Format; // clearCoat_ClearCoatRoughness_AnisotropyDesc;
            RenderTargetState.NumRenderTargets = 6;
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
            psoStream.RasterrizerState      = RHIRasterizerState();
            psoStream.VS                    = &indirectGBufferVS;
            psoStream.PS                    = &indirectGBufferPS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pIndirectGBufferPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectGBuffer", psoDesc);
        }


	}

    void IndirectGBufferPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        RHI::RgResourceHandle meshBufferHandle     = passInput.meshBufferHandle;
        RHI::RgResourceHandle materialBufferHandle = passInput.materialBufferHandle;

        RHI::RgResourceHandleExt perframeBufferHandle = RHI::ToRgResourceHandle(passInput.perframeBufferHandle, RHI::RgResourceSubType::VertexAndConstantBuffer);
        RHI::RgResourceHandleExt opaqueDrawHandle = RHI::ToRgResourceHandle(passInput.opaqueDrawHandle, RHI::RgResourceSubType::IndirectArgBuffer);

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectGBufferPass");


        drawpass.Read(passInput.meshBufferHandle);
        drawpass.Read(passInput.materialBufferHandle);
        drawpass.Read(perframeBufferHandle);
        drawpass.Read(opaqueDrawHandle);
        drawpass.Write(passOutput.albedoHandle);
        drawpass.Write(passOutput.depthHandle);
        drawpass.Write(passOutput.normalHandle);
        drawpass.Write(passOutput.clearCoatNormalHandle);
        drawpass.Write(passOutput.emissiveHandle);
        drawpass.Write(passOutput.metallic_Roughness_Reflectance_AO_Handle);
        drawpass.Write(passOutput.clearCoat_ClearCoatRoughness_Anisotropy_Handle);

        RHI::RgResourceHandle albedoHandle          = passOutput.albedoHandle;
        RHI::RgResourceHandle depthHandle           = passOutput.depthHandle;
        RHI::RgResourceHandle normalHandle          = passOutput.normalHandle;
        RHI::RgResourceHandle clearCoatNormalHandle = passOutput.clearCoatNormalHandle;
        RHI::RgResourceHandle emissiveHandle        = passOutput.emissiveHandle;
        RHI::RgResourceHandle metallic_Roughness_Reflectance_AO_Handle =
            passOutput.metallic_Roughness_Reflectance_AO_Handle;
        RHI::RgResourceHandle clearCoat_ClearCoatRoughness_Anisotropy_Handle =
            passOutput.clearCoat_ClearCoatRoughness_Anisotropy_Handle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12RenderTargetView* renderTargetView = registry->GetD3D12Texture(albedoHandle)->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = registry->GetD3D12Texture(depthHandle)->GetDefaultDSV().get();
            
            RHI::D3D12RenderTargetView* normalRTView = registry->GetD3D12Texture(normalHandle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* clearCoatNormalRTView = registry->GetD3D12Texture(clearCoatNormalHandle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* emissiveRTView = registry->GetD3D12Texture(emissiveHandle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* metallic_Roughness_Reflectance_AO_RTView = registry->GetD3D12Texture(metallic_Roughness_Reflectance_AO_Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* clearCoat_ClearCoatRoughness_Anisotropy_RTView = registry->GetD3D12Texture(clearCoat_ClearCoatRoughness_Anisotropy_Handle)->GetDefaultRTV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)albedoTexDesc.Width, (float)albedoTexDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)albedoTexDesc.Width, (int)albedoTexDesc.Height});

            std::vector<RHI::D3D12RenderTargetView*> _rtviews = {renderTargetView,
                                                                 normalRTView,
                                                                 clearCoatNormalRTView,
                                                                 emissiveRTView,
                                                                 metallic_Roughness_Reflectance_AO_RTView,
                                                                 clearCoat_ClearCoatRoughness_Anisotropy_RTView};

            graphicContext->SetRenderTargets(_rtviews, depthStencilView);
            graphicContext->ClearRenderTarget(_rtviews, depthStencilView);

            graphicContext->SetRootSignature(pIndirectGBufferSignature.get());
            graphicContext->SetPipelineState(pIndirectGBufferPSO.get());
            graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle.rgHandle)->GetGpuVirtualAddress());
            graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(meshBufferHandle));
            graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(materialBufferHandle));

            auto pIndirectCommandBuffer = registry->GetD3D12Buffer(opaqueDrawHandle.rgHandle);

            graphicContext->ExecuteIndirect(CommandSignatures::pIndirectDraw.get(),
                                            pIndirectCommandBuffer,
                                            0,
                                            HLSL::MeshLimit,
                                            pIndirectCommandBuffer->GetCounterBuffer().get(),
                                            0);
        });
    }


    void IndirectGBufferPass::destroy()
    {

    }

    bool IndirectGBufferPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->albedoHandle.IsValid())
        {
            needClearRenderTarget        = true;
            drawPassOutput->albedoHandle = graph.Create<RHI::D3D12Texture>(albedoTexDesc);
        }
        if (!drawPassOutput->depthHandle.IsValid())
        {
            needClearRenderTarget       = true;
            drawPassOutput->depthHandle = graph.Create<RHI::D3D12Texture>(depthTexDesc);
        }
        if (!drawPassOutput->normalHandle.IsValid())
        {
            needClearRenderTarget        = true;
            drawPassOutput->normalHandle = graph.Create<RHI::D3D12Texture>(normalTexDesc);
        }
        if (!drawPassOutput->clearCoatNormalHandle.IsValid())
        {
            needClearRenderTarget       = true;
            drawPassOutput->clearCoatNormalHandle = graph.Create<RHI::D3D12Texture>(clearCoatNormalTexDesc);
        }
        if (!drawPassOutput->emissiveHandle.IsValid())
        {
            needClearRenderTarget       = true;
            drawPassOutput->emissiveHandle = graph.Create<RHI::D3D12Texture>(emissiveDesc);
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
        return needClearRenderTarget;
    }

}
