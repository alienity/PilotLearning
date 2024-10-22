#pragma once

#include "subsurface_scattering_pass.h"
#include "pass_helper.h"
#include "diffusion_pfoile_settings.h"

namespace MoYu
{
    
    void SubsurfaceScatteringPass::initialize(const PassInitInfo& init_info)
    {
        cameraFilteringTexDesc = init_info.colorTexDesc;

        cameraFilteringTexDesc.SetAllowUnorderedAccess(true);
        cameraFilteringTexDesc.Name = "CameraFilteringTexture";

		ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        mSubsurfaceScatteringCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/SSS/SubsurfaceScatteringCS.hlsl", ShaderCompileOptions(L"SubsurfaceScattering"));

        mCombineLightingCS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "pipeline/Runtime/Tools/SSS/CombineLighting.hlsl", ShaderCompileOptions(L"CombineLightingMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();
            pSubsurfaceScatteringSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();
            pCombineLightingSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pSubsurfaceScatteringSignature.get());
            psoStream.CS = &mSubsurfaceScatteringCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };
            pSubsurfaceScatteringPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"SubsurfaceScatteringPSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pCombineLightingSignature.get());
            psoStream.CS = &mCombineLightingCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };
            pCombineLightingPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"CombineLightingPSO", psoDesc);
        }

    }

    void SubsurfaceScatteringPass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        RenderResource* real_resource = (RenderResource*)render_resource.get();
     
        HLSL::SSSUniform* sssUniform = &real_resource->m_FrameUniforms.sssUniform;

        sssUniform->_SssSampleBudget = 80;
        sssUniform->_DiffusionProfileCount = 1;
        sssUniform->_EnableSubsurfaceScattering = 1u;
        sssUniform->_TexturingModeFlags = DiffusionProfile::TexturingMode::PreAndPostScatter;
        sssUniform->_TransmissionFlags = DiffusionProfile::TransmissionMode::Regular;

        for (int i = 0; i < sssUniform->_DiffusionProfileCount; i++)
        {
            for (int c = 0; c < 4; ++c) // Vector4 component
            {
                sssUniform->_ShapeParamsAndMaxScatterDists[i * 4 + c] = diffusionProfileSettings.shapeParamAndMaxScatterDist;
                // To disable transmission, we simply nullify the transmissionTint
                sssUniform->_TransmissionTintsAndFresnel0[i * 4 + c] = diffusionProfileSettings.transmissionTintAndFresnel0;
                sssUniform->_WorldScalesAndFilterRadiiAndThicknessRemaps[i * 4 + c] = diffusionProfileSettings.worldScaleAndFilterRadiusAndThicknessRemap;
            }
            sssUniform->_DiffusionProfileHashTable[i * 4] = glm::uvec4(diffusionProfileSettings.profile.hash);
        }
    }

    void SubsurfaceScatteringPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle sceneDepthHandle = passInput.renderTargetDepthHandle;
        RHI::RgResourceHandle irradianceHandle = passInput.irradianceSourceHandle;
        RHI::RgResourceHandle specularSourceHandle = passInput.specularSourceHandle;
        RHI::RgResourceHandle volumeLight3DHandle = passInput.volumeLight3DHandle;
        RHI::RgResourceHandle sssBufferTexHandle = passInput.sssBufferTexHandle;

        RHI::RgResourceHandle mOutCameraFilteringHandle = graph.Create<RHI::D3D12Texture>(cameraFilteringTexDesc);

        RHI::RenderPass& subsurfaceScatteringPass = graph.AddRenderPass("SubsurfaceScattering");

        subsurfaceScatteringPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        subsurfaceScatteringPass.Read(sceneDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        subsurfaceScatteringPass.Read(irradianceHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        subsurfaceScatteringPass.Read(sssBufferTexHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        subsurfaceScatteringPass.Write(mOutCameraFilteringHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        subsurfaceScatteringPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12Buffer* mPerframeBuffer = RegGetBuf(perframeBufferHandle);
            RHI::D3D12Texture* mSceneDepth = RegGetTex(sceneDepthHandle);
            RHI::D3D12Texture* mIrradiance2D = RegGetTex(irradianceHandle);
            RHI::D3D12Texture* mSSSBuffer2D = RegGetTex(sssBufferTexHandle);
            RHI::D3D12Texture* mOutCameraFiltering = RegGetTex(mOutCameraFilteringHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView* perframeBufferCBV = mPerframeBuffer->GetDefaultCBV().get();
            RHI::D3D12ShaderResourceView* sceneDepthSRV = mSceneDepth->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* irradiance2DSRV = mIrradiance2D->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* mSSSBuffer2DSRV = mSSSBuffer2D->GetDefaultSRV().get();
            RHI::D3D12UnorderedAccessView* outCameraFilteringUAV = mOutCameraFiltering->GetDefaultUAV().get();

            struct
            {
                int perFrameBufferIndex;
                int depthTextureIndex; // Z-buffer
                int irradianceSourceIndex; // Includes transmitted light
                int sssTextureIndex; // SSSBuffer
                int outCameraFilteringTextureIndex; // Target texture
            } subsurfaceScatteringCB = {
                    perframeBufferCBV->GetIndex(),
                    sceneDepthSRV->GetIndex(),
                    irradiance2DSRV->GetIndex(),
                    mSSSBuffer2DSRV->GetIndex(),
                    outCameraFilteringUAV->GetIndex(),
            };

            pContext->SetRootSignature(pSubsurfaceScatteringSignature.get());
            pContext->SetPipelineState(pSubsurfaceScatteringPSO.get());
            pContext->SetConstantArray(0, sizeof(subsurfaceScatteringCB) / sizeof(uint32_t), &subsurfaceScatteringCB);

            uint32_t dispatchWidth = cameraFilteringTexDesc.Width;
            uint32_t dispatchHeight = cameraFilteringTexDesc.Height;

            pContext->Dispatch((dispatchWidth + 8 - 1) / 8, (dispatchHeight + 8 - 1) / 8, 1);
        });

        RHI::RenderPass& combineLightingPass = graph.AddRenderPass("CombineLighting");

        combineLightingPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        combineLightingPass.Read(sceneDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        combineLightingPass.Read(specularSourceHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        combineLightingPass.Read(volumeLight3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        combineLightingPass.Read(mOutCameraFilteringHandle, true);
        combineLightingPass.Write(mOutCameraFilteringHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        combineLightingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12Buffer* mPerframeBuffer = RegGetBuf(perframeBufferHandle);
            RHI::D3D12Texture* mSceneDepth = RegGetTex(sceneDepthHandle);
            RHI::D3D12Texture* volumeLight3D = RegGetTex(volumeLight3DHandle);
            RHI::D3D12Texture* specularSource = RegGetTex(specularSourceHandle);
            RHI::D3D12Texture* mOutCameraFiltering = RegGetTex(mOutCameraFilteringHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView* perframeBufferCBV = mPerframeBuffer->GetDefaultCBV().get();
            RHI::D3D12ShaderResourceView* sceneDepthSRV = mSceneDepth->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* volumeLight3DSRV = volumeLight3D->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* specularSourceSRV = specularSource->GetDefaultSRV().get();
            RHI::D3D12UnorderedAccessView* outCameraFilteringUAV = mOutCameraFiltering->GetDefaultUAV().get();

            struct
            {
                int perFrameBufferIndex;
                int depthTextureIndex;
                int volumeLight3DIndex;
                int specularSourceIndex;
                int outColorTextureIndex;
            } combineLightingCB = {
                    perframeBufferCBV->GetIndex(),
                    sceneDepthSRV->GetIndex(),
                    volumeLight3DSRV->GetIndex(),
                    specularSourceSRV->GetIndex(),
                    outCameraFilteringUAV->GetIndex(),
            };

            pContext->SetRootSignature(pCombineLightingSignature.get());
            pContext->SetPipelineState(pCombineLightingPSO.get());
            pContext->SetConstantArray(0, sizeof(combineLightingCB) / sizeof(uint32_t), &combineLightingCB);

            uint32_t dispatchWidth = cameraFilteringTexDesc.Width;
            uint32_t dispatchHeight = cameraFilteringTexDesc.Height;

            pContext->Dispatch((dispatchWidth + 8 - 1) / 8, (dispatchHeight + 8 - 1) / 8, 1);
        });

        passOutput.cameraFilteringTexHandle = mOutCameraFilteringHandle;

    }
     
    void SubsurfaceScatteringPass::destroy()
    {




    }

}