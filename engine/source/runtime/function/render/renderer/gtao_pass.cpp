#include "runtime/function/render/renderer/gtao_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include "runtime/function/render/renderer/renderer_config.h"

#include <cassert>

namespace MoYu
{

	void GTAOPass::initialize(const AOInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        SSAOCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/SSAOCS.hlsl", ShaderCompileOptions(L"CSMain"));

        HBAOCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/HBAOCS.hlsl", ShaderCompileOptions(L"CSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(4)
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

            pSSAOSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(4)
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

            pHBAOSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSAOSignature.get());
            psoStream.CS                    = &SSAOCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSAOPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SSAOPSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pHBAOSignature.get());
            psoStream.CS                    = &HBAOCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pHBAOPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"HBAOPSO", psoDesc);
        }

    }


    void GTAOPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        
        if (EngineConfig::g_AOConfig._aoMode == EngineConfig::AOConfig::SSAO)
        {
            RHI::RenderPass& drawpass = graph.AddRenderPass("SSAOPass");

            drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            drawpass.Read(passInput.worldNormalHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            drawpass.Read(passInput.depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            drawpass.Write(passOutput.outputAOHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

            RHI::RgResourceHandle normalHandle = passInput.worldNormalHandle;
            RHI::RgResourceHandle depthHandle  = passInput.depthHandle;
            RHI::RgResourceHandle outAOHandle  = passOutput.outputAOHandle;

            drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                RHI::D3D12ShaderResourceView* worldNormalSRV = registry->GetD3D12Texture(normalHandle)->GetDefaultSRV().get();
                RHI::D3D12ShaderResourceView* depthSRV = registry->GetD3D12Texture(depthHandle)->GetDefaultSRV().get();
                RHI::D3D12UnorderedAccessView* outAOUAV = registry->GetD3D12Texture(outAOHandle)->GetDefaultUAV().get();

                pContext->SetRootSignature(pSSAOSignature.get());
                pContext->SetPipelineState(pSSAOPSO.get());
                pContext->SetConstant(0, 0, worldNormalSRV->GetIndex());
                pContext->SetConstant(0, 1, depthSRV->GetIndex());
                pContext->SetConstant(0, 2, outAOUAV->GetIndex());
                pContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());

                pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);
            });
        }
        else // HBAO
        {
            RHI::RenderPass& drawpass = graph.AddRenderPass("HBAOPass");

            drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            drawpass.Read(passInput.worldNormalHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            drawpass.Read(passInput.depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            drawpass.Write(passOutput.outputAOHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

            RHI::RgResourceHandle normalHandle = passInput.worldNormalHandle;
            RHI::RgResourceHandle depthHandle  = passInput.depthHandle;
            RHI::RgResourceHandle outAOHandle  = passOutput.outputAOHandle;

            drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                RHI::D3D12ShaderResourceView* worldNormalSRV = registry->GetD3D12Texture(normalHandle)->GetDefaultSRV().get();
                RHI::D3D12ShaderResourceView* depthSRV = registry->GetD3D12Texture(depthHandle)->GetDefaultSRV().get();
                RHI::D3D12UnorderedAccessView* outAOUAV = registry->GetD3D12Texture(outAOHandle)->GetDefaultUAV().get();

                pContext->SetRootSignature(pHBAOSignature.get());
                pContext->SetPipelineState(pHBAOPSO.get());
                pContext->SetConstant(0, 0, worldNormalSRV->GetIndex());
                pContext->SetConstant(0, 1, depthSRV->GetIndex());
                pContext->SetConstant(0, 2, outAOUAV->GetIndex());
                pContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());

                pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);
            });
        }

    }

    void GTAOPass::destroy()
    {


    }

    bool GTAOPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->outputAOHandle.IsValid())
        {
            needClearRenderTarget = true;

            RHI::RgTextureDesc _targetAODesc = colorTexDesc;

            _targetAODesc.Name = "AOTex";
            _targetAODesc.SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT);
            _targetAODesc.SetAllowUnorderedAccess(true);
            
            drawPassOutput->outputAOHandle = graph.Create<RHI::D3D12Texture>(_targetAODesc);
        }

        return false;
    }

}
