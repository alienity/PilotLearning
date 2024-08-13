#include "runtime/function/render/renderer/camera_motion_vector_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace MoYu
{

	void CameraMotionVectorPass::initialize(const DrawPassInitInfo& init_info)
	{
        motionVectorDesc = init_info.colorTexDesc;
        motionVectorDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        motionVectorDesc.Name = "MotionVectorBuffer";

        ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        cameraMotionVectorVS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Vertex, 
            m_ShaderRootPath / "pipeline/Runtime/Material/Lit/CameraMotionVectorsShader.hlsl", ShaderCompileOptions(L"Vert"));
        cameraMotionVectorPS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Pixel, 
            m_ShaderRootPath / "pipeline/Runtime/Material/Lit/CameraMotionVectorsShader.hlsl", ShaderCompileOptions(L"Frag"));

        {
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                .Add32BitConstants<0, 0>(16)
                .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                .AddStaticSampler<11, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                .AddStaticSampler<12, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4)
                .AddStaticSampler<13, 0>(D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 4)
                .AllowInputLayout()
                .AllowResourceDescriptorHeapIndexing()
                .AllowSampleDescriptorHeapIndexing();
                
            pCameraMotionVectorSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);
        }
        {
            RHI::D3D12InputLayout InputLayout = {};

            RHIDepthStencilState DepthStencilState;
            DepthStencilState.DepthEnable = false;
            DepthStencilState.DepthWrite = false;
            DepthStencilState.DepthFunc = RHI_COMPARISON_FUNC::Always;

            RHIRenderTargetState RenderTargetState;
            RenderTargetState.RTFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
            RenderTargetState.NumRenderTargets = 1;
            RenderTargetState.DSFormat = DXGI_FORMAT_D32_FLOAT;

            RHISampleState SampleState;
            SampleState.Count = 1;

            struct PsoStream
            {
                PipelineStateStreamRootSignature     RootSignature;
                PipelineStateStreamInputLayout       InputLayout;
                PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
                PipelineStateStreamVS                VS;
                PipelineStateStreamPS                PS;
                PipelineStateStreamDepthStencilState DepthStencilState;
                PipelineStateStreamRenderTargetState RenderTargetState;
                PipelineStateStreamSampleState       SampleState;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pCameraMotionVectorSignature.get());
            psoStream.InputLayout = &InputLayout;
            psoStream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
            psoStream.VS = &cameraMotionVectorVS;
            psoStream.PS = &cameraMotionVectorPS;
            psoStream.DepthStencilState = DepthStencilState;
            psoStream.RenderTargetState = RenderTargetState;
            psoStream.SampleState = SampleState;

            PipelineStateStreamDesc psoDesc = { sizeof(PsoStream), &psoStream };

            pCameraMotionVectorPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"CameraMotionVector", psoDesc);
        }

	}

    void CameraMotionVectorPass::prepareMatBuffer(std::shared_ptr<RenderResource> render_resource)
    {
        
    }

    void CameraMotionVectorPass::update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput)
    {
        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        RHI::RgResourceHandle depthPyramidHandle = passInput.depthPyramidHandle;

        RHI::RgResourceHandle motionVectorHandle = passOutput.motionVectorHandle;
        
        RHI::RenderPass& drawpass = graph.AddRenderPass("CameraMotionVectorPass");

        drawpass.Read(passInput.perframeBufferHandle, false, RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        drawpass.Read(passInput.depthPyramidHandle, false, RHIResourceState::RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        drawpass.Write(motionVectorHandle, false, RHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
        
        drawpass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {

            RHI::D3D12GraphicsContext* graphicContext = context->GetGraphicsContext();

            RHI::D3D12RenderTargetView* motionVectorView = registry->GetD3D12Texture(motionVectorHandle)->GetDefaultRTV().get();
            
            graphicContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            graphicContext->SetViewport(RHIViewport{ 0.0f, 0.0f, (float)motionVectorDesc.Width, (float)motionVectorDesc.Height, 0.0f, 1.0f });
            graphicContext->SetScissorRect(RHIRect{ 0, 0, (int)motionVectorDesc.Width, (int)motionVectorDesc.Height });
            graphicContext->SetRenderTarget(motionVectorView, nullptr);

            graphicContext->SetRootSignature(pCameraMotionVectorSignature.get());
            graphicContext->SetPipelineState(pCameraMotionVectorPSO.get());

            graphicContext->SetConstant(0, 0, registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV()->GetIndex());
            graphicContext->SetConstant(0, 1, registry->GetD3D12Texture(depthPyramidHandle)->GetDefaultSRV()->GetIndex());

            graphicContext->Draw(3);
        });

        passOutput.motionVectorHandle = motionVectorHandle;
    }

    void CameraMotionVectorPass::destroy()
    {
        pCameraMotionVectorSignature = nullptr;
        pCameraMotionVectorPSO = nullptr;
    }

}
