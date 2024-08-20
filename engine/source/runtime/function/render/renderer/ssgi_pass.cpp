#include "runtime/function/render/renderer/ssgi_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/jitter_helper.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include <cassert>

namespace MoYu
{

	void SSGIPass::initialize(const SSGIInitInfo& init_info)
	{
        passIndex = 0;

        colorTexDesc = init_info.m_ColorTexDesc;

        indirectDiffuseHitPointDesc = RHI::RgTextureDesc("IndirectDiffuseHitPointTexture")
            .SetType(RHI::RgTextureType::Texture2D)
            .SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT)
            .SetExtent(colorTexDesc.Width, colorTexDesc.Height)
            .SetAllowUnorderedAccess(true);

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        {
            SSTraceGICS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceTraceGICS.hlsl",
                ShaderCompileOptions(L"TRACE_GLOBAL_ILLUMINATION"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(16)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pSSTraceGISignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSTraceGISignature.get());
            psoStream.CS                    = &SSTraceGICS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSTraceGIPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"ScreenSpaceTraceGIPSO", psoDesc);
            
        }

        {
            SSReporjectGICS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Lighting/ScreenSpaceLighting/ScreenSpaceReprojectGICS.hlsl",
                ShaderCompileOptions(L"REPROJECT_GLOBAL_ILLUMINATION"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pSSReporjectGISignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSReporjectGISignature.get());
            psoStream.CS                    = &SSReporjectGICS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSReporjectGIPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"ScreenSpaceReprojectGIPSO", psoDesc);
        }
    }

    void SSGIPass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        m_owenScrambled256Tex = m_render_scene->m_bluenoise_map.m_owenScrambled256Tex;
        m_scramblingTile8SPP = m_render_scene->m_bluenoise_map.m_scramblingTile8SPP;
        m_rankingTile8SPP = m_render_scene->m_bluenoise_map.m_rankingTile8SPP;
        m_scramblingTex = m_render_scene->m_bluenoise_map.m_scramblingTex;

        //===================================================================================

        if (pSSGIConsBuffer == nullptr)
        {
            pSSGIConsBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                RHI::RHIBufferTargetNone,
                1,
                MoYu::AlignUp(sizeof(ScreenSpaceGIStruct), 256),
                L"SSGICB",
                RHI::RHIBufferModeDynamic,
                D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        float n = RenderPass::m_render_camera->m_nearClipPlane;
        float f = RenderPass::m_render_camera->m_farClipPlane;
        float thicknessScale = 1.0f / (1.0f + 0.01f);
        float thicknessBias = -n / (f - n) * (0.01f * thicknessScale);

        mSSGIStruct._RayMarchingSteps = EngineConfig::g_SSGIConfig.SSGIRaySteps;
        mSSGIStruct._RayMarchingThicknessScale = thicknessScale;
        mSSGIStruct._RayMarchingThicknessBias = thicknessBias;
        mSSGIStruct._RayMarchingReflectsSky = 0;

        mSSGIStruct._RayMarchingFallbackHierarchy = 0;
        mSSGIStruct._IndirectDiffuseProbeFallbackFlag = 0;
        mSSGIStruct._IndirectDiffuseProbeFallbackBias = 0;
        mSSGIStruct._SsrStencilBit = 0;

        mSSGIStruct._IndirectDiffuseFrameIndex = m_Device->GetLinkedDevice()->m_FrameIndex;;
        mSSGIStruct._ObjectMotionStencilBit = (int)EngineConfig::StencilUsage::Clear;
        mSSGIStruct._RayMarchingLowResPercentageInv = 1.0f;

        mSSGIStruct._ColorPyramidUvScaleAndLimitPrevFrame = glm::float4(colorTexDesc.Width, colorTexDesc.Height, colorTexDesc.Width, colorTexDesc.Height);

        memcpy(pSSGIConsBuffer->GetCpuVirtualAddress<ScreenSpaceGIStruct>(), &mSSGIStruct, sizeof(mSSGIStruct));
    }

    void SSGIPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        
        RHI::RgResourceHandle mSSGIConsBufferHandle = graph.Import<RHI::D3D12Buffer>(pSSGIConsBuffer.get());

        RHI::RgResourceHandle owenScrambledTextureHandle = graph.Import<RHI::D3D12Texture>(m_owenScrambled256Tex.get());
        RHI::RgResourceHandle scramblingTileXSPPHandle = graph.Import<RHI::D3D12Texture>(m_scramblingTile8SPP.get());
        RHI::RgResourceHandle rankingTileXSPPHandle = graph.Import<RHI::D3D12Texture>(m_rankingTile8SPP.get());
        
        RHI::RgResourceHandle indirectDiffuseHitPointTexhandle = graph.Create<RHI::D3D12Texture>(indirectDiffuseHitPointDesc);

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle depthPyramidHandle = passInput.depthPyramidHandle;
        RHI::RgResourceHandle normalBufferHandle = passInput.normalBufferHandle;
        
        RHI::RenderPass& ssTraceGIPass = graph.AddRenderPass("SSTraceGIPass");

        ssTraceGIPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        ssTraceGIPass.Read(depthPyramidHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssTraceGIPass.Read(normalBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ssTraceGIPass.Read(mSSGIConsBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        ssTraceGIPass.Write(indirectDiffuseHitPointTexhandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        ssTraceGIPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->SetRootSignature(pSSTraceGISignature.get());
            pContext->SetPipelineState(pSSTraceGIPSO.get());

            struct RootIndexBuffer
            {
                uint32_t frameUniformIndex;
                uint32_t depthPyramidTextureIndex;
                uint32_t normalBufferIndex;
                uint32_t screenSpaceTraceGIStructIndex;
                uint32_t indirectDiffuseHitPointTextureRWIndex;
                uint32_t _OwenScrambledTextureIndex;
                uint32_t _ScramblingTileXSPPIndex;
                uint32_t _RankingTileXSPPIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetTexDefSRVIdx(depthPyramidHandle),
                                                               RegGetTexDefSRVIdx(normalBufferHandle),
                                                               RegGetBufDefCBVIdx(mSSGIConsBufferHandle),
                                                               RegGetTexDefUAVIdx(indirectDiffuseHitPointTexhandle),
                                                               RegGetTexDefSRVIdx(owenScrambledTextureHandle),
                                                               RegGetTexDefSRVIdx(scramblingTileXSPPHandle),
                                                               RegGetTexDefSRVIdx(rankingTileXSPPHandle) };

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(indirectDiffuseHitPointDesc.Width, indirectDiffuseHitPointDesc.Height, 8, 8);
        });
        

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

    void SSGIPass::destroy()
    {



    }

}
