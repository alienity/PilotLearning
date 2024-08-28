#include "runtime/function/render/renderer/diffuseFilter.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include <cassert>

namespace MoYu
{
    DiffuseFilter::ShaderSignaturePSO CreateSpecificPSO(
        ShaderCompiler* InShaderCompiler, std::filesystem::path InShaderRootPath, 
        RHI::D3D12Device* InDevice, ShaderCompileOptions InSCO, std::wstring InName)
    {
        DiffuseFilter::ShaderSignaturePSO outSSPSO;

        outSSPSO.m_Kernal = InShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, InShaderRootPath / "pipeline/Runtime/Tools/Denoising/DiffuseDenoiserCS.hlsl", InSCO);

        RHI::RootSignatureDesc rootSigDesc =
            RHI::RootSignatureDesc()
            .Add32BitConstants<0, 0>(16)
            .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1)
            .AllowResourceDescriptorHeapIndexing()
            .AllowSampleDescriptorHeapIndexing();
        outSSPSO.pKernalSignature = std::make_shared<RHI::D3D12RootSignature>(InDevice, rootSigDesc);

        struct PsoStream
        {
            PipelineStateStreamRootSignature RootSignature;
            PipelineStateStreamCS            CS;
        } psoStream;
        psoStream.RootSignature = PipelineStateStreamRootSignature(outSSPSO.pKernalSignature.get());
        psoStream.CS = &outSSPSO.m_Kernal;
        PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

        outSSPSO.pKernalPSO = std::make_shared<RHI::D3D12PipelineState>(InDevice, InName, psoDesc);

        return outSSPSO;
    }
    
    void DiffuseFilter::Init(DiffuseFilterInitInfo InitInfo)
	{
        colorTexDesc = InitInfo.m_ColorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = InitInfo.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = InitInfo.m_ShaderRootPath;

        m_Device = InitInfo.m_Device;
        if (mGeneratePointDistribution.pKernalPSO == nullptr)
        {
            ShaderCompileOptions mShaderCompileOptions(L"GeneratePointDistribution");
            mShaderCompileOptions.SetDefine(L"GENERATE_POINT_DISTRIBUTION", L"1");
            mGeneratePointDistribution = CreateSpecificPSO(m_ShaderCompiler, m_ShaderRootPath, m_Device, mShaderCompileOptions, L"GeneratePointDistributionPSO");
        }
        if (mBilateralFilter.pKernalPSO == nullptr)
        {
            ShaderCompileOptions mShaderCompileOptions(L"BilateralFilter");
            mShaderCompileOptions.SetDefine(L"BILATERAL_FILTER", L"1");
            mShaderCompileOptions.SetDefine(L"SINGLE_CHANNEL", L"0");
            mBilateralFilter = CreateSpecificPSO(m_ShaderCompiler, m_ShaderRootPath, m_Device, mShaderCompileOptions, L"BilateralFilterPSO");
        }
    }

    void DiffuseFilter::PrepareUniforms(RenderScene* render_scene, RenderCamera* camera)
    {
        if (mBilateralFilterParameterBuffer == nullptr)
        {
            mBilateralFilterParameterBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetNone,
                    1,
                    MoYu::AlignUp(sizeof(BilateralFilterParameter), 256),
                    L"BilateralFilterParameter",
                    RHI::RHIBufferModeDynamic,
                    D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        mBilateralFilterParameter._DenoiserResolutionMultiplierVals = glm::float4(1, 1, 1, 1);
        mBilateralFilterParameter._DenoiserFilterRadius = 0.5f;
        mBilateralFilterParameter._PixelSpreadAngleTangent = 
            glm::tan(glm::radians(camera->m_fieldOfViewY * 0.5f)) * 2.0f / glm::min(camera->m_pixelWidth, camera->m_pixelHeight);
        mBilateralFilterParameter._JitterFramePeriod = m_Device->GetLinkedDevice()->m_FrameIndex % 4;
        mBilateralFilterParameter._HalfResolutionFilter = 0;

        memcpy(mBilateralFilterParameterBuffer->GetCpuVirtualAddress(), &mBilateralFilterParameter, sizeof(mBilateralFilterParameter));

        if (pointDistributionStructureBuffer == nullptr)
        {
            pointDistributionStructureBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetNone,
                    m_PointDistributionSize,
                    MoYu::AlignUp(sizeof(glm::float2), 256),
                    L"PointDistributionStructure",
                    RHI::RHIBufferModeImmutable,
                    D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        owenScrambled256Tex = render_scene->m_bluenoise_map.m_owenScrambled256Tex;
    }

    void DiffuseFilter::GeneratePointDistribution(RHI::RenderGraph& graph, GeneratePointDistributionData& passData)
    {
        RHI::RgResourceHandle pointDistributionRWHandle = graph.Import<RHI::D3D12Buffer>(pointDistributionStructureBuffer.get());
        
        if (m_DenoiserInitialized)
        {
            m_DenoiserInitialized = true;

            RHI::RenderPass& generatePointDistributionPass = graph.AddRenderPass("GeneratePointDistribution");

            generatePointDistributionPass.Write(pointDistributionRWHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

            generatePointDistributionPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                pContext->SetRootSignature(mGeneratePointDistribution.pKernalSignature.get());
                pContext->SetPipelineState(mGeneratePointDistribution.pKernalPSO.get());

                struct RootIndexBuffer
                {
                    uint32_t pointDistributionRWIndex;
                };

                RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetTexDefUAVIdx(pointDistributionRWHandle) };

                pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

                pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);

            });
        }

        passData.pointDistributionRWHandle = pointDistributionRWHandle;
    }

    void DiffuseFilter::BilateralFilter(RHI::RenderGraph& graph, BilateralFilterData& passData, GeneratePointDistributionData& distributeData)
    {
        RHI::RgResourceHandle mBilateralFilterParameterHandle = graph.Import<RHI::D3D12Buffer>(mBilateralFilterParameterBuffer.get());
        RHI::RgResourceHandle owenScrambled256TexHandle = graph.Import<RHI::D3D12Texture>(owenScrambled256Tex.get());

        RHI::RgResourceHandle pointDistributionHandle = distributeData.pointDistributionRWHandle;

        RHI::RgResourceHandle perframeBufferHandle = passData.perFrameBufferHandle;
        RHI::RgResourceHandle depthTextureHandle = passData.depthBufferHandle;
        RHI::RgResourceHandle normalBufferHandle = passData.normalBufferHandle;
        RHI::RgResourceHandle noisyBufferHandle = passData.noisyBufferHandle;
        RHI::RgResourceHandle outputBufferHandle = passData.outputBufferHandle;

        RHI::RenderPass& bilateralFilterPass = graph.AddRenderPass("BilateralFilter");

        bilateralFilterPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        bilateralFilterPass.Read(mBilateralFilterParameterHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        bilateralFilterPass.Read(pointDistributionHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        bilateralFilterPass.Read(depthTextureHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        bilateralFilterPass.Read(normalBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        bilateralFilterPass.Read(owenScrambled256TexHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        bilateralFilterPass.Read(noisyBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        bilateralFilterPass.Write(outputBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        bilateralFilterPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            auto CreateHandleIndexFunc = [&registry](RHI::RgResourceHandle InHandle) {
                RHI::D3D12Texture* TempTex = registry->GetD3D12Texture(InHandle);
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
                srvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.PlaneSlice = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                std::shared_ptr<RHI::D3D12ShaderResourceView> InTexSRV = TempTex->CreateSRV(srvDesc);
                UINT InTexSRVIndex = InTexSRV->GetIndex();
                return InTexSRVIndex;
            };

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->SetRootSignature(mBilateralFilter.pKernalSignature.get());
            pContext->SetPipelineState(mBilateralFilter.pKernalPSO.get());

            struct RootIndexBuffer
            {
                uint32_t perFrameBufferIndex;
                uint32_t bilateralFilterStructIndex;
                uint32_t depthTextureIndex;
                uint32_t normalTextureIndex;
                uint32_t OwenScrambledTextureIndex;
                uint32_t pointDistributionIndex;
                uint32_t denoiseInputTextureIndex;
                uint32_t denoiseOutputTextureRWIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetBufDefCBVIdx(mBilateralFilterParameterHandle),
                                                               RegGetTexDefSRVIdx(depthTextureHandle),
                                                               RegGetTexDefSRVIdx(normalBufferHandle),
                                                               CreateHandleIndexFunc(owenScrambled256TexHandle),
                                                               RegGetBufDefSRVIdx(pointDistributionHandle),
                                                               RegGetTexDefSRVIdx(noisyBufferHandle),
                                                               RegGetTexDefUAVIdx(outputBufferHandle) };

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);

        });

    }

    void DiffuseFilter::Denoise(RHI::RenderGraph& graph, BilateralFilterData& passData)
    {
        GeneratePointDistributionData pointDistributionData;
        GeneratePointDistribution(graph, pointDistributionData);
        BilateralFilter(graph, passData, pointDistributionData);
    }


}
