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

        pipleineColorFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        pipleineDepthFormat = DXGI_FORMAT_D32_FLOAT;

        SetViewPort(0, 0, backBufferWidth, backBufferHeight);

        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        const std::filesystem::path&   shaderRootPath = config_manager->getShaderFolder();

        // 初始化全局对象
        Shaders::Compile(compiler, shaderRootPath);
        RootSignatures::Compile(device, renderGraphRegistry);
        CommandSignatures::Compile(device, renderGraphRegistry);
        PipelineStates::Compile(pipleineColorFormat, pipleineDepthFormat, backBufferFormat, depthBufferFormat, device, renderGraphRegistry);

        InitGlobalBuffer();
        InitPass();
    }

    void DeferredRenderer::InitGlobalBuffer()
    {
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

            const FLOAT renderTargetClearColor[4] = {0, 0, 0, 0};

            drawPassInit.colorTexDesc = RHI::RgTextureDesc("ColorBuffer")
                                            .SetFormat(pipleineColorFormat)
                                            .SetExtent(viewport.width, viewport.height)
                                            .SetAllowRenderTarget()
                                            .SetClearValue(renderTargetClearColor);

            drawPassInit.depthTexDesc = RHI::RgTextureDesc("DepthBuffer")
                                            .SetFormat(pipleineDepthFormat)
                                            .SetExtent(viewport.width, viewport.height)
                                            .SetAllowDepthStencil()
                                            .SetClearValue(RHI::RgClearValue(0.0f, 0xff));

            mIndirectDrawPass = std::make_shared<IndirectDrawPass>();
            mIndirectDrawPass->setCommonInfo(renderPassCommonInfo);
            mIndirectDrawPass->initialize(drawPassInit);
        }

        {
            IndirectShadowPass::ShadowPassInitInfo shadowPassInit;

            mIndirectShadowPass = std::make_shared<IndirectShadowPass>();
            mIndirectShadowPass->setCommonInfo(renderPassCommonInfo);
            mIndirectShadowPass->initialize(shadowPassInit);
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
        mIndirectCullPass->prepareMeshData(render_resource);
        mIndirectShadowPass->prepareShadowmaps(render_resource);
    }

    DeferredRenderer::~DeferredRenderer() 
    {
        mUIPass = nullptr;
        mIndirectCullPass = nullptr;
        mIndirectDrawPass = nullptr;
    }

    void DeferredRenderer::OnRender(RHI::D3D12CommandContext& context)
    {
        IndirectCullPass::IndirectCullOutput indirectCullOutput;
        mIndirectCullPass->cullMeshs(context, renderGraphRegistry, indirectCullOutput);

        RHI::D3D12SwapChainResource backBufferResource = swapChain->GetCurrentBackBufferResource();

        RHI::RenderGraph graph(renderGraphAllocator, renderGraphRegistry);

        // backbuffer output
        RHI::RgResourceHandle backBufColorHandle    = graph.Import(backBufferResource.BackBuffer);
        RHI::RgResourceHandle backBufColorRTVHandle = graph.Import(backBufferResource.RtView);

        // game view output
        RHI::RgResourceHandle renderTargetColorHandle = graph.Import(p_RenderTargetTex.get());
        RHI::RgResourceHandle renderTargetColorRTVHandle = graph.Import(p_RenderTargetTexRTV.get());

        // indirect draw shadow
        IndirectShadowPass::ShadowInputParameters  mShadowmapIntputParams;
        IndirectShadowPass::ShadowOutputParameters mShadowmapOutputParams;

        mShadowmapIntputParams.pPerframeBuffer            = indirectCullOutput.pPerframeBuffer;
        mShadowmapIntputParams.pMeshBuffer                = indirectCullOutput.pMeshBuffer;
        mShadowmapIntputParams.pMaterialBuffer            = indirectCullOutput.pMaterialBuffer;
        mShadowmapIntputParams.p_DirectionalCommandBuffer = indirectCullOutput.p_DirShadowmapCommandBuffer;
        mShadowmapIntputParams.p_SpotCommandBuffer        = indirectCullOutput.p_SpotShadowmapCommandBuffers;

        mIndirectShadowPass->update(context, graph, mShadowmapIntputParams, mShadowmapOutputParams);


        // indirect draw
        IndirectDrawPass::DrawInputParameters  mDrawIntputParams;
        IndirectDrawPass::DrawOutputParameters mDrawOutputParams;

        mDrawIntputParams.pPerframeBuffer        = indirectCullOutput.pPerframeBuffer;
        mDrawIntputParams.pMeshBuffer            = indirectCullOutput.pMeshBuffer;
        mDrawIntputParams.pMaterialBuffer        = indirectCullOutput.pMaterialBuffer;
        mDrawIntputParams.pIndirectCommandBuffer = indirectCullOutput.p_IndirectCommandBuffer;
        mDrawIntputParams.directionalShadowmapTexHandle =
            mShadowmapOutputParams.directionalShadowmapRGHandle.shadowmapTextureHandle;
        for (size_t i = 0; i < mShadowmapOutputParams.spotShadowmapRGHandle.size(); i++)
        {
            mDrawIntputParams.spotShadowmapTexHandles.push_back(
                mShadowmapOutputParams.spotShadowmapRGHandle[i].shadowmapTextureHandle);
        }

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

