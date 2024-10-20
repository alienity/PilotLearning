#include "runtime/function/render/renderer/temporalFilter.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include <cassert>

namespace MoYu
{
    TemporalFilter::ShaderSignaturePSO CreateSpecificPSO(
        ShaderCompiler* InShaderCompiler, std::filesystem::path InShaderRootPath, 
        RHI::D3D12Device* InDevice, ShaderCompileOptions InSCO, std::wstring InName)
    {
        TemporalFilter::ShaderSignaturePSO outSSPSO;

        outSSPSO.m_Kernal = InShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, InShaderRootPath / "pipeline/Runtime/Tools/Denoising/TemporalFilterCS.hlsl", InSCO);

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
    
    void TemporalFilter::Init(TemporalFilterInitInfo InitInfo)
	{
        colorTexDesc = InitInfo.m_ColorTexDesc;

        ShaderCompiler*       m_ShaderCompiler = InitInfo.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = InitInfo.m_ShaderRootPath;

        m_Device = InitInfo.m_Device;
        if (mValidateHistory.pKernalPSO == nullptr)
        {
            ShaderCompileOptions mShaderCompileOptions(L"ValidateHistory");
            mShaderCompileOptions.SetDefine(L"VALIDATEHISTORY", L"1");
            mValidateHistory = CreateSpecificPSO(m_ShaderCompiler, m_ShaderRootPath, m_Device, mShaderCompileOptions, L"ValidateHistoryPSO");
        }
        if (mTemporalAccumulationSingle.pKernalPSO == nullptr)
        {
            ShaderCompileOptions mShaderCompileOptions(L"TemporalAccumulation");
            mShaderCompileOptions.SetDefine(L"TEMPORALACCUMULATIONSINGLE", L"1");
            mShaderCompileOptions.SetDefine(L"SINGLE_CHANNEL", L"1");
            mTemporalAccumulationSingle = CreateSpecificPSO(m_ShaderCompiler, m_ShaderRootPath, m_Device, mShaderCompileOptions, L"TemporalAccumulationSinglePSO");
        }
        if (mTemporalAccumulationColor.pKernalPSO == nullptr)
        {
            ShaderCompileOptions mShaderCompileOptions(L"TemporalAccumulation");
            mShaderCompileOptions.SetDefine(L"TEMPORALACCUMULATIONSINGLE", L"1");
            mShaderCompileOptions.SetDefine(L"SINGLE_CHANNEL", L"0");
            mTemporalAccumulationColor = CreateSpecificPSO(m_ShaderCompiler, m_ShaderRootPath, m_Device, mShaderCompileOptions, L"TemporalAccumulationColorPSO");
        }
        if (mCopyHistory.pKernalPSO == nullptr)
        {
            ShaderCompileOptions mShaderCompileOptions(L"CopyHistory");
            mShaderCompileOptions.SetDefine(L"COPYHISTORY", L"1");
            mCopyHistory = CreateSpecificPSO(m_ShaderCompiler, m_ShaderRootPath, m_Device, mShaderCompileOptions, L"CopyHistoryPSO");
        }
    }

    void TemporalFilter::PrepareUniforms(RenderScene* render_scene, RenderCamera* camera)
    {
        if (mTemporalFilterStructBuffer == nullptr)
        {
            mTemporalFilterStructBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                    RHI::RHIBufferTargetNone,
                    1,
                    MoYu::AlignUp(sizeof(TemporalFilterStruct), 256),
                    L"TemporalFilterStruct",
                    RHI::RHIBufferModeDynamic,
                    D3D12_RESOURCE_STATE_GENERIC_READ);
        }


        mTemporalFilterStructData._PixelSpreadAngleTangent =
            glm::tan(glm::radians(camera->fovy() * 0.5f)) * 2.0f / glm::min(camera->getWidth(), camera->getHeight());
        mTemporalFilterStructData._HistoryValidity = 1;
        mTemporalFilterStructData._ReceiverMotionRejection = 0;
        mTemporalFilterStructData._OccluderMotionRejection = 0;
        mTemporalFilterStructData._HistorySizeAndScale = glm::float4(camera->getWidth(), camera->getHeight(), 1, 1);
        mTemporalFilterStructData._DenoiserResolutionMultiplierVals = glm::float4(1, 1, 1, 1);
        mTemporalFilterStructData._EnableExposureControl = 0;

        memcpy(mTemporalFilterStructBuffer->GetCpuVirtualAddress(), &mTemporalFilterStructData, sizeof(mTemporalFilterStructData));

        mTemporalFilterStructHandle.Invalidate();
    }

    void TemporalFilter::HistoryValidity(RHI::RenderGraph& graph, HistoryValidityPassData& passData)
    {
        RHI::RgResourceHandle perFrameUniformHandle = passData.perframeBufferHandle;
        RHI::RgResourceHandle cameraMotionVectorHandle = passData.cameraMotionVectorHandle;
        RHI::RgResourceHandle depthTextureHandle = passData.depthTextureHandle;
        RHI::RgResourceHandle historyDepthTextureHandle = passData.historyDepthTextureHandle;
        RHI::RgResourceHandle normalBufferHandle = passData.normalBufferHandle;
        RHI::RgResourceHandle historyNormalTextureHandle = passData.historyNormalTextureHandle;
        
        RHI::RgTextureDesc validationTexDesc = RHI::RgTextureDesc("ValidationBuffer");
        validationTexDesc.SetType(RHI::RgTextureType::Texture2D);
        validationTexDesc.SetExtent(colorTexDesc.Width, colorTexDesc.Height);
        validationTexDesc.SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32_UINT);
        validationTexDesc.SetAllowUnorderedAccess(true);

        RHI::RgResourceHandle validationBufferHandle = graph.Create<RHI::D3D12Texture>(validationTexDesc);

        if (!mTemporalFilterStructHandle.IsValid())
        {
            mTemporalFilterStructHandle = graph.Import<RHI::D3D12Buffer>(mTemporalFilterStructBuffer.get());
        }

        RHI::RenderPass& validatePass = graph.AddRenderPass("History Validity Evaluation");

        validatePass.Read(perFrameUniformHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        validatePass.Read(mTemporalFilterStructHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        validatePass.Read(cameraMotionVectorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        validatePass.Read(depthTextureHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        validatePass.Read(historyDepthTextureHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        validatePass.Read(normalBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        validatePass.Read(historyNormalTextureHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        validatePass.Write(validationBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        validatePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->SetRootSignature(mValidateHistory.pKernalSignature.get());
            pContext->SetPipelineState(mValidateHistory.pKernalPSO.get());

            struct RootIndexBuffer
            {
                uint32_t perFrameBufferIndex;
                uint32_t temporalFilterStructIndex;
                uint32_t cameraMotionVectorIndex; // Velocity buffer for history rejection
                uint32_t minDepthBufferIndex; // Depth buffer of the current frame
                uint32_t historyDepthTextureIndex; // Depth buffer of the previous frame
                uint32_t normalBufferIndex; // // Normal buffer of the current frame
                uint32_t historyNormalTextureIndex; // Normal buffer of the previous frame
                uint32_t validationBufferRWIndex; // Buffer that stores the result of the validation pass of the history
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(perFrameUniformHandle),
                                                               RegGetBufDefCBVIdx(mTemporalFilterStructHandle),
                                                               RegGetTexDefSRVIdx(cameraMotionVectorHandle),
                                                               RegGetTexDefSRVIdx(depthTextureHandle),
                                                               RegGetTexDefSRVIdx(historyDepthTextureHandle),
                                                               RegGetTexDefSRVIdx(normalBufferHandle),
                                                               RegGetTexDefSRVIdx(historyNormalTextureHandle),
                                                               RegGetTexDefUAVIdx(validationBufferHandle) };


            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);

        });

        passData.validationBufferHandle = validationBufferHandle;
    }

    void TemporalFilter::Denoise(RHI::RenderGraph& graph, TemporalFilterPassData& passData)
    {
        RHI::RgResourceHandle perFrameBufferHandle = passData.perFrameBufferHandle;
        RHI::RgResourceHandle validationBufferHandle = passData.validationBufferHandle;
        RHI::RgResourceHandle cameraMotionVectorHandle = passData.cameraMotionVectorHandle;
        RHI::RgResourceHandle depthTextureHandle = passData.depthTextureHandle;
        RHI::RgResourceHandle historyBufferHandle = passData.historyBufferHandle;
        RHI::RgResourceHandle denoiseInputTextureHandle = passData.denoiseInputTextureHandle;
        RHI::RgResourceHandle accumulationOutputTextureRWHandle = passData.accumulationOutputTextureRWHandle;

        if (!mTemporalFilterStructHandle.IsValid())
        {
            mTemporalFilterStructHandle = graph.Import<RHI::D3D12Buffer>(mTemporalFilterStructBuffer.get());
        }

        RHI::RenderPass& temporalDenoisePass = graph.AddRenderPass("TemporalDenoiser(Color)");

        temporalDenoisePass.Read(perFrameBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        temporalDenoisePass.Read(mTemporalFilterStructHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        temporalDenoisePass.Read(validationBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        temporalDenoisePass.Read(cameraMotionVectorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        temporalDenoisePass.Read(depthTextureHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        temporalDenoisePass.Read(historyBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        temporalDenoisePass.Read(denoiseInputTextureHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        temporalDenoisePass.Write(accumulationOutputTextureRWHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        temporalDenoisePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->SetRootSignature(mTemporalAccumulationColor.pKernalSignature.get());
            pContext->SetPipelineState(mTemporalAccumulationColor.pKernalPSO.get());

            struct RootIndexBuffer
            {
                uint32_t perFrameBufferIndex;
                uint32_t temporalFilterStructIndex;
                uint32_t validationBufferIndex; // Validation buffer that tells us if the history should be ignored for a given pixel.
                uint32_t cameraMotionVectorIndex; // Velocity buffer for history rejection
                uint32_t minDepthBufferIndex; // Depth buffer of the current frame
                uint32_t historyBufferIndex; // This buffer holds the previously accumulated signal
                uint32_t denoiseInputTextureIndex; // Noisy Input Buffer from the current frame
                uint32_t accumulationOutputTextureRWIndex; // Generic output buffer for our kernels
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer{ RegGetBufDefCBVIdx(perFrameBufferHandle),
                                                               RegGetBufDefCBVIdx(mTemporalFilterStructHandle),
                                                               RegGetTexDefSRVIdx(validationBufferHandle),
                                                               RegGetTexDefSRVIdx(cameraMotionVectorHandle),
                                                               RegGetTexDefSRVIdx(depthTextureHandle),
                                                               RegGetTexDefSRVIdx(historyBufferHandle),
                                                               RegGetTexDefSRVIdx(denoiseInputTextureHandle),
                                                               RegGetTexDefUAVIdx(accumulationOutputTextureRWHandle) };


            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch2D(colorTexDesc.Width, colorTexDesc.Height, 8, 8);

        });

        passData.accumulationOutputTextureRWHandle = accumulationOutputTextureRWHandle;
    }


}
