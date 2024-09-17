#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    // https://therealmjp.github.io/posts/bindless-texturing-for-deferred-rendering-and-decals/
    class IndirectLightLoopPass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;

            RHI::RgResourceHandle gbuffer0Handle;
            RHI::RgResourceHandle gbuffer1Handle;
            RHI::RgResourceHandle gbuffer2Handle;
            RHI::RgResourceHandle gbuffer3Handle;
            RHI::RgResourceHandle gbufferDepthHandle;

            RHI::RgResourceHandle ssgiHandle;
            RHI::RgResourceHandle ssrHandle;

            RHI::RgResourceHandle mAOHandle;

            RHI::RgResourceHandle directionalCascadeShadowmapHandle;
            std::vector<RHI::RgResourceHandle> spotShadowmapHandles;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                specularLightinghandle.Invalidate();
            }

            RHI::RgResourceHandle specularLightinghandle;
        };

    public:
        ~IndirectLightLoopPass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    protected:
        bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);

        RHI::RgTextureDesc colorTexDesc;

    private:
        Shader indirectLightLoopCS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirectLightLoopSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirectLightLoopPSO;
	};
}

