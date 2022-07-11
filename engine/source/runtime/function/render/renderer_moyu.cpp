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

    void RendererManager::Initialize(RendererInitInfo initialize_info)
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

        // 目前测试先使用默认的
        MoYuRenderer = std::make_unique<DeferredRenderer>(Device.get(), Compiler.get(), SwapChain.get());
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

    Renderer::Renderer(RHI::D3D12Device* Device, ShaderCompiler* Compiler, RHI::D3D12SwapChain* SwapChain) :
        Device(Device), Compiler(Compiler), SwapChain(SwapChain), Allocator(65536)
    {}

    Renderer::~Renderer() {}

    void Renderer::OnRender(RHI::D3D12CommandContext& Context) {}

} // namespace Pilot
