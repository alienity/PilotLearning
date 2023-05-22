#pragma once

#include <memory>

#include "runtime/function/render/rhi/d3d12/d3d12_device.h"
#include "runtime/function/render/rhi/d3d12/d3d12_swapChain.h"
#include "runtime/function/render/rhi/shader_compiler.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/function/render/render_resource_base.h"
#include "runtime/function/ui/window_ui.h"

namespace MoYu
{
    class WindowSystem;
    class RendererPresent;
    class RenderResourceBase;
    class Renderer;

    struct EngineContentViewport
    {
        float x {0.f};
        float y {0.f};
        float width {0.f};
        float height {0.f};
    };

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
        
        std::unique_ptr<RHI::D3D12Device>    p_Device;
        std::unique_ptr<ShaderCompiler>      p_Compiler;
        std::unique_ptr<RHI::D3D12SwapChain> p_SwapChain;
        std::shared_ptr<WindowSystem>        p_WinSystem;
        std::unique_ptr<Renderer>            p_MoYuRenderer;
    };

    class RendererPresent : public RHI::IPresent
    {
    public:
        RendererPresent(RHI::D3D12CommandContext* pContext) noexcept : pContext(pContext) {}

        void PrePresent() override { SyncHandle = pContext->Execute(false); }

        void PostPresent() override { SyncHandle.WaitForCompletion(); }

        RHI::D3D12CommandContext* pContext;
        RHI::D3D12SyncHandle      SyncHandle;
    };

    struct RendererInitParams
    {
        RHI::D3D12Device*    pDevice;
        ShaderCompiler*      pCompiler;
        RHI::D3D12SwapChain* pSwapChain;
        WindowSystem*        pWindowSystem;
    };

    class Renderer
    {
    public:
        Renderer(RendererInitParams renderer_init_info);

        virtual void Initialize();
        virtual void InitializeUIRenderBackend(WindowUI* window_ui);
        virtual void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource);

        virtual ~Renderer();

        virtual void OnRender(RHI::D3D12CommandContext* pContext);

        virtual void OnResize() {}

        EngineContentViewport GetViewPort() { return viewport; }

        void SetViewPort(float offset_x, float offset_y, float width, float height)
        {
            viewport = {offset_x, offset_y, width, height};
        }

    protected:
        RHI::D3D12Device*    pDevice       = nullptr;
        ShaderCompiler*      pCompiler     = nullptr;
        RHI::D3D12SwapChain* pSwapChain    = nullptr;
        WindowSystem*        pWindowSystem = nullptr;

        RHI::RenderGraphAllocator renderGraphAllocator;
        RHI::RenderGraphRegistry  renderGraphRegistry;

        size_t frameIndex = 0;
        EngineContentViewport viewport;
    };

}