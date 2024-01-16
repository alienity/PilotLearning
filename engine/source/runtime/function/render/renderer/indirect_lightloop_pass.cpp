#include "runtime/function/render/renderer/indirect_lightloop_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include <cassert>

namespace MoYu
{

	void IndirectLightLoopPass::initialize(const DrawPassInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        indirectLightLoopCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/IndirectLightLoopCS.hlsl", ShaderCompileOptions(L"CSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(32)
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

            pIndirectLightLoopSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pIndirectLightLoopSignature.get());
            psoStream.CS                    = &indirectLightLoopCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectLightLoopPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"LightLoopPSO", psoDesc);
        }

	}

    void IndirectLightLoopPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        RHI::RenderPass& drawpass = graph.AddRenderPass("LightLoopPass");

        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.albedoHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.gbufferDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.worldNormalHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.ambientOcclusionHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.metallic_Roughness_Reflectance_AO_Handle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.clearCoat_ClearCoatRoughness_Anisotropy_Handle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.ssrResolveHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        drawpass.Write(passOutput.colorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle albedoHandle         = passInput.albedoHandle;
        RHI::RgResourceHandle depthHandle          = passInput.gbufferDepthHandle;
        RHI::RgResourceHandle worldNormalHandle    = passInput.worldNormalHandle;
        RHI::RgResourceHandle ambientOcclusionHandle = passInput.ambientOcclusionHandle;
        RHI::RgResourceHandle metallic_Roughness_Reflectance_AO_Handle = passInput.metallic_Roughness_Reflectance_AO_Handle;
        RHI::RgResourceHandle clearCoat_ClearCoatRoughness_Anisotropy_Handle = passInput.clearCoat_ClearCoatRoughness_Anisotropy_Handle;
        RHI::RgResourceHandle ssrResolveHandle = passInput.ssrResolveHandle;
        RHI::RgResourceHandle outColorHandle = passOutput.colorHandle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            //RHI::D3D12ShaderResourceView* albedoSRV = registry->GetD3D12Texture(albedoHandle)->GetDefaultSRV().get();
            //RHI::D3D12ShaderResourceView* worldNormalSRV  = registry->GetD3D12Texture(worldNormalHandle)->GetDefaultSRV().get();
            //RHI::D3D12ShaderResourceView* ambientOcclusionSRV = registry->GetD3D12Texture(ambientOcclusionHandle)->GetDefaultSRV().get();
            //RHI::D3D12ShaderResourceView* metallic_Roughness_Reflectance_AO_SRV = registry->GetD3D12Texture(metallic_Roughness_Reflectance_AO_Handle)->GetDefaultSRV().get();
            //RHI::D3D12ShaderResourceView* clearCoat_ClearCoatRoughness_Anisotropy_SRV = registry->GetD3D12Texture(clearCoat_ClearCoatRoughness_Anisotropy_Handle)->GetDefaultSRV().get();
            //RHI::D3D12ShaderResourceView* depthSRV = registry->GetD3D12Texture(depthHandle)->GetDefaultSRV().get();
            //RHI::D3D12ShaderResourceView* ssrResolveSRV = registry->GetD3D12Texture(ssrResolveHandle)->GetDefaultSRV().get();
            //RHI::D3D12UnorderedAccessView* outColorUAV = registry->GetD3D12Texture(outColorHandle)->GetDefaultUAV().get();

            pContext->SetRootSignature(pIndirectLightLoopSignature.get());
            pContext->SetPipelineState(pIndirectLightLoopPSO.get());

            struct RootIndexBuffer
            {
                uint32_t perFrameBufferIndex;
                uint32_t albedoIndex;
                uint32_t worldNormalIndex;
                uint32_t ambientOcclusionIndex;
                uint32_t metallic_Roughness_Reflectance_AOIndex;
                uint32_t clearCoat_ClearCoatRoughness_AnisotropyIndex;
                uint32_t depthIndex;
                uint32_t ssrResolveIndex;
                uint32_t outColorIndex;
            };

            RootIndexBuffer rootIndexBuffer = {RegGetBufDefCBVIdx(perframeBufferHandle),
                                               RegGetTexDefSRVIdx(albedoHandle),
                                               RegGetTexDefSRVIdx(worldNormalHandle),
                                               RegGetTexDefSRVIdx(ambientOcclusionHandle),
                                               RegGetTexDefSRVIdx(metallic_Roughness_Reflectance_AO_Handle),
                                               RegGetTexDefSRVIdx(clearCoat_ClearCoatRoughness_Anisotropy_Handle),
                                               RegGetTexDefSRVIdx(depthHandle),
                                               RegGetTexDefSRVIdx(ssrResolveHandle),
                                               RegGetTexDefUAVIdx(outColorHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);

        });
    }


    void IndirectLightLoopPass::destroy()
    {


    }

    bool IndirectLightLoopPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->colorHandle.IsValid())
        {
            needClearRenderTarget = true;

            RHI::RgTextureDesc _targetColorDesc = colorTexDesc;
            _targetColorDesc.SetAllowUnorderedAccess(true);
            _targetColorDesc.SetAllowRenderTarget(true);

            drawPassOutput->colorHandle = graph.Create<RHI::D3D12Texture>(_targetColorDesc);
        }
        return needClearRenderTarget;
    }

}
