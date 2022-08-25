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

        InitGlobalBuffer();

        // 初始化
        RenderPassCommonInfo renderPassCommonInfo = {
            &renderGraphAllocator, &renderGraphRegistry, device, windowsSystem};

        mIndirectCullPass = std::make_shared<IndirectCullPass>();
        mIndirectCullPass->setCommonInfo(renderPassCommonInfo);
        mIndirectCullPass->initialize({});

        mIndirectDrawPass = std::make_shared<IndirectDrawPass>();
        mIndirectDrawPass->setCommonInfo(renderPassCommonInfo);
        mIndirectDrawPass->initialize({});

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
        pMeshBuffer     = std::make_shared<RHI::D3D12Buffer>(device->GetLinkedDevice(),
                                                         sizeof(HLSL::MeshInstance) * HLSL::MeshLimit,
                                                         sizeof(HLSL::MeshInstance),
                                                         D3D12_HEAP_TYPE_DEFAULT,
                                                         D3D12_RESOURCE_FLAG_NONE);

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

        auto backBufDesc   = backBufferResource.BackBuffer->GetDesc();
        int  backBufWidth  = backBufDesc.Width;
        int  backBufHeight = backBufDesc.Height;

        RHI::RenderGraph graph(renderGraphAllocator, renderGraphRegistry);

        RHI::RgResourceHandle backBufColor = graph.Import(backBufferResource.BackBuffer);

        {
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

        if (mIndirectDrawPass != nullptr)
        {
            IndirectDrawPass::DrawInputParameters  mDrawIntputParams;
            IndirectDrawPass::DrawOutputParameters mDrawOutputParams;
            mDrawIntputParams.numMeshes = numMeshes;
            mDrawIntputParams.commandBufferCounterOffset = commandBufferCounterOffset;
            mDrawIntputParams.pPerframeBuffer            = pPerframeBuffer;
            mDrawIntputParams.pMeshBuffer                = pMeshBuffer;
            mDrawIntputParams.p_IndirectCommandBuffer    = p_IndirectCommandBuffer;

            mDrawOutputParams.backBufColor = backBufColor;
            mDrawOutputParams.backBufRtv   = backBufferResource.RtView;

            mIndirectDrawPass->update(context, graph, mDrawIntputParams, mDrawOutputParams);
        }

        if (mUIPass != nullptr)
        {
            UIPass::UIInputParameters mUIIntputParams;
            UIPass::UIOutputParameters mUIOutputParams;
            mUIOutputParams.backBufColor = backBufColor;
            mUIOutputParams.backBufRtv   = backBufferResource.RtView;
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

