#include "runtime/function/render/renderer/object_motion_vector_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void IndirectMotionVectorPass::initialize(const DrawPassInitInfo& init_info)
	{
        motionVectorDesc = init_info.colorTexDesc;
        motionVectorDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        motionVectorDesc.Name = "MotionVectorBuffer";

        ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        drawMotionVectorVS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Vertex, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LitMotionVectorShader.hlsl", ShaderCompileOptions(L"Vert"));
        drawMotionVectorPS = m_ShaderCompiler->CompileShader(
            RHI_SHADER_TYPE::Pixel, m_ShaderRootPath / "pipeline/Runtime/Material/Lit/LitMotionVectorShader.hlsl", ShaderCompileOptions(L"Frag"));

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

            pMotionVectorSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::CommandSignatureDesc Builder(4);
            Builder.AddConstant(0, 0, 1);
            Builder.AddVertexBufferView(0);
            Builder.AddIndexBufferView();
            Builder.AddDrawIndexed();

            pMotionVectorCommandSignature = std::make_shared<RHI::D3D12CommandSignature>(
                m_Device, Builder, pMotionVectorSignature->GetApiHandle());
        }

        {
            RHI::D3D12InputLayout InputLayout = MoYu::D3D12MeshVertexStandard::InputLayout;

            RHIRasterizerState rasterizerState = RHIRasterizerState();
            rasterizerState.CullMode = RHI_CULL_MODE::Back;

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = true;
            DepthStencilState.DepthFunc   = RHI_COMPARISON_FUNC::GreaterEqual;
            DepthStencilState.DepthWrite = false;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0] = motionVectorDesc.Format;
            RenderTargetState.NumRenderTargets = 1;
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
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pMotionVectorSignature.get());
            psoStream.InputLayout           = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.RasterrizerState      = rasterizerState;
            psoStream.VS                    = &drawMotionVectorVS;
            psoStream.PS                    = &drawMotionVectorPS;
            psoStream.DepthStencilState     = DepthStencilState;
            psoStream.RenderTargetState     = RenderTargetState;
            psoStream.SampleState           = SampleState;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};
            pMotionVectorPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirectObjectMotionVector", psoDesc);
        }


	}

    void IndirectMotionVectorPass::prepareMatBuffer(std::shared_ptr<RenderResource> render_resource)
    {
        if (pMotionVectorRT == nullptr)
        {
            constexpr FLOAT tmpClearColor[4] = { 0, 0, 0, 0 };
            auto tmpClearValue = CD3DX12_CLEAR_VALUE(motionVectorDesc.Format, tmpClearColor);

            pMotionVectorRT =
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                    motionVectorDesc.Width,
                    motionVectorDesc.Height,
                    8,
                    motionVectorDesc.Format,
                    RHI::RHISurfaceCreateRenderTarget,
                    1,
                    L"MotionVectorBuffer",
                    D3D12_RESOURCE_STATE_COMMON,
                    tmpClearValue);
        }
    }

    void IndirectMotionVectorPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle renderDataPerDrawHandle = passInput.renderDataPerDrawHandle;
        RHI::RgResourceHandle propertiesPerMaterialHandle = passInput.propertiesPerMaterialHandle;
        RHI::RgResourceHandle opaqueDrawHandle = passInput.opaqueDrawHandle;
        RHI::RgResourceHandle depthBufferHandle = passInput.depthBufferHandle;

        RHI::RgResourceHandle motionVectorHandle = graph.Import<RHI::D3D12Texture>(pMotionVectorRT.get());

        RHI::RenderPass& drawpass = graph.AddRenderPass("IndirectMotionVectorPass");

        drawpass.Read(passInput.renderDataPerDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.propertiesPerMaterialHandle, false, RHIResourceState::RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.opaqueDrawHandle, false, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT, RHIResourceState::RHI_RESOURCE_STATE_INDIRECT_ARGUMENT);
        drawpass.Write(motionVectorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        drawpass.Write(depthBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);

        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12RenderTargetView* motionVectorView = registry->GetD3D12Texture(motionVectorHandle)->GetDefaultRTV().get();
            RHI::D3D12DepthStencilView* depthStencilView = registry->GetD3D12Texture(depthBufferHandle)->GetDefaultDSV().get();

            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport {0.0f, 0.0f, (float)motionVectorDesc.Width, (float)motionVectorDesc.Height, 0.0f, 1.0f});
            graphicContext->SetScissorRect(RHIRect {0, 0, (int)motionVectorDesc.Width, (int)motionVectorDesc.Height});

            graphicContext->SetRenderTarget(motionVectorView, depthStencilView);
            graphicContext->ClearRenderTarget(motionVectorView, nullptr);

            graphicContext->SetRootSignature(pMotionVectorSignature.get());
            graphicContext->SetPipelineState(pMotionVectorPSO.get());
            graphicContext->SetConstant(0, 1, registry->GetD3D12Buffer(renderDataPerDrawHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 2, registry->GetD3D12Buffer(propertiesPerMaterialHandle)->GetDefaultSRV()->GetIndex());
            graphicContext->SetConstant(0, 3, registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV()->GetIndex());


            auto pIndirectCommandBuffer = registry->GetD3D12Buffer(opaqueDrawHandle);

            graphicContext->ExecuteIndirect(pMotionVectorCommandSignature.get(),
                                            pIndirectCommandBuffer,
                                            0,
                                            HLSL::MeshLimit,
                                            pIndirectCommandBuffer->GetCounterBuffer().get(),
                                            0);
        });

        passOutput.depthBufferHandle = depthBufferHandle;
        passOutput.motionVectorHandle = motionVectorHandle;
    }

    void IndirectMotionVectorPass::destroy()
    {
        pMotionVectorSignature = nullptr;
        pMotionVectorPSO = nullptr;
        pMotionVectorCommandSignature = nullptr;
    }

}
