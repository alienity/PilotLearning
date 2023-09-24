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
                    .Add32BitConstants<0, 0>(3)
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
    
    void ToolPass::update(RHI::RenderGraph& graph, ToolInputParameters& passInput, ToolOutputParameters& passOutput)
    {

    }

    void ToolPass::preUpdate(ToolInputParameters& passInput, ToolOutputParameters& passOutput)
    {
        this->preUpdate1(passInput, passOutput);
        this->preUpdate2(passInput, passOutput);
    }

    void ToolPass::preUpdate1(ToolInputParameters& passInput, ToolOutputParameters& passOutput)
    {
        std::shared_ptr<RHI::D3D12CommandContext> context =
            RHI::D3D12CommandContext::Begin(m_Device->GetLinkedDevice(), RHI::RHID3D12CommandQueueType::Direct);

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

            // lod 0 --- �� 0
            // lod 1 --- �� 0.2
            // lod 2 --- �� 0.4
            // lod 3 --- �� 0.6
            // lod 4 --- �� 0.8

            float roughnessArray[5] = {0.0f, 0.2f, 0.4f, 0.6f, 0.8f};

            computeContext->SetRootSignature(p_LDRootsignature.get());
            computeContext->SetPipelineState(p_LDPSO.get());

            for (size_t i = 0; i < 5; i++)
            {
                UINT _MipWidth = width >> i;
                UINT _MipHeight = height >> i;

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = RHI::D3D12UnorderedAccessView::GetDesc(p_LD.get(), 0, i);
                int ldUAVIndex = p_LD->CreateUAV(uavDesc)->GetIndex();

                float _roughness = roughnessArray[i];

                computeContext->SetConstant(0, 0, specularSRVIndex);
                computeContext->SetConstant(0, 1, ldUAVIndex);
                computeContext->SetConstant(0, 2, _roughness);

                computeContext->Dispatch3D(_MipWidth, _MipHeight, 6, 8, 8, 1);
            }

        }

        context->Finish(true);
    }


    void ToolPass::preUpdate2(ToolInputParameters& passInput, ToolOutputParameters& passOutput)
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

            std::filesystem::path m_AssertRootPath = g_runtime_global_context.m_config_manager->getAssetFolder();

            std::filesystem::path m_LDPath = m_AssertRootPath.append(L"texture\\global\\LD.dds");

            Tools::ReadCubemapToFile(m_Device, p_LD.get(), m_LDPath.c_str());

            // ------------------
            // https://learn.microsoft.com/en-us/windows/win32/direct3d12/readback-data-using-heaps
            // ------------------
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

    namespace Tools
    {
        void ReadCubemapToFile(RHI::D3D12Device* pDevice, RHI::D3D12Texture* pCubemap, const wchar_t* filename)
        {
            std::shared_ptr<RHI::D3D12CommandContext> context =
                RHI::D3D12CommandContext::Begin(pDevice->GetLinkedDevice(), RHI::RHID3D12CommandQueueType::Direct);

            ID3D12Device* device = pDevice->GetD3D12Device();

            RHI::CResourceState state = pCubemap->GetResourceState();

            context->TransitionBarrier(pCubemap, D3D12_RESOURCE_STATE_COPY_SOURCE);


            ID3D12Resource* cubemap = pCubemap->GetResource();

            UINT numSubresources = pCubemap->GetNumSubresources();

            // Create a staging resource for the cubemap
            D3D12_RESOURCE_DESC desc = cubemap->GetDesc();
            
            UINT64 totalResourceSize = 0;
            UINT64 fpRowPitch[128]   = {0};
            UINT   fpRowCount[128]   = {0};
            // Get the rowcount, pitch and size of the top mip
            device->GetCopyableFootprints(
                &desc, 0, numSubresources, 0, nullptr, fpRowCount, fpRowPitch, &totalResourceSize);

            // Round up the srcPitch to multiples of 256
            const UINT64 dstRowPitch = (totalResourceSize + 255) & ~0xFFu;

            D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dstRowPitch);

            D3D12_HEAP_PROPERTIES readbackHeapProperties {CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK)};

            Microsoft::WRL::ComPtr<ID3D12Resource> stagingResource;
            device->CreateCommittedResource(&readbackHeapProperties,
                                            D3D12_HEAP_FLAG_NONE,
                                            &bufferDesc,
                                            D3D12_RESOURCE_STATE_COPY_DEST,
                                            nullptr,
                                            IID_PPV_ARGS(&stagingResource));

            UINT faceCount = desc.DepthOrArraySize;
            UINT mipLevels = desc.MipLevels;

            UINT baseOffset = 0;

            // Copy the cubemap to the staging resource
            for (UINT i = 0; i < faceCount; i++)
            {
                for (UINT j = 0; j < mipLevels; j++)
                {
                    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
                    srcLoc.pResource                   = cubemap;
                    srcLoc.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    srcLoc.SubresourceIndex = D3D12CalcSubresource(j, i, 0, desc.MipLevels, desc.DepthOrArraySize);

                    UINT64 dstBlockSize = 0;

                    D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint;
                    device->GetCopyableFootprints(&desc,
                                                  srcLoc.SubresourceIndex,
                                                  1,
                                                  baseOffset,
                                                  &bufferFootprint,
                                                  nullptr,
                                                  nullptr,
                                                  &dstBlockSize);

                    baseOffset += dstBlockSize;
                    

                    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
                    dstLoc.pResource                   = stagingResource.Get();
                    dstLoc.Type                        = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                    dstLoc.PlacedFootprint             = bufferFootprint;

                    context->GetGraphicsCommandList()->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
                }
            }

            //context->GetGraphicsCommandList()->CopyBufferRegion(
            //    stagingResource.Get(), 0, cubemap, 0, totalResourceSize);

            context->TransitionBarrier(pCubemap, state.GetSubresourceState(0));

            context->Finish(true);

            // Map the staging resource and write it to a file
            DirectX::ScratchImage outScratchImage;
            outScratchImage.InitializeCube(desc.Format, desc.Width, desc.Height, 1, desc.MipLevels);
            uint8_t*    pMappedMemory = outScratchImage.GetPixels();
            D3D12_RANGE readRange     = {0, outScratchImage.GetPixelsSize()};

            char* pReadbackBufferData {};
            stagingResource->Map(0, &readRange, reinterpret_cast<void**>(&pReadbackBufferData));
            
            memcpy(pMappedMemory, pReadbackBufferData, outScratchImage.GetPixelsSize());

            const DirectX::Image* images = outScratchImage.GetImages();
            UINT imageCount = outScratchImage.GetImageCount();

            DirectX::TexMetadata mdata = outScratchImage.GetMetadata();

            HRESULT hr = DirectX::SaveToDDSFile(images, imageCount, mdata, DirectX::DDS_FLAGS::DDS_FLAGS_NONE, filename);
            
            stagingResource->Unmap(0, nullptr);
            

        }


    }

}