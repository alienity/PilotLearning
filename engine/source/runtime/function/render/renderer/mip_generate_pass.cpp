#include "runtime/function/render/renderer/mip_generate_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include "runtime/function/render/renderer/renderer_config.h"

#include <cassert>

namespace MoYu
{

	void MipGeneratePass::initialize(const DrawInitInfo& init_info)
	{
        //depthMinPyramidDesc = init_info.colorTexDesc;
        //depthMinPyramidDesc.SetAllowUnorderedAccess(true);
        //depthMinPyramidDesc.SetMipLevels(8);
        //depthMaxPyramidDesc = depthMinPyramidDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

    }


    void MipGeneratePass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        MipGenerateMode _MipGenMode = passInput.mipGenMode;

        RHI::RgResourceHandle _TexHandle = passInput.texHandle;

        passOutput.outputPyramidHandle = _TexHandle;
        
        RHI::RenderPass& drawpass = graph.AddRenderPass("GenerateMipsPass");

        drawpass.Read(_TexHandle, true);
        drawpass.Write(passOutput.outputPyramidHandle, true);

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12Texture* _SrcTexture = registry->GetD3D12Texture(_TexHandle);
            
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0);
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 1);
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 2);
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 3);
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 4);

            pContext->FlushResourceBarriers();

            auto& _SrcDesc = _SrcTexture->GetDesc();

            auto _Mip0SRVDesc = RHI::D3D12ShaderResourceView::GetDesc(_SrcTexture, false, 0, 0);
            auto _Mip1UAVDesc = RHI::D3D12UnorderedAccessView::GetDesc(_SrcTexture, 0, 1);
            auto _Mip2UAVDesc = RHI::D3D12UnorderedAccessView::GetDesc(_SrcTexture, 0, 2);
            auto _Mip3UAVDesc = RHI::D3D12UnorderedAccessView::GetDesc(_SrcTexture, 0, 3);
            auto _Mip4UAVDesc = RHI::D3D12UnorderedAccessView::GetDesc(_SrcTexture, 0, 4);
            
            struct MipGenInBuffer
            {
                glm::uint   SrcMipLevel;  // Texture level of source mip
                glm::uint   NumMipLevels; // Number of OutMips to write: [1, 4]
                glm::float2 TexelSize;    // 1.0 / OutMip1.Dimensions

                glm::uint SrcIndex;
                glm::uint OutMip1Index;
                glm::uint OutMip2Index;
                glm::uint OutMip3Index;
                glm::uint OutMip4Index;
            };

            glm::float2 _Mip1TexelSize = {_SrcDesc.Width >> 2, _SrcDesc.Height >> 2};
            glm::uint _SrcMipLevel = 0;
            glm::uint _NumMipLevels = 4;

            glm::uint _SrcIndex = _SrcTexture->CreateSRV(_Mip0SRVDesc)->GetIndex();
            glm::uint _OutMip1Index = _SrcTexture->CreateUAV(_Mip1UAVDesc)->GetIndex();
            glm::uint _OutMip2Index = _SrcTexture->CreateUAV(_Mip2UAVDesc)->GetIndex();
            glm::uint _OutMip3Index = _SrcTexture->CreateUAV(_Mip3UAVDesc)->GetIndex();
            glm::uint _OutMip4Index = _SrcTexture->CreateUAV(_Mip4UAVDesc)->GetIndex();

            MipGenInBuffer _MipGenInBuffer = {_SrcMipLevel, _NumMipLevels, _Mip1TexelSize, _SrcIndex, _OutMip1Index, _OutMip2Index, _OutMip3Index, _OutMip4Index};

            pContext->SetRootSignature(RootSignatures::pGenerateMipsLinearSignature.get());
            if (_MipGenMode == MipGenerateMode::AverageType)
            {
                pContext->SetPipelineState(PipelineStates::pGenerateMipsLinearPSO.get());
            }
            else if (_MipGenMode == MipGenerateMode::MaxType)
            {
                pContext->SetPipelineState(PipelineStates::pGenerateMaxMipsLinearPSO.get());
            }
            else if (_MipGenMode == MipGenerateMode::MinType)
            {
                pContext->SetPipelineState(PipelineStates::pGenerateMinMipsLinearPSO.get());
            }

            size_t _MipBufSize = sizeof(MipGenInBuffer) / sizeof(glm::uint);

            for (size_t i = 0; i < _MipBufSize; i++)
            {
                unsigned int _bufVal = *((unsigned int*)&_MipGenInBuffer + i);
                pContext->SetConstant(0, i, _bufVal);
            }

            pContext->Dispatch2D(_SrcDesc.Width, _SrcDesc.Height, 8, 8);

            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0);
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 1);
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 2);
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 3);
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 4);

            pContext->FlushResourceBarriers();
        });

    }

    void MipGeneratePass::destroy()
    {
        
    }
}
