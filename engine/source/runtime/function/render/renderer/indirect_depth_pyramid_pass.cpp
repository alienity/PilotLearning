#include "runtime/function/render/renderer/indirect_depth_pyramid_pass.h"
#include "runtime/function/render/renderer/pass_helper.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include <cassert>

namespace MoYu
{

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
            if (mDepthPyramid[i].pAverageDpethPyramid == nullptr)
            {
                mDepthPyramid[i].pAverageDpethPyramid =
                    RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                        depthTexDesc.Width,
                        depthTexDesc.Height,
                        8,
                        DXGI_FORMAT_R32_FLOAT,
                        RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                        1,
                        L"AverageDepthPyramid",
                        D3D12_RESOURCE_STATE_COMMON,
                        std::nullopt);
            }
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

            mDepthPyramidHandle[i] = {};
        }

        passIndex = 0;
    }

    void DepthPyramidPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        {
            //std::shared_ptr<RHI::D3D12Texture> pAverageDpethPyramid = GetDepthPyramid(DepthMipGenerateMode::AverageType, true);
            std::shared_ptr<RHI::D3D12Texture> pMinDpethPyramid = GetDepthPyramid(DepthMipGenerateMode::MinType, true);
            std::shared_ptr<RHI::D3D12Texture> pMaxDpethPyramid = GetDepthPyramid(DepthMipGenerateMode::MaxType, true);

            //RHI::RgResourceHandle averageDepthPyramidHandle = GImport(graph, pAverageDpethPyramid.get());
            RHI::RgResourceHandle minDepthPyramidHandle = GImport(graph, pMinDpethPyramid.get());
            RHI::RgResourceHandle maxDepthPyramidHandle = GImport(graph, pMaxDpethPyramid.get());

            RHI::RgResourceHandle depthHandle = passInput.depthHandle;

            std::string_view passName;
            if (passIndex == 0)
                passName = std::string_view("GenerateDepthPyramidPass_0");
            else
                passName = std::string_view("GenerateDepthPyramidPass_1");

            RHI::RenderPass& genHeightMapMipMapPass = graph.AddRenderPass(passName);

            genHeightMapMipMapPass.Read(depthHandle, true);
            //genHeightMapMipMapPass.Write(averageDepthPyramidHandle, true);
            genHeightMapMipMapPass.Write(minDepthPyramidHandle, true);
            genHeightMapMipMapPass.Write(maxDepthPyramidHandle, true);

            genHeightMapMipMapPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                {
                    pContext->TransitionBarrier(RegGetTex(depthHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
                    //pContext->TransitionBarrier(RegGetTex(averageDepthPyramidHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->TransitionBarrier(RegGetTex(minDepthPyramidHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->TransitionBarrier(RegGetTex(maxDepthPyramidHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->FlushResourceBarriers();

                    const CD3DX12_TEXTURE_COPY_LOCATION src(RegGetTex(depthHandle)->GetResource(), 0);

                    //const CD3DX12_TEXTURE_COPY_LOCATION dst_avg(RegGetTex(averageDepthPyramidHandle)->GetResource(), 0);
                    //context->GetGraphicsCommandList()->CopyTextureRegion(&dst_avg, 0, 0, 0, &src, nullptr);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst_min(RegGetTex(minDepthPyramidHandle)->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst_min, 0, 0, 0, &src, nullptr);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst_max(RegGetTex(maxDepthPyramidHandle)->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst_max, 0, 0, 0, &src, nullptr);

                    context->FlushResourceBarriers();
                }

                ////--------------------------------------------------
                //// 生成AverageDepthPyramid
                ////--------------------------------------------------
                //{
                //    RHI::D3D12Texture* _SrcTexture = RegGetTex(averageDepthPyramidHandle);
                //    generateMipmapForDepthPyramid(pContext, _SrcTexture, DepthMipGenerateMode::AverageType);
                //}

                //--------------------------------------------------
                // 生成MinDepthPyramid
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(minDepthPyramidHandle);
                    generateMipmapForDepthPyramid(pContext, _SrcTexture, DepthMipGenerateMode::MinType);
                }

                //--------------------------------------------------
                // 生成MaxDepthPyramid
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(maxDepthPyramidHandle);
                    generateMipmapForDepthPyramid(pContext, _SrcTexture, DepthMipGenerateMode::MaxType);
                }
            });

            //passOutput.averageDepthPyramidHandle = averageDepthPyramidHandle;
            passOutput.minDepthPtyramidHandle = minDepthPyramidHandle;
            passOutput.maxDepthPtyramidHandle = maxDepthPyramidHandle;
        }

        passIndex += 1;
    }


    void DepthPyramidPass::destroy()
    {
        
    }

    bool DepthPyramidPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        
        return needClearRenderTarget;
    }

    void DepthPyramidPass::generateMipmapForDepthPyramid(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, DepthMipGenerateMode mode)
    {
        int numSubresource = srcTexture->GetNumSubresources();
        int iterCount = glm::ceil(numSubresource / 4.0f);
        for (int i = 0; i < iterCount; i++)
        {
            generateMipmapForDepthPyramid(context, srcTexture, 4 * i, mode);
        }
    }

    void DepthPyramidPass::generateMipmapForDepthPyramid(RHI::D3D12ComputeContext* pContext, RHI::D3D12Texture* srcTexture, int srcIndex, DepthMipGenerateMode mode)
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

        if (mode == DepthMipGenerateMode::MinType)
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMinMipsLinearPSO.get());
        }
        else if (mode == DepthMipGenerateMode::MaxType)
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMaxMipsLinearPSO.get());
        }
        else if (mode == DepthMipGenerateMode::AverageType)
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMipsLinearPSO.get());
        }

        pContext->SetConstantArray(0, sizeof(MipGenInBuffer) / sizeof(int), &maxMipGenBuffer);

        pContext->Dispatch2D(_width, _height, 8, 8);
    }

    std::shared_ptr<RHI::D3D12Texture> DepthPyramidPass::GetDepthPyramid(DepthMipGenerateMode mode, bool lastFrame)
    {
        int curIndex = m_Device->GetLinkedDevice()->m_FrameIndex;

        std::shared_ptr<RHI::D3D12Texture> depthRT = nullptr;

        if (lastFrame)
        {
            curIndex = (curIndex + 1) % 2;
        }
        switch (mode)
        {
        case MoYu::AverageType:
            depthRT = mDepthPyramid[curIndex].pAverageDpethPyramid;
            break;
        case MoYu::MaxType:
            depthRT = mDepthPyramid[curIndex].pMaxDpethPyramid;
            break;
        case MoYu::MinType:
            depthRT = mDepthPyramid[curIndex].pMinDpethPyramid;
            break;
        default:
            break;
        }

        return depthRT;
    }

    RHI::RgResourceHandle DepthPyramidPass::GetDepthPyramidHandle(RHI::RenderGraph& graph, DepthMipGenerateMode mode, bool lastFrame)
    {
        int curIndex = m_Device->GetLinkedDevice()->m_FrameIndex;

        RHI::RgResourceHandle depthHandle;
        
        if (lastFrame)
        {
            curIndex = (curIndex + 1) % 2;
        }
        switch (mode)
        {
        case MoYu::AverageType:
            if (!mDepthPyramidHandle[curIndex].pAverageDpethPyramidHandle.IsValid())
            {
                std::shared_ptr<RHI::D3D12Texture>& depthRT = mDepthPyramid[curIndex].pAverageDpethPyramid;
                mDepthPyramidHandle[curIndex].pAverageDpethPyramidHandle = graph.Import<RHI::D3D12Texture>(depthRT.get());
            }
            depthHandle = mDepthPyramidHandle[curIndex].pAverageDpethPyramidHandle;
            break;
        case MoYu::MaxType:
            if (!mDepthPyramidHandle[curIndex].pMaxDpethPyramidHandle.IsValid())
            {
                std::shared_ptr<RHI::D3D12Texture>& depthRT = mDepthPyramid[curIndex].pMaxDpethPyramid;
                mDepthPyramidHandle[curIndex].pMaxDpethPyramidHandle = graph.Import<RHI::D3D12Texture>(depthRT.get());
            }
            depthHandle = mDepthPyramidHandle[curIndex].pMaxDpethPyramidHandle;
            break;
        case MoYu::MinType:
            if (!mDepthPyramidHandle[curIndex].pMinDpethPyramidHandle.IsValid())
            {
                std::shared_ptr<RHI::D3D12Texture>& depthRT = mDepthPyramid[curIndex].pMinDpethPyramid;
                mDepthPyramidHandle[curIndex].pMinDpethPyramidHandle = graph.Import<RHI::D3D12Texture>(depthRT.get());
            }
            depthHandle = mDepthPyramidHandle[curIndex].pMinDpethPyramidHandle;
            break;
        default:
            break;
        }

        return depthHandle;
    }
}
