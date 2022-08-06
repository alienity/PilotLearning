#pragma once

#include "runtime/function/render/render_resource_base.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/function/render/rhi/d3d12/d3d12_commandContext.h"
#include "runtime/function/render/rhi/d3d12/d3d12_device.h"

namespace Pilot
{
    struct RenderPassInitInfo
    {};

    struct PassInput
    {};

    struct PassOutput
    {};

    struct RenderPassCommonInfo
    {
        RHI::RenderGraphAllocator* renderGraphAllocator;
        RHI::RenderGraphRegistry*  renderGraphRegistry;
        RHI::D3D12Device*          device;
        WindowSystem*              windowsSystem;
    };
    
    class RenderPassBase
    {
    public:
        virtual void setCommonInfo(RenderPassCommonInfo common_info);

        virtual void initialize(const RenderPassInitInfo& init_info) = 0;
        virtual void update(RHI::D3D12CommandContext& context,
                            RHI::RenderGraph&         graph,
                            PassInput&                passInput,
                            PassOutput&               passOutput)    = 0;
        virtual void destroy()                                       = 0;

    protected:
        RHI::RenderGraphAllocator* m_RenderGraphAllocator;
        RHI::RenderGraphRegistry*  m_RenderGraphRegistry;
        RHI::D3D12Device*          m_Device;
        WindowSystem*              m_WindowSystem;

    };
} // namespace Piccolo
