#include "runtime/function/render/renderer/tool_pass.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include "Inc/ScreenGrab.h"

namespace MoYu
{
    void ToolPass::initialize(const ToolPassInitInfo& init_info)
    {
        ShaderCompiler* m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = g_runtime_global_context.m_config_manager->getShaderFolder();

        {
            m_DFGCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/DFGPrefilter.hlsl", ShaderCompileOptions(L"CSMain"));
            m_LDCS = m_ShaderCompiler->CompileShader(
                RHI_SHADER_TYPE::Compute, m_ShaderRootPath / "hlsl/LDPrefilter.hlsl", ShaderCompileOptions(L"CSMain"));
        }

        {
            RHI::RootSignatureDesc dfgRootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(1)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            p_DFGRootsignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, dfgRootSigDesc);
        }
        {
            RHI::RootSignatureDesc ldRootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(1)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            p_LDRootsignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, ldRootSigDesc);
        }

        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(p_DFGRootsignature.get());
            psoStream.CS            = &m_DFGCS;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            p_DFGPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"DFGPSO", psoDesc);
        }
        {
            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(p_LDRootsignature.get());
            psoStream.CS            = &m_LDCS;

            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            p_LDPSO = std::make_shared<RHI::D3D12PipelineState>(m_Device, L"LDPSO", psoDesc);
        }


        if (p_DFG == nullptr)
        {
            p_DFG = RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                512,
                                                512,
                                                1,
                                                DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                RHI::RHISurfaceCreateFlags::RHISurfaceCreateRandomWrite,
                                                1,
                                                L"DFGTex");
        }

        if (p_LD == nullptr)
        {
            p_LD = RHI::D3D12Texture::CreateCubeMap(m_Device->GetLinkedDevice(),
                                                    512,
                                                    512,
                                                    5,
                                                    DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
                                                    RHI::RHISurfaceCreateFlags::RHISurfaceCreateRandomWrite,
                                                    1,
                                                    L"LDTex");
        }

        isDFGGenerated = false;
        isDFGSaved = false;

        isLDGenerated  = false;
        isRadiansGenerated = false;


    }

    void ToolPass::editorUpdate(RHI::D3D12CommandContext* context, ToolInputParameters& passInput, ToolOutputParameters& passOutput)
    {
        if (!isDFGGenerated)
        {
            isDFGGenerated = true;

            int width  = p_DFG->GetWidth();
            int height = p_DFG->GetHeight();

            int dfgUAVIndex = p_DFG->GetDefaultUAV()->GetIndex();

            RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

            computeContext->SetRootSignature(p_DFGRootsignature.get());
            computeContext->SetPipelineState(p_DFGPSO.get());
            computeContext->SetConstants(0, dfgUAVIndex);

            computeContext->Dispatch2D(width, height, 8, 8);
        }

        {
            //std::shared_ptr<RHI::D3D12Texture> p_IBLSpecular = m_render_scene->m_skybox_map.m_skybox_specular_map;

        }


    }

    void ToolPass::update(RHI::RenderGraph& graph, ToolInputParameters& passInput, ToolOutputParameters& passOutput)
    {

    }

    void ToolPass::lateUpdate()
    {
        if (!isDFGSaved)
        {
            isDFGSaved = true;

            RHI::D3D12CommandQueue* _commandQueue = m_Device->GetLinkedDevice()->GetCommandQueue(RHI::RHID3D12CommandQueueType::Direct);

            std::filesystem::path m_AssertRootPath = g_runtime_global_context.m_config_manager->getAssetFolder();

            std::filesystem::path m_DFGPath = m_AssertRootPath.append(L"texture\\global\\DFG.dds");

            D3D12_RESOURCE_STATES m_ResState = p_DFG->GetResourceState().GetSubresourceState(0);

            DirectX::SaveDDSTextureToFile(
                _commandQueue->GetCommandQueue(), p_DFG->GetResource(), m_DFGPath.c_str(), m_ResState, m_ResState);
        }


        //DirectX::SaveDDSTextureToFile(m_Device->getcommand, p_DFG->GetResource(), L"DFG");

    }


    void ToolPass::destroy()
    {
        p_DFG = nullptr;
        p_LD  = nullptr;

        p_DFGRootsignature = nullptr;
        p_LDRootsignature  = nullptr;

        p_DFGPSO = nullptr;
        p_LDPSO  = nullptr;
    }

}
