#include "runtime/function/render/renderer/indirect_terrain_gbuffer_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/renderer/pass_helper.h"
#include <cassert>

namespace MoYu
{

    void IndirectTerrainGBufferPass::prepareMatBuffer(std::shared_ptr<RenderResource> render_resource)
    {
        /*
        if (pMatTextureIndexBuffer == nullptr)
        {
            pMatTextureIndexBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                              RHI::RHIBufferTargetNone,
                                                              2,
                                                              sizeof(MaterialIndexStruct),
                                                              L"TerrainMatTextureIndex",
                                                              RHI::RHIBufferModeDynamic,
                                                              D3D12_RESOURCE_STATE_GENERIC_READ);
        }
        if (pMatTextureTillingBuffer == nullptr)
        {
            pMatTextureTillingBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                                RHI::RHIBufferTargetNone,
                                                                2,
                                                                sizeof(MaterialTillingStruct),
                                                                L"TerrainMatTextureTilling",
                                                                RHI::RHIBufferModeDynamic,
                                                                D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        if (m_render_scene->m_terrain_renderers.size() > 0)
        {
            MaterialIndexStruct* pMaterialViewIndexBuffer = pMatTextureIndexBuffer->GetCpuVirtualAddress<MaterialIndexStruct>();
            MaterialTillingStruct* pMaterialTillingBuffer = pMatTextureTillingBuffer->GetCpuVirtualAddress<MaterialTillingStruct>();

            for (int i = 0; i < 1; i++)
            {
                auto terrainBaseTextures = m_render_scene->m_terrain_renderers[i].internalTerrainRenderer.ref_material.terrain_base_textures;
                (&pMaterialViewIndexBuffer[i])->albedoIndex = terrainBaseTextures->albedo->GetDefaultSRV()->GetIndex();
                (&pMaterialViewIndexBuffer[i])->armIndex    = terrainBaseTextures->ao_roughness_metallic->GetDefaultSRV()->GetIndex();
                (&pMaterialViewIndexBuffer[i])->displacementIndex = terrainBaseTextures->displacement->GetDefaultSRV()->GetIndex();
                (&pMaterialViewIndexBuffer[i])->normalIndex       = terrainBaseTextures->normal->GetDefaultSRV()->GetIndex();

                (&pMaterialTillingBuffer[i])->albedoTilling = terrainBaseTextures->albedo_tilling;
                (&pMaterialTillingBuffer[i])->armTilling = terrainBaseTextures->ao_roughness_metallic_tilling;
                (&pMaterialTillingBuffer[i])->displacementTilling = terrainBaseTextures->displacement_tilling;
                (&pMaterialTillingBuffer[i])->normalTilling = terrainBaseTextures->normal_tilling;
            }
        }
        */
    }

	void IndirectTerrainGBufferPass::initialize(const DrawPassInitInfo& init_info)
	{
        /*
        albedoDesc                                  = init_info.albedoDesc;
        depthDesc                                   = init_info.depthDesc;
        worldNormalDesc                             = init_info.worldNormalDesc;
        motionVectorDesc                            = init_info.motionVectorDesc;
        metallic_Roughness_Reflectance_AO_Desc      = init_info.metallic_Roughness_Reflectance_AO_Desc;
        clearCoat_ClearCoatRoughness_AnisotropyDesc = init_info.clearCoat_ClearCoatRoughness_AnisotropyDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        indirectTerrainGBufferVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "hlsl/IndirectTerrainGBufferPS.hlsl", ShaderCompileOptions(L"VSMain"));
        indirectTerrainGBufferPS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Pixel, m_ShaderRootPath / "hlsl/IndirectTerrainGBufferPS.hlsl", ShaderCompileOptions(L"PSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                             8,
                                             D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                                             D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pIndirectTerrainGBufferSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::CommandSignatureDesc mBuilder(4);
            mBuilder.AddConstant(0, 0, 1);
            mBuilder.AddVertexBufferView(0);
            mBuilder.AddIndexBufferView();
            mBuilder.AddDrawIndexed();

            pIndirectTerrainGBufferCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, mBuilder, pIndirectTerrainGBufferSignature->GetApiHandle());
        }

        {
            //RHI::D3D12InputLayout InputLayout = MoYu::D3D12TerrainPatch::InputLayout;
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexPosition::InputLayout;

            RHIRasterizerState rasterizerState = RHIRasterizerState();
            //rasterizerState.FillMode = RHI_FILL_MODE::Wireframe;
            rasterizerState.CullMode = RHI_CULL_MODE::Back;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0] = albedoDesc.Format;   // albedoTexDesc;
            RenderTargetState.RTFormats[1] = worldNormalDesc.Format;  // worldNormal;
            RenderTargetState.RTFormats[2] = metallic_Roughness_Reflectance_AO_Desc.Format; // metallic_Roughness_Reflectance_AO_Desc;
            RenderTargetState.RTFormats[3] = clearCoat_ClearCoatRoughness_AnisotropyDesc.Format; // clearCoat_ClearCoatRoughness_AnisotropyDesc;
            RenderTargetState.RTFormats[4] = motionVectorDesc.Format; // emissiveDesc;
            RenderTargetState.NumRenderTargets = 7;
            RenderTargetState.DSFormat         = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT;

            RHISampleState SampleState;
            SampleState.Count = 1;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamRasterizerState   RasterrizerState;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pIndirectTerrainGBufferSignature.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState      = rasterizerState;
            psoStream.VS                    = &indirectTerrainGBufferVS;
            psoStream.PS                    = &indirectTerrainGBufferPS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pIndirectTerrainGBufferPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectTerrainGBuffer", psoDesc);
        }
        */
	}

    void IndirectTerrainGBufferPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, GBufferOutput& passOutput)
    {
        /*
        RHI::RgResourceHandle terrainMatIndexHandle = GImport(graph, pMatTextureIndexBuffer.get());
        RHI::RgResourceHandle terrainMatTillingHandle = GImport(graph, pMatTextureTillingBuffer.get());

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle terrainHeightmapHandle = passInput.terrainHeightmapHandle;
        RHI::RgResourceHandle terrainNormalmapHandle = passInput.terrainNormalmapHandle;

        RHI::RgResourceHandle terrainCommandSigHandle = passInput.terrainCommandSigHandle;
        RHI::RgResourceHandle transformBufferHandle   = passInput.transformBufferHandle;

        InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;
        uint32_t clipTransCount = internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_transform_counts;

        std::string_view passName = std::string_view("IndirectTerrainGBufferPass");

        RHI::RenderPass& drawpass = graph.AddRenderPass(passName);

        drawpass.Read(terrainMatIndexHandle, false, RHIResourceState::RHI_RESOURCE_STATE_GENERIC_READ);
        drawpass.Read(terrainMatTillingHandle, false, RHIResourceState::RHI_RESOURCE_STATE_GENERIC_READ);

        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.terrainHeightmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        drawpass.Read(passInput.terrainNormalmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        
        drawpass.Read(passInput.terrainCommandSigHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        drawpass.Read(passInput.transformBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        drawpass.Read(passOutput.albedoHandle, true);
        drawpass.Read(passOutput.depthHandle, true);
        drawpass.Read(passOutput.worldNormalHandle, true);
        drawpass.Read(passOutput.metallic_Roughness_Reflectance_AO_Handle, true);
        drawpass.Read(passOutput.clearCoat_ClearCoatRoughness_Anisotropy_Handle, true);
        drawpass.Read(passOutput.motionVectorHandle, true);

        drawpass.Write(passOutput.albedoHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);
        drawpass.Write(passOutput.worldNormalHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.metallic_Roughness_Reflectance_AO_Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.clearCoat_ClearCoatRoughness_Anisotropy_Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.motionVectorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);

        RHI::RgResourceHandle albedoHandle       = passOutput.albedoHandle;
        RHI::RgResourceHandle depthHandle        = passOutput.depthHandle;
        RHI::RgResourceHandle worldNormalHandle  = passOutput.worldNormalHandle;
        RHI::RgResourceHandle metallic_Roughness_Reflectance_AO_Handle = passOutput.metallic_Roughness_Reflectance_AO_Handle;
        RHI::RgResourceHandle clearCoat_ClearCoatRoughness_Anisotropy_Handle = passOutput.clearCoat_ClearCoatRoughness_Anisotropy_Handle;
        RHI::RgResourceHandle motionVectorHandle = passOutput.motionVectorHandle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12RenderTargetView* renderTargetView = registry->GetD3D12Texture(albedoHandle)->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = registry->GetD3D12Texture(depthHandle)->GetDefaultDSV().get();
            
            RHI::D3D12RenderTargetView* worldNormalRTView = registry->GetD3D12Texture(worldNormalHandle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* metallic_Roughness_Reflectance_AO_RTView = registry->GetD3D12Texture(metallic_Roughness_Reflectance_AO_Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* clearCoat_ClearCoatRoughness_Anisotropy_RTView = registry->GetD3D12Texture(clearCoat_ClearCoatRoughness_Anisotropy_Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* motionVectorRTView = registry->GetD3D12Texture(motionVectorHandle)->GetDefaultRTV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)albedoDesc.Width, (float)albedoDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)albedoDesc.Width, (int)albedoDesc.Height});

            std::vector<RHI::D3D12RenderTargetView*> _rtviews = {renderTargetView,
                                                                 worldNormalRTView,
                                                                 metallic_Roughness_Reflectance_AO_RTView,
                                                                 clearCoat_ClearCoatRoughness_Anisotropy_RTView,
                                                                 motionVectorRTView};

            graphicContext->SetRenderTargets(_rtviews, depthStencilView);
            //graphicContext->ClearRenderTarget(_rtviews, depthStencilView);

            graphicContext->SetRootSignature(pIndirectTerrainGBufferSignature.get());
            graphicContext->SetPipelineState(pIndirectTerrainGBufferPSO.get());

            struct RootIndexBuffer
            {
                uint32_t transformBufferIndex;
                uint32_t perFrameBufferIndex;
                uint32_t terrainHeightmapIndex;
                uint32_t terrainNormalmapIndex;
                uint32_t terrainMatIndexBufferIndex;
                uint32_t terrainMatTillingBufferIndex;
            };

            RootIndexBuffer rootIndexBuffer =
                RootIndexBuffer {RegGetBufDefSRVIdx(transformBufferHandle),
                                 RegGetBufDefCBVIdx(perframeBufferHandle),
                                 RegGetTexDefSRVIdx(terrainHeightmapHandle),
                                 RegGetTexDefSRVIdx(terrainNormalmapHandle),
                                 RegGetBufDefSRVIdx(terrainMatIndexHandle),
                                 RegGetBufDefSRVIdx(terrainMatTillingHandle)};

            graphicContext->SetConstantArray(0, 1, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);
            
            auto pDrawCallCommandSigBuffer = registry->GetD3D12Buffer(terrainCommandSigHandle);

            graphicContext->ExecuteIndirect(pIndirectTerrainGBufferCommandSignature.get(),
                                            pDrawCallCommandSigBuffer,
                                            0,
                                            clipTransCount,
                                            pDrawCallCommandSigBuffer->GetCounterBuffer().get(),
                                            0);

        });
        */
    }

    void IndirectTerrainGBufferPass::destroy()
    {
        pIndirectTerrainGBufferSignature = nullptr;
        pIndirectTerrainGBufferPSO       = nullptr;
        pIndirectTerrainGBufferCommandSignature = nullptr;
    }

}
