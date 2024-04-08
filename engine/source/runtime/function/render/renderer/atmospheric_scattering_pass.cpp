#pragma once

#include "atmospheric_scattering_pass.h"
#include "pass_helper.h"

namespace MoYu
{
    
    void AtmosphericScatteringPass::initialize(const PassInitInfo& init_info)
    {
        colorTexDesc = init_info.colorTexDesc;

		ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        // Compute the transmittance, and store it in transmittance_texture_.
        mComputeTransmittanceCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                                  m_ShaderRootPath / "hlsl/ComputeTransmittanceCS.hlsl",
                                                                  ShaderCompileOptions(L"CSMain"));

        // Compute the direct irradiance, store it in delta_irradiance_texture and,
        // depending on 'blend', either initialize irradiance_texture_ with zeros or
        // leave it unchanged (we don't want the direct irradiance in
        // irradiance_texture_, but only the irradiance from the sky).
        mComputeDirectIrrdianceCS =
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                                  m_ShaderRootPath / "hlsl/ComputeDirectIrradianceCS.hlsl",
                                                                  ShaderCompileOptions(L"CSMain"));

        // Compute the rayleigh and mie single scattering, store them in
        // delta_rayleigh_scattering_texture and delta_mie_scattering_texture, and
        // either store them or accumulate them in scattering_texture_ and
        // optional_single_mie_scattering_texture_.
        mComputeSingleScatteringCS =
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                            m_ShaderRootPath / "hlsl/ComputeSingleScatteringCS.hlsl",
                                            ShaderCompileOptions(L"CSMain"));

        // Compute the 2nd, 3rd and 4th order of scattering, in sequence.
        // Compute the scattering density, and store it in
        // delta_scattering_density_texture.
        mComputeScatteringDensityCS =
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                            m_ShaderRootPath / "hlsl/ComputeScatteringDensityCS.hlsl",
                                            ShaderCompileOptions(L"CSMain"));

        // Compute the indirect irradiance, store it in delta_irradiance_texture and
        // accumulate it in irradiance_texture_.
        mComputeIdirectIrradianceCS = 
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                            m_ShaderRootPath / "hlsl/ComputeIdirectIrradianceCS.hlsl",
                                            ShaderCompileOptions(L"CSMain"));

        // Compute the multiple scattering, store it in
        // delta_multiple_scattering_texture, and accumulate it in
        // scattering_texture_.
        mComputeMultipleScatteringCS = 
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                            m_ShaderRootPath / "hlsl/ComputeMultipleScatteringCS.hlsl",
                                            ShaderCompileOptions(L"CSMain"));

#define CommonRootSigDesc(rootSigName) \
    { \
        RHI::RootSignatureDesc rootSigDesc = \
            RHI::RootSignatureDesc() \
                .Add32BitConstants<0, 0>(16) \
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, \
                                         D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, \
                                         4) \
                .AllowInputLayout() \
                .AllowResourceDescriptorHeapIndexing() \
                .AllowSampleDescriptorHeapIndexing(); \
        rootSigName = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc); \
    }


        #define CommonPSO(psoVar, psoName, rootSig, csShader) \
    { \
        struct PsoStream \
        { \
            PipelineStateStreamRootSignature RootSignature; \
            PipelineStateStreamCS            CS; \
        } psoStream; \
        psoStream.RootSignature = PipelineStateStreamRootSignature(rootSig.get()); \
        psoStream.CS            = &csShader; \
        PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream}; \
        psoVar = std::make_shared<RHI::D3D12PipelineState>(m_Device, psoName, psoDesc); \
    }


        {
            CommonRootSigDesc(pComputeTransmittanceSignature)
            CommonRootSigDesc(pComputeDirectIrrdianceSignature)
            CommonRootSigDesc(pComputeSingleScatteringSignature)
            CommonRootSigDesc(pComputeScatteringDensitySignature)
            CommonRootSigDesc(pComputeIdirectIrradianceSignature)
            CommonRootSigDesc(pComputeMultipleScatteringSignature)
        
            CommonPSO(pComputeTransmittancePSO,
                      L"ComputeTransmittancePSO",
                      pComputeTransmittanceSignature,
                      mComputeTransmittanceCS)
            CommonPSO(pComputeDirectIrrdiancePSO,
                      L"ComputeDirectIrrdiancePSO",
                      pComputeDirectIrrdianceSignature,
                      mComputeDirectIrrdianceCS)
            CommonPSO(pComputeSingleScatteringPSO,
                      L"ComputeSingleScatteringPSO",
                      pComputeSingleScatteringSignature,
                      mComputeSingleScatteringCS)
            CommonPSO(pComputeScatteringDensityPSO,
                      L"ComputeScatteringDensityPSO",
                      pComputeScatteringDensitySignature,
                      mComputeScatteringDensityCS)
            CommonPSO(pComputeIdirectIrradiancePSO,
                      L"ComputeIdirectIrradiancePSO",
                      pComputeIdirectIrradianceSignature,
                      mComputeIdirectIrradianceCS)
            CommonPSO(pComputeMultipleScatteringPSO,
                      L"ComputeMultipleScattering",
                      pComputeMultipleScatteringSignature,
                      mComputeMultipleScatteringCS)

        }

        mTransmittance2D = RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                       AtmosphereScattering::TRANSMITTANCE_TEXTURE_WIDTH,
                                                       AtmosphereScattering::TRANSMITTANCE_TEXTURE_HEIGHT,
                                                       1,
                                                       DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                       RHI::RHISurfaceCreateRandomWrite,
                                                       1,
                                                       L"ASTransmittance2D",
                                                       D3D12_RESOURCE_STATE_COMMON);

        mScattering3D = RHI::D3D12Texture::Create3D(m_Device->GetLinkedDevice(),
                                                    AtmosphereScattering::SCATTERING_TEXTURE_WIDTH,
                                                    AtmosphereScattering::SCATTERING_TEXTURE_HEIGHT,
                                                    AtmosphereScattering::SCATTERING_TEXTURE_DEPTH,
                                                    1,
                                                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                    RHI::RHISurfaceCreateRandomWrite,
                                                    1,
                                                    L"ASScattering3D",
                                                    D3D12_RESOURCE_STATE_COMMON);

        mSingleMieScattering3D = RHI::D3D12Texture::Create3D(m_Device->GetLinkedDevice(),
                                                             AtmosphereScattering::SCATTERING_TEXTURE_WIDTH,
                                                             AtmosphereScattering::SCATTERING_TEXTURE_HEIGHT,
                                                             AtmosphereScattering::SCATTERING_TEXTURE_DEPTH,
                                                             1,
                                                             DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                             RHI::RHISurfaceCreateRandomWrite,
                                                             1,
                                                             L"ASSingleMieScattering3D",
                                                             D3D12_RESOURCE_STATE_COMMON);

        mIrradiance2D = RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                    AtmosphereScattering::IRRADIANCE_TEXTURE_WIDTH,
                                                    AtmosphereScattering::IRRADIANCE_TEXTURE_HEIGHT,
                                                    1,
                                                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                    RHI::RHISurfaceCreateRandomWrite,
                                                    1,
                                                    L"ASIrradiance2D",
                                                    D3D12_RESOURCE_STATE_COMMON);

        mAtmosphereUniformBuffer =
            RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                     RHI::RHIBufferTargetNone,
                                     1,
                                     MoYu::AlignUp(sizeof(AtmosphereScattering::AtmosphereUniform), 256),
                                     L"ASUniformCB",
                                     RHI::RHIBufferModeDynamic,
                                     D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    void AtmosphericScatteringPass::prepareMetaData(std::shared_ptr<RenderResource> render_resource)
    {

        AtmosphereScattering::AtmosphereUniform atmosphereUniform =
            AtmosphereScattering::AtmosphericScatteringGenerator::Init();

        memcpy(mAtmosphereUniformBuffer->GetCpuVirtualAddress<AtmosphereScattering::AtmosphereUniform>(),
               &atmosphereUniform,
               sizeof(AtmosphereScattering::AtmosphereUniform));


    }

    void AtmosphericScatteringPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        preCompute(graph);




    }

    void AtmosphericScatteringPass::destroy()
    {
        mTransmittance2D = nullptr;
        mScattering3D    = nullptr;
        mIrradiance2D    = nullptr;
        mSingleMieScattering3D = nullptr;

    }

    void AtmosphericScatteringPass::preCompute(RHI::RenderGraph& graph)
    {
        //if (hasPrecomputed)
        //    return;
        //hasPrecomputed = true;

        // Compute the transmittance, and store it in transmittance_texture_.
        RHI::RgResourceHandle atmosphereUniformBufferHandle = graph.Import<RHI::D3D12Buffer>(mAtmosphereUniformBuffer.get());
        RHI::RgResourceHandle transmittance2DHandle = graph.Import<RHI::D3D12Texture>(mTransmittance2D.get());

        RHI::RenderPass& transmittancePass = graph.AddRenderPass("ASComputeTransmittancePass");

        transmittancePass.Read(atmosphereUniformBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        transmittancePass.Write(transmittance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        transmittancePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12Buffer*  mASUniformBuffer        = RegGetBuf(atmosphereUniformBufferHandle);
            RHI::D3D12Texture* mTransmittance2DTexture = RegGetTex(transmittance2DHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView* asUniformCBV = mASUniformBuffer->GetDefaultCBV().get();
            RHI::D3D12UnorderedAccessView* transmittance2DUAV = mTransmittance2DTexture->GetDefaultUAV().get();

            __declspec(align(16)) struct
            {
                uint32_t atmosphereUniformCBVIndex;
                uint32_t transmittance2DUAVIndex;
            } transmittanceCB = {asUniformCBV->GetIndex(), transmittance2DUAV->GetIndex()};

            pContext->SetRootSignature(pComputeTransmittanceSignature.get());
            pContext->SetPipelineState(pComputeTransmittancePSO.get());
            pContext->SetConstantArray(0, sizeof(transmittanceCB) / sizeof(uint32_t), &transmittanceCB);

            uint32_t dispatchWidth  = AtmosphereScattering::TRANSMITTANCE_TEXTURE_WIDTH;
            uint32_t dispatchHeight = AtmosphereScattering::TRANSMITTANCE_TEXTURE_HEIGHT;

            pContext->Dispatch((dispatchWidth + 8 - 1) / 8, (dispatchHeight + 8 - 1) / 8, 1);
        });




    }

}