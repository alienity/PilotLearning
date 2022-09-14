#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class UIPass : public RenderPass
	{
    public:
        struct UIPassInitInfo : public RenderPassInitInfo
        {};

        struct UIInputParameters : public PassInput
        {};

        struct UIOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle backBufColor;
            RHI::D3D12RenderTargetView* backBufRtv;
        };

    public:
        ~UIPass() { destroy(); }

        void initialize(const UIPassInitInfo& init_info);
        void update(RHI::D3D12CommandContext& context,
                    RHI::RenderGraph&         graph,
                    UIInputParameters&        passInput,
                    UIOutputParameters&       passOutput);
        void destroy() override final;

    private:
        std::shared_ptr<RHI::D3D12DynamicDescriptor<D3D12_SHADER_RESOURCE_VIEW_DESC>> pD3D12SRVDescriptor;
	};
}

