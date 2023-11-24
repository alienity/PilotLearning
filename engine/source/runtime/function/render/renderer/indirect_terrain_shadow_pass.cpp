#include "runtime/function/render/renderer/indirect_terrain_shadow_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

namespace MoYu
{

    void IndirectTerrainShadowPass::initialize(const ShadowPassInitInfo& init_info)
    {
        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        indirectTerrainShadowmapVS =
            m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "hlsl/IndirectTerrainDrawShadowmap.hlsl", ShaderCompileOptions(L"VSMain"));
        
        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(5)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pIndirectTerrainShadowmapSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::CommandSignatureDesc mBuilder(3);
            mBuilder.AddVertexBufferView(0);
            mBuilder.AddIndexBufferView();
            mBuilder.AddDrawIndexed();

            pIndirectTerrainShadowmapCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, mBuilder, pIndirectTerrainShadowmapSignature->GetApiHandle());
        }
        {
            RHI::D3D12InputLayout inputLayout = MoYu::D3D12TerrainPatch::InputLayout;

            RHIDepthStencilState depthStencilState;
            depthStencilState.DepthEnable = true;
            depthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState renderTargetState;
            renderTargetState.NumRenderTargets = 0;
            renderTargetState.DSFormat         = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pIndirectTerrainShadowmapSignature.get());
            psoStream.InputLayout           = &inputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.VS                    = &indirectTerrainShadowmapVS;
            psoStream.DepthStencilState     = depthStencilState;
            psoStream.RenderTargetState     = renderTargetState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirectTerrainShadowmapPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectTerrainDrawShadowmap", psoDesc);
        }
    }

    void IndirectTerrainShadowPass::prepareShadowmaps(std::shared_ptr<RenderResource> render_resource)
    {
        m_shadowmap_size = m_render_scene->m_directional_light.m_shadowmap_size;
        m_casccade = m_render_scene->m_directional_light.m_cascade;
    }

    void IndirectTerrainShadowPass::update(RHI::RenderGraph& graph, ShadowInputParameters& passInput, ShadowOutputParameters& passOutput)
    {
        RHI::RgResourceHandle perframeBufferHandle   = passInput.perframeBufferHandle;
        RHI::RgResourceHandle terrainPatchNodeHandle = passInput.terrainPatchNodeHandle;
        RHI::RgResourceHandle terrainHeightmapHandle = passInput.terrainHeightmapHandle;

        std::vector<DrawIndexAndCommandSigHandle> dirShadowIndexAndSigHandle = passInput.dirShadowIndexAndSigHandle;

        RHI::RgResourceHandle directionalShadowmapHandle = passOutput.directionalShadowmapHandle;
        
        RHI::RenderPass& shadowpass = graph.AddRenderPass("IndirectTerrainShadowPass");

        shadowpass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        shadowpass.Read(terrainPatchNodeHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        shadowpass.Read(terrainHeightmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        for (int i = 0; i < dirShadowIndexAndSigHandle.size(); i++)
        {
            shadowpass.Read(dirShadowIndexAndSigHandle[i].drawCallCommandSigBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
            shadowpass.Read(dirShadowIndexAndSigHandle[i].drawIndexBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        shadowpass.Write(passOutput.directionalShadowmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        shadowpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            #define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
            
            //if (m_DirectionalShadowmap.m_identifier != UndefCommonIdentifier)
            {
                RHI::D3D12Texture* pShadowmapStencilTex = registry->GetD3D12Texture(directionalShadowmapHandle);

                RHI::D3D12DepthStencilView* shadowmapStencilView = pShadowmapStencilTex->GetDefaultDSV().get();

                graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                //graphicContext->ClearRenderTarget(nullptr, shadowmapStencilView);
                graphicContext->SetRenderTarget(nullptr, shadowmapStencilView);

                graphicContext->SetRootSignature(pIndirectTerrainShadowmapSignature.get());
                graphicContext->SetPipelineState(pIndirectTerrainShadowmapPSO.get());

                glm::float2 shadowmap_size = m_shadowmap_size;
                
                for (uint32_t i = 0; i < m_casccade; i++)
                {
                     RHIViewport _viewport = {};

                    _viewport.MinDepth = 0.0f;
                    _viewport.MaxDepth = 1.0f;
                    _viewport.Width    = 0.5 * shadowmap_size.x;
                    _viewport.Height   = 0.5 * shadowmap_size.y;

                    _viewport.TopLeftX = 0.5 * shadowmap_size.x * ((i & 0x1) != 0 ? 1 : 0);
                    _viewport.TopLeftY = 0.5 * shadowmap_size.y * ((i & 0x2) != 0 ? 1 : 0);

                    graphicContext->SetViewport(RHIViewport {_viewport.TopLeftX, _viewport.TopLeftY, (float)_viewport.Width, (float)_viewport.Height, _viewport.MinDepth, _viewport.MaxDepth});
                    graphicContext->SetScissorRect(RHIRect {0, 0, (int)shadowmap_size.x, (int)shadowmap_size.y});

                    graphicContext->SetConstant(0, 0, i);
                    graphicContext->SetConstant(0, 1, registry->GetD3D12Buffer(terrainPatchNodeHandle)->GetDefaultSRV()->GetIndex());
                    graphicContext->SetConstant(0, 2, registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV()->GetIndex());
                    graphicContext->SetConstant(0, 3, registry->GetD3D12Texture(terrainHeightmapHandle)->GetDefaultSRV()->GetIndex());
                    graphicContext->SetConstant(0, 4, registry->GetD3D12Buffer(dirShadowIndexAndSigHandle[i].drawIndexBufferHandle)->GetDefaultSRV()->GetIndex());

                    RHI::D3D12Buffer* pDrawCallCommandSigBuffer =
                        registry->GetD3D12Buffer(dirShadowIndexAndSigHandle[i].drawCallCommandSigBufferHandle);

                    graphicContext->ExecuteIndirect(
                        pIndirectTerrainShadowmapCommandSignature.get(), pDrawCallCommandSigBuffer, 0, 1, nullptr, 0);
                }
            }

        });
    }

    void IndirectTerrainShadowPass::destroy() {}

}



