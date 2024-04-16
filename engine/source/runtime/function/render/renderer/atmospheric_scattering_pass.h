#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/atmospheric_scattering_helper.h"
#include "runtime/function/render/renderer/atmospheric_scattering_pass.h"

namespace MoYu
{
    class AtmosphericScatteringPass : public RenderPass
	{
    public:
        struct PassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;
            
            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle renderTargetColorHandle;
            RHI::RgResourceHandle renderTargetDepthHandle;
        };

        struct ImportGraphHandle
        {
            RHI::RgResourceHandle atmosphereUniformBufferHandle;
            RHI::RgResourceHandle transmittance2DHandle;
            RHI::RgResourceHandle scattering3DHandle;
            RHI::RgResourceHandle irradiance2DHandle;
        };

    public:
        ~AtmosphericScatteringPass() { destroy(); }

        void initialize(const PassInitInfo& init_info);
        void prepareMetaData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

        void preCompute(RHI::RenderGraph& graph, ImportGraphHandle& graphHandle);
        void updateAtmosphere(RHI::RenderGraph& graph, ImportGraphHandle& graphHandle, DrawInputParameters& passInput, DrawOutputParameters& passOutput);

    private:
        bool hasPrecomputed = false;

        RHI::RgTextureDesc colorTexDesc;

        RHI::RgTextureDesc deltaIrradiance2DDesc;
        RHI::RgTextureDesc deltaRayleighScattering3DDesc;
        RHI::RgTextureDesc deltaMieScattering3DDesc;
        RHI::RgTextureDesc deltaScatteringDensity3DDesc;

        std::shared_ptr<RHI::D3D12Buffer> mAtmosphereUniformBuffer;

        std::shared_ptr<RHI::D3D12Texture> mTransmittance2D;
        std::shared_ptr<RHI::D3D12Texture> mScattering3D;
        std::shared_ptr<RHI::D3D12Texture> mIrradiance2D;

        Shader mComputeTransmittanceCS;
        std::shared_ptr<RHI::D3D12RootSignature> pComputeTransmittanceSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pComputeTransmittancePSO;

        Shader mComputeDirectIrrdianceCS;
        std::shared_ptr<RHI::D3D12RootSignature> pComputeDirectIrrdianceSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pComputeDirectIrrdiancePSO;

        Shader mComputeSingleScatteringCS;
        std::shared_ptr<RHI::D3D12RootSignature> pComputeSingleScatteringSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pComputeSingleScatteringPSO;

        Shader mComputeScatteringDensityCS;
        std::shared_ptr<RHI::D3D12RootSignature> pComputeScatteringDensitySignature;
        std::shared_ptr<RHI::D3D12PipelineState> pComputeScatteringDensityPSO;

        Shader mComputeIdirectIrradianceCS;
        std::shared_ptr<RHI::D3D12RootSignature> pComputeIdirectIrradianceSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pComputeIdirectIrradiancePSO;

        Shader mComputeMultipleScatteringCS;
        std::shared_ptr<RHI::D3D12RootSignature> pComputeMultipleScatteringSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pComputeMultipleScatteringPSO;
        
        Shader mAtmosphericSkyProceduralVS;
        Shader mAtmosphericSkyProceduralPS;
        std::shared_ptr<RHI::D3D12RootSignature> pAtmosphericSkyProceduralSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pAtmosphericSkyProceduralPSO;
	};
}