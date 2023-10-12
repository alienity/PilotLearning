#include "runtime/function/render/renderer/indirect_shadow_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rhi_core.h"

namespace MoYu
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


    void IndirectShadowPass::prepareShadowmaps(std::shared_ptr<RenderResource> render_resource)
    {
        RenderResource* real_resource = (RenderResource*)render_resource.get();

        if (m_render_scene->m_directional_light.m_identifier != UndefCommonIdentifier && m_render_scene->m_directional_light.m_shadowmap)
        {
            m_DirectionalShadowmap.m_identifier = m_render_scene->m_directional_light.m_identifier;
            m_DirectionalShadowmap.m_shadowmap_size = m_render_scene->m_directional_light.m_shadowmap_size;
            m_DirectionalShadowmap.m_casccade = m_render_scene->m_directional_light.m_cascade;
            if (m_DirectionalShadowmap.p_LightShadowmap == nullptr)
            {
                Vector2 shadowmap_size = m_render_scene->m_directional_light.m_shadowmap_size;
                //Vector2 cascade_shadowmap_size = shadowmap_size * 2;

                m_DirectionalShadowmap.p_LightShadowmap =
                    RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                shadowmap_size.x,
                                                shadowmap_size.y,
                                                1,
                                                DXGI_FORMAT_D32_FLOAT,
                                                RHI::RHISurfaceCreateShadowmap,
                                                1,
                                                L"DirectionShadowmapCascade4",
                                                D3D12_RESOURCE_STATE_COMMON,
                                                CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 0, 1));

            }

            real_resource->m_FrameUniforms.directionalLight.directionalLightShadowmap.shadowmap_srv_index =
                m_DirectionalShadowmap.p_LightShadowmap->GetDefaultSRV()->GetIndex();
        }
        else
        {
            m_DirectionalShadowmap.Reset();
        }

        if (m_render_scene->m_spot_light_list.size() != 0)
        {
            for (size_t i = 0; i < m_render_scene->m_spot_light_list.size(); i++)
            {
                auto& curSpotLightDesc = m_render_scene->m_spot_light_list[i];
                
                bool curSpotLighShaodwmaptExist = false;
                int  curShadowmapIndex  = -1;
                for (size_t j = 0; j < m_SpotShadowmaps.size(); j++)
                {
                    if (m_SpotShadowmaps[j].m_identifier == curSpotLightDesc.m_identifier)
                    {
                        curShadowmapIndex = j;
                        curSpotLighShaodwmaptExist = true;
                        break;
                    }
                }
                
                // if current light does not have shadowmap, but shadowmap exist
                if (!curSpotLightDesc.m_shadowmap && curSpotLighShaodwmaptExist)
                {
                    m_SpotShadowmaps.erase(m_SpotShadowmaps.begin() + curShadowmapIndex);
                    curShadowmapIndex = -1;
                }

                // if shadowmap does not exist, but current light has shadowmap
                if (curSpotLightDesc.m_shadowmap)
                {
                    if (!curSpotLighShaodwmaptExist)
                    {
                        Vector2 shadowmap_size = curSpotLightDesc.m_shadowmap_size;
                    
                        std::shared_ptr<RHI::D3D12Texture> p_SpotLightShadowmap =
                            RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                        shadowmap_size.x,
                                                        shadowmap_size.y,
                                                        1,
                                                        DXGI_FORMAT_D32_FLOAT,
                                                        RHI::RHISurfaceCreateShadowmap,
                                                        1,
                                                        L"SpotLightShadowmap",
                                                        D3D12_RESOURCE_STATE_COMMON,
                                                        CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 0, 1));

                        SpotShadowmapStruct spotShadow = {};
                        spotShadow.m_identifier        = curSpotLightDesc.m_identifier;
                        spotShadow.m_spot_index        = i;
                        spotShadow.m_shadowmap_size    = shadowmap_size;
                        spotShadow.p_LightShadowmap    = p_SpotLightShadowmap;

                        m_SpotShadowmaps.push_back(spotShadow);

                        curShadowmapIndex = m_SpotShadowmaps.size() - 1;
                    }

                    if (curShadowmapIndex != -1)
                    {
                        real_resource->m_FrameUniforms.spotLightUniform.scene_spot_lights[i]
                            .spotLightShadowmap.shadowmap_srv_index =
                            m_SpotShadowmaps[curShadowmapIndex].p_LightShadowmap->GetDefaultSRV()->GetIndex();
                    }

                }
            }
        }
        else
        {
            for (size_t i = 0; i < m_SpotShadowmaps.size(); i++)
            {
                m_SpotShadowmaps[i].Reset();
            }
            m_SpotShadowmaps.clear();
        }

    }

    void IndirectShadowPass::update(RHI::RenderGraph& graph, ShadowInputParameters& passInput, ShadowOutputParameters& passOutput)
    {
        //ShadowInputParameters  drawPassInput  = passInput;
        //ShadowOutputParameters drawPassOutput = passOutput;

        RHI::RgResourceHandle meshBufferHandle     = passInput.meshBufferHandle;
        RHI::RgResourceHandle materialBufferHandle = passInput.materialBufferHandle;
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;

        std::vector<RHI::RgResourceHandle> dirIndirectSortBufferHandles(passInput.dirIndirectSortBufferHandles);
        std::vector<RHI::RgResourceHandle> spotsIndirectSortBufferHandles(passInput.spotsIndirectSortBufferHandles);

        RHI::RenderPass& shadowpass = graph.AddRenderPass("IndirectShadowPass");

        shadowpass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        shadowpass.Read(meshBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        shadowpass.Read(materialBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        for (size_t i = 0; i < dirIndirectSortBufferHandles.size(); i++)
        {
            shadowpass.Read(dirIndirectSortBufferHandles[i],
                            false,
                            RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT,
                            RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        }
        for (size_t i = 0; i < spotsIndirectSortBufferHandles.size(); i++)
        {
            shadowpass.Read(spotsIndirectSortBufferHandles[i],
                            false,
                            RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT,
                            RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        }

        RHI::RgResourceHandle directionalShadowmapHandle = RHI::_DefaultRgResourceHandle;
        if (m_DirectionalShadowmap.p_LightShadowmap != nullptr)
        {
            passOutput.directionalShadowmapHandle = graph.Import<RHI::D3D12Texture>(m_DirectionalShadowmap.p_LightShadowmap.get());
            shadowpass.Write(passOutput.directionalShadowmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);
            directionalShadowmapHandle = passOutput.directionalShadowmapHandle;
        }

        for (size_t i = 0; i < m_SpotShadowmaps.size(); i++)
        {
            RHI::RgResourceHandle spotShadowMapHandle =
                graph.Import<RHI::D3D12Texture>(m_SpotShadowmaps[i].p_LightShadowmap.get());
            shadowpass.Write(spotShadowMapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);
            passOutput.spotShadowmapHandle.push_back(spotShadowMapHandle);
        }
        std::vector<RHI::RgResourceHandle> spotShadowmapHandles = passOutput.spotShadowmapHandle;

        shadowpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            #define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
            
            if (m_DirectionalShadowmap.m_identifier != UndefCommonIdentifier)
            {
                RHI::D3D12Texture* pShadowmapStencilTex = registry->GetD3D12Texture(directionalShadowmapHandle);

                RHI::D3D12DepthStencilView* shadowmapStencilView = pShadowmapStencilTex->GetDefaultDSV().get();

                graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                graphicContext->ClearRenderTarget(nullptr, shadowmapStencilView);
                graphicContext->SetRenderTarget(nullptr, shadowmapStencilView);

                graphicContext->SetRootSignature(RootSignatures::pIndirectDrawDirectionShadowmap.get());
                graphicContext->SetPipelineState(PipelineStates::pIndirectDrawDirectionShadowmap.get());

                Vector2 shadowmap_size = m_DirectionalShadowmap.m_shadowmap_size;

                graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
                graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(meshBufferHandle));
                graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(materialBufferHandle));

                for (uint32_t i = 0; i < m_DirectionalShadowmap.m_casccade; i++)
                {
                     RHIViewport _viewport = {};

                    _viewport.MinDepth = 0.0f;
                    _viewport.MaxDepth = 1.0f;
                    _viewport.Width    = 0.5 * shadowmap_size.x;
                    _viewport.Height   = 0.5 * shadowmap_size.y;

                    _viewport.TopLeftX = 0.5 * shadowmap_size.x * ((i & 0x1) != 0 ? 1 : 0);
                    _viewport.TopLeftY = 0.5 * shadowmap_size.y * ((i & 0x2) != 0 ? 1 : 0);

                    //graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)shadowmap_size.x, (float)shadowmap_size.y, 0.0f, 1.0f});
                    //graphicContext->SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});
                    graphicContext->SetViewport(RHIViewport {_viewport.TopLeftX, _viewport.TopLeftY, (float)_viewport.Width, (float)_viewport.Height, _viewport.MinDepth, _viewport.MaxDepth});
                    graphicContext->SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});

                    graphicContext->SetConstant(0, 1, i);

                    auto pDirectionCommandBuffer = registry->GetD3D12Buffer(dirIndirectSortBufferHandles[i]);
                    graphicContext->ExecuteIndirect(CommandSignatures::pIndirectDrawDirectionShadowmap.get(),
                                                        pDirectionCommandBuffer,
                                                        0,
                                                        HLSL::MeshLimit,
                                                        pDirectionCommandBuffer->GetCounterBuffer().get(),
                                                        0);
                }
            }

            for (size_t i = 0; i < m_SpotShadowmaps.size(); i++)
            {
                RHI::D3D12Texture* pShadowmapDepthTex = registry->GetD3D12Texture(spotShadowmapHandles[i]);

                RHI::D3D12DepthStencilView* shadowmapStencilView = pShadowmapDepthTex->GetDefaultDSV().get();

                graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                Vector2  shadowmap_size = m_SpotShadowmaps[i].m_shadowmap_size;
                uint32_t spot_index     = m_SpotShadowmaps[i].m_spot_index;

                graphicContext->SetRootSignature(RootSignatures::pIndirectDrawSpotShadowmap.get());
                graphicContext->SetPipelineState(PipelineStates::pIndirectDrawSpotShadowmap.get());
                graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)shadowmap_size.x, (float)shadowmap_size.y, 0.0f, 1.0f});
                graphicContext->SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});

                graphicContext->SetConstant(0, 1, spot_index);
                graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
                graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(meshBufferHandle));
                graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(materialBufferHandle));

                graphicContext->ClearRenderTarget(nullptr, shadowmapStencilView);
                graphicContext->SetRenderTarget(nullptr, shadowmapStencilView);

                auto pSpotCommandBuffer = registry->GetD3D12Buffer(spotsIndirectSortBufferHandles[i]);

                graphicContext->ExecuteIndirect(CommandSignatures::pIndirectDrawSpotShadowmap.get(),
                                                pSpotCommandBuffer,
                                                0,
                                                HLSL::MeshLimit,
                                                pSpotCommandBuffer->GetCounterBuffer().get(),
                                                0);
            }

        });
    }

    void IndirectShadowPass::destroy() {}

}



