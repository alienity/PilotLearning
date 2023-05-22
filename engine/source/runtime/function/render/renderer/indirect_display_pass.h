#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class DisplayPass : public RenderPass
    {
    public:
        struct DisplayPassInitInfo : public RenderPassInitInfo
        {};

        struct DisplayInputParameters : public PassInput
        {
            RHI::RgResourceHandle inputRTColorHandle;
        };

        struct DisplayOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle renderTargetColorHandle;
        };

    public:
        ~DisplayPass() { destroy(); }

        void initialize(const DisplayPassInitInfo& init_info);
        void update(RHI::RenderGraph&         graph,
                    DisplayInputParameters&   passInput,
                    DisplayOutputParameters&  passOutput);
        void destroy() override final;

    private:

    };
}

