#include "renderer_moyu.h"
#include "window_system.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

namespace Pilot
{
    Renderer::Renderer() : Allocator(65536) {}

    Renderer::~Renderer()
    {
        Device->WaitIdle();
    }

    void Renderer::Initialize(RendererInitInfo initialize_info)
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

    void Renderer::Tick()
    {
        RHI::D3D12CommandContext& Context = Device->GetLinkedDevice()->GetCommandContext();
        
        Device->OnBeginFrame();
        
        Context.Open();

        Context.Close();

        ++FrameIndex;

        RendererPresent Present(Context);
        SwapChain->Present(true, Present);
        Device->OnEndFrame();
    }

    void Renderer::Shutdown()
    {

    }


} // namespace Pilot
