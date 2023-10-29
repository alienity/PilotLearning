#include "runtime/function/render/renderer/mip_generate_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include "runtime/function/render/renderer/renderer_config.h"

#include <cassert>

namespace MoYu
{
    std::shared_ptr<RHI::D3D12PipelineState> CreatePSO(RHI::D3D12RootSignature* rootSig, Shader& cs, RHI::D3D12Device* device, std::wstring name)
    {
        struct PsoStream
        {
            PipelineStateStreamRootSignature RootSignature;
            PipelineStateStreamCS            CS;
        } psoStream;
        psoStream.RootSignature         = PipelineStateStreamRootSignature(rootSig);
        psoStream.CS                    = &cs;
        PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

        return std::make_shared<RHI::D3D12PipelineState>(device, name, psoDesc);
    }

	void MipGeneratePass::initialize(const DrawInitInfo& init_info)
	{
        //depthMinPyramidDesc = init_info.colorTexDesc;
        //depthMinPyramidDesc.SetAllowUnorderedAccess(true);
        //depthMinPyramidDesc.SetMipLevels(8);
        //depthMaxPyramidDesc = depthMinPyramidDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        #define GetCompiledShader(ShaderName) m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,m_ShaderRootPath / ShaderName, ShaderCompileOptions(L"main"));

        GenerateMipsLinearCS = GetCompiledShader("hlsl/GenerateMipsLinearCS.hlsl")
        GenerateMipsLinearOddCS = GetCompiledShader("hlsl/GenerateMipsLinearOddCS.hlsl")
        GenerateMipsLinearOddXCS = GetCompiledShader("hlsl/GenerateMipsLinearOddXCS.hlsl")
        GenerateMipsLinearOddYCS = GetCompiledShader("hlsl/GenerateMipsLinearOddYCS.hlsl")

        GenerateMaxMipsLinearCS = GetCompiledShader("hlsl/GenerateMaxMipsLinearCS.hlsl")
        GenerateMaxMipsLinearOddCS = GetCompiledShader("hlsl/GenerateMaxMipsLinearOddCS.hlsl")
        GenerateMaxMipsLinearOddXCS = GetCompiledShader("hlsl/GenerateMaxMipsLinearOddXCS.hlsl")
        GenerateMaxMipsLinearOddYCS = GetCompiledShader("hlsl/GenerateMaxMipsLinearOddYCS.hlsl")

        GenerateMinMipsLinearCS = GetCompiledShader("hlsl/GenerateMinMipsLinearCS.hlsl")
        GenerateMinMipsLinearOddCS = GetCompiledShader("hlsl/GenerateMinMipsLinearOddCS.hlsl")
        GenerateMinMipsLinearOddXCS = GetCompiledShader("hlsl/GenerateMinMipsLinearOddXCS.hlsl")
        GenerateMinMipsLinearOddYCS = GetCompiledShader("hlsl/GenerateMinMipsLinearOddYCS.hlsl")

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pGenerateMipsLinearSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            pGenerateMipsLinearPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMipsLinearCS, m_Device, L"GenerateMipsLinearPSO");
            pGenerateMipsLinearOddPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMipsLinearOddCS, m_Device, L"GenerateMipsLinearOddPSO");
            pGenerateMipsLinearOddXPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMipsLinearOddXCS, m_Device, L"GenerateMipsLinearOddXPSO");
            pGenerateMipsLinearOddYPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMipsLinearOddYCS, m_Device, L"GenerateMipsLinearOddYPSO");

            pGenerateMaxMipsLinearPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMaxMipsLinearCS, m_Device, L"GenerateMaxMipsLinearPSO");
            pGenerateMaxMipsLinearOddPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMaxMipsLinearOddCS, m_Device, L"GenerateMaxMipsLinearOddPSO");
            pGenerateMaxMipsLinearOddXPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMaxMipsLinearOddXCS, m_Device, L"GenerateMaxMipsLinearOddXPSO");
            pGenerateMaxMipsLinearOddYPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMaxMipsLinearOddYCS, m_Device, L"GenerateMaxMipsLinearOddYPSO");

            pGenerateMinMipsLinearPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMinMipsLinearCS, m_Device, L"GenerateMinMipsLinearPSO");
            pGenerateMinMipsLinearOddPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMinMipsLinearOddCS, m_Device, L"GenerateMinMipsLinearOddPSO");
            pGenerateMinMipsLinearOddXPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMinMipsLinearOddXCS, m_Device, L"GenerateMinMipsLinearOddXPSO");
            pGenerateMinMipsLinearOddYPSO = CreatePSO(pGenerateMipsLinearSignature.get(), GenerateMinMipsLinearOddYCS, m_Device, L"GenerateMinMipsLinearOddYPSO");
        }
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

            pContext->SetRootSignature(pGenerateMipsLinearSignature.get());
            if (_MipGenMode == MipGenerateMode::Average)
            {
                pContext->SetPipelineState(pGenerateMipsLinearPSO.get());
            }
            else if (_MipGenMode == MipGenerateMode::Max)
            {
                pContext->SetPipelineState(pGenerateMaxMipsLinearPSO.get());
            }
            else if (_MipGenMode == MipGenerateMode::Min)
            {
                pContext->SetPipelineState(pGenerateMinMipsLinearPSO.get());
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
        pGenerateMipsLinearSignature = nullptr;

        pGenerateMipsLinearPSO = nullptr;
        pGenerateMipsLinearOddPSO = nullptr;
        pGenerateMipsLinearOddXPSO = nullptr;
        pGenerateMipsLinearOddYPSO = nullptr;

        pGenerateMaxMipsLinearPSO = nullptr;
        pGenerateMaxMipsLinearOddPSO = nullptr;
        pGenerateMaxMipsLinearOddXPSO = nullptr;
        pGenerateMaxMipsLinearOddYPSO = nullptr;

        pGenerateMinMipsLinearPSO = nullptr;
        pGenerateMinMipsLinearOddPSO = nullptr;
        pGenerateMinMipsLinearOddXPSO = nullptr;
        pGenerateMinMipsLinearOddYPSO = nullptr;

    }
}
