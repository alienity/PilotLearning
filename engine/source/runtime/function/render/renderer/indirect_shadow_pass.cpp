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
        RenderResource* real_resource = (RenderResource*)render_resource.get();

        if (m_visiable_nodes.p_directional_light != nullptr && m_visiable_nodes.p_directional_light->m_shadowmap)
        {
            directionalShadowmap.m_gobject_id = m_visiable_nodes.p_directional_light->m_gobject_id;
            directionalShadowmap.m_gcomponent_id = m_visiable_nodes.p_directional_light->m_gcomponent_id;
            directionalShadowmap.m_shadowmap_size = m_visiable_nodes.p_directional_light->m_shadowmap_size;
            if (directionalShadowmap.p_LightShadowmap == nullptr)
            {
                Vector2 shadowmap_size = m_visiable_nodes.p_directional_light->m_shadowmap_size;

                D3D12_RESOURCE_FLAGS  shadowmapFlags   = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                CD3DX12_RESOURCE_DESC shadowmapTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                    DXGI_FORMAT_D32_FLOAT, shadowmap_size.x, shadowmap_size.y, 1, 1, 1, 0, shadowmapFlags);
                CD3DX12_CLEAR_VALUE shadowmapClearVal = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 0, 1);

                directionalShadowmap.p_LightShadowmap = std::make_shared<RHI::D3D12Texture>(
                    m_Device->GetLinkedDevice(), shadowmapTexDesc, shadowmapClearVal);

                //directionalShadowmap.p_LightShadowmapDSV = std::make_shared<RHI::D3D12DepthStencilView>(
                //    m_Device->GetLinkedDevice(), directionalShadowmap.p_LightShadowmap.get());

                //directionalShadowmap.p_LightShadowmapSRV = std::make_shared<RHI::D3D12ShaderResourceView>(
                //    m_Device->GetLinkedDevice(), directionalShadowmap.p_LightShadowmap.get(), false, 0, 1);
            }

            real_resource->m_mesh_perframe_storage_buffer_object.scene_directional_light.shadowmap_srv_index =
                directionalShadowmap.p_LightShadowmap->GetDefaultSRV()->GetIndex();
        }
        else
        {
            directionalShadowmap.Reset();
        }

        if (m_visiable_nodes.p_spot_light_list != nullptr)
        {
            int spotLightCount = m_visiable_nodes.p_spot_light_list->size();
            for (size_t i = 0; i < spotLightCount; i++)
            {
                Pilot::SpotLightDesc curSpotLightDesc = m_visiable_nodes.p_spot_light_list->at(i);
                
                bool curSpotLighShaodwmaptExist = false;
                int  curShadowmapIndex          = -1;
                for (size_t j = 0; j < spotShadowmaps.size(); j++)
                {
                    if (spotShadowmaps[i].m_gobject_id == curSpotLightDesc.m_gobject_id &&
                        spotShadowmaps[i].m_gcomponent_id == curSpotLightDesc.m_gcomponent_id)
                    {
                        curShadowmapIndex          = i;
                        curSpotLighShaodwmaptExist = true;
                        break;
                    }
                }
                
                // if shadowmap exist but spotlight has disappered
                if (!curSpotLightDesc.m_is_active && curSpotLighShaodwmaptExist)
                {
                    spotShadowmaps[curShadowmapIndex].Reset();
                    spotShadowmaps.erase(spotShadowmaps.begin() + curShadowmapIndex);
                }

                // if shadowmap does not exist
                if (curSpotLightDesc.m_is_active && !curSpotLighShaodwmaptExist)
                {
                    Vector2 shadowmap_size = curSpotLightDesc.m_shadowmap_size;

                    D3D12_RESOURCE_FLAGS  shadowmapFlags   = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                    CD3DX12_RESOURCE_DESC shadowmapTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                        DXGI_FORMAT_D32_FLOAT, shadowmap_size.x, shadowmap_size.y, 1, 1, 1, 0, shadowmapFlags);
                    CD3DX12_CLEAR_VALUE shadowmapClearVal = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 0, 1);

                    std::shared_ptr<RHI::D3D12Texture> p_SpotLightShadowmap = std::make_shared<RHI::D3D12Texture>(
                        m_Device->GetLinkedDevice(), shadowmapTexDesc, shadowmapClearVal);

                    std::shared_ptr<RHI::D3D12DepthStencilView> p_SpotLightShadowmapDSV =
                        std::make_shared<RHI::D3D12DepthStencilView>(m_Device->GetLinkedDevice(),
                                                                     p_SpotLightShadowmap.get());

                    std::shared_ptr<RHI::D3D12ShaderResourceView> p_SpotLightShadowmapSRV =
                        std::make_shared<RHI::D3D12ShaderResourceView>(
                            m_Device->GetLinkedDevice(), p_SpotLightShadowmap.get(), false, 0, 1);

                    SpotShadowmapStruct spotShadow = {};
                    spotShadow.m_gobject_id        = curSpotLightDesc.m_gobject_id;
                    spotShadow.m_gcomponent_id     = curSpotLightDesc.m_gcomponent_id;
                    spotShadow.m_spot_index        = i;
                    spotShadow.m_shadowmap_size    = shadowmap_size;
                    spotShadow.p_LightShadowmap    = p_SpotLightShadowmap;
                    //spotShadow.p_LightShadowmapDSV = p_SpotLightShadowmapDSV;
                    //spotShadow.p_LightShadowmapSRV = p_SpotLightShadowmapSRV;

                    spotShadowmaps.push_back(spotShadow);

                    real_resource->m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].shadowmap_srv_index = p_SpotLightShadowmapSRV->GetIndex();
                }
            }
        }
        else
        {
            for (size_t i = 0; i < spotShadowmaps.size(); i++)
            {
                spotShadowmaps[i].Reset();
            }
            spotShadowmaps.clear();
        }

    }

    void IndirectShadowPass::update(RHI::D3D12CommandContext& context,
                                    RHI::RenderGraph&         graph,
                                    ShadowInputParameters&    passInput,
                                    ShadowOutputParameters&   passOutput)
    {
        ShadowInputParameters*  drawPassInput  = &passInput;
        ShadowOutputParameters* drawPassOutput = &passOutput;

        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer = drawPassInput->pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer     = drawPassInput->pMeshBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer = drawPassInput->pMaterialBuffer;

        std::shared_ptr<RHI::D3D12Buffer> pDirectionCommandBuffer = drawPassInput->p_DirectionalCommandBuffer;
        std::vector<std::shared_ptr<RHI::D3D12Buffer>> pSpotCommandBuffers = drawPassInput->p_SpotCommandBuffer;

        RHI::RenderPass& shadowpass = graph.AddRenderPass("IndirectShadowPass");

        if (directionalShadowmap.p_LightShadowmap != nullptr)
        {
            RHI::RgResourceHandle dirShadowMapHandle = graph.Import<RHI::D3D12Texture>(directionalShadowmap.p_LightShadowmap.get());
            //RHI::RgResourceHandle dirShadowmapDSVHandle = graph.Import<RHI::D3D12DepthStencilView>(directionalShadowmap.p_LightShadowmapDSV.get());
            //RHI::RgResourceHandle dirShadowmapSRVHandle = graph.Import<RHI::D3D12ShaderResourceView>(directionalShadowmap.p_LightShadowmapSRV.get());

            //drawPassOutput->directionalShadowmapRGHandle = {dirShadowMapHandle, dirShadowmapDSVHandle, dirShadowmapSRVHandle};
            drawPassOutput->directionalShadowmapRGHandle = {dirShadowMapHandle};

            shadowpass.Write(&dirShadowMapHandle);
        }

        for (size_t i = 0; i < spotShadowmaps.size(); i++)
        {
            RHI::RgResourceHandle spotShadowMapHandle = graph.Import<RHI::D3D12Texture>(spotShadowmaps[i].p_LightShadowmap.get());
            //RHI::RgResourceHandle spotShadowmapDSVHandle = graph.Import<RHI::D3D12DepthStencilView>(spotShadowmaps[i].p_LightShadowmapDSV.get());
            //RHI::RgResourceHandle spotShadowmapSRVHandle = graph.Import<RHI::D3D12ShaderResourceView>(spotShadowmaps[i].p_LightShadowmapSRV.get());

            //drawPassOutput->spotShadowmapRGHandle.push_back({spotShadowMapHandle, spotShadowmapDSVHandle, spotShadowmapSRVHandle});
            drawPassOutput->spotShadowmapRGHandle.push_back({spotShadowMapHandle});
            
            shadowpass.Write(&spotShadowMapHandle);
        }


        shadowpass.Execute([=](RHI::RenderGraphRegistry& registry, RHI::D3D12CommandContext& context) {

            RHI::D3D12GraphicsContext& graphicContext = context.GetGraphicsContext();

            if (directionalShadowmap.m_gobject_id != k_invalid_gobject_id &&
                directionalShadowmap.m_gcomponent_id != k_invalid_gcomponent_id)
            {
                ID3D12CommandSignature* pCommandSignature = CommandSignatures::pIndirectDrawDirectionShadowmap->GetApiHandle();

                RHI::D3D12Texture* shadowmapStencilTex =
                    registry.GetD3D12Texture(drawPassOutput->directionalShadowmapRGHandle.shadowmapTextureHandle);

                RHI::D3D12DepthStencilView* shadowmapStencilView = shadowmapStencilTex->GetDefaultDSV().get();

                graphicContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                Vector2 shadowmap_size = directionalShadowmap.m_shadowmap_size;

                graphicContext.SetRootSignature(RootSignatures::pIndirectDrawDirectionShadowmap.get());
                graphicContext.SetPipelineState(PipelineStates::pIndirectDrawDirectionShadowmap.get());
                graphicContext.SetViewport(
                    RHIViewport {0.0f, 0.0f, (float)shadowmap_size.x, (float)shadowmap_size.y, 0.0f, 1.0f});
                graphicContext.SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});

                graphicContext->SetGraphicsRootConstantBufferView(1, pPerframeBuffer->GetGpuVirtualAddress());
                graphicContext->SetGraphicsRootShaderResourceView(2, pMeshBuffer->GetGpuVirtualAddress());
                graphicContext->SetGraphicsRootShaderResourceView(3, pMaterialBuffer->GetGpuVirtualAddress());

                graphicContext.ClearRenderTarget(nullptr, shadowmapStencilView);
                graphicContext.SetRenderTarget(nullptr, shadowmapStencilView);

                graphicContext->ExecuteIndirect(pCommandSignature,
                                                HLSL::MeshLimit,
                                                pDirectionCommandBuffer->GetResource(),
                                                0,
                                                pDirectionCommandBuffer->GetCounterBuffer()->GetResource(),
                                                0);
            }

            for (size_t i = 0; i < spotShadowmaps.size(); i++)
            {
                ID3D12CommandSignature* pCommandSignature =
                    CommandSignatures::pIndirectDrawSpotShadowmap->GetApiHandle();

                RHI::D3D12Texture* shadowmapDepthTex =
                    registry.GetD3D12Texture(drawPassOutput->spotShadowmapRGHandle[i].shadowmapTextureHandle);

                RHI::D3D12DepthStencilView* shadowmapStencilView = shadowmapDepthTex->GetDefaultDSV().get();

                graphicContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                Vector2  shadowmap_size = spotShadowmaps[i].m_shadowmap_size;
                uint32_t spot_index     = spotShadowmaps[i].m_spot_index;

                graphicContext.SetRootSignature(RootSignatures::pIndirectDrawSpotShadowmap.get());
                graphicContext.SetPipelineState(PipelineStates::pIndirectDrawSpotShadowmap.get());
                graphicContext.SetViewport(
                    RHIViewport {0.0f, 0.0f, (float)shadowmap_size.x, (float)shadowmap_size.y, 0.0f, 1.0f});
                graphicContext.SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});

                graphicContext->SetGraphicsRoot32BitConstant(0, spot_index, 1);
                graphicContext->SetGraphicsRootConstantBufferView(1, pPerframeBuffer->GetGpuVirtualAddress());
                graphicContext->SetGraphicsRootShaderResourceView(2, pMeshBuffer->GetGpuVirtualAddress());
                graphicContext->SetGraphicsRootShaderResourceView(3, pMaterialBuffer->GetGpuVirtualAddress());

                graphicContext.ClearRenderTarget(nullptr, shadowmapStencilView);
                graphicContext.SetRenderTarget(nullptr, shadowmapStencilView);

                graphicContext->ExecuteIndirect(pCommandSignature,
                                                HLSL::MeshLimit,
                                                pSpotCommandBuffers[i]->GetResource(),
                                                0,
                                                pSpotCommandBuffers[i]->GetCounterBuffer()->GetResource(),
                                                0);
            }

        });
    }

    void IndirectShadowPass::destroy() {}

}



