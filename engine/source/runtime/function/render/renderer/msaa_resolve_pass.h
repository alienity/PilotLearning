#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class MSAAResolvePass : public RenderPass
	{
    public:
        struct MSAAResolveInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;
            RHI::RgTextureDesc depthTexDesc;
        };

        struct DrawInputParameters : public PassInput
        {
            DrawInputParameters()
            {
                renderTargetColorHandle.Invalidate();
                //renderTargetDepthHandle.Invalidate();
            }

            RHI::RgResourceHandle renderTargetColorHandle;
            //RHI::RgResourceHandle renderTargetDepthHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                resolveTargetColorHandle.Invalidate();
                //resolveTargetDepthHandle.Invalidate();
            }

            RHI::RgResourceHandle resolveTargetColorHandle;
            //RHI::RgResourceHandle resolveTargetDepthHandle;
        };

    public:
        ~MSAAResolvePass() { destroy(); }

        void initialize(const MSAAResolveInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    protected:
        //bool initializeResolveTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);

    private:
        RHI::RgTextureDesc colorTexDesc;
        //RHI::RgTextureDesc depthTexDesc;
	};
}

