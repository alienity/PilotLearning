#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class IndirectDrawPass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc depthBufferTexDesc;
        };

        struct DrawInputParameters : public PassInput
        {
            uint32_t commandBufferCounterOffset;

            std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;

            std::shared_ptr<RHI::D3D12Buffer> pIndirectCommandBuffer;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle renderTargetColorHandle;
            RHI::RgResourceHandle renderTargetColorRTVHandle;

            RHI::RgResourceHandle renderTargetDepthHandle;
            RHI::RgResourceHandle renderTargetDepthDSVHandle;
        };

    public:
        ~IndirectDrawPass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::D3D12CommandContext& context,
                    RHI::RenderGraph&         graph,
                    DrawInputParameters&      passInput,
                    DrawOutputParameters&     passOutput);
        void destroy() override final;

    private:
        std::shared_ptr<RHI::D3D12DynamicDescriptor<D3D12_SHADER_RESOURCE_VIEW_DESC>> pD3D12SRVDescriptor;

        RHI::RgTextureDesc depthBufferTexDesc;
	};
}

