#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/volume_cloud_helper.h"

namespace MoYu
{
    class VolumeCloudPass : public RenderPass
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
            RHI::RgResourceHandle renderTargetColorHandle;
            RHI::RgResourceHandle renderTargetDepthHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle outColorHandle;
        };

    public:
        ~VolumeCloudPass() { destroy(); }

        void initialize(const PassInitInfo& init_info);
        void prepareMetaData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        RHI::RgTextureDesc colorTexDesc;

        VolumeCloudSpace::CloudsConsCB mCloudsConsCB;

        std::shared_ptr<RHI::D3D12Buffer> mCloudConstantsBuffer;

        std::shared_ptr<RHI::D3D12Texture> mWeather2D;
        std::shared_ptr<RHI::D3D12Texture> mCloud3D;
        std::shared_ptr<RHI::D3D12Texture> mWorley3D;

        Shader mVolumeCloudCS;
        std::shared_ptr<RHI::D3D12RootSignature> pVolumeCloudSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pVolumeCloudPSO;

	};
}