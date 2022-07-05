#pragma once

#include <memory>

#include "runtime/function/render/rhi/d3d12/d3d12_device.h"
#include "runtime/function/render/rhi/d3d12/d3d12_swapChain.h"
#include "runtime/function/render/rhi/shader_compiler.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"

namespace Pilot
{
    class WindowSystem;
    class RendererPresent;
    class Renderer;

    struct RendererInitInfo
    {
        RHI::DeviceOptions            Options;
        std::shared_ptr<WindowSystem> Window_system;
    };

    class RendererManager
    {
    public:
        RendererManager();
        ~RendererManager();

        void  Initialize(RendererInitInfo initialize_info);
        void  Tick();
        
    protected:
        std::unique_ptr<RHI::D3D12Device>    Device;
        std::unique_ptr<ShaderCompiler>      Compiler;
        std::unique_ptr<RHI::D3D12SwapChain> SwapChain;
        std::shared_ptr<WindowSystem>        WinSystem;
        std::unique_ptr<Renderer>            MoYuRenderer;
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
        Renderer(RHI::D3D12Device* Device, ShaderCompiler* Compiler);

        virtual ~Renderer();

        virtual void OnRender(RHI::D3D12CommandContext& Context);

        [[nodiscard]] void* GetViewportPtr() const { return Viewport; }

    protected:
        RHI::D3D12Device* Device   = nullptr;
        ShaderCompiler*   Compiler = nullptr;

        RHI::RenderGraphAllocator Allocator;
        RHI::RenderGraphRegistry  Registry;

        size_t FrameIndex = 0;

        void* Viewport = nullptr;
    };

}