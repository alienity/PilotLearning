#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/renderer/msaa_resolve_pass.h"
#include "runtime/function/render/renderer/fxaa_pass.h"

namespace Pilot
{
    class PostprocessPasses : public RenderPass
	{
    public:
        struct PostprocessInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;
            ShaderCompiler*    m_ShaderCompiler;
        };

        struct PostprocessInputParameters : public PassInput
        {
            PostprocessInputParameters()
            {
                renderTargetColorHandle.Invalidate();
                renderTargetDepthHandle.Invalidate();
            }

            RHI::RgResourceHandle renderTargetColorHandle;
            RHI::RgResourceHandle renderTargetDepthHandle;
        };

        struct PostprocessOutputParameters : public PassOutput
        {
            PostprocessOutputParameters()
            {
                postTargetColorHandle.Invalidate();
            }

            RHI::RgResourceHandle postTargetColorHandle;
        };

    public:
        ~PostprocessPasses() { destroy(); }

        void initialize(const PostprocessInitInfo& init_info);
        void update(RHI::RenderGraph& graph, PostprocessInputParameters& passInput, PostprocessOutputParameters& passOutput);
        void destroy() override final;

    protected:
        bool initializeResolveTarget(RHI::RenderGraph& graph, PostprocessOutputParameters* drawPassOutput);

    private:
        RHI::RgTextureDesc colorTexDesc;

        std::shared_ptr<MSAAResolvePass> mResolvePass;
        std::shared_ptr<FXAAPass>        mFXAAPass;

	};
}

