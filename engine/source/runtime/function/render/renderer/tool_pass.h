#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class ToolPass : public RenderPass
    {
    public:
        struct ToolPassInitInfo : public RenderPassInitInfo
        {};

        struct ToolInputParameters : public PassInput
        {
            //RHI::RgResourceHandle renderTargetColorHandle;
        };

        struct ToolOutputParameters : public PassOutput
        {
            //RHI::RgResourceHandle backBufColorHandle;
        };

    public:
        ~ToolPass() { destroy(); }

        void initialize(const ToolPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, ToolInputParameters& passInput, ToolOutputParameters& passOutput);
        void destroy() override final;
    };
}

