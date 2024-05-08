#pragma once

#include "volume_cloud_pass.h"
#include "pass_helper.h"
#include "platform/system/systemtime.h"
#include <string>
#include <locale>
#include <codecvt>

namespace MoYu
{
    
    void VolumeCloudPass::initialize(const PassInitInfo& init_info)
    {
        colorTexDesc = init_info.colorTexDesc;

        colorTexDesc.SetAllowUnorderedAccess(true);
        colorTexDesc.Name = "VolumeCloudOutput";

		ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        // Compute the transmittance, and store it in transmittance_texture_.
        mVolumeCloudCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/VolumetricCloudCS.hlsl", ShaderCompileOptions(L"CSMain"));
        mVolumeShadowCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/VolumetricCloudShadowCS.hlsl", ShaderCompileOptions(L"CSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(16)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();
            pVolumeCloudSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
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
            pVolumeShadowSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pVolumeCloudSignature.get());
            psoStream.CS                    = &mVolumeCloudCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pVolumeCloudPSO                 = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"VolumetricCloudPSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pVolumeShadowSignature.get());
            psoStream.CS = &mVolumeShadowCS;
            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };
            pVolumeShadowPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"VolumetricCloudShadowPSO", psoDesc);
        }

        mWeather2D = m_render_scene->m_cloud_map.m_weather2D;
        mCloud3D = m_render_scene->m_cloud_map.m_cloud3D;
        mWorley3D = m_render_scene->m_cloud_map.m_worley3D;

        mCloudConstantsBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                         RHI::RHIBufferTargetNone,
                                                         1,
                                                         MoYu::AlignUp(sizeof(VolumeCloudSpace::CloudsConsCB), 256),
                                                         L"VolumeCloudCB",
                                                         RHI::RHIBufferModeDynamic,
                                                         D3D12_RESOURCE_STATE_GENERIC_READ);

        mCloudsConsCB = VolumeCloudSpace::InitDefaultCloudsCons(g_SystemTime.GetTimeSecs(), glm::float3(1, 0, 1));

    }

    void VolumeCloudPass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        mCloudsConsCB = VolumeCloudSpace::InitDefaultCloudsCons(g_SystemTime.GetTimeSecs(), glm::float3(1, 0, 1));

        memcpy(mCloudConstantsBuffer->GetCpuVirtualAddress<VolumeCloudSpace::CloudsConsCB>(),
            &mCloudsConsCB, sizeof(VolumeCloudSpace::CloudsConsCB));

        volumeCloudShadowMapSize = glm::int2(512, 512);
        volumeCloudShadowBounds = glm::int2(32768, 32768);

        float shadow_near_plane = 0.1f;
        float shadow_far_plane = 20000.0f;

        if (mVolumeShadowmap == nullptr)
        {
            mVolumeShadowmap = RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                volumeCloudShadowMapSize.x,
                volumeCloudShadowMapSize.y,
                1,
                DXGI_FORMAT_R32_FLOAT,
                RHI::RHISurfaceCreateRandomWrite,
                1,
                L"VolumeCloudShadowmap",
                D3D12_RESOURCE_STATE_COMMON);
        }

        RenderResource* real_resource = (RenderResource*)render_resource.get();

        float sun_to_earth_distance = mCloudsConsCB.cloudsCons.PlanetRadius * 1000;

        glm::float3 m_direction = real_resource->m_FrameUniforms.directionalLight.lightDirection;
        glm::float3 m_translation = sun_to_earth_distance * m_direction;
         
        glm::float4x4 sunLightViewMat =
            MoYu::MYMatrix4x4::createLookAtMatrix(m_translation, glm::float3(0, 0, 0), MYFloat3::Up);
        glm::float4x4 sunLightProjMat = MYMatrix4x4::createOrthographic(
            volumeCloudShadowBounds.x, volumeCloudShadowBounds.y, shadow_near_plane, shadow_far_plane);

        real_resource->m_FrameUniforms.volumeCloudUniform.cloud_shadow_view_matrix = sunLightViewMat;
        real_resource->m_FrameUniforms.volumeCloudUniform.cloud_shadow_proj_matrix = sunLightProjMat;
        real_resource->m_FrameUniforms.volumeCloudUniform.cloud_shadow_proj_view_matrix = sunLightProjMat * sunLightViewMat;
        real_resource->m_FrameUniforms.volumeCloudUniform.cloud_shadowmap_srv_index = mVolumeShadowmap->GetDefaultSRV()->GetIndex();
        real_resource->m_FrameUniforms.volumeCloudUniform.cloud_shadowmap_size = volumeCloudShadowMapSize.x;
        real_resource->m_FrameUniforms.volumeCloudUniform.cloud_shadowmap_bounds = volumeCloudShadowBounds;
        real_resource->m_FrameUniforms.volumeCloudUniform.sunlight_direction = m_direction;
        real_resource->m_FrameUniforms.volumeCloudUniform.sun_to_earth_distance = sun_to_earth_distance;
    }

    void VolumeCloudPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle sceneColorHandle = passInput.renderTargetColorHandle;
        RHI::RgResourceHandle sceneDepthHandle = passInput.renderTargetDepthHandle;

        RHI::RgResourceHandle cloudConstantsBufferHandle = graph.Import<RHI::D3D12Buffer>(mCloudConstantsBuffer.get());
        RHI::RgResourceHandle mWeather2DHandle = graph.Import<RHI::D3D12Texture>(mWeather2D.get());
        RHI::RgResourceHandle mCloud3DHandle = graph.Import<RHI::D3D12Texture>(mCloud3D.get());
        RHI::RgResourceHandle mWorley3DHandle = graph.Import<RHI::D3D12Texture>(mWorley3D.get());

        RHI::RgResourceHandle mVolumeShadowmapHandle = graph.Import<RHI::D3D12Texture>(mVolumeShadowmap.get());
        
        RHI::RgResourceHandle mOutColorHandle = graph.Create<RHI::D3D12Texture>(colorTexDesc);

        // Compute the transmittance, and store it in transmittance_texture_.
        RHI::RenderPass& volumeCloudPass = graph.AddRenderPass("VolumeCloudPass");

        volumeCloudPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        volumeCloudPass.Read(cloudConstantsBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        volumeCloudPass.Read(sceneColorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        volumeCloudPass.Read(sceneDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        volumeCloudPass.Read(mWeather2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        volumeCloudPass.Read(mCloud3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        volumeCloudPass.Read(mWorley3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        volumeCloudPass.Write(mOutColorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        volumeCloudPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12Buffer* mPerframeBuffer = RegGetBuf(perframeBufferHandle);
            RHI::D3D12Buffer* mCloudConstantsBuffer = RegGetBuf(cloudConstantsBufferHandle);
            RHI::D3D12Texture* mSceneColor = RegGetTex(sceneColorHandle);
            RHI::D3D12Texture* mSceneDepth = RegGetTex(sceneDepthHandle);
            RHI::D3D12Texture* mWeather2D = RegGetTex(mWeather2DHandle);
            RHI::D3D12Texture* mCloud3D = RegGetTex(mCloud3DHandle);
            RHI::D3D12Texture* mWorley3D = RegGetTex(mWorley3DHandle);
            RHI::D3D12Texture* mOutColor = RegGetTex(mOutColorHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView* perframeBufferCBV = mPerframeBuffer->GetDefaultCBV().get();
            RHI::D3D12ConstantBufferView* cloudConsBufferCBV = mCloudConstantsBuffer->GetDefaultCBV().get();
            RHI::D3D12ShaderResourceView* sceneColorSRV = mSceneColor->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* sceneDepthSRV = mSceneDepth->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* weather2DSRV = mWeather2D->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* cloud3DSRV = mCloud3D->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* worly3DSRV = mWorley3D->GetDefaultSRV().get();
            RHI::D3D12UnorderedAccessView* outColorUAV = mOutColor->GetDefaultUAV().get();

            struct
            {
                int perFrameBufferIndex;
                int sceneColorIndex;
                int sceneDepthIndex;
                int weatherTexIndex;
                int cloudTexIndex; // 3d
                int worlyTexIndex; // 3d
                int cloudConstBufferIndex; // constant buffer
                int outColorIndex;
            } transmittanceCB = {
                    perframeBufferCBV->GetIndex(),
                    sceneColorSRV->GetIndex(),
                    sceneDepthSRV->GetIndex(),
                    weather2DSRV->GetIndex(),
                    cloud3DSRV->GetIndex(),
                    worly3DSRV->GetIndex(),
                    cloudConsBufferCBV->GetIndex(),
                    outColorUAV->GetIndex()
                };

            pContext->SetRootSignature(pVolumeCloudSignature.get());
            pContext->SetPipelineState(pVolumeCloudPSO.get());
            pContext->SetConstantArray(0, sizeof(transmittanceCB) / sizeof(uint32_t), &transmittanceCB);

            uint32_t dispatchWidth = colorTexDesc.Width;
            uint32_t dispatchHeight = colorTexDesc.Height;

            pContext->Dispatch((dispatchWidth + 8 - 1) / 8, (dispatchHeight + 8 - 1) / 8, 1);
        });

        RHI::RenderPass& volumeCloudShadowPass = graph.AddRenderPass("VolumeCloudShadowPass");

        volumeCloudShadowPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        volumeCloudShadowPass.Read(cloudConstantsBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        volumeCloudShadowPass.Read(mWeather2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        volumeCloudShadowPass.Read(mCloud3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        volumeCloudShadowPass.Read(mWorley3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        volumeCloudShadowPass.Write(mVolumeShadowmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        volumeCloudShadowPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12Buffer* mPerframeBuffer = RegGetBuf(perframeBufferHandle);
            RHI::D3D12Buffer* mCloudConstantsBuffer = RegGetBuf(cloudConstantsBufferHandle);
            RHI::D3D12Texture* mWeather2D = RegGetTex(mWeather2DHandle);
            RHI::D3D12Texture* mCloud3D = RegGetTex(mCloud3DHandle);
            RHI::D3D12Texture* mWorley3D = RegGetTex(mWorley3DHandle);
            RHI::D3D12Texture* mVolumeShadowmap = RegGetTex(mVolumeShadowmapHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView* perframeBufferCBV = mPerframeBuffer->GetDefaultCBV().get();
            RHI::D3D12ConstantBufferView* cloudConsBufferCBV = mCloudConstantsBuffer->GetDefaultCBV().get();
            RHI::D3D12ShaderResourceView* weather2DSRV = mWeather2D->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* cloud3DSRV = mCloud3D->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* worly3DSRV = mWorley3D->GetDefaultSRV().get();
            RHI::D3D12UnorderedAccessView* mVolumeShadowmapUAV = mVolumeShadowmap->GetDefaultUAV().get();

            struct
            {
                int perFrameBufferIndex;
                int weatherTexIndex;
                int cloudTexIndex; // 3d
                int worlyTexIndex; // 3d
                int cloudConstBufferIndex; // constant buffer
                int outShadowIndex;
            } transmittanceCB = {
                    perframeBufferCBV->GetIndex(),
                    weather2DSRV->GetIndex(),
                    cloud3DSRV->GetIndex(),
                    worly3DSRV->GetIndex(),
                    cloudConsBufferCBV->GetIndex(),
                    mVolumeShadowmapUAV->GetIndex()
            };

            pContext->SetRootSignature(pVolumeShadowSignature.get());
            pContext->SetPipelineState(pVolumeShadowPSO.get());
            pContext->SetConstantArray(0, sizeof(transmittanceCB) / sizeof(uint32_t), &transmittanceCB);

            uint32_t dispatchWidth = volumeCloudShadowMapSize.x;
            uint32_t dispatchHeight = volumeCloudShadowMapSize.y;

            pContext->Dispatch((dispatchWidth + 8 - 1) / 8, (dispatchHeight + 8 - 1) / 8, 1);
        });
         
        passOutput.outColorHandle = mOutColorHandle;
        passOutput.outCloudShadowHandle = mVolumeShadowmapHandle;
    }
    
    void VolumeCloudPass::destroy()
    {
        mCloudConstantsBuffer = nullptr;
        mWeather2D = nullptr;
        mCloud3D = nullptr;
        mWorley3D = nullptr;
        mVolumeShadowmap = nullptr;
    }

}