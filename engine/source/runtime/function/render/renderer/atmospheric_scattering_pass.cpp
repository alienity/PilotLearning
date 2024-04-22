#pragma once

#include "atmospheric_scattering_pass.h"
#include "pass_helper.h"
#include <string>
#include <locale>
#include <codecvt>

namespace MoYu
{
    
    void AtmosphericScatteringPass::initialize(const PassInitInfo& init_info)
    {
        colorTexDesc = init_info.colorTexDesc;

        deltaIrradiance2DDesc = RHI::RgTextureDesc("DeltaIrradiance2D");
        deltaIrradiance2DDesc.SetType(RHI::RgTextureType::Texture2D);
        deltaIrradiance2DDesc.SetExtent(AtmosphereScattering::IRRADIANCE_TEXTURE_WIDTH,
                                        AtmosphereScattering::IRRADIANCE_TEXTURE_HEIGHT);
        deltaIrradiance2DDesc.SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT);
        deltaIrradiance2DDesc.SetAllowUnorderedAccess(true);

        deltaRayleighScattering3DDesc = RHI::RgTextureDesc("DeltaRayleighScattering3D");
        deltaRayleighScattering3DDesc.SetType(RHI::RgTextureType::Texture3D);
        deltaRayleighScattering3DDesc.SetExtent(AtmosphereScattering::SCATTERING_TEXTURE_WIDTH,
                                                AtmosphereScattering::SCATTERING_TEXTURE_HEIGHT,
                                                AtmosphereScattering::SCATTERING_TEXTURE_DEPTH);
        deltaRayleighScattering3DDesc.SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT);
        deltaRayleighScattering3DDesc.SetAllowUnorderedAccess(true);

        deltaMieScattering3DDesc = RHI::RgTextureDesc("DeltaMieScattering3D");
        deltaMieScattering3DDesc.SetType(RHI::RgTextureType::Texture3D);
        deltaMieScattering3DDesc.SetExtent(AtmosphereScattering::SCATTERING_TEXTURE_WIDTH,
                                                AtmosphereScattering::SCATTERING_TEXTURE_HEIGHT,
                                                AtmosphereScattering::SCATTERING_TEXTURE_DEPTH);
        deltaMieScattering3DDesc.SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT);
        deltaMieScattering3DDesc.SetAllowUnorderedAccess(true);

        deltaScatteringDensity3DDesc = RHI::RgTextureDesc("DeltaScatteringDensity3D");
        deltaScatteringDensity3DDesc.SetType(RHI::RgTextureType::Texture3D);
        deltaScatteringDensity3DDesc.SetExtent(AtmosphereScattering::SCATTERING_TEXTURE_WIDTH,
                                           AtmosphereScattering::SCATTERING_TEXTURE_HEIGHT,
                                           AtmosphereScattering::SCATTERING_TEXTURE_DEPTH);
        deltaScatteringDensity3DDesc.SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT);
        deltaScatteringDensity3DDesc.SetAllowUnorderedAccess(true);


		ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        // Compute the transmittance, and store it in transmittance_texture_.
        mComputeTransmittanceCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                                  m_ShaderRootPath / "hlsl/ASComputeTransmittanceCS.hlsl",
                                                                  ShaderCompileOptions(L"CSMain"));

        // Compute the direct irradiance, store it in delta_irradiance_texture and,
        // depending on 'blend', either initialize irradiance_texture_ with zeros or
        // leave it unchanged (we don't want the direct irradiance in
        // irradiance_texture_, but only the irradiance from the sky).
        mComputeDirectIrrdianceCS =
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                                  m_ShaderRootPath / "hlsl/ASComputeDirectIrradianceCS.hlsl",
                                                                  ShaderCompileOptions(L"CSMain"));

        // Compute the rayleigh and mie single scattering, store them in
        // delta_rayleigh_scattering_texture and delta_mie_scattering_texture, and
        // either store them or accumulate them in scattering_texture_ and
        // optional_single_mie_scattering_texture_.
        mComputeSingleScatteringCS =
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                            m_ShaderRootPath / "hlsl/ASComputeSingleScatteringCS.hlsl",
                                            ShaderCompileOptions(L"CSMain"));

        // Compute the 2nd, 3rd and 4th order of scattering, in sequence.
        // Compute the scattering density, and store it in
        // delta_scattering_density_texture.
        mComputeScatteringDensityCS =
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                            m_ShaderRootPath / "hlsl/ASComputeScatteringDensityCS.hlsl",
                                            ShaderCompileOptions(L"CSMain"));

        // Compute the indirect irradiance, store it in delta_irradiance_texture and
        // accumulate it in irradiance_texture_.
        mComputeIdirectIrradianceCS = 
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                            m_ShaderRootPath / "hlsl/ASComputeIdirectIrradianceCS.hlsl",
                                            ShaderCompileOptions(L"CSMain"));

        // Compute the multiple scattering, store it in
        // delta_multiple_scattering_texture, and accumulate it in
        // scattering_texture_.
        mComputeMultipleScatteringCS = 
            m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                            m_ShaderRootPath / "hlsl/ASComputeMultipleScatteringCS.hlsl",
                                            ShaderCompileOptions(L"CSMain"));

#define CommonRootSigDesc(rootSigName) \
    { \
        RHI::RootSignatureDesc rootSigDesc = \
            RHI::RootSignatureDesc() \
                .Add32BitConstants<0, 0>(16) \
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4) \
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4) \
                .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4) \
                .AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4) \
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

        {
            CommonRootSigDesc(pAtmosphericSkyProceduralSignature)
        }
        {
            mAtmosphericSkyProceduralVS =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Vertex,
                                                m_ShaderRootPath / "hlsl/AtmosphericSkyProcedural.hlsl",
                                                ShaderCompileOptions(L"VSMain"));

            mAtmosphericSkyProceduralPS =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Pixel,
                                                m_ShaderRootPath / "hlsl/AtmosphericSkyProcedural.hlsl",
                                                ShaderCompileOptions(L"PSMain"));

            RHI::D3D12InputLayout InputLayout = {};

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthWrite  = false;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0]     = DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat         = DXGI_FORMAT_D32_FLOAT;

            RHISampleState SampleState;
            SampleState.Count = 1;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pAtmosphericSkyProceduralSignature.get());
            psoStream.InputLayout   = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.VS                    = &mAtmosphericSkyProceduralVS;
            psoStream.PS                    = &mAtmosphericSkyProceduralPS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pAtmosphericSkyProceduralPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"AtmosphericSkyProcedural", psoDesc);
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
        RHI::RgResourceHandle atmosphereUniformBufferHandle = graph.Import<RHI::D3D12Buffer>(mAtmosphereUniformBuffer.get());
        RHI::RgResourceHandle transmittance2DHandle = graph.Import<RHI::D3D12Texture>(mTransmittance2D.get());
        RHI::RgResourceHandle scattering3DHandle    = graph.Import<RHI::D3D12Texture>(mScattering3D.get());
        RHI::RgResourceHandle irradiance2DHandle    = graph.Import<RHI::D3D12Texture>(mIrradiance2D.get());

        ImportGraphHandle graphHandle = {
            atmosphereUniformBufferHandle, transmittance2DHandle, scattering3DHandle, irradiance2DHandle};

        preCompute(graph, graphHandle);
        updateAtmosphere(graph, graphHandle, passInput, passOutput);
    }

    void AtmosphericScatteringPass::destroy()
    {
        mTransmittance2D = nullptr;
        mScattering3D    = nullptr;
        mIrradiance2D    = nullptr;
        mAtmosphereUniformBuffer = nullptr;

        pComputeTransmittanceSignature = nullptr;
        pComputeTransmittancePSO       = nullptr;

        pComputeDirectIrrdianceSignature = nullptr;
        pComputeDirectIrrdiancePSO       = nullptr;

        pComputeSingleScatteringSignature = nullptr;
        pComputeSingleScatteringPSO       = nullptr;
        
        pComputeScatteringDensitySignature = nullptr;
        pComputeScatteringDensityPSO       = nullptr;
        
        pComputeIdirectIrradianceSignature = nullptr;
        pComputeIdirectIrradiancePSO       = nullptr;
        
        pComputeMultipleScatteringSignature = nullptr;
        pComputeMultipleScatteringPSO       = nullptr;
        
        pAtmosphericSkyProceduralSignature = nullptr;
        pAtmosphericSkyProceduralPSO       = nullptr;
    }

    void AtmosphericScatteringPass::preCompute(RHI::RenderGraph& graph, ImportGraphHandle& graphHandle)
    {

        RHI::RgResourceHandle atmosphereUniformBufferHandle = graphHandle.atmosphereUniformBufferHandle;
        RHI::RgResourceHandle transmittance2DHandle         = graphHandle.transmittance2DHandle;
        RHI::RgResourceHandle scattering3DHandle            = graphHandle.scattering3DHandle;
        RHI::RgResourceHandle irradiance2DHandle            = graphHandle.irradiance2DHandle;

        RHI::RgResourceHandle deltaIrradiance2DHandle = graph.Create<RHI::D3D12Texture>(deltaIrradiance2DDesc);
        RHI::RgResourceHandle deltaRayleighScattering3DHandle = graph.Create<RHI::D3D12Texture>(deltaRayleighScattering3DDesc);
        RHI::RgResourceHandle deltaMieScattering3DHandle = graph.Create<RHI::D3D12Texture>(deltaMieScattering3DDesc);
        RHI::RgResourceHandle deltaScatteringDensity3DHandle = graph.Create<RHI::D3D12Texture>(deltaScatteringDensity3DDesc);

        if (hasPrecomputed)
            return;
        hasPrecomputed = true;

        // Compute the transmittance, and store it in transmittance_texture_.
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

        // Compute the direct irradiance, store it in delta_irradiance_texture and,
        // depending on 'blend', either initialize irradiance_texture_ with zeros or
        // leave it unchanged (we don't want the direct irradiance in
        // irradiance_texture_, but only the irradiance from the sky).
        RHI::RenderPass& directIrradiancePass = graph.AddRenderPass("ASComputeDirectIrradiancePass");

        directIrradiancePass.Read(atmosphereUniformBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        directIrradiancePass.Read(transmittance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        directIrradiancePass.Write(deltaIrradiance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
        directIrradiancePass.Write(irradiance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

        directIrradiancePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12Buffer*  mASUniformBuffer          = RegGetBuf(atmosphereUniformBufferHandle);
            RHI::D3D12Texture* mTransmittance2DTexture   = RegGetTex(transmittance2DHandle);
            RHI::D3D12Texture* mDeltaIrradiance2DTexture = RegGetTex(deltaIrradiance2DHandle);
            RHI::D3D12Texture* mIrradiance2DTexture      = RegGetTex(irradiance2DHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView*  asUniformCBV         = mASUniformBuffer->GetDefaultCBV().get();
            RHI::D3D12ShaderResourceView*  transmittance2DSRV   = mTransmittance2DTexture->GetDefaultSRV().get();
            RHI::D3D12UnorderedAccessView* deltaIrradiance2DUAV = mDeltaIrradiance2DTexture->GetDefaultUAV().get();
            RHI::D3D12UnorderedAccessView* irradiance2DUAV      = mIrradiance2DTexture->GetDefaultUAV().get();

            __declspec(align(16)) struct
            {
                uint32_t atmosphereUniformCBVIndex;
                uint32_t transmittance2DSRVIndex;
                uint32_t deltaIrradiance2DUAVIndex;
                uint32_t irradiance2DUAVIndex;
            } irradianceCB = {asUniformCBV->GetIndex(),
                              transmittance2DSRV->GetIndex(),
                              deltaIrradiance2DUAV->GetIndex(),
                              irradiance2DUAV->GetIndex()};

            pContext->SetRootSignature(pComputeDirectIrrdianceSignature.get());
            pContext->SetPipelineState(pComputeDirectIrrdiancePSO.get());
            pContext->SetConstantArray(0, sizeof(irradianceCB) / sizeof(uint32_t), &irradianceCB);

            uint32_t dispatchWidth  = AtmosphereScattering::IRRADIANCE_TEXTURE_WIDTH;
            uint32_t dispatchHeight = AtmosphereScattering::IRRADIANCE_TEXTURE_HEIGHT;

            pContext->Dispatch((dispatchWidth + 8 - 1) / 8, (dispatchHeight + 8 - 1) / 8, 1);
        });

        // Compute the rayleigh and mie single scattering, store them in
        // delta_rayleigh_scattering_texture and delta_mie_scattering_texture, and
        // either store them or accumulate them in scattering_texture_ and
        // optional_single_mie_scattering_texture_.
        RHI::RenderPass& singleScatteringPass = graph.AddRenderPass("ASComputeSingleScatteringPass");

        singleScatteringPass.Read(atmosphereUniformBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        singleScatteringPass.Read(transmittance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        singleScatteringPass.Write(deltaRayleighScattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
        singleScatteringPass.Write(deltaMieScattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
        singleScatteringPass.Write(scattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
        singleScatteringPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12Buffer*  mASUniformBuffer                 = RegGetBuf(atmosphereUniformBufferHandle);
            RHI::D3D12Texture* mTransmittance2DTexture          = RegGetTex(transmittance2DHandle);
            RHI::D3D12Texture* deltaRayleighScattering3DTexture = RegGetTex(deltaRayleighScattering3DHandle);
            RHI::D3D12Texture* deltaMieScattering3DTexture      = RegGetTex(deltaMieScattering3DHandle);
            RHI::D3D12Texture* scattering3DTexture              = RegGetTex(scattering3DHandle);

            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            RHI::D3D12ConstantBufferView*  asUniformCBV       = mASUniformBuffer->GetDefaultCBV().get();
            RHI::D3D12ShaderResourceView*  transmittance2DSRV = mTransmittance2DTexture->GetDefaultSRV().get();
            RHI::D3D12UnorderedAccessView* deltaRayleighScattering3DUAV = deltaRayleighScattering3DTexture->GetDefaultUAV().get();
            RHI::D3D12UnorderedAccessView* deltaMieScattering3DUAV = deltaMieScattering3DTexture->GetDefaultUAV().get();
            RHI::D3D12UnorderedAccessView* scattering3DUAV         = scattering3DTexture->GetDefaultUAV().get();

            __declspec(align(16)) struct
            {
                uint32_t atmosphereUniformCBVIndex;
                uint32_t transmittance2DSRVIndex;
                uint32_t deltaRayleighUAVIndex;
                uint32_t deltaMieUAVIndex;
                uint32_t scatteringUAVIndex;
            } mCB = {asUniformCBV->GetIndex(),
                     transmittance2DSRV->GetIndex(),
                     deltaRayleighScattering3DUAV->GetIndex(),
                     deltaMieScattering3DUAV->GetIndex(),
                     scattering3DUAV->GetIndex()};

            pContext->SetRootSignature(pComputeSingleScatteringSignature.get());
            pContext->SetPipelineState(pComputeSingleScatteringPSO.get());
            pContext->SetConstantArray(0, sizeof(mCB) / sizeof(uint32_t), &mCB);

            uint32_t dispatchWidth  = AtmosphereScattering::SCATTERING_TEXTURE_WIDTH;
            uint32_t dispatchHeight = AtmosphereScattering::SCATTERING_TEXTURE_HEIGHT;
            uint32_t dispatchDepth = AtmosphereScattering::SCATTERING_TEXTURE_DEPTH;

            pContext->Dispatch((dispatchWidth + 8 - 1) / 8, (dispatchHeight + 8 - 1) / 8, (dispatchDepth + 8 - 1) / 8);
        });
        
        //RHI::RgResourceHandle deltaMultipleScattering3DHandle = deltaRayleighScattering3DHandle;
        #define deltaMultipleScattering3DHandle deltaRayleighScattering3DHandle
        
        // Compute the 2nd, 3rd and 4th order of scattering, in sequence.
        uint32_t num_scattering_orders = 4;
        for (unsigned int scattering_order = 2; scattering_order <= num_scattering_orders; ++scattering_order)
        {
            {
                // Compute the scattering density, and store it in
                // delta_scattering_density_texture.
                RHI::RenderPass& scatteringDensityPass = graph.AddRenderPass("ASComputeScatteringDensityPass");

                scatteringDensityPass.Read(atmosphereUniformBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                scatteringDensityPass.Read(transmittance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                scatteringDensityPass.Read(deltaRayleighScattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                scatteringDensityPass.Read(deltaMieScattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                scatteringDensityPass.Read(deltaMultipleScattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                scatteringDensityPass.Read(deltaIrradiance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
                scatteringDensityPass.Write(deltaScatteringDensity3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
                scatteringDensityPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12Buffer*  mASUniformBuffer                 = RegGetBuf(atmosphereUniformBufferHandle);
                    RHI::D3D12Texture* mTransmittance2DTexture          = RegGetTex(transmittance2DHandle);
                    RHI::D3D12Texture* deltaRayleighScattering3DTexture = RegGetTex(deltaRayleighScattering3DHandle);
                    RHI::D3D12Texture* deltaMieScattering3DTexture      = RegGetTex(deltaMieScattering3DHandle);
                    RHI::D3D12Texture* deltaMultipleScattering3DTexture = RegGetTex(deltaMultipleScattering3DHandle);
                    RHI::D3D12Texture* deltaIrradiance2DTexture         = RegGetTex(deltaIrradiance2DHandle);
                    RHI::D3D12Texture* deltaScatteringDensity3DTexture  = RegGetTex(deltaScatteringDensity3DHandle);

                    RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                    RHI::D3D12ConstantBufferView*  asUniformCBV       = mASUniformBuffer->GetDefaultCBV().get();
                    RHI::D3D12ShaderResourceView*  transmittance2DSRV = mTransmittance2DTexture->GetDefaultSRV().get();
                    RHI::D3D12ShaderResourceView* deltaRayleighScattering3DSRV = deltaRayleighScattering3DTexture->GetDefaultSRV().get();
                    RHI::D3D12ShaderResourceView* deltaMieScattering3DSRV = deltaMieScattering3DTexture->GetDefaultSRV().get();
                    RHI::D3D12ShaderResourceView* deltaMultipleScattering3DSRV = deltaMultipleScattering3DTexture->GetDefaultSRV().get();
                    RHI::D3D12ShaderResourceView* deltaIrradiance2DSRV = deltaIrradiance2DTexture->GetDefaultSRV().get();
                    RHI::D3D12UnorderedAccessView* deltaScatteringDensity3DUAV = deltaScatteringDensity3DTexture->GetDefaultUAV().get();

                    __declspec(align(16)) struct
                    {
                        uint32_t atmosphereUniformCBVIndex;
                        uint32_t transmittance2DSRVIndex;
                        uint32_t singleRayleighScattering3DSRVIndex;
                        uint32_t singleMieScattering3DSRVIndex;
                        uint32_t multipleScattering3DSRVIndex;
                        uint32_t irradiance2DSRVIndex;
                        uint32_t scatteringDensityUAVIndex;

                        int scattering_order;
                    } mCB = {asUniformCBV->GetIndex(),
                             transmittance2DSRV->GetIndex(),
                             deltaRayleighScattering3DSRV->GetIndex(),
                             deltaMieScattering3DSRV->GetIndex(),
                             deltaMultipleScattering3DSRV->GetIndex(),
                             deltaIrradiance2DSRV->GetIndex(),
                             deltaScatteringDensity3DUAV->GetIndex(),
                             scattering_order};

                    pContext->SetRootSignature(pComputeScatteringDensitySignature.get());
                    pContext->SetPipelineState(pComputeScatteringDensityPSO.get());
                    pContext->SetConstantArray(0, sizeof(mCB) / sizeof(uint32_t), &mCB);

                    uint32_t dispatchWidth  = AtmosphereScattering::SCATTERING_TEXTURE_WIDTH;
                    uint32_t dispatchHeight = AtmosphereScattering::SCATTERING_TEXTURE_HEIGHT;
                    uint32_t dispatchDepth = AtmosphereScattering::SCATTERING_TEXTURE_DEPTH;

                    pContext->Dispatch((dispatchWidth + 8 - 1) / 8, (dispatchHeight + 8 - 1) / 8, (dispatchDepth + 8 - 1) / 8);
                });
            }

            {
                // Compute the indirect irradiance, store it in delta_irradiance_texture and
                // accumulate it in irradiance_texture_.
                RHI::RenderPass& indirectIrradiancePass = graph.AddRenderPass("ASComputeIndirectIrradiancePass");

                indirectIrradiancePass.Read(atmosphereUniformBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                indirectIrradiancePass.Read(deltaRayleighScattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                indirectIrradiancePass.Read(deltaMieScattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                indirectIrradiancePass.Read(deltaMultipleScattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                indirectIrradiancePass.Read(irradiance2DHandle, true);
                indirectIrradiancePass.Write(deltaIrradiance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
                indirectIrradiancePass.Write(irradiance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
                indirectIrradiancePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12Buffer*  mASUniformBuffer                 = RegGetBuf(atmosphereUniformBufferHandle);
                    RHI::D3D12Texture* deltaRayleighScattering3DTexture = RegGetTex(deltaRayleighScattering3DHandle);
                    RHI::D3D12Texture* deltaMieScattering3DTexture      = RegGetTex(deltaMieScattering3DHandle);
                    RHI::D3D12Texture* deltaMultipleScattering3DTexture = RegGetTex(deltaMultipleScattering3DHandle);
                    RHI::D3D12Texture* deltaIrradiance2DTexture         = RegGetTex(deltaIrradiance2DHandle);
                    RHI::D3D12Texture* irradiance2DTexture              = RegGetTex(irradiance2DHandle);

                    RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                    RHI::D3D12ConstantBufferView*  asUniformCBV       = mASUniformBuffer->GetDefaultCBV().get();
                    RHI::D3D12ShaderResourceView* deltaRayleighScattering3DSRV = deltaRayleighScattering3DTexture->GetDefaultSRV().get();
                    RHI::D3D12ShaderResourceView* deltaMieScattering3DSRV = deltaMieScattering3DTexture->GetDefaultSRV().get();
                    RHI::D3D12ShaderResourceView* deltaMultipleScattering3DSRV = deltaMultipleScattering3DTexture->GetDefaultSRV().get();
                    RHI::D3D12ShaderResourceView* deltaIrradiance2DUAV = deltaIrradiance2DTexture->GetDefaultSRV().get();
                    RHI::D3D12UnorderedAccessView* irradiance2DUAV = irradiance2DTexture->GetDefaultUAV().get();

                    __declspec(align(16)) struct
                    {
                        uint32_t atmosphereUniformCBVIndex;
                        uint32_t singleRayleighScattering3DSRVIndex;
                        uint32_t singleMieScattering3DSRVIndex;
                        uint32_t multipleScattering3DSRVIndex;
                        uint32_t deltaIrradiance2DUAVIndex;
                        uint32_t irradiance2DUAVIndex;
                        int scattering_order;
                    } mCB = {asUniformCBV->GetIndex(),
                             deltaRayleighScattering3DSRV->GetIndex(),
                             deltaMieScattering3DSRV->GetIndex(),
                             deltaMultipleScattering3DSRV->GetIndex(),
                             deltaIrradiance2DUAV->GetIndex(),
                             irradiance2DUAV->GetIndex(),
                             scattering_order - 1};

                    pContext->SetRootSignature(pComputeIdirectIrradianceSignature.get());
                    pContext->SetPipelineState(pComputeIdirectIrradiancePSO.get());
                    pContext->SetConstantArray(0, sizeof(mCB) / sizeof(uint32_t), &mCB);

                    uint32_t dispatchWidth  = AtmosphereScattering::IRRADIANCE_TEXTURE_WIDTH;
                    uint32_t dispatchHeight = AtmosphereScattering::IRRADIANCE_TEXTURE_HEIGHT;

                    pContext->Dispatch((dispatchWidth + 8 - 1) / 8, (dispatchHeight + 8 - 1) / 8, 1);
                });
            }

            {
                // Compute the multiple scattering, store it in
                // delta_multiple_scattering_texture, and accumulate it in
                // scattering_texture_.
                RHI::RenderPass& multipleScatteringPass = graph.AddRenderPass("ASComputeMultipleScatteringPass");

                multipleScatteringPass.Read(atmosphereUniformBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                multipleScatteringPass.Read(transmittance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                multipleScatteringPass.Read(deltaScatteringDensity3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                multipleScatteringPass.Read(scattering3DHandle, true);
                multipleScatteringPass.Write(deltaMultipleScattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
                multipleScatteringPass.Write(scattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
                multipleScatteringPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12Buffer*  mASUniformBuffer                 = RegGetBuf(atmosphereUniformBufferHandle);
                    RHI::D3D12Texture* mTransmittance2DTexture          = RegGetTex(transmittance2DHandle);
                    RHI::D3D12Texture* deltaScatteringDensity3DTexture  = RegGetTex(deltaScatteringDensity3DHandle);

                    RHI::D3D12Texture* deltaMultipleScattering3DTexture = RegGetTex(deltaMultipleScattering3DHandle);
                    RHI::D3D12Texture* scattering3DTexture              = RegGetTex(scattering3DHandle);

                    RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                    RHI::D3D12ConstantBufferView* asUniformCBV       = mASUniformBuffer->GetDefaultCBV().get();
                    RHI::D3D12ShaderResourceView* transmittance2DSRV = mTransmittance2DTexture->GetDefaultSRV().get();
                    RHI::D3D12ShaderResourceView* deltaScatteringDensity3DSRV = deltaScatteringDensity3DTexture->GetDefaultSRV().get();
                    RHI::D3D12UnorderedAccessView* deltaMultipleScattering3DUAV = deltaMultipleScattering3DTexture->GetDefaultUAV().get();
                    RHI::D3D12UnorderedAccessView* scattering3DUAV = scattering3DTexture->GetDefaultUAV().get();

                    __declspec(align(16)) struct
                    {
                        uint32_t atmosphereUniformCBVIndex;
                        uint32_t transmittance2DSRVIndex;
                        uint32_t scatteringDensity3DSRVIndex;
                        uint32_t deltaMultipleScattering3DUAVIndex;
                        uint32_t scattering3DUAVIndex;

                        int scattering_order;
                    } mCB = {asUniformCBV->GetIndex(),
                             transmittance2DSRV->GetIndex(),
                             deltaScatteringDensity3DSRV->GetIndex(),
                             deltaMultipleScattering3DUAV->GetIndex(),
                             scattering3DUAV->GetIndex(),
                             scattering_order};

                    pContext->SetRootSignature(pComputeMultipleScatteringSignature.get());
                    pContext->SetPipelineState(pComputeMultipleScatteringPSO.get());
                    pContext->SetConstantArray(0, sizeof(mCB) / sizeof(uint32_t), &mCB);

                    uint32_t dispatchWidth  = AtmosphereScattering::SCATTERING_TEXTURE_WIDTH;
                    uint32_t dispatchHeight = AtmosphereScattering::SCATTERING_TEXTURE_HEIGHT;
                    uint32_t dispatchDepth = AtmosphereScattering::SCATTERING_TEXTURE_DEPTH;

                    pContext->Dispatch((dispatchWidth + 8 - 1) / 8, (dispatchHeight + 8 - 1) / 8, (dispatchDepth + 8 - 1) / 8);
                });

            }

        }
        
        graphHandle.atmosphereUniformBufferHandle = atmosphereUniformBufferHandle;
        graphHandle.transmittance2DHandle         = transmittance2DHandle;
        graphHandle.scattering3DHandle            = scattering3DHandle;
        graphHandle.irradiance2DHandle            = irradiance2DHandle;
    }

    void AtmosphericScatteringPass::updateAtmosphere(RHI::RenderGraph& graph, ImportGraphHandle& graphHandle, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        if (!hasPrecomputed)
            return;

        RHI::RgResourceHandle atmosphereUniformBufferHandle = graphHandle.atmosphereUniformBufferHandle;
        RHI::RgResourceHandle transmittance2DHandle         = graphHandle.transmittance2DHandle;
        RHI::RgResourceHandle scattering3DHandle            = graphHandle.scattering3DHandle;
        RHI::RgResourceHandle irradiance2DHandle            = graphHandle.irradiance2DHandle;

        RHI::RenderPass& asProceduralPass = graph.AddRenderPass("AtmosphericSkyProceduralPass");

        RHI::RgResourceHandle perframeBufferHandle    = passInput.perframeBufferHandle;
        RHI::RgResourceHandle renderTargetColorHandle = passOutput.renderTargetColorHandle;
        RHI::RgResourceHandle renderTargetDepthHandle = passOutput.renderTargetDepthHandle;

        asProceduralPass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        asProceduralPass.Read(atmosphereUniformBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        asProceduralPass.Read(transmittance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        asProceduralPass.Read(scattering3DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        asProceduralPass.Read(irradiance2DHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        asProceduralPass.Read(renderTargetColorHandle, true);
        asProceduralPass.Read(renderTargetDepthHandle, true);

        asProceduralPass.Write(renderTargetColorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        asProceduralPass.Write(renderTargetDepthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        asProceduralPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12Buffer*  mPerFrameBufferBuffer   = RegGetBuf(perframeBufferHandle);
            RHI::D3D12Buffer*  mASUniformBuffer        = RegGetBuf(atmosphereUniformBufferHandle);
            RHI::D3D12Texture* mTransmittance2DTexture = RegGetTex(transmittance2DHandle);
            RHI::D3D12Texture* scattering3DTexture     = RegGetTex(scattering3DHandle);
            RHI::D3D12Texture* irradiance2DTexture     = RegGetTex(irradiance2DHandle);

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12Texture* pRenderTargetColor = registry->GetD3D12Texture(renderTargetColorHandle);
            RHI::D3D12Texture* pRenderTargetDepth = registry->GetD3D12Texture(renderTargetDepthHandle);

            RHI::D3D12RenderTargetView* renderTargetView = pRenderTargetColor->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = pRenderTargetDepth->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)colorTexDesc.Width, (float)colorTexDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)colorTexDesc.Width, (int)colorTexDesc.Height});
            graphicContext->SetRenderTarget(renderTargetView, depthStencilView);

            graphicContext->SetRootSignature(pAtmosphericSkyProceduralSignature.get());
            graphicContext->SetPipelineState(pAtmosphericSkyProceduralPSO.get());

            struct
            {
                int perFrameBufferIndex;
                int atmosphereUniformIndex;
                int transmittance2DSRVIndex;
                int scattering3DSRVIndex;
                int irradiance2DSRVIndex;

                float exposure;
                float white_point_x;
                float white_point_y;
                float white_point_z;
            } mCB = {mPerFrameBufferBuffer->GetDefaultCBV()->GetIndex(),
                     mASUniformBuffer->GetDefaultCBV()->GetIndex(),
                     mTransmittance2DTexture->GetDefaultSRV()->GetIndex(),
                     scattering3DTexture->GetDefaultSRV()->GetIndex(),
                     irradiance2DTexture->GetDefaultSRV()->GetIndex(),
                     2.6f,
                     1.0f, 1.0f, 1.0f};

            graphicContext->SetConstantArray(0, sizeof(mCB) / sizeof(int), &mCB);

            graphicContext->Draw(3);
        });
        
        passOutput.renderTargetColorHandle = renderTargetColorHandle;
        passOutput.renderTargetDepthHandle = renderTargetDepthHandle;
    }

}