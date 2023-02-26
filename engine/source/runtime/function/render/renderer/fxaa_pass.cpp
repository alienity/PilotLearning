#include "runtime/function/render/renderer/fxaa_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{

	void FXAAPass::initialize(const FXAAInitInfo& init_info)
	{
        mTmpColorTexDesc = init_info.m_ColorTexDesc;
        mTmpColorTexDesc.SetAllowUnorderedAccess();
        mTmpColorTexDesc.SetAllowRenderTarget(false);

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        FXAAToLuminanceCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/FXAAToLuminanceCS.hlsl", ShaderCompileOptions(L"CSMain"));
        FXAALuminanceCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/FXAALuminanceCS.hlsl", ShaderCompileOptions(L"CSMain"));
        FXAAGreenCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/FXAAGreenCS.hlsl", ShaderCompileOptions(L"CSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc = RHI::RootSignatureDesc()
                                                     .Add32BitConstants<0, 0>(2)
                                                     .AllowResourceDescriptorHeapIndexing()
                                                     .AllowSampleDescriptorHeapIndexing();

            pFXAAToLuminanceSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(5)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pFXAALuminanceSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(5)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pFXAAGreenSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pFXAAToLuminanceSignature.get());
            psoStream.CS            = &FXAAToLuminanceCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pFXAAToLuminancePSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"FXAAToLuminancePSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pFXAALuminanceSignature.get());
            psoStream.CS                    = &FXAALuminanceCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pFXAALuminancePSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"FXAALuminancePSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pFXAAGreenSignature.get());
            psoStream.CS                    = &FXAAGreenCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pFXAAGreenPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"FXAAGreenPSO", psoDesc);
        }
    }

    void FXAAPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        DrawInputParameters  drawPassInput  = passInput;
        DrawOutputParameters drawPassOutput = passOutput;

        RHI::RgResourceHandle mTmpColorHandle = graph.Create<RHI::D3D12Texture>(mTmpColorTexDesc);

        RHI::RenderPass& fxaaToLuminancePass = graph.AddRenderPass("FXAAToLuminance");

        fxaaToLuminancePass.Read(drawPassInput.inputLumaColorHandle);
        fxaaToLuminancePass.Read(drawPassInput.inputSceneColorHandle);
        fxaaToLuminancePass.Write(mTmpColorHandle);

        fxaaToLuminancePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

            RHI::D3D12Texture* pInputSceneColor = registry->GetD3D12Texture(drawPassInput.inputSceneColorHandle);
            RHI::D3D12ShaderResourceView*  pSceneColorSRV = pInputSceneColor->GetDefaultSRV().get();
            RHI::D3D12Texture*             pTempColor     = registry->GetD3D12Texture(mTmpColorHandle);
            RHI::D3D12UnorderedAccessView* tmpColorUAV    = pTempColor->GetDefaultUAV().get();

            //computeContext->TransitionBarrier(pInputColor, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            //computeContext->TransitionBarrier(pTempColor, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            ////computeContext->InsertUAVBarrier(pTempColor);
            computeContext->SetRootSignature(pFXAAToLuminanceSignature.get());
            computeContext->SetPipelineState(pFXAAToLuminancePSO.get());
            computeContext->SetConstants(0, pSceneColorSRV->GetIndex(), tmpColorUAV->GetIndex());
            computeContext->Dispatch2D(pInputSceneColor->GetWidth(), pInputSceneColor->GetHeight(), 8);
        });

        RHI::RenderPass& fxaapass = graph.AddRenderPass("FXAA");

        fxaapass.Read(drawPassInput.inputSceneColorHandle);
        fxaapass.Read(mTmpColorHandle);
        fxaapass.Write(drawPassOutput.outputColorHandle);

        fxaapass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();
            
            RHI::D3D12Texture* pInputSceneColor = registry->GetD3D12Texture(drawPassInput.inputSceneColorHandle);
            RHI::D3D12ShaderResourceView* pSceneColorSRV = pInputSceneColor->GetDefaultSRV().get();
            RHI::D3D12Texture* pTempColor = registry->GetD3D12Texture(mTmpColorHandle);
            RHI::D3D12ShaderResourceView* tempColorSRV = pTempColor->GetDefaultSRV().get();

            RHI::D3D12Texture* pOutputColor = registry->GetD3D12Texture(drawPassOutput.outputColorHandle);
            RHI::D3D12UnorderedAccessView* rtOutputUAVView = pOutputColor->GetDefaultUAV().get();

            //computeContext->TransitionBarrier(pInputColor, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            //computeContext->TransitionBarrier(pOutputColor, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            ////computeContext->InsertUAVBarrier(pOutputColor);
            computeContext->SetRootSignature(pFXAALuminanceSignature.get());
            computeContext->SetPipelineState(pFXAALuminancePSO.get());

            __declspec(align(16)) struct LuminanceInput
            {
                float    _ContrastThreshold;
                float    _RelativeThreshold;
                float    _SubpixelBlending;
                uint32_t _BaseMapIndex;
                uint32_t _OutputMapIndex;
            };
            LuminanceInput m_LuminanceInput = {};
            m_LuminanceInput._ContrastThreshold = EngineConfig::g_FXAAConfig.m_ContrastThreshold;
            m_LuminanceInput._RelativeThreshold = EngineConfig::g_FXAAConfig.m_RelativeThreshold;
            m_LuminanceInput._SubpixelBlending  = EngineConfig::g_FXAAConfig.m_SubpixelBlending;
            m_LuminanceInput._BaseMapIndex      = tempColorSRV->GetIndex();
            m_LuminanceInput._OutputMapIndex    = rtOutputUAVView->GetIndex();

            computeContext->SetConstantArray(0, 5, &m_LuminanceInput);
            computeContext->Dispatch2D(pInputSceneColor->GetWidth(), pInputSceneColor->GetHeight(), 8);

        });
    }

    void FXAAPass::destroy()
    {
        pFXAAToLuminanceSignature = nullptr;
        pFXAALuminanceSignature   = nullptr;
        pFXAAGreenSignature       = nullptr;

        pFXAAToLuminancePSO = nullptr;
        pFXAALuminancePSO   = nullptr;
        pFXAAGreenPSO       = nullptr;
    }

}
