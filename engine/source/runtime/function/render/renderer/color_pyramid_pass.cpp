#include "runtime/function/render/renderer/color_pyramid_pass.h"
#include "runtime/function/render/renderer/pass_helper.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include <cassert>

namespace MoYu
{

    #define RegGetTex(h) registry->GetD3D12Texture(h)

	void ColorPyramidPass::initialize(const DrawPassInitInfo& init_info)
	{
        albedoTexDesc = init_info.albedoTexDesc;
        
        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;
	}

    void ColorPyramidPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {

    }

    void ColorPyramidPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle colorHandle = passInput.colorHandle;
        RHI::RgResourceHandle colorPyramidHandle = passOutput.colorPyramidHandle;

        RHI::RenderPass& colorPyramidPass = graph.AddRenderPass("ColorPramidPass");

        colorPyramidPass.Read(colorHandle, true);

        colorPyramidPass.Write(colorHandle, true);
        colorPyramidPass.Write(colorPyramidHandle, true);

        colorPyramidPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            {
                pContext->TransitionBarrier(RegGetTex(colorHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
                pContext->TransitionBarrier(RegGetTex(colorPyramidHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                pContext->FlushResourceBarriers();

                const CD3DX12_TEXTURE_COPY_LOCATION src(RegGetTex(colorHandle)->GetResource(), 0);
                const CD3DX12_TEXTURE_COPY_LOCATION dst(RegGetTex(colorPyramidHandle)->GetResource(), 0);
                context->GetGraphicsCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

                context->FlushResourceBarriers();
            }

            //--------------------------------------------------
            // Éú³ÉColorPyramid
            //--------------------------------------------------
            {
                RHI::D3D12Texture* _SrcTexture = RegGetTex(colorPyramidHandle);
                generateColorPyramid(pContext, _SrcTexture);
            }
        });

        passOutput.colorHandle = colorHandle;
        passOutput.colorPyramidHandle = colorPyramidHandle;
    }


    void ColorPyramidPass::destroy()
    {
        
    }

    bool ColorPyramidPass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        
        return needClearRenderTarget;
    }

    void ColorPyramidPass::generateColorPyramid(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture)
    {
        int numSubresource = srcTexture->GetNumSubresources();
        int iterCount = glm::ceil(numSubresource / 4.0f);
        for (int i = 0; i < iterCount; i++)
        {
            generateColorPyramid(context, srcTexture, 4 * i);
        }
    }

    void ColorPyramidPass::generateColorPyramid(RHI::D3D12ComputeContext* pContext, RHI::D3D12Texture* srcTexture, int srcIndex)
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
        pContext->SetPipelineState(PipelineStates::pGenerateMipsLinearPSO.get());

        pContext->SetConstantArray(0, sizeof(MipGenInBuffer) / sizeof(int), &maxMipGenBuffer);

        pContext->Dispatch2D(_width, _height, 8, 8);
    }

}
