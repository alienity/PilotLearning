#include "runtime/function/render/renderer/ui_pass.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/ui/window_ui.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_dx12.h>

#include <cassert>

namespace Pilot
{
    void UIPass::initialize(const UIPassInitInfo& init_info)
    {
        window_ui = init_info.window_ui;

        pD3D12SRVDescriptor =
            std::make_shared<RHI::D3D12Descriptor<D3D12_SHADER_RESOURCE_VIEW_DESC>>(m_Device->GetLinkedDevice());

        ID3D12DescriptorHeap* pDescriptorHeap =
            m_Device->GetLinkedDevice()->GetDescriptorHeap<D3D12_SHADER_RESOURCE_VIEW_DESC>()->GetDescriptorHeap();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOther(m_WindowSystem->getWindow(), true);
        ImGui_ImplDX12_Init(m_Device->GetD3D12Device(),
                            RHI::D3D12SwapChain::BackBufferCount,
                            RHI::D3D12SwapChain::Format,
                            pDescriptorHeap,
                            pD3D12SRVDescriptor->GetCpuHandle(),
                            pD3D12SRVDescriptor->GetGpuHandle());
    }

    void UIPass::update(RHI::RenderGraph& graph, UIInputParameters& passInput, UIOutputParameters& passOutput)
    {
        RHI::RgResourceHandle backBufColorHandle = passOutput.backBufColorHandle;
        
        graph.AddRenderPass("UIPass")
            .Read(passInput.renderTargetColorHandle)
            .Write(passOutput.backBufColorHandle)
            .Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

                RHI::D3D12GraphicsContext* graphicsContext = context->GetGraphicsContext();

                RHI::D3D12Texture* pBackBufColorTex = registry->GetD3D12Texture(backBufColorHandle);
                RHI::D3D12RenderTargetView* backBufColorRTV  = pBackBufColorTex->GetDefaultRTV().get();

                CD3DX12_RESOURCE_DESC backBufDesc = pBackBufColorTex->GetDesc();

                int backBufWidth  = backBufDesc.Width;
                int backBufHeight = backBufDesc.Height;

                RHIViewport viewport = {0.0f, 0.0f, (float)backBufWidth, (float)backBufHeight, 0.0f, 1.0f};

                graphicsContext->SetViewport(viewport);
                graphicsContext->SetScissorRect(RHIRect {0, 0, backBufWidth, backBufHeight});

                graphicsContext->ClearRenderTarget(backBufColorRTV, nullptr);
                graphicsContext->SetRenderTarget(backBufColorRTV, nullptr);

                ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphicsContext->GetGraphicsCommandList());
            });
    }

    void UIPass::destroy()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

}
