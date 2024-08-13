#include "runtime/function/render/renderer/taa_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/jitter_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include <cassert>

namespace MoYu
{

	void TAAPass::initialize(const TAAInitInfo& init_info)
	{
        colorDesc = init_info.m_ColorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        TemporalAntiAliasingCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/TemporalAntiAliasingShader.hlsl", ShaderCompileOptions(L"CSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                .AddStaticSampler<14, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pTemporalAntiAliasingSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pTemporalAntiAliasingSignature.get());
            psoStream.CS = &TemporalAntiAliasingCS;

            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };
            pTemporalAntiAliasingPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"TemporalAntiAliasingPSO", psoDesc);
        }

    }

    static glm::float2 TAASampleOffsets[] =
    {
        // center
        glm::float2(0.0f,  0.0f),

        // NeighbourOffsets
        glm::float2(0.0f,  1.0f),
        glm::float2(1.0f,  0.0f),
        glm::float2(-1.0f,  0.0f),
        glm::float2(0.0f, -1.0f),
        glm::float2(1.0f,  1.0f),
        glm::float2(1.0f, -1.0f),
        glm::float2(-1.0f,  1.0f),
        glm::float2(-1.0f, -1.0f)
    };

    float taaSampleWeights[9];

    void ComputeWeights(float& centralWeight, glm::float4 filterWeights[2], glm::float2 jitter)
    {
        float totalWeight = 0;
        for (int i = 0; i < 9; ++i)
        {
            float x = TAASampleOffsets[i].x + jitter.x;
            float y = TAASampleOffsets[i].y + jitter.y;
            float d = (x * x + y * y);

            taaSampleWeights[i] = glm::exp((-0.5f / (0.22f)) * d);
            totalWeight += taaSampleWeights[i];
        }

        centralWeight = taaSampleWeights[0] / totalWeight;

        for (int i = 0; i < 8; ++i)
        {
            filterWeights[(i / 4)][(i % 4)] = taaSampleWeights[i + 1] / totalWeight;
        }
    }

    void GetNeighbourOffsets(glm::float4 neighbourOffsets[4])
    {
        for (int i = 0; i < 16; ++i)
        {
            neighbourOffsets[(i / 4)][(i % 4)] = TAASampleOffsets[i / 2 + 1][i % 2];
        }
    }

    void TAAPass::prepareTAAMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        uint32_t taaFrameIndex = m_Device->GetLinkedDevice()->m_FrameIndex;

        indexRead = (taaFrameIndex + 1) % 2;
        indexWrite = taaFrameIndex;

        MoYu::FrustumJitter* m_frustumJitter = m_render_camera->getFrustumJitter();

        float jitterX = m_frustumJitter->activeSample.x;
        float jitterY = m_frustumJitter->activeSample.y;

        glm::float4 taaJitter = glm::float4(jitterX, jitterY, jitterX / colorDesc.Width, jitterY / colorDesc.Height);

        // The anti flicker becomes much more aggressive on higher values
        float temporalContrastForMaxAntiFlicker = 
            0.7f - glm::lerp(0.0f, 0.3f, glm::smoothstep(0.5f, 1.0f, EngineConfig::g_TAAConfig.taaAntiFlicker));

        const float historyContrastBlendStart = 0.51f;
        float historyContrastLerp = 
            glm::clamp((EngineConfig::g_TAAConfig.taaAntiFlicker - historyContrastBlendStart) / (1.0f - historyContrastBlendStart));

        int taaEnabled = 1;

        // FrameUniforms
        HLSL::FrameUniforms* _frameUniforms = &(render_resource->m_FrameUniforms);

        HLSL::TAAUniform taaUniform  = {};

        taaUniform._HistorySharpening = EngineConfig::g_TAAConfig.taaHistorySharpening;
        taaUniform._AntiFlickerIntensity = EngineConfig::g_TAAConfig.taaAntiFlicker;
        taaUniform._SpeedRejectionIntensity = EngineConfig::g_TAAConfig.taaMotionVectorRejection;
        taaUniform._ContrastForMaxAntiFlicker = temporalContrastForMaxAntiFlicker;

        ComputeWeights(taaUniform._CentralWeight, taaUniform._TaaFilterWeights, glm::float2(taaJitter.x, taaJitter.y));
        GetNeighbourOffsets(taaUniform._NeighbourOffsets);

        taaUniform._BaseBlendFactor = EngineConfig::g_TAAConfig.taaBaseBlendFactor;
        taaUniform._ExcludeTAABit = 0;
        taaUniform._HistoryContrastBlendLerp = historyContrastLerp;

        taaUniform._TaaFrameInfo = glm::float4(EngineConfig::g_TAAConfig.taaSharpenStrength, 0, taaFrameIndex, taaEnabled);
        taaUniform._TaaJitterStrength = taaJitter;

        taaUniform._InputSize = 
            glm::float4(colorDesc.Width, colorDesc.Height, 1.0f / colorDesc.Width, 1.0f / colorDesc.Height);

        taaUniform._TaaHistorySize = 
            glm::float4(colorDesc.Width, colorDesc.Height, 1.0f / colorDesc.Width, 1.0f / colorDesc.Height);

        float stdDev = 0.4f;
        float resScale = 1.0f;

        taaUniform._TAAUFilterRcpSigma2 = 1.0f / (stdDev * stdDev);
        taaUniform._TAAUScale = 1.0f / resScale;
        taaUniform._TAAUBoxConfidenceThresh = 0.5f / resScale;
        taaUniform._TAAURenderScale = resScale;

        // TAA Uniform
        _frameUniforms->taaUniform = taaUniform;



        if (historyTexture[0] == nullptr)
        {
            for (int i = 0; i < 2; i++)
            {
                std::wstring _name = fmt::format(L"TAAHistoryTexture_{}", i);
                historyTexture[i] =
                    RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                        colorDesc.Width,
                        colorDesc.Height,
                        8,
                        colorDesc.Format,
                        RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                        1,
                        _name,
                        D3D12_RESOURCE_STATE_COMMON,
                        std::nullopt);
            }
        }
    }

    void TAAPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle taaTextureRead = GImport(graph, historyTexture[indexRead].get());
        RHI::RgResourceHandle taaTextureWrite = GImport(graph, historyTexture[indexWrite].get());

        RHI::RenderPass& trPass = graph.AddRenderPass("TemporalAntiAliasing");

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle colorHandle          = passInput.colorBufferHandle;
        RHI::RgResourceHandle depthHandle          = passInput.depthBufferHandle;
        RHI::RgResourceHandle motionVectorHandle   = passInput.motionVectorHandle;

        RHI::RgResourceHandle aaOutputHandle = passOutput.aaOutHandle;
        
        trPass.Read(perframeBufferHandle, true); // , RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        trPass.Read(motionVectorHandle, true); // , RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        trPass.Read(colorHandle, true); // , RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        trPass.Read(depthHandle, true); // , RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        trPass.Read(taaTextureRead, true); // , RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        trPass.Write(taaTextureWrite, true); // , RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS
        trPass.Write(aaOutputHandle, true); // , RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS
        
        trPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetTex(motionVectorHandle), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(colorHandle), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(depthHandle), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(taaTextureRead), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(taaTextureWrite), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetTex(aaOutputHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pTemporalAntiAliasingSignature.get());
            pContext->SetPipelineState(pTemporalAntiAliasingPSO.get());

            struct RootIndexBuffer
            {
                UINT perFrameBufferIndex;
                UINT motionVectorBufferIndex;
                UINT mainColorBufferIndex;
                UINT depthBufferIndex;
                UINT historyReadIndex;
                UINT historyWriteIndex;
                UINT aaOutIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetTexDefSRVIdx(motionVectorHandle),
                                                               RegGetTexDefSRVIdx(colorHandle),
                                                               RegGetTexDefSRVIdx(depthHandle),
                                                               RegGetTexDefSRVIdx(taaTextureRead),
                                                               RegGetTexDefUAVIdx(taaTextureWrite),
                                                               RegGetTexDefUAVIdx(aaOutputHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(colorDesc.Width, colorDesc.Height, 8, 8);
        });

        passOutput.aaOutHandle = aaOutputHandle;
    }

    void TAAPass::destroy()
    {
        
    }

}
