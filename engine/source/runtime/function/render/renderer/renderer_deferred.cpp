#include "renderer_deferred.h"

#include "runtime/function/render/renderer/renderer_registry.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    DeferredRenderer::DeferredRenderer(RendererInitParams& renderInitParams) : Renderer(renderInitParams) {}

    void DeferredRenderer::Initialize()
    {
        RHI::D3D12Texture* pBackBufferResource = pSwapChain->GetCurrentBackBufferResource();
        
        CD3DX12_RESOURCE_DESC backDesc = pBackBufferResource->GetDesc();

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
        Shaders::Compile(pCompiler, shaderRootPath);
        RootSignatures::Compile(pDevice);
        CommandSignatures::Compile(pDevice);
        PipelineStates::Compile(pipleineColorFormat, pipleineDepthFormat, backBufferFormat, depthBufferFormat, pDevice);

        InitGlobalBuffer();
        InitPass();
    }

    void DeferredRenderer::InitGlobalBuffer()
    {
        const FLOAT         renderTargetClearColor[4] = {0, 0, 0, 0};
        CD3DX12_CLEAR_VALUE renderTargetClearValue    = CD3DX12_CLEAR_VALUE(backBufferFormat, renderTargetClearColor);

        p_RenderTargetTex = RHI::D3D12Texture::Create2D(pDevice->GetLinkedDevice(),
                                                        viewport.width,
                                                        viewport.height,
                                                        1,
                                                        backBufferFormat,
                                                        RHI::RHISurfaceCreateRenderTarget,
                                                        1,
                                                        L"RenderTargetTexture",
                                                        D3D12_RESOURCE_STATE_COMMON,
                                                        renderTargetClearValue);
    }

    void DeferredRenderer::InitPass()
    {
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, pDevice, pWindowSystem};

        // Prepare common resources
        const FLOAT renderTargetClearColor[4] = {0, 0, 0, 0};
        RHI::RgTextureDesc colorTexDesc = RHI::RgTextureDesc("ColorBuffer")
                                              .SetFormat(pipleineColorFormat)
                                              .SetExtent(viewport.width, viewport.height)
                                              .SetAllowRenderTarget()
                                              .SetClearValue(renderTargetClearColor);
        RHI::RgTextureDesc depthTexDesc = RHI::RgTextureDesc("DepthBuffer")
                                              .SetFormat(pipleineDepthFormat)
                                              .SetExtent(viewport.width, viewport.height)
                                              .SetAllowDepthStencil()
                                              .SetClearValue(RHI::RgClearValue(0.0f, 0xff));

        // Cull pass
        {
            mIndirectCullPass = std::make_shared<IndirectCullPass>();
            mIndirectCullPass->setCommonInfo(renderPassCommonInfo);
            mIndirectCullPass->initialize({});
        }
        // Opaque drawing pass
        {
            IndirectDrawPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;

            mIndirectOpaqueDrawPass = std::make_shared<IndirectDrawPass>();
            mIndirectOpaqueDrawPass->setCommonInfo(renderPassCommonInfo);
            mIndirectOpaqueDrawPass->initialize(drawPassInit);
        }
        // Transparent drawing pass
        {
            IndirectDrawTransparentPass::DrawPassInitInfo drawPassInit;
            drawPassInit.colorTexDesc = colorTexDesc;
            drawPassInit.depthTexDesc = depthTexDesc;

            mIndirectTransparentDrawPass = std::make_shared<IndirectDrawTransparentPass>();
            mIndirectTransparentDrawPass->setCommonInfo(renderPassCommonInfo);
            mIndirectTransparentDrawPass->initialize(drawPassInit);
        }
        {
            IndirectShadowPass::ShadowPassInitInfo shadowPassInit;

            mIndirectShadowPass = std::make_shared<IndirectShadowPass>();
            mIndirectShadowPass->setCommonInfo(renderPassCommonInfo);
            mIndirectShadowPass->initialize(shadowPassInit);
        }
        // UI drawing pass
        {
            mDisplayPass = std::make_shared<DisplayPass>();
        }
    }

    void DeferredRenderer::InitializeUIRenderBackend(WindowUI* window_ui)
    {
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, pDevice, pWindowSystem};

        mUIPass = std::make_shared<UIPass>();
        mUIPass->setCommonInfo(renderPassCommonInfo);
        UIPass::UIPassInitInfo uiPassInitInfo;
        uiPassInitInfo.window_ui = window_ui;
        mUIPass->initialize(uiPassInitInfo);

        window_ui->setGameView(p_RenderTargetTex->GetDefaultSRV()->GetGpuHandle(), backBufferWidth, backBufferHeight);
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
        mIndirectOpaqueDrawPass = nullptr;
        mIndirectTransparentDrawPass = nullptr;
    }

    void DeferredRenderer::OnRender(RHI::D3D12CommandContext* context)
    {
        IndirectCullPass::IndirectCullOutput indirectCullOutput;
        mIndirectCullPass->cullMeshs(context, &renderGraphRegistry, indirectCullOutput);

        RHI::D3D12Texture* pBackBufferResource = pSwapChain->GetCurrentBackBufferResource();

        RHI::RenderGraph graph(renderGraphAllocator, renderGraphRegistry);

        // backbuffer output
        RHI::RgResourceHandle backBufColorHandle    = graph.Import(pBackBufferResource);

        // game view output
        RHI::RgResourceHandle renderTargetColorHandle = graph.Import(p_RenderTargetTex.get());
        
        // indirect draw shadow
        IndirectShadowPass::ShadowInputParameters  mShadowmapIntputParams;
        IndirectShadowPass::ShadowOutputParameters mShadowmapOutputParams;

        mShadowmapIntputParams.pPerframeBuffer            = indirectCullOutput.pPerframeBuffer;
        mShadowmapIntputParams.pMeshBuffer                = indirectCullOutput.pMeshBuffer;
        mShadowmapIntputParams.pMaterialBuffer            = indirectCullOutput.pMaterialBuffer;
        mShadowmapIntputParams.p_DirectionalCommandBuffer = indirectCullOutput.p_DirShadowmapCommandBuffer;
        mShadowmapIntputParams.p_SpotCommandBuffer        = indirectCullOutput.p_SpotShadowmapCommandBuffers;
        mIndirectShadowPass->update(graph, mShadowmapIntputParams, mShadowmapOutputParams);


        // indirect opaque draw
        IndirectDrawPass::DrawInputParameters  mDrawIntputParams;
        IndirectDrawPass::DrawOutputParameters mDrawOutputParams;

        mDrawIntputParams.pPerframeBuffer               = indirectCullOutput.pPerframeBuffer;
        mDrawIntputParams.pMeshBuffer                   = indirectCullOutput.pMeshBuffer;
        mDrawIntputParams.pMaterialBuffer               = indirectCullOutput.pMaterialBuffer;
        mDrawIntputParams.pIndirectCommandBuffer        = indirectCullOutput.p_OpaqueDrawCommandBuffer;
        mDrawIntputParams.directionalShadowmapTexHandle = mShadowmapOutputParams.directionalShadowmapRGHandle.shadowmapTextureHandle;
        for (size_t i = 0; i < mShadowmapOutputParams.spotShadowmapRGHandle.size(); i++)
        {
            mDrawIntputParams.spotShadowmapTexHandles.push_back(mShadowmapOutputParams.spotShadowmapRGHandle[i].shadowmapTextureHandle);
        }
        mIndirectOpaqueDrawPass->update(graph, mDrawIntputParams, mDrawOutputParams);
        

        // indirect transparent draw
        IndirectDrawTransparentPass::DrawInputParameters  mDrawTransIntputParams;
        IndirectDrawTransparentPass::DrawOutputParameters mDrawTransOutputParams;

        mDrawTransIntputParams.pPerframeBuffer   = indirectCullOutput.pPerframeBuffer;
        mDrawTransIntputParams.pMeshBuffer       = indirectCullOutput.pMeshBuffer;
        mDrawTransIntputParams.pMaterialBuffer   = indirectCullOutput.pMaterialBuffer;
        mDrawTransIntputParams.pIndirectCommandBuffer = indirectCullOutput.p_TransparentDrawCommandBuffer;
        mDrawTransIntputParams.directionalShadowmapTexHandle = mShadowmapOutputParams.directionalShadowmapRGHandle.shadowmapTextureHandle;
        for (size_t i = 0; i < mShadowmapOutputParams.spotShadowmapRGHandle.size(); i++)
        {
            mDrawTransIntputParams.spotShadowmapTexHandles.push_back(mShadowmapOutputParams.spotShadowmapRGHandle[i].shadowmapTextureHandle);
        }
        mDrawTransOutputParams.renderTargetColorHandle = mDrawOutputParams.renderTargetColorHandle;
        mDrawTransOutputParams.renderTargetDepthHandle = mDrawOutputParams.renderTargetDepthHandle;
        mIndirectTransparentDrawPass->update(graph, mDrawTransIntputParams, mDrawTransOutputParams);


        // display
        DisplayPass::DisplayInputParameters  mDisplayIntputParams;
        DisplayPass::DisplayOutputParameters mDisplayOutputParams;

        mDisplayIntputParams.inputRTColorHandle      = mDrawOutputParams.renderTargetColorHandle;
        mDisplayOutputParams.renderTargetColorHandle = renderTargetColorHandle;

        mDisplayPass->update(graph, mDisplayIntputParams, mDisplayOutputParams);

        if (mUIPass != nullptr)
        {
            UIPass::UIInputParameters mUIIntputParams;
            UIPass::UIOutputParameters mUIOutputParams;

            mUIIntputParams.renderTargetColorHandle = mDisplayOutputParams.renderTargetColorHandle;

            mUIOutputParams.backBufColorHandle = backBufColorHandle;
            
            mUIPass->update(graph, mUIIntputParams, mUIOutputParams);
        }

        graph.Execute(context);

        {
            // Transfer the state of the backbuffer to Present
            context->TransitionBarrier(pBackBufferResource,
                                      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT,
                                      D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                      true);
        }


    }

}

