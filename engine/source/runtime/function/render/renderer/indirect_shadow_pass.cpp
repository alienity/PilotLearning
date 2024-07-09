#include "runtime/function/render/renderer/indirect_shadow_pass.h"

#include "glm/gtc/matrix_inverse.hpp"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

namespace MoYu
{

    void IndirectShadowPass::initialize(const ShadowPassInitInfo& init_info)
    {

    }

    void IndirectShadowPass::prepareShadowmaps(std::shared_ptr<RenderResource> render_resource)
    {
        m_SpotShadowmaps.clear();
        m_DirectionalShadowmap.Reset();

        // FrameUniforms
        HLSL::FrameUniforms* _frameUniforms = &render_resource->m_FrameUniforms;

        // Light Uniform
        HLSL::LightDataUniform _lightDataUniform;

        // EnvLight
        {
            HLSL::EnvLightData m_EnvLightData{};
            m_EnvLightData.lightLayers = 0xFFFFFFFF;
            m_EnvLightData.influenceShapeType = ENVSHAPETYPE_SKY;
            m_EnvLightData.influenceForward = glm::float3(0.0, 0.0, -1.0);
            m_EnvLightData.influenceUp = glm::float3(0.0, 1.0, 0.0);
            m_EnvLightData.influenceRight = glm::float3(1.0, 0.0, 0.0);
            m_EnvLightData.influencePositionRWS = glm::float3(0.0, 0.0, 0.0);
            m_EnvLightData.weight = 1.0f;
            m_EnvLightData.multiplier = 0.0f;
            m_EnvLightData.roughReflections = 1.0f;
            m_EnvLightData.distanceBasedRoughness = 0.0f;
            m_EnvLightData.proxyForward = glm::float3(0.0, 0.0, -1.0);
            m_EnvLightData.proxyUp = glm::float3(0.0, 1.0, 0.0);
            m_EnvLightData.proxyRight = glm::float3(1.0, 0.0, 0.0);
            m_EnvLightData.minProjectionDistance = 65504.0f;

            _lightDataUniform.envData = m_EnvLightData;
        }
        
        int puntualLightCount = 0;
        int shadowDataCount = 0;
        // SpotLight
        int spotLightCounts = m_render_scene->m_spot_light_list.size();
        for (uint32_t i = 0; i < spotLightCounts; i++)
        {
            glm::float3 spot_light_position = m_render_scene->m_spot_light_list[i].m_position;
            glm::float3 spot_light_up = m_render_scene->m_spot_light_list[i].up;
            glm::float3 spot_light_right = m_render_scene->m_spot_light_list[i].right;
            glm::float3 spot_light_direction = m_render_scene->m_spot_light_list[i].m_direction;
            float spot_light_intensity = m_render_scene->m_spot_light_list[i].m_intensity;
            float radius = m_render_scene->m_spot_light_list[i].m_radius;
            Color color = m_render_scene->m_spot_light_list[i].m_color;
            bool useShadowMap = m_render_scene->m_spot_light_list[i].m_shadowmap;

            float inner_degree = m_render_scene->m_spot_light_list[i].m_inner_degree;
            float outer_degree = m_render_scene->m_spot_light_list[i].m_outer_degree;

            HLSL::LightData curLightData{};
            curLightData.positionRWS = spot_light_position;
            curLightData.lightLayers = 0xFFFFFFFF;
            curLightData.forward = spot_light_direction;
            curLightData.up = spot_light_up;
            curLightData.right = spot_light_right;
            curLightData.lightType = GPULIGHTTYPE_SPOT;
            curLightData.shadowDataIndex = shadowDataCount;
            curLightData.lightDimmer = spot_light_intensity;
            curLightData.angleInner = MoYu::degreesToRadians(inner_degree);
            curLightData.angleOuter = MoYu::degreesToRadians(outer_degree);
            curLightData.color = glm::float3(color.r, color.g, color.b);
            curLightData.range = radius;
            curLightData.rangeAttenuationScale = 1.0f / (radius * radius);
            curLightData.rangeAttenuationBias = 1.0f;
            curLightData.diffuseDimmer = 1.0f;
            curLightData.specularDimmer = 1.0f;
            curLightData.size = glm::float4(radius * radius, 0, 0, 0);

            _lightDataUniform.lightData[puntualLightCount] = curLightData;
            puntualLightCount += 1;

            glm::int2 shadowmap_size = m_render_scene->m_spot_light_list[i].m_shadowmap_size;

            RHI::RHIRenderSurfaceBaseDesc rtDesc{};
            rtDesc.width = shadowmap_size.x;
            rtDesc.height = shadowmap_size.y;
            rtDesc.depthOrArray = 1;
            rtDesc.samples = 1;
            rtDesc.mipCount = 1;
            rtDesc.flags = RHI::RHISurfaceCreateShadowmap;
            rtDesc.dim = RHI::RHITextureDimension::RHITexDim2D;
            rtDesc.graphicsFormat = DXGI_FORMAT_D32_FLOAT;
            rtDesc.clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 0, 1);
            rtDesc.colorSurface = true;
            rtDesc.backBuffer = false;
            std::shared_ptr<RHI::D3D12Texture> p_LightShadowmap = render_resource->CreateTransientTexture(rtDesc, L"SpotLightShadowMap", D3D12_RESOURCE_STATE_COMMON);
            
            SpotShadowmapStruct spotShadowmapStruct{};
            spotShadowmapStruct.m_identifier = m_render_scene->m_spot_light_list[i].m_identifier;
            spotShadowmapStruct.m_spot_index = i;
            spotShadowmapStruct.m_shadowmap_size = shadowmap_size;
            spotShadowmapStruct.p_LightShadowmap = p_LightShadowmap;
            m_SpotShadowmaps.push_back(spotShadowmapStruct);

            glm::float4x4 viewMatrix = m_render_scene->m_spot_light_list[i].m_shadow_view_mat;
            glm::float4x4 projMatrix = m_render_scene->m_spot_light_list[i].m_shadow_proj_mats;
            glm::float4x4 viewProjMatrix = m_render_scene->m_spot_light_list[i].m_shadow_view_proj_mat;
            glm::float2 shadowBounds = m_render_scene->m_spot_light_list[i].m_shadow_bounds;
            glm::int2 viewportSize = m_render_scene->m_spot_light_list[i].m_shadowmap_size;
            float dn = m_render_scene->m_spot_light_list[i].m_shadow_near_plane;
            float df = m_render_scene->m_spot_light_list[i].m_shadow_far_plane;
            glm::float3 m_translationDelta = m_render_scene->m_spot_light_list[i].m_position_delation;

            HLSL::HDShadowData _shadowDara;
            _shadowDara.shadowmapIndex = p_LightShadowmap->GetDefaultSRV()->GetIndex();
            _shadowDara.worldTexelSize = 2.0f / projMatrix[0][0] / viewportSize.x * glm::sqrt(2.0f);
            _shadowDara.normalBias = 0.75f;
            _shadowDara.cascadeNumber = 1;
            _shadowDara.viewMatrix = viewMatrix;
            _shadowDara.projMatrix = projMatrix;
            _shadowDara.viewProjMatrix = viewProjMatrix;
            _shadowDara.zBufferParam = glm::float4((df - dn) / dn, 1.0f, (df - dn) / (dn * df), 1.0f / df);
            _shadowDara.shadowBounds = glm::float4(shadowBounds.x, shadowBounds.y, 0, 0);
            _shadowDara.shadowMapSize = glm::float4(viewportSize.x, viewportSize.y, 1.0f / viewportSize.x, 1.0f / viewportSize.y);
            _shadowDara.cacheTranslationDelta = glm::float4(m_translationDelta, 0);
            _shadowDara.shadowToWorld = glm::inverse(viewProjMatrix);

            _lightDataUniform.shadowDatas[shadowDataCount] = _shadowDara;
            shadowDataCount += 1;
        }

        // PointLight
        int pointLightCounts = m_render_scene->m_point_light_list.size();
        for (uint32_t i = 0; i < pointLightCounts; i++)
        {
            glm::float3 point_light_position = m_render_scene->m_point_light_list[i].m_position;
            glm::float3 point_light_forward = m_render_scene->m_point_light_list[i].forward;
            glm::float3 point_light_up = m_render_scene->m_point_light_list[i].up;
            glm::float3 point_light_right = m_render_scene->m_point_light_list[i].right;

            float point_light_intensity = m_render_scene->m_point_light_list[i].m_intensity;
            float radius = m_render_scene->m_point_light_list[i].m_radius;
            Color color = m_render_scene->m_point_light_list[i].m_color;

            HLSL::LightData curLightData{};
            curLightData.positionRWS = point_light_position;
            curLightData.lightLayers = 0xFFFFFFFF;
            curLightData.forward = point_light_forward;
            curLightData.up = point_light_up;
            curLightData.right = point_light_right;
            curLightData.lightType = GPULIGHTTYPE_POINT;
            curLightData.shadowDataIndex = -1;
            curLightData.lightDimmer = point_light_intensity;
            curLightData.color = glm::float3(color.r, color.g, color.b);
            curLightData.range = radius;
            curLightData.rangeAttenuationScale = 1.0f / (radius * radius);
            curLightData.rangeAttenuationBias = 1.0f;
            curLightData.diffuseDimmer = 1.0f;
            curLightData.specularDimmer = 1.0f;
            curLightData.size = glm::float4(radius * radius, 0, 0, 0);

            _lightDataUniform.lightData[puntualLightCount] = curLightData;
            puntualLightCount += 1;
        }

        _lightDataUniform._SpotLightCount = spotLightCounts;
        _lightDataUniform._PointLightCount = pointLightCounts;
        _lightDataUniform._PunctualLightCount = puntualLightCount;

        // DirectionLight
        {
            glm::float3 light_position = m_render_scene->m_directional_light.m_position;
            glm::float3 light_up = m_render_scene->m_directional_light.up;
            glm::float3 light_right = m_render_scene->m_directional_light.right;
            glm::float3 light_forward = m_render_scene->m_directional_light.forward;
            float dir_light_intensity = m_render_scene->m_directional_light.m_intensity;
            Color color = m_render_scene->m_directional_light.m_color;

            HLSL::DirectionalLightData directionalLightData{};
            directionalLightData.positionRWS = light_position;
            directionalLightData.lightLayers = 0xFFFFFFFF;
            directionalLightData.forward = light_forward;
            directionalLightData.up = light_up;
            directionalLightData.right = light_right;
            directionalLightData.shadowIndex = shadowDataCount;
            directionalLightData.lightDimmer = dir_light_intensity;
            directionalLightData.color = glm::float3(color.r, color.g, color.b);
            directionalLightData.diffuseDimmer = 1.0f;
            directionalLightData.specularDimmer = 1.0f;

            _lightDataUniform.directionalLightData = directionalLightData;

            glm::float4x4 viewMatrix = m_render_scene->m_directional_light.m_shadow_view_mat[0];
            glm::float4x4 projMatrix = m_render_scene->m_directional_light.m_shadow_proj_mats[0];
            glm::float4x4 viewProjMatrix = m_render_scene->m_directional_light.m_shadow_view_proj_mats[0];
            glm::float2 shadowBounds = m_render_scene->m_directional_light.m_shadow_bounds;
            glm::int2 viewportSize = m_render_scene->m_directional_light.m_shadowmap_size;
            float dn = m_render_scene->m_directional_light.m_shadow_near_plane;
            float df = m_render_scene->m_directional_light.m_shadow_far_plane;
            glm::float3 m_translationDelta = m_render_scene->m_directional_light.m_position_delation;
            glm::int2 cascade_shadowmap_size = viewportSize * 2;

            RHI::RHIRenderSurfaceBaseDesc rtDesc{};
            rtDesc.width = cascade_shadowmap_size.x;
            rtDesc.height = cascade_shadowmap_size.y;
            rtDesc.depthOrArray = 1;
            rtDesc.samples = 1;
            rtDesc.mipCount = 1;
            rtDesc.flags = RHI::RHISurfaceCreateShadowmap;
            rtDesc.dim = RHI::RHITextureDimension::RHITexDim2D;
            rtDesc.graphicsFormat = DXGI_FORMAT_D32_FLOAT;
            rtDesc.clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 0, 1);
            rtDesc.colorSurface = true;
            rtDesc.backBuffer = false;
            std::shared_ptr<RHI::D3D12Texture> p_LightShadowmap = render_resource->CreateTransientTexture(rtDesc, L"DirectionShadowmapCascade4", D3D12_RESOURCE_STATE_COMMON);

            m_DirectionalShadowmap.m_identifier = m_render_scene->m_directional_light.m_identifier;
            m_DirectionalShadowmap.m_shadowmap_size = cascade_shadowmap_size;
            m_DirectionalShadowmap.m_casccade = 4;
            m_DirectionalShadowmap.p_LightShadowmap = p_LightShadowmap;

            HLSL::HDShadowData _shadowDara;
            _shadowDara.shadowmapIndex = p_LightShadowmap->GetDefaultSRV()->GetIndex();
            _shadowDara.worldTexelSize = 2.0f / projMatrix[0][0] / viewportSize.x * glm::sqrt(2.0f);
            _shadowDara.normalBias = 0.75f;
            _shadowDara.cascadeNumber = 4;
            _shadowDara.viewMatrix = viewMatrix;
            _shadowDara.projMatrix = projMatrix;
            _shadowDara.viewProjMatrix = viewProjMatrix;
            _shadowDara.zBufferParam = glm::float4((df - dn) / dn, 1.0f, (df - dn) / (dn * df), 1.0f / df);
            _shadowDara.shadowBounds = glm::float4(shadowBounds.x, shadowBounds.y, 0, 0);
            _shadowDara.shadowMapSize = glm::float4(viewportSize.x, viewportSize.y, 1.0f / viewportSize.x, 1.0f / viewportSize.y);
            _shadowDara.cacheTranslationDelta = glm::float4(m_translationDelta, 0);
            _shadowDara.shadowToWorld = glm::inverse(viewProjMatrix);

            _lightDataUniform.shadowDatas[shadowDataCount] = _shadowDara;
            shadowDataCount += 1;

            HLSL::HDDirectionalShadowData _dirShadowData;
            for (int i = 0; i < 4; i++)
            {
                glm::float4x4 _viewMatrix = m_render_scene->m_directional_light.m_shadow_view_mat[i];
                glm::float4x4 _projMatrix = m_render_scene->m_directional_light.m_shadow_proj_mats[i];
                glm::float4x4 _viewProjMatrix = m_render_scene->m_directional_light.m_shadow_view_proj_mats[i];

                _dirShadowData.viewMatrix[i] = _viewMatrix;
                _dirShadowData.projMatrix[i] = _projMatrix;
                _dirShadowData.viewprojMatrix[i] = _viewProjMatrix;
            }

            _lightDataUniform.directionalShadowData = _dirShadowData;
        }
        _frameUniforms->lightDataUniform = _lightDataUniform;
    }

    void IndirectShadowPass::update(RHI::RenderGraph& graph, ShadowInputParameters& passInput, ShadowOutputParameters& passOutput)
    {

        RHI::RgResourceHandle renderDataPerDrawHandle = passInput.renderDataPerDrawHandle;
        RHI::RgResourceHandle propertiesPerMaterialHandle = passInput.propertiesPerMaterialHandle;
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;

        std::vector<RHI::RgResourceHandle> dirIndirectSortBufferHandles(passInput.dirIndirectSortBufferHandles);
        std::vector<RHI::RgResourceHandle> spotsIndirectSortBufferHandles(passInput.spotsIndirectSortBufferHandles);

        RHI::RenderPass& shadowpass = graph.AddRenderPass("IndirectShadowPass");

        shadowpass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        shadowpass.Read(renderDataPerDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        shadowpass.Read(propertiesPerMaterialHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);

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

                glm::float2 shadowmap_size = m_DirectionalShadowmap.m_shadowmap_size;

                graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
                graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(renderDataPerDrawHandle));
                graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(propertiesPerMaterialHandle));

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

                glm::float2  shadowmap_size = m_SpotShadowmaps[i].m_shadowmap_size;
                uint32_t spot_index     = m_SpotShadowmaps[i].m_spot_index;

                graphicContext->SetRootSignature(RootSignatures::pIndirectDrawSpotShadowmap.get());
                graphicContext->SetPipelineState(PipelineStates::pIndirectDrawSpotShadowmap.get());
                graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)shadowmap_size.x, (float)shadowmap_size.y, 0.0f, 1.0f});
                graphicContext->SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});

                graphicContext->SetConstant(0, 1, spot_index);
                graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
                graphicContext->SetBufferSRV(2, registry->GetD3D12Buffer(renderDataPerDrawHandle));
                graphicContext->SetBufferSRV(3, registry->GetD3D12Buffer(propertiesPerMaterialHandle));

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



