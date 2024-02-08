#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/atmospheric_scattering_pass.h"

namespace MoYu
{
    class AtmosphericScatteringPass : public RenderPass
	{
    public:
        struct PassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;
            RHI::RgTextureDesc depthTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle maxDepthPtyramidHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle volumeLightHandle;
        };

    public:
        ~AtmosphericScatteringPass() { destroy(); }

        void initialize(const PassInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        RHI::RgTextureDesc colorTexDesc;
        
        Shader mAtmosphericScatteringVS;
        Shader mAtmosphericScatteringPS;
        std::shared_ptr<RHI::D3D12RootSignature> pAtmosphericScatteringSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pAtmosphericScatteringPSO;

        std::shared_ptr<RHI::D3D12Texture> m_transmittance2d;
        std::shared_ptr<RHI::D3D12Texture> m_scattering3d;
        std::shared_ptr<RHI::D3D12Texture> m_irradiance2d;
        std::shared_ptr<RHI::D3D12Texture> m_singlemiescattering3d;
	};
}