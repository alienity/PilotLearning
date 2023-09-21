#include "renderer_moyu.h"
#include "window_system.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "runtime/function/render/renderer/renderer_deferred.h"

namespace MoYu
{
    RendererManager::RendererManager() {}

    RendererManager::~RendererManager()
    {
        p_Device->WaitIdle();

        p_MoYuRenderer = nullptr;

        p_Compiler   = nullptr;
        p_SwapChain = nullptr;
        p_Device     = nullptr;
    }

    void RendererManager::Initialize(RendererManagerInitInfo initialize_info)
    {
        p_Compiler = std::make_unique<ShaderCompiler>();
        p_Device   = std::make_unique<RHI::D3D12Device>(initialize_info.Options);

        if (p_Device->SupportsDynamicResources())
        {
            p_Compiler->SetShaderModel(RHI_SHADER_MODEL::ShaderModel_6_6);
        }

        p_WinSystem = initialize_info.Window_system;

        auto wh = p_WinSystem->getWindowSize();

        HWND win32handle = glfwGetWin32Window(p_WinSystem->getWindow());
        p_SwapChain      = std::make_unique<RHI::D3D12SwapChain>(p_Device.get(), win32handle, wh[0], wh[1]);
    }

    void RendererManager::InitRenderer()
    {
        RendererInitParams rendererInitParams = {
            p_Device.get(), p_Compiler.get(), p_SwapChain.get(), p_WinSystem.get()};

        p_MoYuRenderer = std::make_unique<DeferredRenderer>(rendererInitParams);
        p_MoYuRenderer->Initialize();
    }

    void RendererManager::InitUIRenderer(WindowUI* window_ui)
    {
        p_MoYuRenderer->InitializeUIRenderBackend(window_ui);
    }

    void RendererManager::PreparePassData(std::shared_ptr<RenderResource> render_resource)
    {
        p_MoYuRenderer->PreparePassData(render_resource);
    }


    void RendererManager::Tick(double deltaTime)
    {
        p_Device->OnBeginFrame();
        {
            p_MoYuRenderer->PreRender(deltaTime);
        }
        {
            RHI::D3D12CommandContext* pContext = p_Device->GetLinkedDevice()->GetCommandContext();

            pContext->Open();
            {
                p_MoYuRenderer->OnRender(pContext);
            }
            pContext->Close();

            RendererPresent mPresent(pContext);
            p_SwapChain->Present(true, mPresent);

        }
        {
            p_MoYuRenderer->PostRender(deltaTime);
        }
        p_Device->GetLinkedDevice()->Release(p_SwapChain->GetSyncHandle());

        p_Device->OnEndFrame();
    }

    RHI::D3D12Device* RendererManager::GetDevice() { return p_Device.get(); }

    Renderer::Renderer(RendererInitParams renderer_init_info) :
        pDevice(renderer_init_info.pDevice), pCompiler(renderer_init_info.pCompiler),
        pSwapChain(renderer_init_info.pSwapChain), renderGraphAllocator(65536),
        pWindowSystem(renderer_init_info.pWindowSystem)
    {}

    void Renderer::Initialize() {}
    void Renderer::InitializeUIRenderBackend(WindowUI* window_ui) {}
    void Renderer::PreparePassData(std::shared_ptr<RenderResource> render_resource) {}

    Renderer::~Renderer() {}

    void Renderer::OnRender(RHI::D3D12CommandContext* pContext) {}

    void Renderer::PreRender(double deltaTime) {}
    void Renderer::PostRender(double deltaTime) {}

} // namespace MoYu
