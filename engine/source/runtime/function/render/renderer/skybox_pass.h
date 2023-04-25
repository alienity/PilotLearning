#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class SkyBoxPass : public RenderPass
	{
    public:
        struct SkyBoxInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;
            RHI::RgTextureDesc depthTexDesc;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                renderTargetColorHandle.Invalidate();
                renderTargetDepthHandle.Invalidate();
            }

            RHI::RgResourceHandle renderTargetColorHandle;
            RHI::RgResourceHandle renderTargetDepthHandle;
        };

    public:
        ~SkyBoxPass() { destroy(); }

        void initialize(const SkyBoxInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResourceBase> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    protected:
        bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);

    private:
        RHI::RgTextureDesc colorTexDesc;
        RHI::RgTextureDesc depthTexDesc;

    private:
        int   specularIBLTexIndex;
        float specularIBLTexLevel;
	};
}

