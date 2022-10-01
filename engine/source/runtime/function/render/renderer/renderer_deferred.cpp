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
        
        D3D12_RESOURCE_DESC backDesc = backBufferResource.BackBuffer->GetDesc();

        backBufferWidth   = backDesc.Width;
        backBufferHeight  = backDesc.Height;
        backBufferFormat  = backDesc.Format;
        depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

        SetViewPort(0, 0, backBufferWidth, backBufferHeight);

        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        const std::filesystem::path&   shaderRootPath = config_manager->getShaderFolder();

        // 初始化全局对象
        Shaders::Compile(compiler, shaderRootPath);
        RootSignatures::Compile(device, renderGraphRegistry);
        CommandSignatures::Compile(device, renderGraphRegistry);
        PipelineStates::Compile(backBufferFormat, depthBufferFormat, device, renderGraphRegistry);

        InitGlobalBuffer();
        InitPass();
    }

    void DeferredRenderer::InitGlobalBuffer()
    {
        p_IndirectCommandBuffer    = std::make_shared<RHI::D3D12Buffer>(device->GetLinkedDevice(),
                                                                     commandBufferCounterOffset + sizeof(uint64_t),
                                                                     sizeof(HLSL::CommandSignatureParams),
                                                                     D3D12_HEAP_TYPE_DEFAULT,
                                                                     D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        p_IndirectCommandBufferUav = std::make_shared<RHI::D3D12UnorderedAccessView>(
            device->GetLinkedDevice(), p_IndirectCommandBuffer.get(), HLSL::MeshLimit, commandBufferCounterOffset);

        pPerframeBuffer = std::make_shared<RHI::D3D12Buffer>(device->GetLinkedDevice(),
                                                             sizeof(HLSL::MeshPerframeStorageBufferObject),
                                                             sizeof(HLSL::MeshPerframeStorageBufferObject),
                                                             D3D12_HEAP_TYPE_DEFAULT,
                                                             D3D12_RESOURCE_FLAG_NONE);
        
        pMaterialBuffer = std::make_shared<RHI::D3D12Buffer>(device->GetLinkedDevice(),
                                                             sizeof(HLSL::MaterialInstance) * HLSL::MaterialLimit,
                                                             sizeof(HLSL::MaterialInstance),
                                                             D3D12_HEAP_TYPE_DEFAULT,
                                                             D3D12_RESOURCE_FLAG_NONE);

        pMeshBuffer = std::make_shared<RHI::D3D12Buffer>(device->GetLinkedDevice(),
                                                         sizeof(HLSL::MeshInstance) * HLSL::MeshLimit,
                                                         sizeof(HLSL::MeshInstance),
                                                         D3D12_HEAP_TYPE_DEFAULT,
                                                         D3D12_RESOURCE_FLAG_NONE);

        CD3DX12_RESOURCE_DESC renderTargetBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            backBufferFormat, viewport.width, viewport.height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        const FLOAT         renderTargetClearColor[4] = {0, 0, 0, 0};
        CD3DX12_CLEAR_VALUE renderTargetClearValue    = CD3DX12_CLEAR_VALUE(backBufferFormat, renderTargetClearColor);

        p_RenderTargetTex = std::make_shared<RHI::D3D12Texture>(device->GetLinkedDevice(), renderTargetBufferDesc, renderTargetClearValue);
        p_RenderTargetTexSRV = std::make_shared<RHI::D3D12ShaderResourceView>(device->GetLinkedDevice(), p_RenderTargetTex.get(), false, std::nullopt, std::nullopt);
        p_RenderTargetTexRTV = std::make_shared<RHI::D3D12RenderTargetView>(device->GetLinkedDevice(), p_RenderTargetTex.get());

    }

    void DeferredRenderer::InitPass()
    {
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, device, windowsSystem};

        {
            mIndirectCullPass = std::make_shared<IndirectCullPass>();
            mIndirectCullPass->setCommonInfo(renderPassCommonInfo);
            mIndirectCullPass->initialize({});
        }

        {
            IndirectDrawPass::DrawPassInitInfo drawPassInit;

            const D3D12_RESOURCE_DESC colorBufferDesc = p_RenderTargetTex->GetDesc();
            const D3D12_CLEAR_VALUE   colorBufferClearVal = p_RenderTargetTex->GetClearValue();

            drawPassInit.colorTexDesc = RHI::RgTextureDesc("ColorBuffer")
                                            .SetFormat(colorBufferDesc.Format)
                                            .SetExtent(colorBufferDesc.Width, colorBufferDesc.Height)
                                            .SetAllowRenderTarget()
                                            .SetClearValue(colorBufferClearVal.Color);

            drawPassInit.depthTexDesc = RHI::RgTextureDesc("DepthBuffer")
                                                  .SetFormat(depthBufferFormat)
                                                  .SetExtent(backBufferWidth, backBufferHeight)
                                                  .SetAllowDepthStencil()
                                                  .SetClearValue(RHI::RgClearValue(0.0f, 0xff));

            mIndirectDrawPass = std::make_shared<IndirectDrawPass>();
            mIndirectDrawPass->setCommonInfo(renderPassCommonInfo);
            mIndirectDrawPass->initialize(drawPassInit);
        }

        {
            mDisplayPass = std::make_shared<DisplayPass>();
        }
    }

    void DeferredRenderer::InitializeUIRenderBackend(WindowUI* window_ui)
    {
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, device, windowsSystem};

        mUIPass = std::make_shared<UIPass>();
        mUIPass->setCommonInfo(renderPassCommonInfo);
        UIPass::UIPassInitInfo uiPassInitInfo;
        uiPassInitInfo.window_ui = window_ui;
        mUIPass->initialize(uiPassInitInfo);

        window_ui->setGameView(p_RenderTargetTexSRV->GetGpuHandle(), backBufferWidth, backBufferHeight);
    }

    void DeferredRenderer::PreparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        mIndirectCullPass->prepareMeshData(render_resource, numMeshes);
    }

    DeferredRenderer::~DeferredRenderer() 
    {
        mUIPass = nullptr;
        mIndirectCullPass = nullptr;
        mIndirectDrawPass = nullptr;
    }

    void DeferredRenderer::OnRender(RHI::D3D12CommandContext& context)
    {
        IndirectCullPass::IndirectCullParams indirectCullParams;
        indirectCullParams.numMeshes                  = numMeshes;
        indirectCullParams.commandBufferCounterOffset = commandBufferCounterOffset;
        indirectCullParams.pPerframeBuffer            = pPerframeBuffer;
        indirectCullParams.pMaterialBuffer            = pMaterialBuffer;
        indirectCullParams.pMeshBuffer                = pMeshBuffer;
        indirectCullParams.p_IndirectCommandBuffer    = p_IndirectCommandBuffer;
        indirectCullParams.p_IndirectCommandBufferUav = p_IndirectCommandBufferUav;
        mIndirectCullPass->cullMeshs(context, renderGraphRegistry, indirectCullParams);

        RHI::D3D12SwapChainResource backBufferResource = swapChain->GetCurrentBackBufferResource();

        RHI::RenderGraph graph(renderGraphAllocator, renderGraphRegistry);

        // backbuffer output
        RHI::RgResourceHandle backBufColorHandle    = graph.Import(backBufferResource.BackBuffer);
        RHI::RgResourceHandle backBufColorRTVHandle = graph.Import(backBufferResource.RtView);

        // game view output
        RHI::RgResourceHandle renderTargetColorHandle = graph.Import(p_RenderTargetTex.get());
        RHI::RgResourceHandle renderTargetColorRTVHandle = graph.Import(p_RenderTargetTexRTV.get());


        /*
        {
            auto backBufDesc   = backBufferResource.BackBuffer->GetDesc();
            int  backBufWidth  = backBufDesc.Width;
            int  backBufHeight = backBufDesc.Height;

            graph.AddRenderPass("DisplayTest")
                .Write(&backBufColor)
                .Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {
                    context.SetGraphicsRootSignature(registry.GetRootSignature(RootSignatures::FullScreenPresent));
                    context.SetPipelineState(registry.GetPipelineState(PipelineStates::FullScreenPresent));
                    context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    context.SetViewport(RHIViewport {0, 0, (float)backBufWidth, (float)backBufHeight, 0, 1});
                    context.SetScissorRect(RHIRect {0, 0, (long)backBufWidth, (long)backBufHeight});
                    context.ClearRenderTarget(backBufferResource.RtView, nullptr);
                    context.SetRenderTarget(backBufferResource.RtView, nullptr);
                    context->DrawInstanced(3, 1, 0, 0);
                });
        }
        */

        // indirect draw
        IndirectDrawPass::DrawInputParameters  mDrawIntputParams;
        IndirectDrawPass::DrawOutputParameters mDrawOutputParams;

        mDrawIntputParams.commandBufferCounterOffset = commandBufferCounterOffset;
        mDrawIntputParams.pPerframeBuffer            = pPerframeBuffer;
        mDrawIntputParams.pMeshBuffer                = pMeshBuffer;
        mDrawIntputParams.pMaterialBuffer            = pMaterialBuffer;
        mDrawIntputParams.pIndirectCommandBuffer     = p_IndirectCommandBuffer;

        mIndirectDrawPass->update(context, graph, mDrawIntputParams, mDrawOutputParams);
        
        // display
        DisplayPass::DisplayInputParameters  mDisplayIntputParams;
        DisplayPass::DisplayOutputParameters mDisplayOutputParams;

        mDisplayIntputParams.inputRTColorHandle    = mDrawOutputParams.renderTargetColorHandle;
        mDisplayIntputParams.inputRTColorSRVHandle = mDrawOutputParams.renderTargetColorSRVHandle;

        mDisplayOutputParams.renderTargetColorHandle    = renderTargetColorHandle;
        mDisplayOutputParams.renderTargetColorRTVHandle = renderTargetColorRTVHandle;

        mDisplayPass->update(context, graph, mDisplayIntputParams, mDisplayOutputParams);

        if (mUIPass != nullptr)
        {
            UIPass::UIInputParameters mUIIntputParams;
            UIPass::UIOutputParameters mUIOutputParams;

            mUIIntputParams.renderTargetColorHandle = mDisplayOutputParams.renderTargetColorHandle;

            mUIOutputParams.backBufColorHandle    = backBufColorHandle;
            mUIOutputParams.backBufColorRTVHandle = backBufColorRTVHandle;

            mUIPass->update(context, graph, mUIIntputParams, mUIOutputParams);
        }

        graph.Execute(context);

        {
            // Transfer the state of the backbuffer to Present
            context.TransitionBarrier(backBufferResource.BackBuffer,
                                      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT);
        }


    }

}

