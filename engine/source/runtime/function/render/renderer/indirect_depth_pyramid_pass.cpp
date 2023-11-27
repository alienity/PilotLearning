#include "runtime/function/render/renderer/indirect_depth_pyramid_pass.h"
#include "runtime/function/render/renderer/pass_helper.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include <cassert>

namespace MoYu
{

    #define RegGetTex(h) registry->GetD3D12Texture(h)

	void DepthPyramidPass::initialize(const DrawPassInitInfo& init_info)
	{
        albedoTexDesc = init_info.albedoTexDesc;
        depthTexDesc  = init_info.depthTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;


	}

    void DepthPyramidPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mDepthPyramid[i].pMinDpethPyramid == nullptr)
            {
                mDepthPyramid[i].pMinDpethPyramid =
                    RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                depthTexDesc.Width,
                                                depthTexDesc.Height,
                                                8,
                                                DXGI_FORMAT_R32_FLOAT,
                                                RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                                                1,
                                                L"MinDepthPyramid",
                                                D3D12_RESOURCE_STATE_COMMON,
                                                std::nullopt);
            }
            if (mDepthPyramid[i].pMaxDpethPyramid == nullptr)
            {
                mDepthPyramid[i].pMaxDpethPyramid =
                    RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                depthTexDesc.Width,
                                                depthTexDesc.Height,
                                                8,
                                                DXGI_FORMAT_R32_FLOAT,
                                                RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                                                1,
                                                L"MaxDepthPyramid",
                                                D3D12_RESOURCE_STATE_COMMON,
                                                std::nullopt);
            }
        }
    }

    void DepthPyramidPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        //if (!isDepthMinMaxHeightReady)
        {
            //isDepthMinMaxHeightReady = true;

            std::shared_ptr<RHI::D3D12Texture> pMinDpethPyramid = GetCurrentFrameMinDepthPyramid();
            std::shared_ptr<RHI::D3D12Texture> pMaxDpethPyramid = GetCurrentFrameMaxDepthPyramid();

            RHI::RgResourceHandle minDepthPyramidHandle = GImport(graph, pMinDpethPyramid.get());
            RHI::RgResourceHandle maxDepthPyramidHandle = GImport(graph, pMaxDpethPyramid.get());

            RHI::RgResourceHandle depthHandle = passInput.depthHandle;

            RHI::RenderPass& genHeightMapMipMapPass = graph.AddRenderPass("GenerateMinMaxDepthPyramidPass");

            genHeightMapMipMapPass.Read(depthHandle, true);
            genHeightMapMipMapPass.Write(minDepthPyramidHandle, true);
            genHeightMapMipMapPass.Write(maxDepthPyramidHandle, true);

            genHeightMapMipMapPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                {
                    pContext->TransitionBarrier(RegGetTex(depthHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
                    pContext->TransitionBarrier(RegGetTex(minDepthPyramidHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->TransitionBarrier(RegGetTex(maxDepthPyramidHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->FlushResourceBarriers();

                    const CD3DX12_TEXTURE_COPY_LOCATION src(RegGetTex(depthHandle)->GetResource(), 0);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst(RegGetTex(minDepthPyramidHandle)->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst_max(RegGetTex(maxDepthPyramidHandle)->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst_max, 0, 0, 0, &src, nullptr);

                    context->FlushResourceBarriers();
                }

                //--------------------------------------------------
                // 生成MinDepthPyramid
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(minDepthPyramidHandle);
                    generateMipmapForDepthPyramid(pContext, _SrcTexture, true);
                }

                //--------------------------------------------------
                // 生成MaxDepthPyramid
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(maxDepthPyramidHandle);
                    generateMipmapForDepthPyramid(pContext, _SrcTexture, false);
                }
            });

            passOutput.minDepthPtyramidHandle = minDepthPyramidHandle;
            passOutput.maxDepthPtyramidHandle = maxDepthPyramidHandle;
        }
    }


    void DepthPyramidPass::destroy()
    {
        
    }

    bool DepthPyramidPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        
        return needClearRenderTarget;
    }

    void DepthPyramidPass::generateMipmapForDepthPyramid(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, bool genMin)
    {
        int numSubresource = srcTexture->GetNumSubresources();
        int iterCount = glm::ceil(numSubresource / 4.0f);
        for (int i = 0; i < iterCount; i++)
        {
            generateMipmapForDepthPyramid(context, srcTexture, 4 * i, genMin);
        }
    }

    void DepthPyramidPass::generateMipmapForDepthPyramid(RHI::D3D12ComputeContext* pContext, RHI::D3D12Texture* srcTexture, int srcIndex, bool genMin)
    {
        int numSubresource = srcTexture->GetNumSubresources();
        int outMipCount = glm::max(0, glm::min(numSubresource - 1 - srcIndex, 4));

        pContext->TransitionBarrier(srcTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, srcIndex);
        for (int i = 1; i < outMipCount; i++)
        {
            pContext->TransitionBarrier(srcTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, i + srcIndex);
        }
        pContext->FlushResourceBarriers();

        auto _SrcDesc = srcTexture->GetDesc();
        auto _Mip0SRVDesc = RHI::D3D12ShaderResourceView::GetDesc(srcTexture, false, srcIndex, 1);
        glm::uint _SrcIndex = srcTexture->CreateSRV(_Mip0SRVDesc)->GetIndex();

        int _width  = _SrcDesc.Width >> (srcIndex + 1);
        int _height = _SrcDesc.Height >> (srcIndex + 1);

        glm::float2 _Mip1TexelSize = {1.0f / _width, 1.0f / _height};
        glm::uint   _SrcMipLevel   = srcIndex;
        glm::uint   _NumMipLevels  = outMipCount;

        MipGenInBuffer maxMipGenBuffer = {};
        maxMipGenBuffer.SrcMipLevel   = _SrcMipLevel;
        maxMipGenBuffer.NumMipLevels  = _NumMipLevels;
        maxMipGenBuffer.TexelSize     = _Mip1TexelSize;
        maxMipGenBuffer.SrcIndex      = _SrcIndex;

        for (int i = 0; i < outMipCount; i++)
        {
            auto _Mip1UAVDesc = RHI::D3D12UnorderedAccessView::GetDesc(srcTexture, 0, srcIndex + i + 1);
            glm::uint _OutMip1Index = srcTexture->CreateUAV(_Mip1UAVDesc)->GetIndex();
            *(&maxMipGenBuffer.OutMip1Index + i) = _OutMip1Index;
        }

        pContext->SetRootSignature(RootSignatures::pGenerateMipsLinearSignature.get());
        if (genMin)
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMinMipsLinearPSO.get());
        }
        else
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMaxMipsLinearPSO.get());
        }

        pContext->SetConstantArray(0, sizeof(MipGenInBuffer) / sizeof(int), &maxMipGenBuffer);

        pContext->Dispatch2D(_width, _height, 8, 8);
    }

    std::shared_ptr<RHI::D3D12Texture> DepthPyramidPass::GetLastFrameMinDepthPyramid()
    {
        int curIndex = m_Device->GetLinkedDevice()->m_FrameIndex;
        curIndex = (curIndex + 1) % 2;
        return mDepthPyramid[curIndex].pMinDpethPyramid;
    }

    std::shared_ptr<RHI::D3D12Texture> DepthPyramidPass::GetLastFrameMaxDepthPyramid()
    {
        int curIndex = m_Device->GetLinkedDevice()->m_FrameIndex;
        curIndex = (curIndex + 1) % 2;
        return mDepthPyramid[curIndex].pMaxDpethPyramid;
    }

    std::shared_ptr<RHI::D3D12Texture> DepthPyramidPass::GetCurrentFrameMinDepthPyramid()
    {
        int curIndex = m_Device->GetLinkedDevice()->m_FrameIndex;
        return mDepthPyramid[curIndex].pMinDpethPyramid;
    }

    std::shared_ptr<RHI::D3D12Texture> DepthPyramidPass::GetCurrentFrameMaxDepthPyramid()
    {
        int curIndex = m_Device->GetLinkedDevice()->m_FrameIndex;
        return mDepthPyramid[curIndex].pMaxDpethPyramid;
    }

}
