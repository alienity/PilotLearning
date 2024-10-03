#include "runtime/function/render/renderer/indirect_terrain_shadow_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/renderer/pass_helper.h"

namespace MoYu
{

    void IndirectTerrainShadowPass::initialize(const ShadowPassInitInfo& init_info)
    {
        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        indirectTerrainShadowmapVS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Vertex, 
                m_ShaderRootPath / "pipeline/Runtime/Tools/Shadow/IndirectTerrainDrawShadowmap.hlsl", ShaderCompileOptions(L"Vert"));
        
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 8)
                .AddStaticSampler<14, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                .AddStaticSampler<15, 0>(D3D12_FILTER::D3D12_FILTER_COMPARISON_ANISOTROPIC,
                    D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                    8,
                    D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                    D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();

            pIndirectTerrainShadowmapSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        } 
        {
            RHI::CommandSignatureDesc Builder(3);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pIndirectTerrainShadowmapCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, Builder, pIndirectTerrainShadowmapSignature->GetApiHandle());
        }
        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexStandard::InputLayout;

            RHIRasterizerState rasterizerState = RHIRasterizerState();
            rasterizerState.CullMode = RHI_CULL_MODE::Back;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.NumRenderTargets = 0;
            RenderTargetState.DSFormat = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT;

            RHISampleState SampleState;
            SampleState.Count = 1;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamRasterizerState   RasterrizerState;
                PipelineStateStreamVS                VS;
                //PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pIndirectTerrainShadowmapSignature.get());
            psoStream.InputLayout = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState = rasterizerState;
            psoStream.VS = &indirectTerrainShadowmapVS;
            psoStream.DepthStencilState = DepthStencilState;
            psoStream.RenderTargetState = RenderTargetState;
            psoStream.SampleState = SampleState;

            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pIndirectTerrainShadowmapPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectTerrainDrawShadowmap", psoDesc);
        }
    }

    void IndirectTerrainShadowPass::prepareShadowmaps(std::shared_ptr<RenderResource> render_resource, DirectionShadowmapStruct directionalShadowmap)
    {
        m_DirectionalShadowmap = directionalShadowmap;
    }

    void IndirectTerrainShadowPass::update(RHI::RenderGraph& graph, ShadowInputParameters& passInput, ShadowOutputParameters& passOutput)
    {
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle terrainHeightmapHandle = passInput.terrainHeightmapHandle;
        RHI::RgResourceHandle terrainNormalmapHandle = passInput.terrainNormalmapHandle;
        RHI::RgResourceHandle terrainRenderDataHandle = passInput.terrainRenderDataHandle;
        RHI::RgResourceHandle terrainMatPropertyHandle = passInput.terrainMatPropertyHandle;
        
        std::vector<RHI::RgResourceHandle> dirConsBufferHandles = std::vector<RHI::RgResourceHandle>(passInput.dirConsBufferHandles);
        std::vector<RHI::RgResourceHandle> dirVisPatchListHandles = std::vector<RHI::RgResourceHandle>(passInput.dirVisPatchListHandles);
        std::vector<RHI::RgResourceHandle> dirVisCmdSigBufferHandles = std::vector<RHI::RgResourceHandle>(passInput.dirVisCmdSigBufferHandles);

        RHI::RgResourceHandle directionalCascadeShadowmapHandle = passOutput.directionalCascadeShadowmapHandle;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectTerrainShadowPass");

        drawpass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(terrainMatPropertyHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(terrainRenderDataHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(terrainHeightmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(terrainNormalmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        for (int i = 0; i < 4; i++)
        {
            drawpass.Read(dirConsBufferHandles[i], false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            drawpass.Read(dirVisPatchListHandles[i], false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            drawpass.Read(dirVisCmdSigBufferHandles[i], false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        }

        drawpass.Write(directionalCascadeShadowmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            if (m_DirectionalShadowmap.m_identifier != UndefCommonIdentifier)
            {
                glm::float2 atlas_size = m_DirectionalShadowmap.m_atlas_size;
                glm::float2 shadowmap_size = m_DirectionalShadowmap.m_shadowmap_size;

                int shadowMapSubCount = glm::ceil(glm::log2((float)m_DirectionalShadowmap.m_casccade));
                glm::int2 subAtlasWdith = glm::int2(atlas_size.x / shadowMapSubCount, atlas_size.y / shadowMapSubCount);

                for (int i = 0; i < m_DirectionalShadowmap.m_casccade; i++)
                {
                    RHI::RgResourceHandle dirConsBufferHandle = dirConsBufferHandles[i];
                    RHI::RgResourceHandle dirPatchListBufferHandle = dirVisPatchListHandles[i];
                    RHI::RgResourceHandle dirVisCmdSigBufferHandle = dirVisCmdSigBufferHandles[i];

                    int atlasOffsetX = i % shadowMapSubCount * subAtlasWdith.x;
                    int atlasOffsetY = i / shadowMapSubCount * subAtlasWdith.y;

                    RHI::D3D12Texture* pCascadeShadowmapStencil = registry->GetD3D12Texture(directionalCascadeShadowmapHandle);
                    RHI::D3D12DepthStencilView* shadowmapStencilView = pCascadeShadowmapStencil->GetDefaultDSV().get();

                    graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                    //if (i == 0)
                    //{
                    //    graphicContext->ClearRenderTarget(nullptr, shadowmapStencilView);
                    //}
                    graphicContext->SetRenderTarget(nullptr, shadowmapStencilView);

                    graphicContext->SetRootSignature(pIndirectTerrainShadowmapSignature.get());
                    graphicContext->SetPipelineState(pIndirectTerrainShadowmapPSO.get());

                    struct RootIndexBuffer
                    {
                        uint32_t dirConsBufferIndex;
                        uint32_t patchListBufferIndex;
                        uint32_t terrainHeightMapIndex;
                        uint32_t terrainNormalMapIndex;
                        uint32_t terrainRenderDataIndex;
                        uint32_t terrainPropertyDataIndex;
                    };

                    RootIndexBuffer rootIndexBuffer =
                        RootIndexBuffer{ RegGetBufDefCBVIdx(dirConsBufferHandle),
                                         RegGetBufDefSRVIdx(dirPatchListBufferHandle),
                                         RegGetTexDefSRVIdx(terrainHeightmapHandle),
                                         RegGetTexDefSRVIdx(terrainNormalmapHandle),
                                         RegGetBufDefSRVIdx(terrainRenderDataHandle),
                                         RegGetBufDefSRVIdx(terrainMatPropertyHandle) };

                    graphicContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);


                    RHIViewport _viewport = {};
                    _viewport.MinDepth = 0.0f;
                    _viewport.MaxDepth = 1.0f;
                    _viewport.Width = subAtlasWdith.x;
                    _viewport.Height = subAtlasWdith.y;
                    _viewport.TopLeftX = atlasOffsetX;
                    _viewport.TopLeftY = atlasOffsetY;

                    //graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)shadowmap_size.x, (float)shadowmap_size.y, 0.0f, 1.0f});
                    //graphicContext->SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});
                    graphicContext->SetViewport(RHIViewport{ _viewport.TopLeftX, _viewport.TopLeftY, (float)_viewport.Width, (float)_viewport.Height, _viewport.MinDepth, _viewport.MaxDepth });
                    graphicContext->SetScissorRect(RHIRect{ 0, 0, (int)atlas_size.x, (int)atlas_size.y });

                    auto pDirectionCommandBuffer = registry->GetD3D12Buffer(dirVisCmdSigBufferHandle);
                    graphicContext->ExecuteIndirect(pIndirectTerrainShadowmapCommandSignature.get(), pDirectionCommandBuffer, 0, 1, nullptr, 0);

                }
            }

        });

    }

    void IndirectTerrainShadowPass::destroy() {}

}



