#include "runtime/function/render/renderer/ui_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/window_system.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include "runtime/function/ui/window_ui.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_dx12.h>

#include <cassert>

namespace Pilot
{
    void UIPass::initialize(const RenderPassInitInfo& init_info)
    {
        UIPassInitInfo* m_UIPassInitInfo = (UIPassInitInfo*)(&init_info);
        initializeUIRenderBackend(m_UIPassInitInfo->window_ui);
    }

    void UIPass::initializeUIRenderBackend(WindowUI* window_ui)
    {
        m_window_ui = window_ui;

        pD3D12SRVDescriptor =
            std::make_shared<RHI::D3D12DynamicDescriptor<D3D12_SHADER_RESOURCE_VIEW_DESC>>(m_Device->GetLinkedDevice());

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOther(m_WindowSystem->getWindow(), true);
        ImGui_ImplDX12_Init(m_Device->GetD3D12Device(),
                            RHI::D3D12SwapChain::BackBufferCount,
                            RHI::D3D12SwapChain::Format,
                            m_Device->GetLinkedDevice()->GetDescriptorHeap<D3D12_SHADER_RESOURCE_VIEW_DESC>(),
                            pD3D12SRVDescriptor->GetCpuHandle(),
                            pD3D12SRVDescriptor->GetGpuHandle());


    }

    void UIPass::update(RHI::D3D12CommandContext& context,
                        RHI::RenderGraph&         graph,
                        PassInput&                passInput,
                        PassOutput&               passOutput)
    {
        UIInputParameters*  uiPassInput = (UIInputParameters*)(&passInput);
        UIOutputParameters* uiPassOutput = (UIOutputParameters*)(&passOutput);

        RHI::RgResourceHandle       inputBufColor = uiPassInput->backBufColor;
        RHI::RgResourceHandle       backBufColor  = uiPassOutput->backBufColor;
        RHI::D3D12RenderTargetView* backBufRTV    = uiPassOutput->backBufRtv;

        graph.AddRenderPass("UIPass")
            .Read(inputBufColor)
            .Write(&backBufColor)
            .Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {
                RHI::D3D12Texture*  backBufTex    = registry.GetImportedResource(backBufColor);
                D3D12_RESOURCE_DESC backBufDesc   = backBufTex->GetDesc();
                int                 backBufWidth  = backBufDesc.Width;
                int                 backBufHeight = backBufDesc.Height;

                draw(context, backBufRTV, backBufWidth, backBufHeight);
            });
    }

    void UIPass::draw(RHI::D3D12CommandContext&   context,
                      RHI::D3D12RenderTargetView* pRTV,
                      int                         backBufWidth,
                      int                         backBufHeight)
    {
        if (m_window_ui)
        {
            context.SetViewport(RHIViewport {0.0f, 0.0f, (float)backBufWidth, (float)backBufHeight, 0.0f, 1.0f});
            context.SetScissorRect(RHIRect {0, 0, backBufWidth, backBufHeight});
            context.SetRenderTarget(pRTV, nullptr);

            // Start the Dear ImGui frame
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            m_window_ui->preRender();

            // Rendering
            ImGui::EndFrame();
            ImGui::Render();

            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context.GetGraphicsCommandList());
        }
    }

    void UIPass::destroy()
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui_ImplDX12_Shutdown();
        ImGui::DestroyContext();
    }

}
