#include "runtime/function/render/renderer/tool_pass.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"

namespace MoYu
{
    void ToolPass::initialize(const ToolPassInitInfo& init_info)
    {
        //pD3D12SRVDescriptor =
        //    std::make_shared<RHI::D3D12Descriptor<D3D12_SHADER_RESOURCE_VIEW_DESC>>(m_Device->GetLinkedDevice());

        //ID3D12DescriptorHeap* pDescriptorHeap =
        //    m_Device->GetLinkedDevice()->GetDescriptorHeap<D3D12_SHADER_RESOURCE_VIEW_DESC>()->GetDescriptorHeap();




    }

    void ToolPass::editorUpdate(RHI::D3D12CommandContext* context, ToolInputParameters& passInput, ToolOutputParameters& passOutput)
    {
        if (p_DFG == nullptr)
        {
            p_DFG = RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                512,
                                                512,
                                                1,
                                                DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                RHI::RHISurfaceCreateFlags::RHISurfaceCreateRandomWrite,
                                                1,
                                                L"DFG");
        }





    }

    void ToolPass::update(RHI::RenderGraph& graph, ToolInputParameters& passInput, ToolOutputParameters& passOutput)
    {
        /*
        RHI::RgResourceHandle backBufColorHandle = passOutput.backBufColorHandle;
        
        graph.AddRenderPass("ToolPass")
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


            });
        */



    }

    void ToolPass::destroy()
    {
        p_DFG = nullptr;


    }

}
