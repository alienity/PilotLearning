#include "runtime/function/render/renderer/indirect_shadow_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

namespace Pilot
{

    void IndirectShadowPass::initialize(const ShadowPassInitInfo& init_info)
    {
        Vector2 shadowmap_size = m_visiable_nodes.p_directional_light->m_shadowmap_size;

        shadowmapTexDesc = RHI::RgTextureDesc("ShadowmapTex")
                               .SetFormat(DXGI_FORMAT_D32_FLOAT)
                               .SetExtent(shadowmap_size.x, shadowmap_size.y)
                               .SetAllowDepthStencil()
                               .SetClearValue(RHI::RgClearValue(0.0f, 0xff));

    }

    void IndirectShadowPass::prepareShadowmapSRVs(std::shared_ptr<RenderResourceBase> render_resource)
    {
        // 每帧执行，判断是否需要shadowmap，同时创建shadowmaptexture和对应的srv和uav，这些都可以提前创建好，然后把对应的
        // srv的index设置到buffer上，之后等待上传，就可以在shader中采样使用了，但是这样就不能使用rendergraph的自动创建
        // 纹理和释放纹理了，只能在这个pass里面自己释放，但是也没差，挺好的

    }

    void IndirectShadowPass::update(RHI::D3D12CommandContext& context,
                                    RHI::RenderGraph&         graph,
                                    ShadowInputParameters&    passInput,
                                    ShadowOutputParameters&   passOutput)
    {
        ShadowInputParameters*  drawPassInput  = &passInput;
        ShadowOutputParameters* drawPassOutput = &passOutput;

        int commandBufferCounterOffset = drawPassInput->commandBufferCounterOffset;

        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = drawPassInput->pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer     = drawPassInput->pMeshBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer = drawPassInput->pMaterialBuffer;

        std::shared_ptr<RHI::D3D12Buffer> p_IndirectShadowmapCommandBuffer = drawPassInput->p_IndirectShadowmapCommandBuffer;

        Vector2 shadowmap_size  = m_visiable_nodes.p_directional_light->m_shadowmap_size;
        shadowmapTexDesc.Width  = shadowmap_size.x;
        shadowmapTexDesc.Height = shadowmap_size.y; 

        drawPassOutput->shadowmapTextureHandle = graph.Create<RHI::D3D12Texture>(shadowmapTexDesc);

        drawPassOutput->shadowmapDSVHandle = graph.Create<RHI::D3D12DepthStencilView>(
            RHI::RgViewDesc().SetResource(drawPassOutput->shadowmapTextureHandle).AsDsv());

        drawPassOutput->shadowmapSRVHandle = graph.Create<RHI::D3D12ShaderResourceView>(
            RHI::RgViewDesc().SetResource(drawPassOutput->shadowmapTextureHandle).AsTextureSrv());

        graph.AddRenderPass("IndirectShadowPass")
            .Write(&drawPassOutput->shadowmapTextureHandle)
            .Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {

                ID3D12CommandSignature* pCommandSignature =
                    registry.GetCommandSignature(CommandSignatures::IndirectDrawShadowmap)->GetApiHandle();

                RHI::D3D12DepthStencilView* shadowmapStencilView = registry.GetD3D12DepthStencilView(drawPassOutput->shadowmapDSVHandle);

                context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                context.SetViewport(RHIViewport {0.0f, 0.0f, (float)shadowmap_size.x, (float)shadowmap_size.y, 0.0f, 1.0f});
                context.SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});

                context.SetGraphicsRootSignature(registry.GetRootSignature(RootSignatures::IndirectDraw));
                context.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectDrawShadowmap));
                context->SetGraphicsRootConstantBufferView(1, pPerframeBuffer->GetGpuVirtualAddress());
                context->SetGraphicsRootShaderResourceView(2, pMeshBuffer->GetGpuVirtualAddress());
                context->SetGraphicsRootShaderResourceView(3, pMaterialBuffer->GetGpuVirtualAddress());

                context.ClearRenderTarget(nullptr, shadowmapStencilView);
                context.SetRenderTarget(nullptr, shadowmapStencilView);

                context->ExecuteIndirect(pCommandSignature,
                                         HLSL::MeshLimit,
                                         p_IndirectShadowmapCommandBuffer->GetResource(),
                                         0,
                                         p_IndirectShadowmapCommandBuffer->GetResource(),
                                         commandBufferCounterOffset);

            });
    }

    void IndirectShadowPass::destroy() {}

}



