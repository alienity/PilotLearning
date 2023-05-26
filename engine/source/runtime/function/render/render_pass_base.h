#pragma once

#include "runtime/function/render/render_resource_base.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/function/render/rhi/d3d12/d3d12_commandContext.h"
#include "runtime/function/render/rhi/d3d12/d3d12_device.h"

namespace MoYu
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

        template<typename PInit = RenderPassInitInfo>
        void initialize(const PInit& init_info)
        {}

        template<typename PI = PassInput, typename PO = PassOutput>
        void update(RHI::D3D12CommandContext& context, RHI::RenderGraph& graph, PI& passInput, PO& passOutput)
        {}

        virtual void destroy() {};

    protected:
        RHI::RenderGraphAllocator* m_RenderGraphAllocator;
        RHI::RenderGraphRegistry*  m_RenderGraphRegistry;
        RHI::D3D12Device*          m_Device;
        WindowSystem*              m_WindowSystem;

    };
} // namespace MoYu
