#include "runtime/function/render/renderer/terrainDepthprepass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void TerrainDepthPrePass::initialize(const DrawPassInitInfo& init_info)
	{
        depthDesc = init_info.depthTexDesc;
        depthDesc.Name = "TerrainDepthBuffer";

        ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        drawDepthVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LightTerrainGBufferShader.hlsl", ShaderCompileOptions(L"Vert"));

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

            pDrawDepthSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::CommandSignatureDesc Builder(3);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pDepthCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, Builder, pDrawDepthSignature->GetApiHandle());
        }

        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexStandard::InputLayout;

            RHIRasterizerState rasterizerState = RHIRasterizerState();
            rasterizerState.CullMode = RHI_CULL_MODE::Back;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.NumRenderTargets = 0;
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
                //PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pDrawDepthSignature.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState      = rasterizerState;
            psoStream.VS                    = &drawDepthVS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pDrawDepthPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"TerrainDepthPrePass", psoDesc);
        }

	}

    void TerrainDepthPrePass::prepareMatBuffer(std::shared_ptr<RenderResource> render_resource)
    {

    }

    void TerrainDepthPrePass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutput& passOutput)
    {
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle terrainRenderDataHandle = passInput.terrainRenderDataHandle;
        RHI::RgResourceHandle terrainMatPropertyHandle = passInput.terrainMatPropertyHandle;
        RHI::RgResourceHandle terrainHeightmapHandle = passInput.terrainHeightmapHandle;
        RHI::RgResourceHandle terrainNormalmapHandle = passInput.terrainNormalmapHandle;
        RHI::RgResourceHandle patchListBufferHandle = passInput.culledPatchListBufferHandle;
        RHI::RgResourceHandle mainCamVisCmdSigHandle = passInput.mainCamVisCmdSigHandle;

        RHI::RgResourceHandle depthBufferHandle = passOutput.depthBufferHandle;

        RHI::RenderPass& drawpass = graph.AddRenderPass("TerrainDepthPrePass");

        drawpass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(terrainMatPropertyHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(terrainRenderDataHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(terrainHeightmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(terrainNormalmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(patchListBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(mainCamVisCmdSigHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);

        drawpass.Write(depthBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12DepthStencilView* depthStencilView = registry->GetD3D12Texture(depthBufferHandle)->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)depthDesc.Width, (float)depthDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)depthDesc.Width, (int)depthDesc.Height});

            graphicContext->SetRenderTarget(nullptr, depthStencilView);

            graphicContext->SetRootSignature(pDrawDepthSignature.get());
            graphicContext->SetPipelineState(pDrawDepthPSO.get());

            struct RootIndexBuffer
            {
                uint32_t frameUniformIndex;
                uint32_t patchListBufferIndex;
                uint32_t terrainHeightMapIndex;
                uint32_t terrainNormalMapIndex;
                uint32_t terrainRenderDataIndex;
                uint32_t terrainPropertyDataIndex;
            };

            RootIndexBuffer rootIndexBuffer =
                RootIndexBuffer{ RegGetBufDefCBVIdx(perframeBufferHandle),
                                 RegGetBufDefSRVIdx(patchListBufferHandle),
                                 RegGetTexDefSRVIdx(terrainHeightmapHandle),
                                 RegGetTexDefSRVIdx(terrainNormalmapHandle),
                                 RegGetBufDefSRVIdx(terrainRenderDataHandle),
                                 RegGetBufDefSRVIdx(terrainMatPropertyHandle) };

            graphicContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(uint32_t), &rootIndexBuffer);

            auto pMainCamVisCmdSigBuffer = registry->GetD3D12Buffer(mainCamVisCmdSigHandle);
            
            graphicContext->ExecuteIndirect(
                pDepthCommandSignature.get(),
                pMainCamVisCmdSigBuffer,
                0,
                1,
                nullptr,
                0);
        });

        passOutput.depthBufferHandle = depthBufferHandle;
    }


    void TerrainDepthPrePass::destroy()
    {
        pDrawDepthSignature = nullptr;
        pDrawDepthPSO = nullptr;
        pDepthCommandSignature = nullptr;
    }

}
