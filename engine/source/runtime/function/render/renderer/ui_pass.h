#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/ui/window_ui.h"

namespace Pilot
{
    class UIPass : public RenderPass
	{
    public:
        struct UIPassInitInfo : public RenderPassInitInfo
        {
            WindowUI* window_ui;
        };

        struct UIInputParameters : public PassInput
        {
            RHI::RgResourceHandle renderTargetColorHandle;
        };

        struct UIOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle backBufColorHandle;
        };

    public:
        ~UIPass() { destroy(); }

        void initialize(const UIPassInitInfo& init_info);
        void update(RHI::RenderGraph&         graph,
                    UIInputParameters&        passInput,
                    UIOutputParameters&       passOutput);
        void destroy() override final;

    private:
        std::shared_ptr<RHI::D3D12Descriptor<D3D12_SHADER_RESOURCE_VIEW_DESC>> pD3D12SRVDescriptor;

        WindowUI* window_ui;
	};
}

