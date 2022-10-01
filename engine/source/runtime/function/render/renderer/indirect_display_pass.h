#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class DisplayPass : public RenderPass
    {
    public:
        struct DisplayPassInitInfo : public RenderPassInitInfo
        {};

        struct DisplayInputParameters : public PassInput
        {
            RHI::RgResourceHandle inputRTColorHandle;
            RHI::RgResourceHandle inputRTColorSRVHandle;
        };

        struct DisplayOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle renderTargetColorHandle;
            RHI::RgResourceHandle renderTargetColorRTVHandle;
        };

    public:
        ~DisplayPass() { destroy(); }

        void initialize(const DisplayPassInitInfo& init_info);
        void update(RHI::D3D12CommandContext& context,
                    RHI::RenderGraph&         graph,
                    DisplayInputParameters&   passInput,
                    DisplayOutputParameters&  passOutput);
        void destroy() override final;

    private:
        std::shared_ptr<RHI::D3D12DynamicDescriptor<D3D12_SHADER_RESOURCE_VIEW_DESC>> pD3D12SRVDescriptor;
    };
}

