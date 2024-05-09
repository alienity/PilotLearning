#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class VolumeLightPass : public RenderPass
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
            RHI::RgResourceHandle volumeCloudShadowHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle volumeLightHandle;
        };

    public:
        ~VolumeLightPass() { destroy(); }

        void initialize(const PassInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        RHI::RgTextureDesc colorTexDesc;
        //RHI::RgTextureDesc volume3DDesc;

        Shader mGuassianBlurCS;
        std::shared_ptr<RHI::D3D12RootSignature> pGuassianBlurSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pGuassianBlurPSO;

        Shader mVolumeLightingCS;
        std::shared_ptr<RHI::D3D12RootSignature> pVolumeLightingSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pVolumeLightingPSO;

        std::shared_ptr<RHI::D3D12Texture> m_bluenoise;

        std::shared_ptr<RHI::D3D12Texture> m_volume3d;
	};
}