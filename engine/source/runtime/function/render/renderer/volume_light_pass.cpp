#include "runtime/function/render/renderer/volume_light_pass.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/renderer/renderer_config.h"

#include <cassert>

namespace MoYu
{

	void VolumeLightPass::initialize(const PassInitInfo& init_info)
	{
        colorTexDesc = init_info.colorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        {
            mGuassianBlurCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/GuassianBlur.hlsl", ShaderCompileOptions(L"CSMain"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                                             8,
                                             D3D12_COMPARISON_FUNC_LESS_EQUAL,
                                             D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pGuassianBlurSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pGuassianBlurSignature.get());
            psoStream.CS                    = &mGuassianBlurCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pGuassianBlurPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"GuassianBlurPSO", psoDesc);
        }

        {
            mVolumeLightingCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/VolumetricLighting.hlsl", ShaderCompileOptions(L"CSMain"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                                             8,
                                             D3D12_COMPARISON_FUNC_LESS_EQUAL,
                                             D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pVolumeLightingSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pVolumeLightingSignature.get());
            psoStream.CS                    = &mVolumeLightingCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pVolumeLightingPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"VolumeLightingPS", psoDesc);
        }

	}

    void VolumeLightPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        m_bluenoise = m_render_scene->m_bluenoise_map.m_bluenoise_64x64_uni;

        HLSL::FrameUniforms* _frameUniforms = &(render_resource->m_FrameUniforms);

        HLSL::VolumeLightUniform volumeLightUniform = {};

        volumeLightUniform.scattering_coef = EngineConfig::g_VolumeLightConfig.mScatteringCoef;
        volumeLightUniform.extinction_coef = EngineConfig::g_VolumeLightConfig.mExtinctionCoef;
        volumeLightUniform.volumetrix_range = 1.0f;
        volumeLightUniform.skybox_extinction_coef = 1.0f - EngineConfig::g_VolumeLightConfig.mSkyboxExtinctionCoef;
        
        volumeLightUniform.mieG = EngineConfig::g_VolumeLightConfig.mMieG;
        volumeLightUniform.noise_scale = EngineConfig::g_VolumeLightConfig.mNoiseSccale;
        volumeLightUniform.noise_intensity = EngineConfig::g_VolumeLightConfig.mNoiseIntensity;
        volumeLightUniform.noise_intensity_offset = EngineConfig::g_VolumeLightConfig.mNoiseIntensityOffset;

        volumeLightUniform.noise_velocity = EngineConfig::g_VolumeLightConfig.mNoiseVelocity;
        volumeLightUniform.ground_level = EngineConfig::g_VolumeLightConfig.mGroundLevel;
        volumeLightUniform.height_scale = EngineConfig::g_VolumeLightConfig.mHeightScale;

        volumeLightUniform.maxRayLength = EngineConfig::g_VolumeLightConfig.mMaxRayLength;
        volumeLightUniform.sampleCount = EngineConfig::g_VolumeLightConfig.mSampleCount;

        _frameUniforms->volumeLightUniform = volumeLightUniform;

    }

    void VolumeLightPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        /*
        RHI::RenderPass& drawpass = graph.AddRenderPass("SkyboxPass");

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle renderTargetColorHandle = passOutput.renderTargetColorHandle;
        RHI::RgResourceHandle renderTargetDepthHandle = passOutput.renderTargetDepthHandle;

        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
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
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)colorTexDesc.Width, (float)colorTexDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)colorTexDesc.Width, (int)colorTexDesc.Height});

            if (needClearRenderTarget)
            {
                graphicContext->ClearRenderTarget(renderTargetView, depthStencilView);
            }
            graphicContext->SetRenderTarget(renderTargetView, depthStencilView);

            graphicContext->SetRootSignature(RootSignatures::pSkyBoxRootSignature.get());
            graphicContext->SetPipelineState(PipelineStates::pSkyBoxPSO.get());
            graphicContext->SetConstants(0, specularIBLTexIndex, specularIBLTexLevel);
            //graphicContext->SetConstantBuffer(1, pPerframeBuffer->GetGpuVirtualAddress());
            graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
            
            graphicContext->Draw(3);
        });
        */
    }


    void VolumeLightPass::destroy()
    {

    }

}
