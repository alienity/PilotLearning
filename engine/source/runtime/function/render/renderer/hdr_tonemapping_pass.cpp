#include "runtime/function/render/renderer/hdr_tonemapping_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void HDRToneMappingPass::initialize(const HDRToneMappingInitInfo& init_info)
	{
        m_LumaBufferDesc = RHI::RgTextureDesc("LumaBuffer");
        m_LumaBufferDesc.SetFormat(DXGI_FORMAT_R8_UNORM);
        m_LumaBufferDesc.SetType(RHI::RgTextureType::Texture2D);
        m_LumaBufferDesc.SetExtent(init_info.m_ColorTexDesc.Width, init_info.m_ColorTexDesc.Height);
        m_LumaBufferDesc.SetSampleCount(1);
        m_LumaBufferDesc.SetMipLevels(1);
        m_LumaBufferDesc.SetAllowUnorderedAccess();

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        ToneMapCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/ToneMapCS.hlsl", ShaderCompileOptions(L"main"));
        
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(3)
                    .AddConstantBufferView<1, 0>()
                    .AddStaticSampler<0, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                                            D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                            8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pToneMapCSSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pToneMapCSSignature.get());
            psoStream.CS                    = &ToneMapCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pToneMapCSPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"ToneMapPSO", psoDesc);
        }

    }

	void HDRToneMappingPass::update(RHI::RenderGraph&     graph,
                                    DrawInputParameters&  passInput,
                                    DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle mLumaBufferHandle = graph.Create<RHI::D3D12Texture>(m_LumaBufferDesc);
        passOutput.outputLumaBufferHandle = mLumaBufferHandle;

        //DrawInputParameters  drawPassInput  = passInput;
        //DrawOutputParameters drawPassOutput = passOutput;

        RHI::RenderPass& tonemappingPass = graph.AddRenderPass("ToneMapping");

        tonemappingPass.Read(passInput.inputExposureHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        tonemappingPass.Read(passInput.inputBloomHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        tonemappingPass.Read(passInput.inputSceneColorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        tonemappingPass.Write(passOutput.outputLumaBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
        tonemappingPass.Write(passOutput.outputPostEffectsHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        auto inputExposureHandle     = passInput.inputExposureHandle;
        auto inputBloomHandle        = passInput.inputBloomHandle;
        auto inputSceneColorHandle   = passInput.inputSceneColorHandle;
        auto outputPostEffectsHandle = passOutput.outputPostEffectsHandle;
        auto outputLumaBufferHandle  = passOutput.outputLumaBufferHandle;

        tonemappingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

            RHI::D3D12Buffer* exposureStructure = registry->GetD3D12Buffer(inputExposureHandle);
            RHI::D3D12ShaderResourceView* exposureStructureSRV = exposureStructure->GetDefaultSRV().get();

            RHI::D3D12Texture* bloomColor = registry->GetD3D12Texture(inputBloomHandle);
            RHI::D3D12ShaderResourceView* bloomColorSRV = bloomColor->GetDefaultSRV().get();

            RHI::D3D12Texture* sceneColor = registry->GetD3D12Texture(inputSceneColorHandle);
            RHI::D3D12ShaderResourceView* sceneColorSRV = sceneColor->GetDefaultSRV().get();

            RHI::D3D12Texture* postEffectsColor = registry->GetD3D12Texture(outputPostEffectsHandle);
            RHI::D3D12UnorderedAccessView* postEffectsColorUAV = postEffectsColor->GetDefaultUAV().get();
            
            RHI::D3D12Texture* lumaBuffer = registry->GetD3D12Texture(outputLumaBufferHandle);
            RHI::D3D12UnorderedAccessView* lumaBufferUAV = lumaBuffer->GetDefaultUAV().get();

            computeContext->SetRootSignature(pToneMapCSSignature.get());
            computeContext->SetPipelineState(pToneMapCSPSO.get());

            float m_BloomStrength = EngineConfig::g_BloomConfig.m_BloomStrength;
            
            // Set constants
            computeContext->SetConstants(0, 1.0f / sceneColor->GetWidth(), 1.0f / sceneColor->GetHeight(), (float)m_BloomStrength);

            __declspec(align(16)) struct
            {
                uint32_t m_ExposureIndex;
                uint32_t m_BloomIndex;
                uint32_t m_DstColorIndex;
                uint32_t m_SrcColorIndex;
                uint32_t m_OutLumaIndex;
            } _DescriptorIndexConstants = {exposureStructureSRV->GetIndex(),
                                           bloomColorSRV->GetIndex(),
                                           postEffectsColorUAV->GetIndex(),
                                           sceneColorSRV->GetIndex(),
                                           lumaBufferUAV->GetIndex()};

            computeContext->SetDynamicConstantBufferView(
                1, sizeof(_DescriptorIndexConstants), &_DescriptorIndexConstants);

            computeContext->Dispatch2D(sceneColor->GetWidth(), sceneColor->GetHeight(), 8);
        });
	}

	void HDRToneMappingPass::destroy()
    {
		pToneMapCSSignature = nullptr;
        pToneMapCSPSO       = nullptr;
    }

}
