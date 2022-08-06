#pragma once

#include <memory>

#include "runtime/function/render/rhi/d3d12/d3d12_device.h"
#include "runtime/function/render/rhi/d3d12/d3d12_swapChain.h"
#include "runtime/function/render/rhi/shader_compiler.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/function/render/render_resource_base.h"
#include "runtime/function/ui/window_ui.h"

namespace Pilot
{
    class WindowSystem;
    class RendererPresent;
    class RenderResourceBase;
    class Renderer;

    struct RendererManagerInitInfo
    {
        RHI::DeviceOptions            Options;
        std::shared_ptr<WindowSystem> Window_system;
    };

    class RendererManager
    {
    public:
        RendererManager();
        ~RendererManager();

        void Initialize(RendererManagerInitInfo initialize_info);
        void InitRenderer();
        void InitUIRenderer(WindowUI* window_ui);
        void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource);
        void Tick();

        RHI::D3D12Device* GetDevice();
        
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

    struct RendererInitParams
    {
        RHI::D3D12Device*    device;
        ShaderCompiler*      compiler;
        RHI::D3D12SwapChain* swapChain;
        WindowSystem*        windowSystem;
    };

    class Renderer
    {
    public:
        Renderer(RendererInitParams renderer_init_info);

        virtual void Initialize();
        virtual void InitializeUIRenderBackend(WindowUI* window_ui);
        virtual void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource);

        virtual ~Renderer();

        virtual void OnRender(RHI::D3D12CommandContext& Context);

        [[nodiscard]] void* GetViewportPtr() const { return Viewport; }

    protected:
        RHI::D3D12Device*    device        = nullptr;
        ShaderCompiler*      compiler      = nullptr;
        RHI::D3D12SwapChain* swapChain     = nullptr;
        WindowSystem*        windowsSystem = nullptr;

        RHI::RenderGraphAllocator renderGraphAllocator;
        RHI::RenderGraphRegistry  renderGraphRegistry;

        size_t FrameIndex = 0;

        void* Viewport = nullptr;
    };

}