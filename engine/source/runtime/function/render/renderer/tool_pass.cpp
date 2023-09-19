#include "runtime/function/render/renderer/tool_pass.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraph.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"

#include "Inc/ScreenGrab.h"
#include <DirectXHelpers.h>

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
                    .Add32BitConstants<0, 0>(4)
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
            std::shared_ptr<RHI::D3D12Texture> p_IBLSpecular = m_render_scene->m_skybox_map.m_skybox_specular_map;

            int width = p_IBLSpecular->GetWidth();
            int height = p_IBLSpecular->GetHeight();

            p_LD = RHI::D3D12Texture::CreateCubeMap(m_Device->GetLinkedDevice(),
                                                    width,
                                                    height,
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
            isDFGGenerated = !isDFGGenerated;

            int width  = p_DFG->GetWidth();
            int height = p_DFG->GetHeight();

            int dfgUAVIndex = p_DFG->GetDefaultUAV()->GetIndex();

            RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

            computeContext->SetRootSignature(p_DFGRootsignature.get());
            computeContext->SetPipelineState(p_DFGPSO.get());
            computeContext->SetConstants(0, dfgUAVIndex);

            computeContext->Dispatch2D(width, height, 8, 8);
        }

        if (!isLDGenerated)
        {
            isLDGenerated = !isLDGenerated;

            std::shared_ptr<RHI::D3D12Texture> p_IBLSpecular = m_render_scene->m_skybox_map.m_skybox_specular_map;

            int specularSRVIndex = p_IBLSpecular->GetDefaultSRV()->GetIndex();

            int width = p_LD->GetWidth();
            int height = p_LD->GetHeight();
            
            RHI::D3D12ComputeContext* computeContext = context->GetComputeContext();

            // lod 0 --- ¦Á 0
            // lod 1 --- ¦Á 0.2
            // lod 2 --- ¦Á 0.4
            // lod 3 --- ¦Á 0.6
            // lod 4 --- ¦Á 0.8

            float roughnessArray[5] = {0.0f, 0.2f, 0.4f, 0.6f, 0.8f};

            computeContext->SetRootSignature(p_LDRootsignature.get());
            computeContext->SetPipelineState(p_LDPSO.get());

            for (size_t i = 0; i < 5; i++)
            {
                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = RHI::D3D12UnorderedAccessView::GetDesc(p_LD.get(), 0, i);
                int ldUAVIndex = p_LD->CreateUAV(uavDesc)->GetIndex();

                int _lodIndex = i;
                float _roughness = roughnessArray[i];

                computeContext->SetConstant(0, 0, specularSRVIndex);
                computeContext->SetConstant(0, 1, ldUAVIndex);
                computeContext->SetConstant(0, 2, _lodIndex);
                computeContext->SetConstant(0, 3, _roughness);

                computeContext->Dispatch3D(width, height, 6, 8, 8, 1);
            }

        }


    }

    void ToolPass::update(RHI::RenderGraph& graph, ToolInputParameters& passInput, ToolOutputParameters& passOutput)
    {


    }

    void ToolPass::lateUpdate()
    {
        if (!isDFGSaved)
        {
            isDFGSaved = !isDFGSaved;

            RHI::D3D12CommandQueue* _commandQueue = m_Device->GetLinkedDevice()->GetCommandQueue(RHI::RHID3D12CommandQueueType::Direct);

            std::filesystem::path m_AssertRootPath = g_runtime_global_context.m_config_manager->getAssetFolder();

            std::filesystem::path m_DFGPath = m_AssertRootPath.append(L"texture\\global\\DFG.dds");

            D3D12_RESOURCE_STATES m_ResState = p_DFG->GetResourceState().GetSubresourceState(0);

            DirectX::SaveDDSTextureToFile(
                _commandQueue->GetCommandQueue(), p_DFG->GetResource(), m_DFGPath.c_str(), m_ResState, m_ResState);
        }

        if (!isLDSaved)
        {
            isLDSaved = !isLDSaved;

            RHI::D3D12CommandQueue* _commandQueue = m_Device->GetLinkedDevice()->GetCommandQueue(RHI::RHID3D12CommandQueueType::Direct);

            std::filesystem::path m_AssertRootPath = g_runtime_global_context.m_config_manager->getAssetFolder();

            std::filesystem::path m_LDPath = m_AssertRootPath.append(L"texture\\global\\LD.dds");

            D3D12_RESOURCE_STATES m_ResState = p_LD->GetResourceState().GetSubresourceState(0);

            // ------------------
            // https://learn.microsoft.com/en-us/windows/win32/direct3d12/readback-data-using-heaps
            // ------------------

            ID3D12CommandQueue * commandQueue = _commandQueue->GetCommandQueue();

            ID3D12Resource* pSource = p_LD->GetResource();

            D3D12_RESOURCE_DESC desc = pSource->GetDesc();

            ID3D12Device* device = m_Device->GetD3D12Device();

            UINT numSubresources = p_LD->GetNumSubresources();

            UINT64 totalResourceSize = 0;
            UINT64 fpRowPitch[128]   = {0};
            UINT   fpRowCount[128]   = {0};
            // Get the rowcount, pitch and size of the top mip
            device->GetCopyableFootprints(
                &desc, 0, numSubresources, 0, nullptr, fpRowCount, fpRowPitch, &totalResourceSize);

            // Create a command allocator
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAlloc;
            HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                        IID_GRAPHICS_PPV_ARGS(commandAlloc.GetAddressOf()));
            commandAlloc->SetName(L"ScreenGrab");

            // Spin up a new command list
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
            hr = device->CreateCommandList(0,
                                           D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           commandAlloc.Get(),
                                           nullptr,
                                           IID_GRAPHICS_PPV_ARGS(commandList.GetAddressOf()));
            commandList->SetName(L"ScreenGrab");

            // Create a fence
            Microsoft::WRL::ComPtr<ID3D12Fence> fence;
            hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fence.GetAddressOf()));
            fence->SetName(L"ScreenGrab");

            // Round up the srcPitch to multiples of 256
            const UINT64 dstRowPitch = (totalResourceSize + 255) & ~0xFFu;

            // Readback resources must be buffers
            D3D12_HEAP_PROPERTIES readbackHeapProperties {CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK)};
            D3D12_RESOURCE_DESC   readbackBufferDesc {CD3DX12_RESOURCE_DESC::Buffer(dstRowPitch)};
            Microsoft::WRL::ComPtr<ID3D12Resource> pStaging;

            // Create a staging texture
            hr = device->CreateCommittedResource(&readbackHeapProperties,
                                                 D3D12_HEAP_FLAG_NONE,
                                                 &readbackBufferDesc,
                                                 D3D12_RESOURCE_STATE_COPY_DEST,
                                                 nullptr,
                                                 IID_GRAPHICS_PPV_ARGS(pStaging.ReleaseAndGetAddressOf()));

            // copy resource
            Microsoft::WRL::ComPtr<ID3D12Resource> copySource(pSource);
            {
                D3D12_RESOURCE_BARRIER barrierDesc {
                    CD3DX12_RESOURCE_BARRIER::Transition(copySource.Get(), m_ResState, D3D12_RESOURCE_STATE_COPY_DEST)};
                commandList->ResourceBarrier(1, &barrierDesc);

                commandList->CopyResource(pStaging.Get(), copySource.Get());
            }

            hr = commandList->Close();

            // Execute the command list
            commandQueue->ExecuteCommandLists(1, CommandListCast(commandList.GetAddressOf()));

            // Signal the fence
            hr = commandQueue->Signal(fence.Get(), 1);

            // Block until the copy is complete
            while (fence->GetCompletedValue() < 1)
                SwitchToThread();

            
            DirectX::ScratchImage outScratchImage;
            outScratchImage.InitializeCube(desc.Format, desc.Width, desc.Height, 1, desc.MipLevels);

            void* pMappedMemory = outScratchImage.GetPixels();

            D3D12_RANGE readRange  = {0, static_cast<SIZE_T>(outScratchImage.GetPixelsSize())};
            D3D12_RANGE writeRange = {0, 0};

            hr = pStaging->Map(0, &readRange, &pMappedMemory);

            const DirectX::Image* image = outScratchImage.GetImage(0, 0, 0);

            DirectX::SaveToDDSFile(*image, DirectX::DDS_FLAGS::DDS_FLAGS_NONE, m_LDPath.c_str());

        }
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
