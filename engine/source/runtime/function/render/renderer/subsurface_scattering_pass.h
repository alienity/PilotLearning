#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/volume_cloud_helper.h"
#include "runtime/function/render/renderer/diffusion_pfoile_settings.h"

namespace MoYu
{

    // https://therealmjp.github.io/posts/sss-intro/#screen-space-subsurface-scattering-ssss
    // https://advances.realtimerendering.com/s2018/Efficient%20screen%20space%20subsurface%20scattering%20Siggraph%202018.pdf
    // https://www.slideshare.net/colinbb/colin-barrebrisebois-gdc-2011-approximating-translucency-for-a-fast-cheap-and-convincing-subsurfacescattering-look-7170855
    // http://www.iryoku.com/downloads/Next-Generation-Character-Rendering-v6.pptx

    class SubsurfaceScatteringPass : public RenderPass
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
        ~SubsurfaceScatteringPass() { destroy(); }

        void initialize(const PassInitInfo& init_info);
        void prepareMetaData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        RHI::RgTextureDesc colorTexDesc;

        Shader mSubsurfaceScatteringCS;
        std::shared_ptr<RHI::D3D12RootSignature> pSubsurfaceScatteringSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSubsurfaceScatteringPSO;

        Shader mCombineLightingCS;
        std::shared_ptr<RHI::D3D12RootSignature> pCombineLightingSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pCombineLightingPSO;

        DiffusionProfileSettings diffusionProfileSettings;
	};
}