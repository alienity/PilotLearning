#include "runtime/function/render/renderer/ssr_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/jitter_helper.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include <cassert>

namespace MoYu
{
#define MAX_MIN_Z_LEVELS 7
#define KERNEL_SIZE 16


	void SSRPass::initialize(const SSRInitInfo& init_info)
	{
        passIndex = 0;

        colorTexDesc = init_info.m_ColorTexDesc;

        SSRHitPointTextureDesc =
            RHI::RgTextureDesc("SSRHitPointTexture")
                .SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT)
                .SetExtent(colorTexDesc.Width, colorTexDesc.Height)
                .SetSampleCount(colorTexDesc.SampleCount)
                .SetAllowUnorderedAccess(true)
                .SetClearValue(RHI::RgClearValue(0.0f, 0.0f, 0.0f, 0.0f, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT));

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"ScreenSpaceReflectionsTracing");
            shaderCompileOpt.SetDefine(L"SSR_TRACE", L"1");

            SSRTraceCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceReflectionsCS.hlsl",
                shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(16)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                    .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                    .AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                    .AddStaticSampler<14, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pSSRTraceSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSRTraceSignature.get());
            psoStream.CS                    = &SSRTraceCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSRTracePSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SSRRaycastPSO", psoDesc);
            
        }
         
        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"ScreenSpaceReflectionsReprojection");
            shaderCompileOpt.SetDefine(L"SSR_REPROJECT", L"1");

            SSRReprojectCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceReflectionsCS.hlsl",
                shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                .AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                .AddStaticSampler<14, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pSSRReprojectSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pSSRReprojectSignature.get());
            psoStream.CS = &SSRReprojectCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pSSRReprojectPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SSRReprojectPSO", psoDesc);
        }

        {
            ShaderCompileOptions shaderCompileOpt = ShaderCompileOptions(L"MAIN_ACC");
            shaderCompileOpt.SetDefine(L"SSR_ACCUMULATE", L"1");
            shaderCompileOpt.SetDefine(L"WORLD_SPEED_REJECTION", L"1");
            shaderCompileOpt.SetDefine(L"SSR_SMOOTH_SPEED_REJECTION", L"1");
            shaderCompileOpt.SetDefine(L"USE_SPEED_SURFACE", L"1");
            shaderCompileOpt.SetDefine(L"USE_SPEED_TARGET", L"1");

            SSRAccumulateCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceReflectionsCS.hlsl",
                shaderCompileOpt);

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                .AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                .AddStaticSampler<14, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pSSRAccumulateSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pSSRAccumulateSignature.get());
            psoStream.CS = &SSRAccumulateCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pSSRAccumulatePSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SSRAccumulatePSO", psoDesc);
        }

    }

    void SSRPass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        m_bluenoise = m_render_scene->m_bluenoise_map.m_bluenoise_64x64_uni;

        int curFrameIndex = m_Device->GetLinkedDevice()->m_FrameIndex;

        HLSL::FrameUniforms* _frameUniforms = &(render_resource->m_FrameUniforms);

        //std::shared_ptr<RHI::D3D12Texture> pBlueNoiseUniMap = m_render_scene->m_bluenoise_map.m_bluenoise_64x64_uni;

        const int kMaxLods = 8;
        int lodCount = glm::log2((float)glm::min(colorTexDesc.Width, colorTexDesc.Height));
        lodCount = glm::min(lodCount, kMaxLods);

        float n = m_render_camera->m_nearClipPlane;
        float f = m_render_camera->m_farClipPlane;
        float thickness = EngineConfig::g_SSRConfig.depthBufferThickness;

        float smoothnessFadeStart = 0.5f;
        float minSmoothness = 0.4f;
        int colorPyramidHistoryMipCount = lodCount;
        int mipLevelCount = lodCount;

        HLSL::SSRUniform sb = {};

        sb._SsrThicknessScale = 1.0f / (1.0f + thickness);
        sb._SsrThicknessBias = -n / (f - n) * (thickness * sb._SsrThicknessScale);
        sb._SsrIterLimit = EngineConfig::g_SSRConfig.rayMaxIterations;
        // We disable sky reflection for transparent in case of a scenario where a transparent object seeing the sky through it is visible in the reflection.
        // As it has no depth it will appear extremely distorted (depth at infinity). This scenario happen frequently when you have transparent objects above water.
        // Note that the sky is still visible, it just takes its value from reflection probe/skybox rather than on screen.
        sb._SsrReflectsSky = EngineConfig::g_SSRConfig.reflectSky;
        sb._SsrStencilBit = (int)MoYu::StencilUsage::TraceReflectionRay;;
        float roughnessFadeStart = 1 - smoothnessFadeStart;
        sb._SsrRoughnessFadeEnd = 1 - minSmoothness;
        float roughnessFadeLength = sb._SsrRoughnessFadeEnd - roughnessFadeStart;
        sb._SsrRoughnessFadeEndTimesRcpLength = (roughnessFadeLength != 0) ? (sb._SsrRoughnessFadeEnd * (1.0f / roughnessFadeLength)) : 1;
        sb._SsrRoughnessFadeRcpLength = (roughnessFadeLength != 0) ? (1.0f / roughnessFadeLength) : 0;
        sb._SsrEdgeFadeRcpLength = glm::min(1.0f / EngineConfig::g_SSRConfig.screenFadeDistance, std::numeric_limits<float>::max());
        sb._SsrColorPyramidMaxMip = colorPyramidHistoryMipCount - 1;
        sb._SsrDepthPyramidMaxMip = mipLevelCount - 1;
        if (curFrameIndex <= 3)
        {
            sb._SsrAccumulationAmount = 1.0f;
        }
        else
        {
            sb._SsrAccumulationAmount = glm::pow(2, glm::lerp(0.0f, -7.0f, EngineConfig::g_SSRConfig.accumulationFactor));
        }
        if (EngineConfig::g_SSRConfig.enableWorldSpeedRejection && !EngineConfig::g_SSRConfig.speedSmoothReject)
        {
            sb._SsrPBRSpeedRejection = glm::clamp(1.0f - EngineConfig::g_SSRConfig.speedRejectionParam, 0.0f, 1.0f);
        }
        else
        {
            sb._SsrPBRSpeedRejection = glm::clamp(EngineConfig::g_SSRConfig.speedRejectionParam, 0.0f, 1.0f);
        }
        sb._SsrPBRBias = EngineConfig::g_SSRConfig.biasFactor;
        sb._SsrPRBSpeedRejectionScalerFactor = glm::pow(EngineConfig::g_SSRConfig.speedRejectionScalerFactor * 0.1f, 2.0f);

        _frameUniforms->ssrUniform = sb;


        if (pSSRAccumTexture[0] == nullptr)
        {
            for (int i = 0; i < 2; i++)
            {
                pSSRAccumTexture[i] =
                    RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                colorTexDesc.Width,
                                                colorTexDesc.Height,
                                                1,
                                                colorTexDesc.Format,
                                                RHI::RHISurfaceCreateRandomWrite,
                                                1,
                                                fmt::format(L"SSRAccumTexture_{}", i),
                                                D3D12_RESOURCE_STATE_COMMON,
                                                std::nullopt);
            }
        }
        if (pSSRLightingTexture == nullptr)
        {
            pSSRLightingTexture = 
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                            colorTexDesc.Width,
                                            colorTexDesc.Height,
                                            1,
                                            colorTexDesc.Format,
                                            RHI::RHISurfaceCreateRandomWrite,
                                            1,
                                            fmt::format(L"SSRLightingTexture"),
                                            D3D12_RESOURCE_STATE_COMMON,
                                            std::nullopt);
        }
    }

    void SSRPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {


        /*
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle worldNormalHandle    = passInput.worldNormalHandle;
        RHI::RgResourceHandle mrraMapHandle        = passInput.mrraMapHandle;
        RHI::RgResourceHandle maxDepthPtyramidHandle = passInput.maxDepthPtyramidHandle;
        RHI::RgResourceHandle lastFrameColorHandle   = passInput.lastFrameColorHandle;

        RHI::RgResourceHandle blueNoiseHandle = GImport(graph, m_bluenoise.get());

        RHI::RgResourceHandle curTemporalResult = GImport(graph, getCurTemporalResult().get());
        RHI::RgResourceHandle prevTemporalResult = GImport(graph, getPrevTemporalResult().get());

        RHI::RgResourceHandle raycastResultHandle = graph.Create<RHI::D3D12Texture>(raycastResultDesc);
        RHI::RgResourceHandle raycastMaskHandle   = graph.Create<RHI::D3D12Texture>(raycastMaskDesc);

        RHI::RenderPass& raycastPass = graph.AddRenderPass("SSRRaycastPass");

        raycastPass.Read(perframeBufferHandle, true);
        raycastPass.Read(worldNormalHandle, true);
        raycastPass.Read(mrraMapHandle, true);
        raycastPass.Read(maxDepthPtyramidHandle, true);
        raycastPass.Read(blueNoiseHandle, true);
        raycastPass.Write(raycastResultHandle, true);
        raycastPass.Write(raycastMaskHandle, true);

        raycastPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetTex(worldNormalHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(mrraMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(maxDepthPtyramidHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(blueNoiseHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(raycastResultHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetTex(raycastMaskHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pSSRRaycastSignature.get());
            pContext->SetPipelineState(pSSRRaycastPSO.get());

            struct RootIndexBuffer
            {
                UINT perFrameBufferIndex;
                UINT worldNormalIndex;
                UINT metallicRoughnessReflectanceAOIndex;
                UINT maxDepthBufferIndex;
                UINT blueNoiseIndex;
                UINT RaycastResultIndex;
                UINT MaskResultIndex;
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


            RootIndexBuffer rootIndexBuffer = RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetTexDefSRVIdx(worldNormalHandle),
                                                               RegGetTexDefSRVIdx(mrraMapHandle),
                                                               RegGetTexDefSRVIdx(maxDepthPtyramidHandle),
                                                               blueNoiseSRVIndex,
                                                               RegGetTexDefUAVIdx(raycastResultHandle),
                                                               RegGetTexDefUAVIdx(raycastMaskHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(raycastResultDesc.Width, raycastResultDesc.Height, KERNEL_SIZE, KERNEL_SIZE);
        });

        //------------------------------------------------------------------------------------------------------

        RHI::RgResourceHandle resolveHandle = graph.Create<RHI::D3D12Texture>(resolveResultDesc);

        RHI::RenderPass& resolvePass = graph.AddRenderPass("SSRResolvePass");

        resolvePass.Read(perframeBufferHandle, true);
        resolvePass.Read(worldNormalHandle, true);
        resolvePass.Read(mrraMapHandle, true);
        resolvePass.Read(maxDepthPtyramidHandle, true);
        resolvePass.Read(lastFrameColorHandle, true);
        resolvePass.Read(raycastResultHandle, true);
        resolvePass.Read(raycastMaskHandle, true);
        resolvePass.Write(resolveHandle, true);

        resolvePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetTex(worldNormalHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(mrraMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(maxDepthPtyramidHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(lastFrameColorHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(raycastResultHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(raycastMaskHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(resolveHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pSSRResolveSignature.get());
            pContext->SetPipelineState(pSSRResolvePSO.get());

            struct RootIndexBuffer
            {
                UINT perFrameBufferIndex;
                UINT worldNormalIndex;
                UINT metallicRoughnessReflectanceAOIndex;
                UINT maxDepthBufferIndex;
                UINT ScreenInputIndex;
                UINT RaycastInputIndex;
                UINT MaskInputIndex;
                UINT ResolveResultIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetTexDefSRVIdx(worldNormalHandle),
                                                               RegGetTexDefSRVIdx(mrraMapHandle),
                                                               RegGetTexDefSRVIdx(maxDepthPtyramidHandle),
                                                               RegGetTexDefSRVIdx(lastFrameColorHandle),
                                                               RegGetTexDefSRVIdx(raycastResultHandle),
                                                               RegGetTexDefSRVIdx(raycastMaskHandle),
                                                               RegGetTexDefUAVIdx(resolveHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(resolveResultDesc.Width, resolveResultDesc.Height, KERNEL_SIZE, KERNEL_SIZE);
        });

        RHI::RenderPass& temporalPass = graph.AddRenderPass("SSRTemporalPass");

        temporalPass.Read(perframeBufferHandle, true);
        temporalPass.Read(resolveHandle, true);
        temporalPass.Read(raycastResultHandle, true);
        temporalPass.Read(maxDepthPtyramidHandle, true);
        temporalPass.Read(prevTemporalResult, true);
        temporalPass.Write(curTemporalResult, true);

        temporalPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetTex(resolveHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(raycastResultHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(maxDepthPtyramidHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(prevTemporalResult), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(curTemporalResult), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pSSRTemporalSignature.get());
            pContext->SetPipelineState(pSSRTemporalPSO.get());

            struct RootIndexBuffer
            {
                UINT perFrameBufferIndex;
                UINT screenInputIndex;
                UINT raycastInputIndex;
                UINT maxDepthBufferIndex;
                UINT previousTemporalInputIndex;
                UINT temporalResultIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetTexDefSRVIdx(resolveHandle),
                                                               RegGetTexDefSRVIdx(raycastResultHandle),
                                                               RegGetTexDefSRVIdx(maxDepthPtyramidHandle),
                                                               RegGetTexDefSRVIdx(prevTemporalResult),
                                                               RegGetTexDefUAVIdx(curTemporalResult)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(resolveResultDesc.Width, resolveResultDesc.Height, KERNEL_SIZE, KERNEL_SIZE);
        });

        passOutput.ssrOutHandle = curTemporalResult;

        passIndex = (passIndex + 1) % 2;
        */
    }

    void SSRPass::destroy()
    {



    }

    std::shared_ptr<RHI::D3D12Texture> SSRPass::getCurTemporalResult()
    {
        return p_temporalResults[passIndex];
    }

    std::shared_ptr<RHI::D3D12Texture> SSRPass::getPrevTemporalResult()
    {
        int prevIndex = (passIndex + 1) % 2;
        return p_temporalResults[prevIndex];
    }

}
