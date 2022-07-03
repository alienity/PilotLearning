#pragma once

#include <memory>

#include "runtime/function/render/rhi/d3d12/d3d12_device.h"
#include "runtime/function/render/rhi/d3d12/d3d12_swapChain.h"
#include "runtime/function/render/rhi/shader_compiler.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"

namespace Pilot
{
    class WindowSystem;

    struct RendererInitInfo
    {
        RHI::DeviceOptions            Options;
        std::shared_ptr<WindowSystem> Window_system;
    };

    class RendererPresent : public RHI::IPresent
    {
    public:
        RendererPresent(RHI::D3D12CommandContext& Context) noexcept : Context(Context) {}

        void PrePresent() override { SyncHandle = Context.Execute(false); }

        void PostPresent() override { SyncHandle.WaitForCompletion(); }

        RHI::D3D12CommandContext& Context;
        RHI::D3D12SyncHandle      SyncHandle;
    };

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        void  Initialize(RendererInitInfo initialize_info);
        void  Tick();
        void  Shutdown();
        void* GetViewportPtr() const { return Viewport; }

    protected:
        std::unique_ptr<RHI::D3D12Device>    Device;
        std::unique_ptr<ShaderCompiler>      Compiler;
        std::unique_ptr<RHI::D3D12SwapChain> SwapChain;
        std::shared_ptr<WindowSystem>        WinSystem;

        RHI::RenderGraphAllocator Allocator;
        RHI::RenderGraphRegistry  Registry;

        size_t FrameIndex = 0;

        void* Viewport = nullptr;
    };

}