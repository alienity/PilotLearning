#include "runtime/function/render/renderer/indirect_shadow_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

namespace Pilot
{

    void IndirectShadowPass::initialize(const ShadowPassInitInfo& init_info)
    {
        //Vector2 shadowmap_size = m_visiable_nodes.p_directional_light->m_shadowmap_size;

        //shadowmapTexDesc = RHI::RgTextureDesc("ShadowmapTex")
        //                       .SetFormat(DXGI_FORMAT_D32_FLOAT)
        //                       .SetExtent(shadowmap_size.x, shadowmap_size.y)
        //                       .SetAllowDepthStencil()
        //                       .SetClearValue(RHI::RgClearValue(0.0f, 0xff));

    }

    void IndirectShadowPass::prepareShadowmaps(std::shared_ptr<RenderResourceBase> render_resource)
    {
        if (m_visiable_nodes.p_directional_light->m_shadowmap)
        {
            if (p_DirectionLightShadowmap == nullptr)
            {
                Vector2 shadowmap_size  = m_visiable_nodes.p_directional_light->m_shadowmap_size;

                D3D12_RESOURCE_FLAGS shadowmapFlags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                CD3DX12_RESOURCE_DESC shadowmapTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                    DXGI_FORMAT_D32_FLOAT, shadowmap_size.x, shadowmap_size.y, 1, 1, 1, 0, shadowmapFlags);
                CD3DX12_CLEAR_VALUE shadowmapClearVal = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 0, 1);

                p_DirectionLightShadowmap = std::make_shared<RHI::D3D12Texture>(
                    m_Device->GetLinkedDevice(), shadowmapTexDesc, shadowmapClearVal);

                p_DirectionLightShadowmapDSV = std::make_shared<RHI::D3D12DepthStencilView>(
                    m_Device->GetLinkedDevice(), p_DirectionLightShadowmap.get());

                p_DirectionLightShadowmapSRV = std::make_shared<RHI::D3D12ShaderResourceView>(
                    m_Device->GetLinkedDevice(), p_DirectionLightShadowmap.get(), false, 0, 1);
            }
            RenderResource* real_resource = (RenderResource*)render_resource.get();
            real_resource->m_mesh_perframe_storage_buffer_object.scene_directional_light.shadowmap_srv_index =
                p_DirectionLightShadowmapSRV->GetIndex();
        }
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
        
        RHI::RgResourceHandle dirShadowMapHandle = graph.Import<RHI::D3D12Texture>(p_DirectionLightShadowmap.get());
        RHI::RgResourceHandle dirShadowmapDSVHandle = graph.Import<RHI::D3D12DepthStencilView>(p_DirectionLightShadowmapDSV.get());
        RHI::RgResourceHandle dirShadowmapSRVHandle = graph.Import<RHI::D3D12ShaderResourceView>(p_DirectionLightShadowmapSRV.get());

        drawPassOutput->shadowmapTextureHandle = dirShadowMapHandle;
        drawPassOutput->shadowmapDSVHandle     = dirShadowmapDSVHandle;
        drawPassOutput->shadowmapSRVHandle     = dirShadowmapSRVHandle;

        graph.AddRenderPass("IndirectShadowPass")
            .Write(&drawPassOutput->shadowmapTextureHandle)
            .Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {

                ID3D12CommandSignature* pCommandSignature =
                    registry.GetCommandSignature(CommandSignatures::IndirectDrawShadowmap)->GetApiHandle();

                RHI::D3D12DepthStencilView* shadowmapStencilView = registry.GetD3D12DepthStencilView(drawPassOutput->shadowmapDSVHandle);

                context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                context.SetViewport(RHIViewport {0.0f, 0.0f, (float)shadowmap_size.x, (float)shadowmap_size.y, 0.0f, 1.0f});
                context.SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});

                context.SetGraphicsRootSignature(registry.GetRootSignature(RootSignatures::IndirectDrawShadowmap));
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



