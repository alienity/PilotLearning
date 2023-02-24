#include "runtime/function/render/renderer/hdr_tonemapping_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void HDRToneMappingPass::initialize(const HDRToneMappingInitInfo& init_info)
	{

        m_LumaBufferDesc = init_info.m_ColorTexDesc;
        m_LumaBufferDesc.SetMipLevels(1);
        m_LumaBufferDesc.SetFormat(DXGI_FORMAT_R8_UNORM);
        m_LumaBufferDesc.SetAllowUnorderedAccess();
        m_LumaBufferDesc.SetAllowRenderTarget(false);

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        ToneMapCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/ToneMapCS.hlsl", ShaderCompileOptions(L"main"));
        
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(3)
                    .AddConstantBufferView<1, 0>()
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
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

        //p_UnbindIndexBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
        //                                               RHI::RHIBufferTarget::RHIBufferTargetNone,
        //                                               sizeof(_UnbindIndexBuffer) / sizeof(uint32_t),
        //                                               sizeof(uint32_t),
        //                                               L"ToneMapping",
        //                                               RHI::RHIBufferMode::RHIBufferModeDynamic,
        //                                               D3D12_RESOURCE_STATE_GENERIC_READ);
    }

	void HDRToneMappingPass::update(RHI::RenderGraph&     graph,
                                    DrawInputParameters&  passInput,
                                    DrawOutputParameters& passOutput)
    {
        DrawInputParameters  drawPassInput  = passInput;
        DrawOutputParameters drawPassOutput = passOutput;

        RHI::RgResourceHandle mLumaBufferHandle = graph.Create<RHI::D3D12Texture>(m_LumaBufferDesc);

        drawPassOutput.outputLumaHandle = mLumaBufferHandle;

        RHI::RenderPass& tonemappingPass = graph.AddRenderPass("ToneMapping");

        //tonemappingPass.Read(drawPassInput.inputExposureHandle);
        //tonemappingPass.Read(drawPassInput.inputBloomHandle);
        tonemappingPass.Read(drawPassInput.inputSceneColorHandle);
        tonemappingPass.Write(drawPassOutput.outputLumaHandle);
        tonemappingPass.Write(drawPassOutput.outputPostEffectsHandle);

        tonemappingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

            //RHI::D3D12Buffer* exposureStructure = registry->GetD3D12Buffer(drawPassInput.inputExposureHandle);
            //RHI::D3D12ShaderResourceView* exposureStructureSRV = exposureStructure->GetDefaultSRV().get();

            //RHI::D3D12Texture* bloomColor = registry->GetD3D12Texture(drawPassInput.inputBloomHandle);
            //RHI::D3D12ShaderResourceView* bloomColorSRV = bloomColor->GetDefaultSRV().get();

            RHI::D3D12Texture* sceneColor = registry->GetD3D12Texture(drawPassInput.inputSceneColorHandle);
            RHI::D3D12ShaderResourceView* sceneColorSRV = sceneColor->GetDefaultSRV().get();

            RHI::D3D12Texture* postEffectsColor = registry->GetD3D12Texture(drawPassOutput.outputPostEffectsHandle);
            RHI::D3D12UnorderedAccessView* postEffectsColorUAV = postEffectsColor->GetDefaultUAV().get();
            
            RHI::D3D12Texture* inputLumaColor = registry->GetD3D12Texture(drawPassOutput.outputLumaHandle);
            RHI::D3D12UnorderedAccessView* inputLumaColorUAV = inputLumaColor->GetDefaultUAV().get();

            computeContext->SetRootSignature(pToneMapCSSignature.get());
            computeContext->SetPipelineState(pToneMapCSPSO.get());

            float m_BloomStrength = EngineConfig::g_BloomConfig.m_BloomStrength;
            
            // Set constants
            computeContext->SetConstants(0, 1.0f / sceneColor->GetWidth(), 1.0f / sceneColor->GetHeight(), (float)m_BloomStrength);

            //_UnbindIndexBuffer mTmpUnbindIndexBuffer = _UnbindIndexBuffer {exposureStructureSRV->GetIndex(),
            //                                                               bloomColorSRV->GetIndex(),
            //                                                               postEffectsColorUAV->GetIndex(),
            //                                                               sceneColorSRV->GetIndex(),
            //                                                               inputLumaColorUAV->GetIndex()};

            _UnbindIndexBuffer mTmpUnbindIndexBuffer = _UnbindIndexBuffer {
                postEffectsColorUAV->GetIndex(), sceneColorSRV->GetIndex(), inputLumaColorUAV->GetIndex()};

            computeContext->SetDynamicConstantBufferView(1, mTmpUnbindIndexBuffer);

            computeContext->Dispatch2D(sceneColor->GetWidth(), sceneColor->GetHeight(), 8);
        });
	}

	void HDRToneMappingPass::destroy()
    {
		pToneMapCSSignature = nullptr;
        pToneMapCSPSO       = nullptr;
    }

}
