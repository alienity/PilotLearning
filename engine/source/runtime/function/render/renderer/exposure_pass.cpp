#include "runtime/function/render/renderer/exposure_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void ExposurePass::initialize(const ExposureInitInfo& init_info)
	{
        m_HistogramBufferDesc = RHI::RgBufferDesc("Histogram");
        m_HistogramBufferDesc.SetSize(256, 4);
        m_HistogramBufferDesc.SetRHIBufferMode(RHI::RHIBufferMode::RHIBufferModeImmutable);
        m_HistogramBufferDesc.AllowUnorderedAccess();
        m_HistogramBufferDesc.AsRowBuffer();
        
        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        GenerateHistogramCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                              m_ShaderRootPath / "hlsl/GenerateHistogramCS.hlsl",
                                                              ShaderCompileOptions(L"main"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(1)
                    .AddConstantBufferView<1, 0>()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pGenerateHistogramCSSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pGenerateHistogramCSSignature.get());
            psoStream.CS                    = &GenerateHistogramCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pGenerateHistogramCSPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"GenerateHistogramPSO", psoDesc);
        }

        AdaptExposureCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/AdaptExposureCS.hlsl", ShaderCompileOptions(L"main"));

        {
            RHI::RootSignatureDesc rootSigDesc = RHI::RootSignatureDesc()
                                                     .Add32BitConstants<0, 0>(5)
                                                     .AddConstantBufferView<1, 0>()
                                                     .AllowResourceDescriptorHeapIndexing()
                                                     .AllowSampleDescriptorHeapIndexing();

            pAdaptExposureCSSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pAdaptExposureCSSignature.get());
            psoStream.CS                    = &AdaptExposureCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pAdaptExposureCSPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"AdaptExposureCSPSO", psoDesc);
        }

    }

    void ExposurePass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle inputLumaLRHandle = passInput.inputLumaLRHandle;
        RHI::RgResourceHandle outputExposureHandle = passOutput.exposureHandle;

        if (!EngineConfig::g_HDRConfig.m_EnableAdaptiveExposure)
        {
            RHI::RenderPass& defaultExposurePass = graph.AddRenderPass("DefaultUpdateExposure");

            defaultExposurePass.Write(outputExposureHandle, true);
            defaultExposurePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

                RHI::D3D12Buffer* exposureStructure = registry->GetD3D12Buffer(outputExposureHandle);
                
                __declspec(align(16)) float initExposure[] = {
                    EngineConfig::g_HDRConfig.m_Exposure,
                    1.0f / EngineConfig::g_HDRConfig.m_Exposure,
                    EngineConfig::g_HDRConfig.m_Exposure,
                    0.0f,
                    EngineConfig::kInitialMinLog,
                    EngineConfig::kInitialMaxLog,
                    EngineConfig::kInitialMaxLog - EngineConfig::kInitialMinLog,
                    1.0f / (EngineConfig::kInitialMaxLog - EngineConfig::kInitialMinLog)};

                context->WriteBuffer(exposureStructure, 0, initExposure, sizeof(initExposure));
                context->TransitionBarrier(exposureStructure, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);
            });

            return;
        }

        // Generate an HDR histogram
        {
            RHI::RgResourceHandle histogramHandle = graph.Create<RHI::D3D12Buffer>(m_HistogramBufferDesc);
            passOutput.histogramHandle = histogramHandle;

            RHI::RenderPass& genHistogramPass = graph.AddRenderPass("GenerateHDRDistogram");
            genHistogramPass.Read(inputLumaLRHandle);
            genHistogramPass.Write(histogramHandle);
            genHistogramPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

                RHI::D3D12Texture* m_LumaLRColor = registry->GetD3D12Texture(inputLumaLRHandle);
                RHI::D3D12ShaderResourceView* m_LumaLRColorSRV = m_LumaLRColor->GetDefaultSRV().get();

                RHI::D3D12Buffer* m_HistogramBuffer = registry->GetD3D12Buffer(histogramHandle);
                RHI::D3D12UnorderedAccessView* m_HistogramBufferUAV = m_HistogramBuffer->GetDefaultUAV().get();

                computeContext->TransitionBarrier(m_HistogramBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
                computeContext->ClearUAV(m_HistogramBuffer);
                computeContext->TransitionBarrier(m_LumaLRColor, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

                //computeContext->ClearUAV(m_HistogramBuffer);

                computeContext->SetRootSignature(pGenerateHistogramCSSignature.get());
                computeContext->SetPipelineState(pGenerateHistogramCSPSO.get());

                computeContext->SetConstants(0, (UINT)m_LumaLRColor->GetHeight());

                __declspec(align(16)) struct GenHistogramIndexInput
                {
                    uint32_t _InputLumaBufIndex;
                    uint32_t _OutHistogramIndex;
                };
                GenHistogramIndexInput m_GenHistogramIndexInput = {m_LumaLRColorSRV->GetIndex(),
                                                                   m_HistogramBufferUAV->GetIndex()};

                computeContext->SetDynamicConstantBufferView(1, m_GenHistogramIndexInput);

                computeContext->Dispatch2D(
                    m_LumaLRColor->GetWidth(), m_LumaLRColor->GetHeight(), 16, m_LumaLRColor->GetHeight());
            });
            
            RHI::RenderPass& genHistogram2Pass = graph.AddRenderPass("AdaptExposure");
            genHistogram2Pass.Read(histogramHandle);
            genHistogram2Pass.Write(outputExposureHandle);
            genHistogram2Pass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

                RHI::D3D12Texture* m_LumaLRColor = registry->GetD3D12Texture(inputLumaLRHandle);
                
                RHI::D3D12Buffer* m_HistogramBuffer = registry->GetD3D12Buffer(histogramHandle);
                RHI::D3D12ShaderResourceView*  m_HistogramBufferSRV = m_HistogramBuffer->GetDefaultSRV().get();

                RHI::D3D12Buffer* m_ExposureBuffer = registry->GetD3D12Buffer(outputExposureHandle);
                RHI::D3D12UnorderedAccessView* m_exposureBufferUAV = m_ExposureBuffer->GetDefaultUAV().get();

                computeContext->SetRootSignature(pAdaptExposureCSSignature.get());
                computeContext->SetPipelineState(pAdaptExposureCSPSO.get());

                __declspec(align(16)) struct
                {
                    float    TargetLuminance;
                    float    AdaptationRate;
                    float    MinExposure;
                    float    MaxExposure;
                    uint32_t PixelCount;
                } constants = {EngineConfig::g_HDRConfig.m_TargetLuminance,
                               EngineConfig::g_HDRConfig.m_AdaptationRate,
                               EngineConfig::g_HDRConfig.m_MinExposure,
                               EngineConfig::g_HDRConfig.m_MaxExposure,
                               (uint32_t)m_LumaLRColor->GetWidth() * (uint32_t)m_LumaLRColor->GetHeight()};
                
                //computeContext->SetDynamicConstantBufferView(0, sizeof(constants), &constants);
                computeContext->SetConstantArray(0, 5, &constants);

                __declspec(align(16)) struct
                {
                    uint32_t m_InputHistogramIndex;
                    uint32_t m_OutExposureIndex;
                } indexCb = {m_HistogramBufferSRV->GetIndex(), m_exposureBufferUAV->GetIndex()};

                computeContext->SetDynamicConstantBufferView(1, sizeof(indexCb), &indexCb);

                computeContext->Dispatch(1, 1, 64);

            });
            
        }

    }

    void ExposurePass::destroy()
    {
        pGenerateHistogramCSSignature = nullptr;
        pGenerateHistogramCSPSO       = nullptr;
        pAdaptExposureCSSignature     = nullptr;
        pAdaptExposureCSPSO           = nullptr;
    }

}
