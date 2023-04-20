#pragma once
#include "d3d12_core.h"
#include "d3d12_resource.h"

namespace RHI
{
    class RHISyncHandle;
    class CommandBuffer;

    class ScriptRenderContext : public D3D12LinkedDeviceChild
    {
        void Execute(CommandBuffer commandBuffer);
        void Submit();
    };

} // namespace RHI
