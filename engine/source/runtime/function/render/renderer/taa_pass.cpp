#include "runtime/function/render/renderer/taa_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/jitter_helper.h"

#include <cassert>

namespace MoYu
{
#define RegGetBuf(h) registry->GetD3D12Buffer(h)
#define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
#define RegGetTex(h) registry->GetD3D12Texture(h)
#define RegGetBufDefCBVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultCBV()->GetIndex()
#define RegGetBufDefSRVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultSRV()->GetIndex()
#define RegGetTexDefSRVIdx(h) registry->GetD3D12Texture(h)->GetDefaultSRV()->GetIndex()
#define RegGetBufDefUAVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultUAV()->GetIndex()
#define RegGetBufCounterSRVIdx(h) registry->GetD3D12Buffer(h)->GetCounterBuffer()->GetDefaultSRV()->GetIndex()

	void TAAPass::initialize(const TAAInitInfo& init_info)
	{
        reprojectionTexDesc = init_info.m_ColorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        {
            TemporalReprojectionCS =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                m_ShaderRootPath / "hlsl/TemporalReprojection.hlsl",
                                                ShaderCompileOptions(L"CSMain"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pTemporalReprojectionSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pTemporalReprojectionSignature.get());
            psoStream.CS                    = &TemporalReprojectionCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pTemporalReprojectionPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"TemporalReprojectionPSO", psoDesc);
            
        }

    }

    void TAAPass::prepareTAAMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        uint32_t frameIndex = m_Device->GetLinkedDevice()->m_FrameIndex;

        indexRead = (frameIndex + 1) % 2;
        indexWrite = frameIndex;

        MoYu::FrustumJitter* m_frustumJitter = m_render_camera->getFrustumJitter();

        glm::float4 projectionExtents =
            m_render_camera->getProjectionExtents(m_frustumJitter->activeSample.x, m_frustumJitter->activeSample.y);

        glm::float4 jitterUV = m_frustumJitter->activeSample;
        jitterUV.x /= reprojectionTexDesc.Width;
        jitterUV.y /= reprojectionTexDesc.Height;
        jitterUV.z /= reprojectionTexDesc.Width;
        jitterUV.w /= reprojectionTexDesc.Height;

        // FrameUniforms
        HLSL::FrameUniforms* _frameUniforms = &(render_resource->m_FrameUniforms);

        HLSL::TAAuniform taaUniform  = {};
        taaUniform.projectionExtents = projectionExtents;
        taaUniform.jitterUV          = jitterUV;
        taaUniform.feedbackMin       = m_frustumJitter->feedbackMin;
        taaUniform.feedbackMax       = m_frustumJitter->feedbackMax;
        taaUniform.motionScale       = m_frustumJitter->motionScale;

        // TAA Uniform
        _frameUniforms->taaUniform = taaUniform;

        if (reprojectionBuffer[0] == nullptr)
        {
            for (int i = 0; i < 2; i++)
            {
                std::wstring _name = fmt::format(L"ReprojectionTexture_{}", i);
                reprojectionBuffer[i] =
                    RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                reprojectionTexDesc.Width,
                                                reprojectionTexDesc.Height,
                                                8,
                                                reprojectionTexDesc.Format,
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
        
    }

    void TAAPass::drawCameraMotionVector(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle reprojectionBufferRead  = GImport(graph, reprojectionBuffer[indexRead].get());
        RHI::RgResourceHandle reprojectionBufferWrite = GImport(graph, reprojectionBuffer[indexWrite].get());

        RHI::RenderPass& trPass = graph.AddRenderPass("TemporalReprojectionPass");

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle motionVectorHandle   = passOutput.motionVectorHandle;
        RHI::RgResourceHandle depthHandle          = passOutput.depthHandle;

        trPass.Read(perframeBufferHandle, true); // RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        trPass.Read(motionVectorHandle, true);
        trPass.Read(depthHandle, true);
        trPass.Read(reprojectionBufferRead, true);
        trPass.Write(reprojectionBufferWrite, true); // RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS
        
        trPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(terrainPatchNodeHandle));
            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->FlushResourceBarriers();

            RHI::D3D12Texture* minHeightmap = registry->GetD3D12Texture(terrainMinHeightMapHandle);
            auto minHeightmapDesc = minHeightmap->GetDesc();

            RHI::D3D12ConstantBufferView* perframeCBVHandle = registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV().get();
            RHI::D3D12UnorderedAccessView* terrainNodeHandleUAV = registry->GetD3D12Buffer(terrainPatchNodeHandle)->GetDefaultUAV().get();
            RHI::D3D12ShaderResourceView* terrainMinHeightmapSRV = registry->GetD3D12Texture(terrainMinHeightMapHandle)->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* terrainMaxHeightmapSRV = registry->GetD3D12Texture(terrainMaxHeightMapHandle)->GetDefaultSRV().get();

            pContext->SetRootSignature(pIndirecTerrainPatchNodesGenSignature.get());
            pContext->SetPipelineState(pIndirecTerrainPatchNodesGenPSO.get());
            pContext->SetConstant(0, 0, perframeCBVHandle->GetIndex());
            pContext->SetConstant(0, 1, terrainNodeHandleUAV->GetIndex());
            pContext->SetConstant(0, 2, terrainMinHeightmapSRV->GetIndex());
            pContext->SetConstant(0, 3, terrainMaxHeightmapSRV->GetIndex());
            
            pContext->Dispatch2D(minHeightmapDesc.Width, minHeightmapDesc.Height, 8, 8);
        });

    }

    void TAAPass::destroy()
    {
        
    }

    bool TAAPass::initializeRenderTarget(RHI::RenderGraph& graph, TAAOutput* taaOutput)
    {

    }

}
