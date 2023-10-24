#include "runtime/function/render/renderer/indirect_depth_prepass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void IndirectDepthPrePass::initialize(const DrawPassInitInfo& init_info)
	{
        depthDesc = init_info.depthTexDesc;

        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        indirectDepthPrepassVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "hlsl/IndirectDepthPrePS.hlsl", ShaderCompileOptions(L"VSMain"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(3)
                    .AddConstantBufferView<1, 0>()
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

            pIndirectDepthPrepassSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::CommandSignatureDesc Builder(4);
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pIndirectDepthPrepassCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, Builder, pIndirectDepthPrepassSignature->GetApiHandle());
        }

        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexPositionNormalTangentTexture::InputLayout;

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
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pIndirectDepthPrepassSignature.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState      = rasterizerState;
            psoStream.VS                    = &indirectDepthPrepassVS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pIndirectDepthPrepassPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectDepthPrepass", psoDesc);
        }

	}

    void IndirectDepthPrePass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        RHI::RgResourceHandle meshBufferHandle     = passInput.meshBufferHandle;
        RHI::RgResourceHandle materialBufferHandle = passInput.materialBufferHandle;
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle opaqueDrawHandle     = passInput.opaqueDrawHandle;

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectDepthPrepass");

        drawpass.Read(passInput.meshBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.materialBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.opaqueDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        drawpass.Write(passOutput.depthHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        RHI::RgResourceHandle depthHandle = passOutput.depthHandle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12DepthStencilView* depthStencilView = registry->GetD3D12Texture(depthHandle)->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)depthDesc.Width, (float)depthDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)depthDesc.Width, (int)depthDesc.Height});

            graphicContext->SetRenderTarget(NULL, depthStencilView);
            graphicContext->ClearRenderTarget(NULL, depthStencilView);

            graphicContext->SetRootSignature(pIndirectDepthPrepassSignature.get());
            graphicContext->SetPipelineState(pIndirectDepthPrepassPSO.get());
            graphicContext->SetConstantBuffer(1, registry->GetD3D12Buffer(perframeBufferHandle)->GetGpuVirtualAddress());
            graphicContext->SetConstant(0, 1, registry->GetD3D12Buffer(meshBufferHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 2, registry->GetD3D12Buffer(materialBufferHandle)->GetDefaultSRV()->GetIndex());
            
            auto pIndirectCommandBuffer = registry->GetD3D12Buffer(opaqueDrawHandle);

            graphicContext->ExecuteIndirect(pIndirectDepthPrepassCommandSignature.get(),
                                            pIndirectCommandBuffer,
                                            0,
                                            HLSL::MeshLimit,
                                            pIndirectCommandBuffer->GetCounterBuffer().get(),
                                            0);
        });
    }


    void IndirectDepthPrePass::destroy()
    {
        pIndirectDepthPrepassSignature   = nullptr;
        pIndirectDepthPrepassPSO         = nullptr;
        pIndirectDepthPrepassCommandSignature = nullptr;
    }

    bool IndirectDepthPrePass::initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput)
    {
        bool needClearRenderTarget = false;
        if (!drawPassOutput->depthHandle.IsValid())
        {
            needClearRenderTarget       = true;
            drawPassOutput->depthHandle = graph.Create<RHI::D3D12Texture>(depthDesc);
        }
        return needClearRenderTarget;
    }

}
