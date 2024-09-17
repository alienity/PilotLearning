#include "runtime/function/render/renderer/ssr_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/jitter_helper.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"

namespace MoYu
{
    struct SSRConsBuffer
    {
        float _SsrThicknessScale;
        float _SsrThicknessBias;
        int _SsrStencilBit;
        int _SsrIterLimit;

        float _SsrRoughnessFadeEnd;
        float _SsrRoughnessFadeRcpLength;
        float _SsrRoughnessFadeEndTimesRcpLength;
        float _SsrEdgeFadeRcpLength;

        int _SsrDepthPyramidMaxMip;
        int _SsrColorPyramidMaxMip;
        int _SsrReflectsSky;
        float _SsrAccumulationAmount;

        float _SsrPBRSpeedRejection;
        float _SsrPBRBias;
        float _SsrPRBSpeedRejectionScalerFactor;
        float _SsrPBRPad0;
    };

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

        DebugHitPointTextureDesc =
            RHI::RgTextureDesc("DebugHitPointTexture")
                .SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT)
                .SetExtent(colorTexDesc.Width, colorTexDesc.Height)
                .SetSampleCount(colorTexDesc.SampleCount)
                .SetAllowUnorderedAccess(true)
                .SetClearValue(RHI::RgClearValue(0.0f, 0.0f, 0.0f, 0.0f, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT));

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
            shaderCompileOpt.SetDefine(L"WORLD_SPEED_REJECTION", EngineConfig::g_SSRConfig.enableWorldSpeedRejection ? L"1" : L"0");
            shaderCompileOpt.SetDefine(L"SSR_SMOOTH_SPEED_REJECTION", EngineConfig::g_SSRConfig.speedSmoothReject ? L"1" : L"0");
            shaderCompileOpt.SetDefine(L"USE_SPEED_SURFACE", EngineConfig::g_SSRConfig.speedSurfaceOnly ? L"1" : L"0");
            shaderCompileOpt.SetDefine(L"USE_SPEED_TARGET", EngineConfig::g_SSRConfig.speedTargetOnly ? L"1" : L"0");

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
        m_owenScrambled256Tex = m_render_scene->m_bluenoise_map.m_owenScrambled256Tex;
        m_scramblingTile8SPP = m_render_scene->m_bluenoise_map.m_scramblingTile8SPP;
        m_rankingTile8SPP = m_render_scene->m_bluenoise_map.m_rankingTile8SPP;

        int curFrameIndex = m_Device->GetLinkedDevice()->m_FrameIndex;

        const int kMaxLods = 8;
        int lodCount = glm::log2((float)glm::min(colorTexDesc.Width, colorTexDesc.Height));
        lodCount = glm::min(lodCount, kMaxLods);

        float n = m_render_camera->m_nearClipPlane;
        float f = m_render_camera->m_farClipPlane;
        float thickness = EngineConfig::g_SSRConfig.depthBufferThickness;

        float smoothnessFadeStart = EngineConfig::g_SSRConfig.smoothnessFadeStart;
        float minSmoothness = EngineConfig::g_SSRConfig.minSmoothness;
        int colorPyramidHistoryMipCount = lodCount;
        int mipLevelCount = lodCount;

        SSRConsBuffer sb = {};
        sb._SsrThicknessScale = 1.0f / (1.0f + thickness);
        sb._SsrThicknessBias = -n / (f - n) * (thickness * sb._SsrThicknessScale);
        sb._SsrIterLimit = EngineConfig::g_SSRConfig.rayMaxIterations;
        // We disable sky reflection for transparent in case of a scenario where a transparent object seeing the sky through it is visible in the reflection.
        // As it has no depth it will appear extremely distorted (depth at infinity). This scenario happen frequently when you have transparent objects above water.
        // Note that the sky is still visible, it just takes its value from reflection probe/skybox rather than on screen.
        sb._SsrReflectsSky = EngineConfig::g_SSRConfig.reflectSky ? 1 : 0;
        sb._SsrStencilBit = (int)MoYu::StencilUsage::TraceReflectionRay;
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

        if (pSSRConstBuffer == nullptr)
        {
            pSSRConstBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                RHI::RHIBufferTargetNone,
                1,
                MoYu::AlignUp(sizeof(SSRConsBuffer), 256),
                L"SSRCB",
                RHI::RHIBufferModeDynamic,
                D3D12_RESOURCE_STATE_GENERIC_READ);
        }
        memcpy(pSSRConstBuffer->GetCpuVirtualAddress<SSRConsBuffer>(), &sb, sizeof(sb));

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

        HLSL::FrameUniforms* _frameUniforms = &(render_resource->m_FrameUniforms);

        HLSL::SSRStruct* _ssrStruct = &_frameUniforms->ssrUniform;
        
        _ssrStruct->_SSRLightingTextureIndex = pSSRLightingTexture->GetDefaultSRV()->GetIndex();
    }

    void SSRPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle colorPyramidHandle = passInput.colorPyramidHandle;
        RHI::RgResourceHandle worldNormalHandle = passInput.worldNormalHandle;
        RHI::RgResourceHandle depthTextureHandle = passInput.depthTextureHandle;
        RHI::RgResourceHandle motionVectorHandle = passInput.motionVectorHandle;

        RHI::RgResourceHandle SSRConstBufferHandle = graph.Import<RHI::D3D12Buffer>(pSSRConstBuffer.get());

        RHI::RgResourceHandle owenScrambledTextureHandle = graph.Import<RHI::D3D12Texture>(m_owenScrambled256Tex.get());
        RHI::RgResourceHandle scramblingTileXSPPHandle = graph.Import<RHI::D3D12Texture>(m_scramblingTile8SPP.get());
        RHI::RgResourceHandle rankingTileXSPPHandle = graph.Import<RHI::D3D12Texture>(m_rankingTile8SPP.get());

        RHI::RgResourceHandle ssrAccumHandle = graph.Import<RHI::D3D12Texture>(getSsrAccum().get());
        RHI::RgResourceHandle ssrPrevAccumHandle = graph.Import<RHI::D3D12Texture>(getSsrAccum(true).get());
        RHI::RgResourceHandle ssrLightingTextureHandle = graph.Import<RHI::D3D12Texture>(pSSRLightingTexture.get());

        RHI::RgResourceHandle SSRHitPointTextureHandle = graph.Create<RHI::D3D12Texture>(SSRHitPointTextureDesc);

        RHI::RenderPass& ssrTracePass = graph.AddRenderPass("ScreenSpaceReflectionsTracingPass");

        ssrTracePass.Read(perframeBufferHandle);
        ssrTracePass.Read(SSRConstBufferHandle);
        ssrTracePass.Read(worldNormalHandle);
        ssrTracePass.Read(depthTextureHandle);
        ssrTracePass.Read(owenScrambledTextureHandle);
        ssrTracePass.Read(scramblingTileXSPPHandle);
        ssrTracePass.Read(rankingTileXSPPHandle);
        ssrTracePass.Write(SSRHitPointTextureHandle, true);

        ssrTracePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetTex(SSRHitPointTextureHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
            pContext->ClearUAV(RegGetTex(SSRHitPointTextureHandle));
            
            pContext->SetRootSignature(pSSRTraceSignature.get());
            pContext->SetPipelineState(pSSRTracePSO.get());

            struct RootIndexBuffer
            {
                uint32_t frameUniformIndex;
                uint32_t ssrStructBufferIndex;
                uint32_t normalBufferIndex;
                uint32_t depthTextureIndex;
                uint32_t SsrHitPointTextureIndex;
                uint32_t OwenScrambledTextureIndex;
                uint32_t ScramblingTileXSPPIndex;
                uint32_t RankingTileXSPPIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{
                RegGetBufDefCBVIdx(perframeBufferHandle),
                RegGetBufDefCBVIdx(SSRConstBufferHandle),
                RegGetTexDefSRVIdx(worldNormalHandle),
                RegGetTexDefSRVIdx(depthTextureHandle),
                RegGetTexDefUAVIdx(SSRHitPointTextureHandle),
                CreateHandleIndexFunc(registry, owenScrambledTextureHandle),
                CreateHandleIndexFunc(registry, scramblingTileXSPPHandle),
                CreateHandleIndexFunc(registry, rankingTileXSPPHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

            pContext->Dispatch2D(SSRHitPointTextureDesc.Width, SSRHitPointTextureDesc.Height, 8, 8);
        });

        RHI::RenderPass& ssrReprojectPass = graph.AddRenderPass("ScreenSpaceReflectionsReprojectionPass");

        ssrReprojectPass.Read(perframeBufferHandle);
        ssrReprojectPass.Read(SSRConstBufferHandle);
        ssrReprojectPass.Read(colorPyramidHandle);
        ssrReprojectPass.Read(worldNormalHandle);
        ssrReprojectPass.Read(depthTextureHandle);
        ssrReprojectPass.Read(motionVectorHandle);
        ssrReprojectPass.Read(SSRHitPointTextureHandle);
        ssrReprojectPass.Write(ssrAccumHandle, true);

        ssrReprojectPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetTex(ssrAccumHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
            pContext->ClearUAV(RegGetTex(ssrAccumHandle));
            
            pContext->SetRootSignature(pSSRReprojectSignature.get());
            pContext->SetPipelineState(pSSRReprojectPSO.get());

            struct RootIndexBuffer
            {
                uint32_t frameUniformIndex;
                uint32_t ssrStructBufferIndex;
                uint32_t colorPyramidTextureIndex;
                uint32_t normalBufferIndex;
                uint32_t depthTextureIndex;
                uint32_t cameraMotionVectorsTextureIndex;
                uint32_t SsrHitPointTextureIndex;
                uint32_t SsrAccumTextureIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{
                RegGetBufDefCBVIdx(perframeBufferHandle),
                RegGetBufDefCBVIdx(SSRConstBufferHandle),
                RegGetTexDefSRVIdx(colorPyramidHandle),
                RegGetTexDefSRVIdx(worldNormalHandle),
                RegGetTexDefSRVIdx(depthTextureHandle),
                RegGetTexDefSRVIdx(motionVectorHandle),
                RegGetTexDefSRVIdx(SSRHitPointTextureHandle),
                RegGetTexDefUAVIdx(ssrAccumHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);
        });

        RHI::RenderPass& ssrAccumPass = graph.AddRenderPass("ScreenSpaceAccumulationPass");

        ssrAccumPass.Read(perframeBufferHandle);
        ssrAccumPass.Read(SSRConstBufferHandle);
        ssrAccumPass.Read(worldNormalHandle);
        ssrAccumPass.Read(depthTextureHandle);
        ssrAccumPass.Read(motionVectorHandle);
        ssrAccumPass.Read(SSRHitPointTextureHandle);
        ssrAccumPass.Read(ssrPrevAccumHandle);
        ssrAccumPass.Write(ssrAccumHandle);
        ssrAccumPass.Write(ssrLightingTextureHandle);

        ssrAccumPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->SetRootSignature(pSSRAccumulateSignature.get());
            pContext->SetPipelineState(pSSRAccumulatePSO.get());

            struct RootIndexBuffer
            {
                uint32_t frameUniformIndex;
                uint32_t ssrStructBufferIndex;
                uint32_t normalBufferIndex;
                uint32_t depthTextureIndex;
                uint32_t cameraMotionVectorsTextureIndex;
                uint32_t SsrHitPointTextureIndex;
                uint32_t SsrAccumPrevTextureIndex;
                uint32_t SsrAccumTextureRWIndex;
                uint32_t SsrLightingTextureRWIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{
                RegGetBufDefCBVIdx(perframeBufferHandle),
                RegGetBufDefCBVIdx(SSRConstBufferHandle),
                RegGetTexDefSRVIdx(worldNormalHandle),
                RegGetTexDefSRVIdx(depthTextureHandle),
                RegGetTexDefSRVIdx(motionVectorHandle),
                RegGetTexDefSRVIdx(SSRHitPointTextureHandle),
                RegGetTexDefSRVIdx(ssrPrevAccumHandle),
                RegGetTexDefUAVIdx(ssrAccumHandle),
                RegGetTexDefUAVIdx(ssrLightingTextureHandle)
            };

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);
        });

        passOutput.ssrOutHandle = ssrLightingTextureHandle;
    }

    void SSRPass::destroy()
    {





    }

    std::shared_ptr<RHI::D3D12Texture> SSRPass::getSsrAccum(bool needPrev)
    {
        int frameIndex = m_Device->GetLinkedDevice()->m_FrameIndex;
        if (needPrev)
        {
            frameIndex = (frameIndex + 1) % 2;
        }
        return pSSRAccumTexture[frameIndex];
    }

}
