#include "runtime/function/render/renderer/depthprepass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void DepthPrePass::initialize(const DrawPassInitInfo& init_info)
	{
        depthDesc = init_info.depthTexDesc;

        depthDesc.Name = "DepthBuffer";

        ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        drawDepthVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LightGBufferShader.hlsl", ShaderCompileOptions(L"Vert"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(4)
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
            RHI::CommandSignatureDesc Builder(4);
            Builder.AddConstant(0, 0, 1);
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
            pDrawDepthPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"DepthPrePass", psoDesc);
        }


	}

    void DepthPrePass::prepareMatBuffer(std::shared_ptr<RenderResource> render_resource)
    {
        RHI::RHIRenderSurfaceBaseDesc rtDesc{};
        rtDesc.width = depthDesc.Width;
        rtDesc.height = depthDesc.Height;
        rtDesc.depthOrArray = 1;
        rtDesc.samples = 1;
        rtDesc.mipCount = 1;
        rtDesc.flags = RHI::RHISurfaceCreateDepthStencil;
        rtDesc.dim = RHI::RHITextureDimension::RHITexDim2D;
        rtDesc.graphicsFormat = DXGI_FORMAT_D32_FLOAT;
        rtDesc.clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 0, 1);
        rtDesc.colorSurface = true;
        rtDesc.backBuffer = false;
        pDepthBuffer = render_resource->CreateTransientTexture(rtDesc, L"DepthBuffer", D3D12_RESOURCE_STATE_COMMON);

        HLSL::FrameUniforms* _frameUniforms = &render_resource->m_FrameUniforms;

        _frameUniforms->baseUniform._CameraDepthTextureIndex = pDepthBuffer->GetDefaultSRV()->GetIndex();

    }


    void DepthPrePass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutput& passOutput)
    {
        RHI::RgResourceHandle renderDataPerDrawHandle = passInput.renderDataPerDrawHandle;
        RHI::RgResourceHandle propertiesPerMaterialHandle = passInput.propertiesPerMaterialHandle;
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle opaqueDrawHandle = passInput.opaqueDrawHandle;

        passOutput.depthBufferHandle = graph.Import<RHI::D3D12Texture>(pDepthBuffer.get());

        RHI::RenderPass& drawpass = graph.AddRenderPass("DepthPrePass");

        drawpass.Read(passInput.renderDataPerDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.propertiesPerMaterialHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.opaqueDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        drawpass.Write(passOutput.depthBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        RHI::RgResourceHandle depthHandle = passOutput.depthBufferHandle;

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12DepthStencilView* depthStencilView = registry->GetD3D12Texture(depthHandle)->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)depthDesc.Width, (float)depthDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)depthDesc.Width, (int)depthDesc.Height});

            graphicContext->SetRenderTarget(nullptr, depthStencilView);
            graphicContext->ClearRenderTarget(nullptr, depthStencilView);

            graphicContext->SetRootSignature(pDrawDepthSignature.get());
            graphicContext->SetPipelineState(pDrawDepthPSO.get());
            graphicContext->SetConstant(0, 1, registry->GetD3D12Buffer(renderDataPerDrawHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 2, registry->GetD3D12Buffer(propertiesPerMaterialHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 3, registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV()->GetIndex());

            auto pIndirectCommandBuffer = registry->GetD3D12Buffer(opaqueDrawHandle);

            graphicContext->ExecuteIndirect(pDepthCommandSignature.get(),
                                            pIndirectCommandBuffer,
                                            0,
                                            HLSL::MeshLimit,
                                            pIndirectCommandBuffer->GetCounterBuffer().get(),
                                            0);
        });
    }

    void DepthPrePass::destroy()
    {
        pDepthBuffer = nullptr;

        pDrawDepthSignature = nullptr;
        pDrawDepthPSO = nullptr;
        pDepthCommandSignature = nullptr;
    }

}
