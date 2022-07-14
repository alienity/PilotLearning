#include "renderer_deferred.h"

#include "runtime/function/render/renderer/renderer_registry.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    DeferredRenderer::DeferredRenderer(RHI::D3D12Device*    Device,
                                       ShaderCompiler*      Compiler,
                                       RHI::D3D12SwapChain* SwapChain) :
        Renderer(Device, Compiler, SwapChain)
    {
        RHI::D3D12SwapChainResource backBufferResource = SwapChain->GetCurrentBackBufferResource();
        DXGI_FORMAT                 backBufferFormat   = backBufferResource.BackBuffer->GetDesc().Format;
        DXGI_FORMAT                 depthBufferFormat  = DXGI_FORMAT_D32_FLOAT;

        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        const std::filesystem::path&   shaderRootPath = config_manager->getShaderFolder();

        Shaders::Compile(Compiler, shaderRootPath);
        RootSignatures::Compile(Device, Registry);
        CommandSignatures::Compile(Device, Registry);
        PipelineStates::Compile(backBufferFormat, depthBufferFormat, Device, Registry);
        

    }

    DeferredRenderer::~DeferredRenderer() 
    {

    }

    void DeferredRenderer::OnRender(RHI::D3D12CommandContext& Context)
    {
        RHI::D3D12SwapChainResource backBufferResource = SwapChain->GetCurrentBackBufferResource();

        auto backBufDesc   = backBufferResource.BackBuffer->GetDesc();
        int  backBufWidth  = backBufDesc.Width;
        int  backBufHeight = backBufDesc.Height;

        RHI::RenderGraph Graph(Allocator, Registry);

        struct GBufferParameters
        {
            RHI::RgResourceHandle backBufColor;
        } GBufferArgs;
        
        GBufferArgs.backBufColor = Graph.Import(backBufferResource.BackBuffer);
        
        Graph.AddRenderPass("DisplayTest")
            .Write(&GBufferArgs.backBufColor)
            .Execute([=](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
                {
                    Context.SetGraphicsRootSignature(Registry.GetRootSignature(RootSignatures::FullScreenPresent));
                    Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::FullScreenPresent));
                    Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    Context.SetViewport(RHIViewport {0, 0, (float)backBufWidth, (float)backBufHeight, 0, 1});
                    Context.SetScissorRect(RHIRect {0, 0, (long)backBufWidth, (long)backBufHeight});
                    Context.ClearRenderTarget(backBufferResource.RtView, nullptr);
                    Context.SetRenderTarget(backBufferResource.RtView, nullptr);
                    Context->DrawInstanced(3, 1, 0, 0);
                });

        Graph.Execute(Context);
    }

}

