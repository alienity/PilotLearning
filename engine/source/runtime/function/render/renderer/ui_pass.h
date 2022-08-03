#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class WindowUI;

    class UIPass : public RenderPass
	{
    public:
        struct UIInputParameters : public PassInput
        {

        };

        struct UIOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle backBufColor;
        };

    public:
        void initialize(const RenderPassInitInfo* init_info) override final;
        void initializeUIRenderBackend(WindowUI* window_ui) override final;
        void update(RHI::D3D12CommandContext& context,
                    RHI::RenderGraph&         graph,
                    PassInput&                passInput,
                    PassOutput&               passOutput) override final;
        void destroy() override final;

    private:
        void uploadFonts();

    private:
        WindowUI* m_window_ui;
	};
}

