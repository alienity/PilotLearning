#include "runtime/function/render/renderer/indirect_terrain_gbuffer_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/function/render/renderer/pass_helper.h"
#include <cassert>

namespace MoYu
{

    void IndirectTerrainGBufferPass::prepareMatBuffer(std::shared_ptr<RenderResource> render_resource)
    {

    }

	void IndirectTerrainGBufferPass::initialize(const DrawPassInitInfo& init_info)
	{
        gbufferDesc = init_info.colorTexDesc;
        depthDesc = init_info.depthTexDesc;

        gbufferDesc.SetFormat(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT);

        gbuffer0Desc = gbufferDesc;
        gbuffer0Desc.Name = "GBuffer0";
        gbuffer1Desc = gbufferDesc;
        gbuffer1Desc.Name = "GBuffer1";
        gbuffer2Desc = gbufferDesc;
        gbuffer2Desc.Name = "GBuffer2";
        gbuffer3Desc = gbufferDesc;
        gbuffer3Desc.Name = "GBuffer3";

        depthDesc.Name = "DepthBuffer";

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        terrainGBufferVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LightTerrainGBufferShader.hlsl", ShaderCompileOptions(L"Vert"));
        terrainGBufferPS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Pixel, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LightTerrainGBufferShader.hlsl", ShaderCompileOptions(L"Frag"));

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

            pTerrainGBufferSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::CommandSignatureDesc mBuilder(3);
            mBuilder.AddVertexBufferView(0);
            mBuilder.AddIndexBufferView();
            mBuilder.AddDrawIndexed();

            pTerrainGBufferCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, mBuilder, pTerrainGBufferSignature->GetApiHandle());
        }

        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexStandard::InputLayout;

            RHIRasterizerState rasterizerState = RHIRasterizerState();
            rasterizerState.CullMode = RHI_CULL_MODE::Back;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc = RHI_COMPARISON_FUNC::GreaterEqual;
            DepthStencilState.DepthWrite = false;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0] = gbuffer0Desc.Format;
            RenderTargetState.RTFormats[1] = gbuffer1Desc.Format;
            RenderTargetState.RTFormats[2] = gbuffer2Desc.Format;
            RenderTargetState.RTFormats[3] = gbuffer3Desc.Format;
            RenderTargetState.NumRenderTargets = 4;
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
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pTerrainGBufferSignature.get());
            psoStream.InputLayout = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState = rasterizerState;
            psoStream.VS = &terrainGBufferVS;
            psoStream.PS = &terrainGBufferPS;
            psoStream.DepthStencilState = DepthStencilState;
            psoStream.RenderTargetState = RenderTargetState;
            psoStream.SampleState = SampleState;

            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };
            pTerrainGBufferPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectTerrainGBuffer", psoDesc);
        }
	}

    void IndirectTerrainGBufferPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, GBufferOutput& passOutput)
    {
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle terrainRenderDataHandle = passInput.terrainRenderDataHandle;
        RHI::RgResourceHandle terrainMatPropertyHandle = passInput.terrainMatPropertyHandle;
        RHI::RgResourceHandle terrainHeightmapHandle = passInput.terrainHeightmapHandle;
        RHI::RgResourceHandle terrainNormalmapHandle = passInput.terrainNormalmapHandle;
        RHI::RgResourceHandle patchListBufferHandle = passInput.culledPatchListBufferHandle;
        RHI::RgResourceHandle mainCamVisCmdSigHandle = passInput.mainCamVisCmdSigHandle;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectTerrainGBufferPass");

        drawpass.Read(perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(terrainMatPropertyHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(terrainRenderDataHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(terrainHeightmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(terrainNormalmapHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(patchListBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(mainCamVisCmdSigHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);

        drawpass.Write(passOutput.gbuffer0Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.gbuffer1Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.gbuffer2Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.gbuffer3Handle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(passOutput.depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        RHI::RgResourceHandle gbuffer0Handle = passOutput.gbuffer0Handle;
        RHI::RgResourceHandle gbuffer1Handle = passOutput.gbuffer1Handle;
        RHI::RgResourceHandle gbuffer2Handle = passOutput.gbuffer2Handle;
        RHI::RgResourceHandle gbuffer3Handle = passOutput.gbuffer3Handle;
        RHI::RgResourceHandle depthHandle = passOutput.depthHandle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12RenderTargetView* gbuffer0View = registry->GetD3D12Texture(gbuffer0Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* gbuffer1View = registry->GetD3D12Texture(gbuffer1Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* gbuffer2View = registry->GetD3D12Texture(gbuffer2Handle)->GetDefaultRTV().get();
            RHI::D3D12RenderTargetView* gbuffer3View = registry->GetD3D12Texture(gbuffer3Handle)->GetDefaultRTV().get();

            RHI::D3D12DepthStencilView* depthStencilView = registry->GetD3D12Texture(depthHandle)->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)gbufferDesc.Width, (float)gbufferDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)gbufferDesc.Width, (int)gbufferDesc.Height});

            std::vector<RHI::D3D12RenderTargetView*> _rtviews = { gbuffer0View, gbuffer1View, gbuffer2View, gbuffer3View};

            graphicContext->SetRenderTargets(_rtviews, depthStencilView);
            //graphicContext->ClearRenderTarget(_rtviews, nullptr);

            graphicContext->SetRootSignature(pTerrainGBufferSignature.get());
            graphicContext->SetPipelineState(pTerrainGBufferPSO.get());
            
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

            graphicContext->ExecuteIndirect(pTerrainGBufferCommandSignature.get(), pMainCamVisCmdSigBuffer, 0, 1, nullptr, 0);
        });

    }

    void IndirectTerrainGBufferPass::destroy()
    {
        pTerrainGBufferSignature = nullptr;
        pTerrainGBufferPSO       = nullptr;
        pTerrainGBufferCommandSignature = nullptr;
    }

}
