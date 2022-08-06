#include "renderer_deferred.h"

#include "runtime/function/render/renderer/renderer_registry.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    DeferredRenderer::DeferredRenderer(RendererInitParams& renderInitParams) : Renderer(renderInitParams) {}

    void DeferredRenderer::Initialize()
    {
        RHI::D3D12SwapChainResource backBufferResource = swapChain->GetCurrentBackBufferResource();
        DXGI_FORMAT                 backBufferFormat   = backBufferResource.BackBuffer->GetDesc().Format;
        DXGI_FORMAT                 depthBufferFormat  = DXGI_FORMAT_D32_FLOAT;

        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        const std::filesystem::path&   shaderRootPath = config_manager->getShaderFolder();

        // 初始化全局对象
        Shaders::Compile(compiler, shaderRootPath);
        RootSignatures::Compile(device, renderGraphRegistry);
        CommandSignatures::Compile(device, renderGraphRegistry);
        PipelineStates::Compile(backBufferFormat, depthBufferFormat, device, renderGraphRegistry);

        // 初始化
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, device, windowsSystem};

    }

    void DeferredRenderer::InitializeUIRenderBackend(WindowUI* window_ui)
    {
        mUIPass = std::make_shared<UIPass>();

        // 设置通用变量，那我是不是应该直接用静态变量设置一下算了
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, device, windowsSystem};
        mUIPass->setCommonInfo(renderPassCommonInfo);

        // 真正初始化
        UIPass::UIPassInitInfo uiPassInitInfo;
        uiPassInitInfo.window_ui = window_ui;
        mUIPass->initialize(uiPassInitInfo);
    }

    void DeferredRenderer::PreparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {

    }

    DeferredRenderer::~DeferredRenderer() 
    {
        mUIPass = nullptr;
    }

    void DeferredRenderer::OnRender(RHI::D3D12CommandContext& context)
    {
        RHI::D3D12SwapChainResource backBufferResource = swapChain->GetCurrentBackBufferResource();

        auto backBufDesc   = backBufferResource.BackBuffer->GetDesc();
        int  backBufWidth  = backBufDesc.Width;
        int  backBufHeight = backBufDesc.Height;

        RHI::RenderGraph graph(renderGraphAllocator, renderGraphRegistry);

        RHI::RgResourceHandle backBufColor = graph.Import(backBufferResource.BackBuffer);

        {
            graph.AddRenderPass("DisplayTest")
                .Write(&backBufColor)
                .Execute([=](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context) {
                    Context.SetGraphicsRootSignature(Registry.GetRootSignature(RootSignatures::FullScreenPresent));
                    Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::FullScreenPresent));
                    Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    Context.SetViewport(RHIViewport {0, 0, (float)backBufWidth, (float)backBufHeight, 0, 1});
                    Context.SetScissorRect(RHIRect {0, 0, (long)backBufWidth, (long)backBufHeight});
                    Context.ClearRenderTarget(backBufferResource.RtView, nullptr);
                    Context.SetRenderTarget(backBufferResource.RtView, nullptr);
                    Context->DrawInstanced(3, 1, 0, 0);
                });
        }

        if (mUIPass != nullptr)
        {
            UIPass::UIInputParameters mUIIntputParams;
            mUIIntputParams.backBufColor = backBufColor;
            UIPass::UIOutputParameters mUIOutputParams;
            mUIOutputParams.backBufColor = backBufColor;
            mUIOutputParams.backBufRtv   = backBufferResource.RtView;
            mUIPass->update(context, graph, mUIIntputParams, mUIOutputParams);
        }

        graph.Execute(context);
    }

}

