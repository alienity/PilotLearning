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

		ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        // Compute the transmittance, and store it in transmittance_texture_.
        mVolumeCloudCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                         m_ShaderRootPath / "hlsl/VolumetricCloudCS.hlsl",
                                                         ShaderCompileOptions(L"CSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(16)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();
            pVolumeCloudSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
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

        mCloudConstantsBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                         RHI::RHIBufferTargetNone,
                                                         1,
                                                         MoYu::AlignUp(sizeof(VolumeCloudSpace::CloudsConsCB), 256),
                                                         L"VolumeCloudCB",
                                                         RHI::RHIBufferModeDynamic,
                                                         D3D12_RESOURCE_STATE_GENERIC_READ);

        mCloudsConsCB = VolumeCloudSpace::InitDefaultCloudsCons(0, glm::float3(1, 0, 1));

    }

    void VolumeCloudPass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {
        mCloudsConsCB = VolumeCloudSpace::InitDefaultCloudsCons(g_SystemTime.GetTimeSecs(), glm::float3(1, 0, 1));

        memcpy(mCloudConstantsBuffer->GetCpuVirtualAddress<VolumeCloudSpace::CloudsConsCB>(),
            &mCloudsConsCB, sizeof(VolumeCloudSpace::CloudsConsCB));
    }

    void VolumeCloudPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        //RHI::RgResourceHandle cloudConstantsBufferHandle = graph.Import<RHI::D3D12Buffer>(mCloudConstantsBuffer.get());
        //RHI::RgResourceHandle mWeather2DHandle = graph.Import<RHI::D3D12Texture>(mWeather2D.get());
        //RHI::RgResourceHandle mCloud3DHandle    = graph.Import<RHI::D3D12Texture>(mCloud3D.get());
        //RHI::RgResourceHandle mWorley3DHandle    = graph.Import<RHI::D3D12Texture>(mWorley3D.get());



    }

    void VolumeCloudPass::destroy()
    {
        mWeather2D = nullptr;
        mCloud3D = nullptr;
        mWorley3D = nullptr;
    }

}