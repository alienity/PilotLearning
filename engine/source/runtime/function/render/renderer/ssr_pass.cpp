#include "runtime/function/render/renderer/ssr_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/jitter_helper.h"
#include "runtime/function/render/render_helper.h"

#include <cassert>

namespace MoYu
{
#define GImport(g, b) g.Import(b)
#define RegGetBuf(h) registry->GetD3D12Buffer(h)
#define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
#define RegGetTex(h) registry->GetD3D12Texture(h)
#define RegGetBufDefCBVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultCBV()->GetIndex()
#define RegGetBufDefSRVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultSRV()->GetIndex()
#define RegGetBufDefUAVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultUAV()->GetIndex()
#define RegGetBufCounterSRVIdx(h) registry->GetD3D12Buffer(h)->GetCounterBuffer()->GetDefaultSRV()->GetIndex()
#define RegGetTexDefSRVIdx(h) registry->GetD3D12Texture(h)->GetDefaultSRV()->GetIndex()
#define RegGetTexDefUAVIdx(h) registry->GetD3D12Texture(h)->GetDefaultUAV()->GetIndex()

#define MAX_MIN_Z_LEVELS 7
#define KERNEL_SIZE 16


	void SSRPass::initialize(const SSRInitInfo& init_info)
	{
        passIndex = 0;

        colorTexDesc = init_info.m_ColorTexDesc;

        raycastResultDesc =
            RHI::RgTextureDesc("RaycastResultMap")
                .SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT)
                .SetExtent(colorTexDesc.Width, colorTexDesc.Height)
                .SetSampleCount(colorTexDesc.SampleCount)
                .SetAllowUnorderedAccess(true)
                .SetClearValue(RHI::RgClearValue(0.0f, 0.0f, 0.0f, 0.0f, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT));

        raycastMaskDesc =
            RHI::RgTextureDesc("RaycastMaskMap")
                .SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT)
                .SetExtent(colorTexDesc.Width, colorTexDesc.Height)
                .SetSampleCount(colorTexDesc.SampleCount)
                .SetAllowUnorderedAccess(true)
                .SetClearValue(RHI::RgClearValue(0.0f, 0.0f, 0.0f, 0.0f, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT));

        resolveResultDesc =
            RHI::RgTextureDesc("ResolveMap")
                .SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT)
                .SetExtent(colorTexDesc.Width, colorTexDesc.Height)
                .SetSampleCount(colorTexDesc.SampleCount)
                .SetAllowUnorderedAccess(true)
                .SetClearValue(RHI::RgClearValue(0.0f, 0.0f, 0.0f, 0.0f, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT));

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        {
            SSRRaycastCS =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                m_ShaderRootPath / "hlsl/SSRRaycastCS.hlsl",
                                                ShaderCompileOptions(L"CSRaycast"));

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

            pSSRRaycastSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSRRaycastSignature.get());
            psoStream.CS                    = &SSRRaycastCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSRRaycastPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SSRRaycastPSO", psoDesc);
            
        }

        {
            SSRResolveCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                           m_ShaderRootPath / "hlsl/SSRResolveCS.hlsl",
                                                           ShaderCompileOptions(L"CSResolve"));

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

            pSSRResolveSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSRResolveSignature.get());
            psoStream.CS                    = &SSRResolveCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSRResolvePSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SSRResolvePSO", psoDesc);
        }

        {
            SSRTemporalCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                           m_ShaderRootPath / "hlsl/SSRTemporalCS.hlsl",
                                                           ShaderCompileOptions(L"CSTemporal"));

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

            pSSRTemporalSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pSSRTemporalSignature.get());
            psoStream.CS                    = &SSRTemporalCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pSSRTemporalPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SSRTemporalPSO", psoDesc);
        }

    }

    void SSRPass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        m_bluenoise = m_render_scene->m_bluenoise_map.m_bluenoise_64x64_uni;

        HLSL::FrameUniforms* _frameUniforms = &(render_resource->m_FrameUniforms);

        glm::float2 costMapSize = glm::float2(colorTexDesc.Width, colorTexDesc.Height);
        glm::float2 raycastSize = glm::float2(colorTexDesc.Width, colorTexDesc.Height);
        glm::float2 resolveSize = glm::float2(colorTexDesc.Width, colorTexDesc.Height);

        glm::float2 jitterSample = GenerateRandomOffset();

        std::shared_ptr<RHI::D3D12Texture> pBlueNoiseUniMap = m_render_scene->m_bluenoise_map.m_bluenoise_64x64_uni;
        float noiseWidth = pBlueNoiseUniMap->GetWidth();
        float noiseHeight = pBlueNoiseUniMap->GetHeight();

        const int kMaxLods = 8;
        int lodCount = glm::log2((float)glm::min(colorTexDesc.Width, colorTexDesc.Height));
        lodCount = glm::min(lodCount, kMaxLods);

        HLSL::SSRUniform ssrUniform = {};

        ssrUniform.ScreenSize = glm::float4(colorTexDesc.Width, colorTexDesc.Height, 1.0f / colorTexDesc.Width, 1.0f / colorTexDesc.Height);
        ssrUniform.RayCastSize = glm::float4(raycastSize.x, raycastSize.y, 1.0f / raycastSize.x, 1.0f / raycastSize.y);
        ssrUniform.ResolveSize = glm::float4(resolveSize.x, resolveSize.y, 1.0f / resolveSize.x, 1.0f / resolveSize.y);
        ssrUniform.JitterSizeAndOffset = glm::float4((float)colorTexDesc.Width / (float)noiseWidth,
                                                     (float)colorTexDesc.Height / (float)noiseHeight,
                                                     jitterSample.x,
                                                     jitterSample.y);
        ssrUniform.NoiseSize       = glm::float4(noiseWidth, noiseHeight, 1.0f / noiseWidth, 1.0f / noiseHeight);
        ssrUniform.SmoothnessRange = 1.0f;
        ssrUniform.BRDFBias        = 0.7f;
        ssrUniform.TResponseMin    = 0.85f;
        ssrUniform.TResponseMax    = 1.0f;
        ssrUniform.EdgeFactor      = 0.25f;
        ssrUniform.Thickness       = 0.001f;
        ssrUniform.NumSteps        = 60;
        ssrUniform.MaxMipMap       = lodCount;

        // SSR Uniform
        _frameUniforms->ssrUniform = ssrUniform;


        if (p_temporalResults[0] == nullptr)
        {
            for (int i = 0; i < 2; i++)
            {
                p_temporalResults[i] =
                    RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                colorTexDesc.Width,
                                                colorTexDesc.Height,
                                                1,
                                                colorTexDesc.Format,
                                                RHI::RHISurfaceCreateRandomWrite,
                                                1,
                                                fmt::format(L"SSRTemporalResult_{}", i),
                                                D3D12_RESOURCE_STATE_COMMON,
                                                std::nullopt);
            }
        }
    }

    void SSRPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
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
