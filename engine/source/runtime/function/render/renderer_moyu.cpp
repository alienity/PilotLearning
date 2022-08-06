#include "renderer_moyu.h"
#include "window_system.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "runtime/function/render/renderer/renderer_deferred.h"

namespace Pilot
{
    RendererManager::RendererManager() {}

    RendererManager::~RendererManager()
    {
        Device->WaitIdle();
    }

    void RendererManager::Initialize(RendererManagerInitInfo initialize_info)
    {
        Compiler = std::make_unique<ShaderCompiler>();
        Device   = std::make_unique<RHI::D3D12Device>(initialize_info.Options);

        if (Device->SupportsDynamicResources())
        {
            Compiler->SetShaderModel(RHI_SHADER_MODEL::ShaderModel_6_6);
        }

        WinSystem = initialize_info.Window_system;

        HWND win32handle = glfwGetWin32Window(WinSystem->getWindow());
        SwapChain        = std::make_unique<RHI::D3D12SwapChain>(Device.get(), win32handle);
    }

    void RendererManager::InitRenderer()
    {
        RendererInitParams rendererInitParams = {Device.get(), Compiler.get(), SwapChain.get(), WinSystem.get()};

        MoYuRenderer = std::make_unique<DeferredRenderer>(rendererInitParams);
        MoYuRenderer->Initialize();
    }

    void RendererManager::InitUIRenderer(WindowUI* window_ui)
    {
        MoYuRenderer->InitializeUIRenderBackend(window_ui);
    }

    void RendererManager::PreparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        MoYuRenderer->PreparePassData(render_resource);
    }

    void RendererManager::Tick()
    {
        RHI::D3D12CommandContext& Context = Device->GetLinkedDevice()->GetCommandContext();

        Device->OnBeginFrame();

        Context.Open();
        {
            MoYuRenderer->OnRender(Context);
        }
        Context.Close();

        RendererPresent Present(Context);
        SwapChain->Present(true, Present);
        Device->OnEndFrame();
    }

    RHI::D3D12Device* RendererManager::GetDevice() 
    {
        return Device.get();
    }

    Renderer::Renderer(RendererInitParams renderer_init_info) :
        device(renderer_init_info.device), compiler(renderer_init_info.compiler),
        swapChain(renderer_init_info.swapChain), renderGraphAllocator(65536),
        windowsSystem(renderer_init_info.windowSystem)
    {}

    void Renderer::Initialize() {}
    void Renderer::InitializeUIRenderBackend(WindowUI* window_ui) {}
    void Renderer::PreparePassData(std::shared_ptr<RenderResourceBase> render_resource) {}

    Renderer::~Renderer() {}

    void Renderer::OnRender(RHI::D3D12CommandContext& Context) {}

} // namespace Pilot
