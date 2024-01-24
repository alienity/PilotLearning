#include "runtime/function/render/renderer/atmospheric_scattering_pass.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/renderer/renderer_config.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include <cassert>

namespace MoYu
{

	void AtmosphericScatteringPass::initialize(const PassInitInfo& init_info)
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
                                             D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                                             D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                                             8,
                                             D3D12_COMPARISON_FUNC_LESS_EQUAL,
                                             D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                             8,
                                             D3D12_COMPARISON_FUNC_GREATER_EQUAL,
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
        volumeLightUniform.downscaleMip = EngineConfig::g_VolumeLightConfig.mDownScaleMip;
        volumeLightUniform.minStepSize = EngineConfig::g_VolumeLightConfig.mMinStepSize;

        _frameUniforms->volumeLightUniform = volumeLightUniform;

        if (m_volume3d == nullptr)
        {
            int downScale = 1 << volumeLightUniform.downscaleMip;
            int volumeWidth = colorTexDesc.Width / downScale;
            int volumeHeight = colorTexDesc.Height / downScale;
            int volumeDpeth = volumeLightUniform.sampleCount;

            m_volume3d = RHI::D3D12Texture::Create3D(m_Device->GetLinkedDevice(),
                                                     volumeWidth,
                                                     volumeHeight,
                                                     volumeDpeth,
                                                     1,
                                                     DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                     RHI::RHISurfaceCreateRandomWrite,
                                                     1,
                                                     fmt::format(L"Volume3DTexture"),
                                                     D3D12_RESOURCE_STATE_COMMON,
                                                     std::nullopt);
        }

    }

    void VolumeLightPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        int downScale    = 1 << EngineConfig::g_VolumeLightConfig.mDownScaleMip;
        int volumeWidth  = colorTexDesc.Width / downScale;
        int volumeHeight = colorTexDesc.Height / downScale;
        
        RHI::RenderPass& volumeLightPass = graph.AddRenderPass("VolumeLightPass");

        RHI::RgResourceHandle blueNoiseHandle = GImport(graph, m_bluenoise.get());
        RHI::RgResourceHandle volume3DHandle = GImport(graph, m_volume3d.get());

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle maxDepthPtyramidHandle = passInput.maxDepthPtyramidHandle;

        volumeLightPass.Read(perframeBufferHandle, true);
        volumeLightPass.Read(maxDepthPtyramidHandle, true);
        volumeLightPass.Read(blueNoiseHandle, true);
        volumeLightPass.Write(volume3DHandle, true);

        volumeLightPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetTex(maxDepthPtyramidHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(blueNoiseHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(volume3DHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pVolumeLightingSignature.get());
            pContext->SetPipelineState(pVolumeLightingPSO.get());

            struct RootIndexBuffer
            {
                UINT perFrameBufferIndex;
                UINT maxDepthBufferIndex;
                UINT blueNoiseIndex;
                UINT volume3DIndex;
            };

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
            srvDesc.Format                        = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip     = 0;
            srvDesc.Texture2D.MipLevels           = 1;
            srvDesc.Texture2D.PlaneSlice          = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            std::shared_ptr<RHI::D3D12ShaderResourceView> blueNoiseSRV = m_bluenoise->CreateSRV(srvDesc);
            UINT blueNoiseSRVIndex = blueNoiseSRV->GetIndex();

            UINT volume3DUAVIndex = RegGetTexDefUAVIdx(volume3DHandle);

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetTexDefSRVIdx(maxDepthPtyramidHandle),
                                                               blueNoiseSRVIndex,
                                                               volume3DUAVIndex};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(volumeWidth, volumeHeight, 8, 8);

        });

        passOutput.volumeLightHandle = volume3DHandle;
    }


    void VolumeLightPass::destroy()
    {
        m_volume3d = nullptr;



    }

}
