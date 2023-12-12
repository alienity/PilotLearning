#include "runtime/function/render/renderer/terrain_cull_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsCommon.h"
#include "runtime/function/render/terrain_render_helper.h"
#include "runtime/function/render/renderer/pass_helper.h"

#include "fmt/core.h"
#include <cassert>

namespace MoYu
{
#define RegGetBuf(h) registry->GetD3D12Buffer(h)
#define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
#define RegGetTex(h) registry->GetD3D12Texture(h)
#define RegGetBufDefCBVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultCBV()->GetIndex()
#define RegGetBufDefSRVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultSRV()->GetIndex()
#define RegGetTexDefSRVIdx(h) registry->GetD3D12Texture(h)->GetDefaultSRV()->GetIndex()
#define RegGetBufDefUAVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultUAV()->GetIndex()
#define RegGetBufCounterSRVIdx(h) registry->GetD3D12Buffer(h)->GetCounterBuffer()->GetDefaultSRV()->GetIndex()

    void IndirectTerrainCullPass::initialize(const TerrainCullInitInfo& init_info)
    {
        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        //--------------------------------------------------------------------------
        {
            indirecTerrainPatchNodesGenCS =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                m_ShaderRootPath / "hlsl/IndirecTerrainPatchNodesGen.hlsl",
                                                ShaderCompileOptions(L"CSMain"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(4)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pIndirecTerrainPatchNodesGenSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pIndirecTerrainPatchNodesGenSignature.get());
            psoStream.CS            = &indirecTerrainPatchNodesGenCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pIndirecTerrainPatchNodesGenPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"IndirecTerrainPatchNodesGenPSO", psoDesc);
        }
        //--------------------------------------------------------------------------

        //--------------------------------------------------------------------------
        {

            patchNodeVisToMainCamIndexGenCS =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                m_ShaderRootPath / "hlsl/PatchNodeCullingForMainCamera.hlsl",
                                                ShaderCompileOptions(L"CSMain"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(4)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pPatchNodeVisToMainCamIndexGenSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pPatchNodeVisToMainCamIndexGenSignature.get());
            psoStream.CS            = &patchNodeVisToMainCamIndexGenCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pPatchNodeVisToMainCamIndexGenPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"TerrainPatchNodeCullingForMainCameraPSO", psoDesc);
        }
        //-----------------------------------------------------------

        //-----------------------------------------------------------
        {
            patchNodeVisToDirCascadeIndexGenCS =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                m_ShaderRootPath / "hlsl/PatchNodeCullingForDirShadow.hlsl",
                                                ShaderCompileOptions(L"CSMain"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(5)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pPatchNodeVisToDirCascadeIndexGenSignature =
                std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature =
                PipelineStateStreamRootSignature(pPatchNodeVisToDirCascadeIndexGenSignature.get());
            psoStream.CS            = &patchNodeVisToDirCascadeIndexGenCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pPatchNodeVisToDirCascadeIndexGenPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"TerrainPatchNodeCullingForDirShadowPSO", psoDesc);
        }
        //-----------------------------------------------------------

        //-----------------------------------------------------------
        {
            patchNodeCullByDepthCS =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                m_ShaderRootPath / "hlsl/PatchNodeCullByDepth.hlsl",
                                                ShaderCompileOptions(L"CSMain"));
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pPatchNodeCullByDepthSignature =
                std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pPatchNodeCullByDepthSignature.get());
            psoStream.CS                    = &patchNodeCullByDepthCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pPatchNodeCullByDepthPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"PatchNodeCullByDepthPSO", psoDesc);
        }

        {
            patchNodeCullByDepthCS2 =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                m_ShaderRootPath / "hlsl/PatchNodeCullByDepth2.hlsl",
                                                ShaderCompileOptions(L"CSMain"));
            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC,
                                             D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                             8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pPatchNodeCullByDepthSignature2 =
                std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature = PipelineStateStreamRootSignature(pPatchNodeCullByDepthSignature2.get());
            psoStream.CS                    = &patchNodeCullByDepthCS2;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pPatchNodeCullByDepthPSO2 =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"PatchNodeCullByDepthPSO2", psoDesc);
        }

        terrainPatchNodeBuffer = RHI::D3D12Buffer::Create(
            m_Device->GetLinkedDevice(),
            RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
            MaxTerrainNodeCount,
            sizeof(HLSL::TerrainPatchNode),
            L"TerrainPatchIndexBuffer");

        isTerrainMinMaxHeightReady = false;

    }

    void IndirectTerrainCullPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        if (terrainMinHeightMap == nullptr || terrainMaxHeightMap == nullptr)
        {
            InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;

            std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
            auto heightmapDesc = terrainHeightmap->GetDesc();

            terrainMinHeightMap =
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                            heightmapDesc.Width,
                                            heightmapDesc.Height,
                                            8,
                                            heightmapDesc.Format,
                                            RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                                            1,
                                            L"TerrainMinHeightmap",
                                            D3D12_RESOURCE_STATE_COMMON,
                                            std::nullopt);


            terrainMaxHeightMap =
                RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                            heightmapDesc.Width,
                                            heightmapDesc.Height,
                                            8,
                                            heightmapDesc.Format,
                                            RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                                            1,
                                            L"TerrainMaxHeightmap",
                                            D3D12_RESOURCE_STATE_COMMON,
                                            std::nullopt);
        }

        if (terrainUploadCommandSigBuffer == nullptr)
        {
            terrainUploadCommandSigBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                         RHI::RHIBufferTargetStructured,
                                         1,
                                         MoYu::AlignUp(sizeof(MoYu::TerrainCommandSignatureParams), 256),
                                         L"TerrainCommandSigUploadBuffer",
                                         RHI::RHIBufferModeDynamic,
                                         D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        // 更新CommandBuffer通用数据
        {
            std::vector<CachedTerrainRenderer>& _terrain_renderers = m_render_scene->m_terrain_renderers;
            
            if (_terrain_renderers.size() > 0)
            {
                InternalTerrainRenderer& _internalTerrainRenderer = _terrain_renderers[0].internalTerrainRenderer;

                MoYu::InternalIndexBuffer& _index_buffer = _internalTerrainRenderer.ref_terrain.terrain_patch_mesh.index_buffer;
                MoYu::InternalVertexBuffer& _vertex_buffer = _internalTerrainRenderer.ref_terrain.terrain_patch_mesh.vertex_buffer;

                D3D12_DRAW_INDEXED_ARGUMENTS _drawIndexedArguments = {};
                _drawIndexedArguments.IndexCountPerInstance = _index_buffer.index_count;
                _drawIndexedArguments.InstanceCount         = 1;
                _drawIndexedArguments.StartIndexLocation    = 0;
                _drawIndexedArguments.BaseVertexLocation    = 0;
                _drawIndexedArguments.StartInstanceLocation = 0;

                MoYu::TerrainCommandSignatureParams _terrainCommandSigParams = {};
                (&_terrainCommandSigParams)->VertexBuffer         = _vertex_buffer.vertex_buffer->GetVertexBufferView();
                (&_terrainCommandSigParams)->IndexBuffer          = _index_buffer.index_buffer->GetIndexBufferView();
                (&_terrainCommandSigParams)->DrawIndexedArguments = _drawIndexedArguments;

                MoYu::TerrainCommandSignatureParams* pTerrainCommandSigParams =
                    terrainUploadCommandSigBuffer->GetCpuVirtualAddress<MoYu::TerrainCommandSignatureParams>();

                memcpy(pTerrainCommandSigParams, &_terrainCommandSigParams, sizeof(MoYu::TerrainCommandSignatureParams));

                terrainInstanceCountOffset = ((char*)&_terrainCommandSigParams.DrawIndexedArguments.InstanceCount - (char*)&_terrainCommandSigParams) / sizeof(char);
            }
        }

        // 主相机视锥可见的地块的indexbuffer
        if (m_MainCameraVisiableIndexBuffer == nullptr)
        {
            m_MainCameraVisiableIndexBuffer = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                MaxTerrainNodeCount,
                sizeof(uint32_t),
                L"MainCameraFrustumVisiableIndexBuffer");
        }

        // 使用上一帧深度剪裁的可见和不可见的indexbuffer和CommandSignature
        if (lastDepthCullRemainIndexBuffer.m_PatchNodeVisiableIndexBuffer == nullptr)
        {
            lastDepthCullRemainIndexBuffer.m_PatchNodeVisiableIndexBuffer = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                MaxTerrainNodeCount,
                sizeof(uint32_t),
                L"PatchNodeVisiableToLstDepthIndexBuffer");

            lastDepthCullRemainIndexBuffer.m_PatchNodeNonVisiableIndexBuffer = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                MaxTerrainNodeCount,
                sizeof(uint32_t),
                L"PatchNodeNonVisiableToLstDepthIndexBuffer");

            lastDepthCullRemainIndexBuffer.m_TerrainCommandSignatureBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                         RHI::RHIBufferTargetStructured,
                                         1,
                                         MoYu::AlignUp(sizeof(MoYu::TerrainCommandSignatureParams), 256),
                                         L"PatchNodeCommandSigBuffer",
                                         RHI::RHIBufferModeImmutable,
                                         D3D12_RESOURCE_STATE_GENERIC_READ);
        }
        // 使用当前帧深度剪裁的可见和不可见的indexbuffer和CommandSignature
        if (curDepthCullRemainIndexBuffer.m_PatchNodeVisiableIndexBuffer == nullptr)
        {
            curDepthCullRemainIndexBuffer.m_PatchNodeVisiableIndexBuffer = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                MaxTerrainNodeCount,
                sizeof(uint32_t),
                L"PatchNodeVisiableToCurDepthIndexBuffer");

            curDepthCullRemainIndexBuffer.m_PatchNodeNonVisiableIndexBuffer = RHI::D3D12Buffer::Create(
                m_Device->GetLinkedDevice(),
                RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                MaxTerrainNodeCount,
                sizeof(uint32_t),
                L"PatchNodeNonVisiableToCurDepthIndexBuffer");

            curDepthCullRemainIndexBuffer.m_TerrainCommandSignatureBuffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                         RHI::RHIBufferTargetStructured,
                                         1,
                                         MoYu::AlignUp(sizeof(MoYu::TerrainCommandSignatureParams), 256),
                                         L"PatchNodeCommandSigBuffer",
                                         RHI::RHIBufferModeImmutable,
                                         D3D12_RESOURCE_STATE_GENERIC_READ);
        }


        // 主光源会使用到的IndexBuffer和CommandSignature
        {
            if (m_render_scene->m_directional_light.m_identifier != UndefCommonIdentifier)
            {
                if (dirShadowmapCommandBuffers.m_identifier != m_render_scene->m_directional_light.m_identifier)
                {
                    dirShadowmapCommandBuffers.Reset();
                }

                if (dirShadowmapCommandBuffers.m_PatchNodeVisiableIndexBuffers.size() != m_render_scene->m_directional_light.m_cascade)
                {
                    dirShadowmapCommandBuffers.m_PatchNodeVisiableIndexBuffers.clear();
                    dirShadowmapCommandBuffers.m_TerrainCommandSignatureBuffers.clear();

                    dirShadowmapCommandBuffers.m_identifier = m_render_scene->m_directional_light.m_identifier;

                    for (size_t i = 0; i < m_render_scene->m_directional_light.m_cascade; i++)
                    {
                        std::shared_ptr<RHI::D3D12Buffer> shadowPatchNodeVisiableIndexBuffer =
                            RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                     RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                                                     MaxTerrainNodeCount,
                                                     sizeof(uint32_t),
                                                     fmt::format(L"DirectionPatchNodeIndexBuffer_Cascade_{}", i));

                        dirShadowmapCommandBuffers.m_PatchNodeVisiableIndexBuffers.push_back(shadowPatchNodeVisiableIndexBuffer);

                        std::shared_ptr<RHI::D3D12Buffer> terrainCommandSignatureBuffer =
                            RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                     RHI::RHIBufferTargetStructured,
                                                     1,
                                                     MoYu::AlignUp(sizeof(MoYu::TerrainCommandSignatureParams), 256),
                                                     fmt::format(L"DirectionTerrainCommandSigBuffer_Cascade_{}", i),
                                                     RHI::RHIBufferModeImmutable,
                                                     D3D12_RESOURCE_STATE_GENERIC_READ);

                        dirShadowmapCommandBuffers.m_TerrainCommandSignatureBuffers.push_back(terrainCommandSignatureBuffer);

                    }
                }
            }
            else
            {
                dirShadowmapCommandBuffers.Reset();
            }
        }
    }

    ////===============================================================
    //// 内部缓存handle
    RHI::RgResourceHandle terrainHeightmapHandle;
    RHI::RgResourceHandle terrainNormalmapHandle;
    RHI::RgResourceHandle terrainMinHeightMapHandle;
    RHI::RgResourceHandle terrainMaxHeightMapHandle;
    RHI::RgResourceHandle mainCamVisiableIndexHandle;
    RHI::RgResourceHandle terrainPatchNodeHandle;
    RHI::RgResourceHandle mainCommandSigUploadBufferHandle;
    RHI::RgResourceHandle lastFrameNonVisiableIndexBufferHandle;
    ////===============================================================

    void IndirectTerrainCullPass::update(RHI::RenderGraph& graph, TerrainCullInput& passInput, TerrainCullOutput& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;
        std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrainNormalmap = internalTerrainRenderer.ref_terrain.terrain_normalmap;

        /*RHI::RgResourceHandle*/ terrainHeightmapHandle = GImport(graph, terrainHeightmap.get());
        /*RHI::RgResourceHandle*/ terrainNormalmapHandle = GImport(graph, terrainNormalmap.get());

        /*RHI::RgResourceHandle*/ terrainMinHeightMapHandle = GImport(graph, terrainMinHeightMap.get());
        /*RHI::RgResourceHandle*/ terrainMaxHeightMapHandle = GImport(graph, terrainMaxHeightMap.get());

        /*RHI::RgResourceHandle*/ terrainPatchNodeHandle = GImport(graph, terrainPatchNodeBuffer.get());

        /*RHI::RgResourceHandle*/ mainCamVisiableIndexHandle = GImport(graph, m_MainCameraVisiableIndexBuffer.get());

        std::vector<RHI::RgResourceHandle> dirShadowPatchNodeVisiableIndexHandles {};
        std::vector<RHI::RgResourceHandle> dirShadowCommandSigBufferHandles {};
        for (int i = 0; i < dirShadowmapCommandBuffers.m_PatchNodeVisiableIndexBuffers.size(); i++)
        {
            dirShadowPatchNodeVisiableIndexHandles.push_back(
                GImport(graph, dirShadowmapCommandBuffers.m_PatchNodeVisiableIndexBuffers[i].get()));
            dirShadowCommandSigBufferHandles.push_back(
                GImport(graph, dirShadowmapCommandBuffers.m_TerrainCommandSignatureBuffers[i].get()));
        }

        /*RHI::RgResourceHandle*/ mainCommandSigUploadBufferHandle = GImport(graph, terrainUploadCommandSigBuffer.get());

        RHI::RgResourceHandle perframeBufferHandle = passInput.perframeBufferHandle;
        
        //=================================================================================
        // 生成HeightMap的MaxHeightMap和MinHeightMap
        //=================================================================================
        if (!isTerrainMinMaxHeightReady)
        {
            isTerrainMinMaxHeightReady = true;

            RHI::RenderPass& genHeightMapMipMapPass = graph.AddRenderPass("GenerateHeightMapMipMapPass");

            genHeightMapMipMapPass.Read(terrainHeightmapHandle, true);
            genHeightMapMipMapPass.Read(terrainMinHeightMapHandle, true);
            genHeightMapMipMapPass.Read(terrainMaxHeightMapHandle, true);
            genHeightMapMipMapPass.Write(terrainMinHeightMapHandle, true);
            genHeightMapMipMapPass.Write(terrainMaxHeightMapHandle, true);

            genHeightMapMipMapPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                {
                    pContext->TransitionBarrier(RegGetTex(terrainHeightmapHandle), D3D12_RESOURCE_STATE_COPY_SOURCE);
                    pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_COPY_DEST, 0);
                    pContext->FlushResourceBarriers();

                    const CD3DX12_TEXTURE_COPY_LOCATION src(terrainHeightmap->GetResource(), 0);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst(terrainMinHeightMap->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

                    const CD3DX12_TEXTURE_COPY_LOCATION dst_max(terrainMaxHeightMap->GetResource(), 0);
                    context->GetGraphicsCommandList()->CopyTextureRegion(&dst_max, 0, 0, 0, &src, nullptr);

                    context->FlushResourceBarriers();
                }

                //--------------------------------------------------
                // 生成MinHeightMap
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMinHeightMapHandle);
                    generateMipmapForTerrainHeightmap(pContext, _SrcTexture, true);
                }
            
                //--------------------------------------------------
                // 生成MaxHeightMap
                //--------------------------------------------------
                {
                    RHI::D3D12Texture* _SrcTexture = RegGetTex(terrainMaxHeightMapHandle);
                    generateMipmapForTerrainHeightmap(pContext, _SrcTexture, false);
                }
            
            });
        }
        //=================================================================================
        // 根据相机位置生成Node节点
        //=================================================================================

        RHI::RenderPass& terrainNodesBuildPass = graph.AddRenderPass("TerrainNodesBuildPass");

        terrainNodesBuildPass.Read(perframeBufferHandle, true); // RHIResourceState::RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        terrainNodesBuildPass.Read(terrainMinHeightMapHandle, true);
        terrainNodesBuildPass.Read(terrainMaxHeightMapHandle, true);
        terrainNodesBuildPass.Write(terrainPatchNodeHandle, true); // RHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS
        
        terrainNodesBuildPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(terrainPatchNodeHandle));
            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->FlushResourceBarriers();

            RHI::D3D12Texture* minHeightmap = registry->GetD3D12Texture(terrainMinHeightMapHandle);
            auto minHeightmapDesc = minHeightmap->GetDesc();

            RHI::D3D12ConstantBufferView* perframeCBVHandle = registry->GetD3D12Buffer(perframeBufferHandle)->GetDefaultCBV().get();
            RHI::D3D12UnorderedAccessView* terrainNodeHandleUAV = registry->GetD3D12Buffer(terrainPatchNodeHandle)->GetDefaultUAV().get();
            RHI::D3D12ShaderResourceView* terrainMinHeightmapSRV = registry->GetD3D12Texture(terrainMinHeightMapHandle)->GetDefaultSRV().get();
            RHI::D3D12ShaderResourceView* terrainMaxHeightmapSRV = registry->GetD3D12Texture(terrainMaxHeightMapHandle)->GetDefaultSRV().get();

            pContext->SetRootSignature(pIndirecTerrainPatchNodesGenSignature.get());
            pContext->SetPipelineState(pIndirecTerrainPatchNodesGenPSO.get());
            pContext->SetConstant(0, 0, perframeCBVHandle->GetIndex());
            pContext->SetConstant(0, 1, terrainNodeHandleUAV->GetIndex());
            pContext->SetConstant(0, 2, terrainMinHeightmapSRV->GetIndex());
            pContext->SetConstant(0, 3, terrainMaxHeightmapSRV->GetIndex());
            
            pContext->Dispatch2D(minHeightmapDesc.Width, minHeightmapDesc.Height, 8, 8);
        });

        //=================================================================================
        // 主相机剪裁
        //=================================================================================
        
        RHI::RenderPass& terrainMainCullingPass = graph.AddRenderPass("TerrainMainCullingPass");

        terrainMainCullingPass.Read(perframeBufferHandle, true);
        terrainMainCullingPass.Read(terrainPatchNodeHandle, true);
        terrainMainCullingPass.Write(mainCamVisiableIndexHandle, true);

        terrainMainCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(mainCamVisiableIndexHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(mainCamVisiableIndexHandle));

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(mainCamVisiableIndexHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(mainCamVisiableIndexHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pPatchNodeVisToMainCamIndexGenSignature.get());
            pContext->SetPipelineState(pPatchNodeVisToMainCamIndexGenPSO.get());

            struct RootIndexBuffer
            {
                UINT meshPerFrameBufferIndex;
                UINT terrainPatchNodeIndex;
                UINT terrainPatchNodeCountIndex;
                UINT terrainVisNodeIdxIndex;
            };

            RootIndexBuffer rootIndexBuffer =
                RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                 RegGetBufDefSRVIdx(terrainPatchNodeHandle),
                                 RegGetBufCounterSRVIdx(terrainPatchNodeHandle),
                                 RegGetBufDefUAVIdx(mainCamVisiableIndexHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch1D(4096, 128);
        });
        
        //=================================================================================
        // 光源Shadow剪裁
        //=================================================================================
        int dirShadowmapVisiableBufferCount = dirShadowmapCommandBuffers.m_PatchNodeVisiableIndexBuffers.size();
        if (dirShadowmapVisiableBufferCount != 0)
        {
            RHI::RenderPass& dirLightShadowCullPass = graph.AddRenderPass("DirectionLightTerrainPatchNodeCullPass");

            dirLightShadowCullPass.Read(perframeBufferHandle, true);
            dirLightShadowCullPass.Read(terrainPatchNodeHandle, true);            
            for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
            {
                dirLightShadowCullPass.Write(dirShadowPatchNodeVisiableIndexHandles[i], true);
            }
            dirLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATE_COPY_DEST);
                }
                pContext->FlushResourceBarriers();
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->ResetCounter(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]));
                }
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBuf(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pContext->TransitionBarrier(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                }
                pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetBufCounter(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->FlushResourceBarriers();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->SetRootSignature(pPatchNodeVisToDirCascadeIndexGenSignature.get());
                    pContext->SetPipelineState(pPatchNodeVisToDirCascadeIndexGenPSO.get());

                    struct RootIndexBuffer
                    {
                        UINT cascadeLevel;
                        UINT meshPerFrameBufferIndex;
                        UINT terrainPatchNodeIndex;
                        UINT terrainPatchNodeCountIndex;
                        UINT terrainVisNodeIdxIndex;
                    };

                    RootIndexBuffer rootIndexBuffer =
                        RootIndexBuffer {(UINT)i,
                                         RegGetBufDefCBVIdx(perframeBufferHandle),
                                         RegGetBufDefSRVIdx(terrainPatchNodeHandle),
                                         RegGetBufCounterSRVIdx(terrainPatchNodeHandle),
                                         RegGetBufDefUAVIdx(dirShadowPatchNodeVisiableIndexHandles[i])};

                    pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

                    pContext->Dispatch1D(4096, 128);
                }
            });
        }

        //=================================================================================
        // 阴影相机地形CommandSignature准备
        //=================================================================================

        if (dirShadowmapVisiableBufferCount != 0)
        {
            RHI::RenderPass& dirShadowCullingPass = graph.AddRenderPass("TerrainCommandSignatureForDirShadowPreparePass");

            dirShadowCullingPass.Read(mainCommandSigUploadBufferHandle, true);
            for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
            {
                dirShadowCullingPass.Read(dirShadowPatchNodeVisiableIndexHandles[i], true);
                dirShadowCullingPass.Write(dirShadowCommandSigBufferHandles[i], true);
            }

            dirShadowCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                pContext->TransitionBarrier(RegGetBuf(mainCommandSigUploadBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ);
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);
                    pContext->TransitionBarrier(RegGetBuf(dirShadowCommandSigBufferHandles[i]), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                }
                pContext->FlushResourceBarriers();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->CopyBufferRegion(RegGetBuf(dirShadowCommandSigBufferHandles[i]),
                                               0,
                                               RegGetBuf(mainCommandSigUploadBufferHandle),
                                               0,
                                               sizeof(MoYu::TerrainCommandSignatureParams));
                    pContext->CopyBufferRegion(RegGetBuf(dirShadowCommandSigBufferHandles[i]),
                                               terrainInstanceCountOffset,
                                               RegGetBufCounter(dirShadowPatchNodeVisiableIndexHandles[i]),
                                               0,
                                               sizeof(int));
                    pContext->TransitionBarrier(RegGetBuf(dirShadowCommandSigBufferHandles[i]),
                                                D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                }

                pContext->FlushResourceBarriers();

            });
        }
        
        //=================================================================================
        // 输出Pass结果
        //=================================================================================

        passOutput.terrainHeightmapHandle       = terrainHeightmapHandle;
        passOutput.terrainNormalmapHandle       = terrainNormalmapHandle;
        passOutput.maxHeightmapPyramidHandle    = terrainMaxHeightMapHandle;
        passOutput.minHeightmapPyramidHandle    = terrainMinHeightMapHandle;
        passOutput.terrainPatchNodeBufferHandle = terrainPatchNodeHandle;

        for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
        {
            passOutput.directionShadowmapHandles.push_back(DrawCallCommandBufferHandle {dirShadowCommandSigBufferHandles[i], dirShadowPatchNodeVisiableIndexHandles[i]});
        }


    }

    void IndirectTerrainCullPass::cullingPassLastFrame(RHI::RenderGraph& graph, DepthCullInput& cullPassInput)
    {
        RHI::RgResourceHandle perframeBufferHandle         = cullPassInput.perframeBufferHandle;
        RHI::RgResourceHandle lastMinDepthPyramidHandle    = cullPassInput.minDepthPyramidHandle;
        RHI::RgResourceHandle terrainPatchNodeHandle       = cullPassInput.terrainPatchNodeHandle;
        RHI::RgResourceHandle inputIndexBufferHandle       = cullPassInput.inputIndexBufferHandle;
        RHI::RgResourceHandle nonVisiableIndexBufferHandle = cullPassInput.nonVisiableIndexBufferHandle;
        RHI::RgResourceHandle visiableIndexBufferHandle    = cullPassInput.visiableIndexBufferHandle;

        RHI::RenderPass& depthCullingPass = graph.AddRenderPass("DepthCullingForTerrain_0");

        depthCullingPass.Read(perframeBufferHandle, true);
        depthCullingPass.Read(lastMinDepthPyramidHandle, true);
        depthCullingPass.Read(terrainPatchNodeHandle, true);
        depthCullingPass.Read(inputIndexBufferHandle, true);
        depthCullingPass.Write(nonVisiableIndexBufferHandle, true);
        depthCullingPass.Write(visiableIndexBufferHandle, true);
        
        depthCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(nonVisiableIndexBufferHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->TransitionBarrier(RegGetBufCounter(visiableIndexBufferHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(nonVisiableIndexBufferHandle));
            pContext->ResetCounter(RegGetBufCounter(visiableIndexBufferHandle));

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetTex(lastMinDepthPyramidHandle), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(inputIndexBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBufCounter(inputIndexBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(nonVisiableIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(nonVisiableIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBuf(visiableIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(visiableIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();
            
            pContext->SetRootSignature(pPatchNodeCullByDepthSignature.get());
            pContext->SetPipelineState(pPatchNodeCullByDepthPSO.get());

            struct RootIndexBuffer
            {
                UINT perframeBufferIndex;
                UINT maxDepthPyramidIndex;
                UINT terrainPatchNodeIndex;
                UINT inputPatchNodeIdxBufferIndex;
                UINT inputPatchNodeIdxCounterIndex;
                UINT nonVisiableIndexBufferIndex;
                UINT visiableIndexBufferIndex;
            };

            RootIndexBuffer rootIndexBuffer =
                RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                 RegGetTexDefSRVIdx(lastMinDepthPyramidHandle),
                                 RegGetBufDefSRVIdx(terrainPatchNodeHandle),
                                 RegGetBufDefSRVIdx(inputIndexBufferHandle),
                                 RegGetBufCounterSRVIdx(inputIndexBufferHandle),
                                 RegGetBufDefUAVIdx(nonVisiableIndexBufferHandle),
                                 RegGetBufDefUAVIdx(visiableIndexBufferHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch1D(4096, 128);

        });

        cullPassInput.nonVisiableIndexBufferHandle = nonVisiableIndexBufferHandle;
        cullPassInput.visiableIndexBufferHandle    = visiableIndexBufferHandle;
    }

    void IndirectTerrainCullPass::cullingPassCurFrame(RHI::RenderGraph& graph, DepthCullInput& cullPassInput)
    {
        RHI::RgResourceHandle perframeBufferHandle         = cullPassInput.perframeBufferHandle;
        RHI::RgResourceHandle lastMinDepthPyramidHandle    = cullPassInput.minDepthPyramidHandle;
        RHI::RgResourceHandle terrainPatchNodeHandle       = cullPassInput.terrainPatchNodeHandle;
        RHI::RgResourceHandle inputIndexBufferHandle       = cullPassInput.inputIndexBufferHandle;
        RHI::RgResourceHandle visiableIndexBufferHandle    = cullPassInput.visiableIndexBufferHandle;

        RHI::RenderPass& depthCullingPass = graph.AddRenderPass("DepthCullingForTerrain_1");

        depthCullingPass.Read(perframeBufferHandle, true);
        depthCullingPass.Read(lastMinDepthPyramidHandle, true);
        depthCullingPass.Read(terrainPatchNodeHandle, true);
        depthCullingPass.Read(inputIndexBufferHandle, true);
        depthCullingPass.Write(visiableIndexBufferHandle, true);
        
        depthCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(visiableIndexBufferHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(visiableIndexBufferHandle));

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetTex(lastMinDepthPyramidHandle), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(terrainPatchNodeHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(inputIndexBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBufCounter(inputIndexBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(visiableIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(visiableIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();
            
            pContext->SetRootSignature(pPatchNodeCullByDepthSignature2.get());
            pContext->SetPipelineState(pPatchNodeCullByDepthPSO2.get());

            struct RootIndexBuffer
            {
                UINT perframeBufferIndex;
                UINT maxDepthPyramidIndex;
                UINT terrainPatchNodeIndex;
                UINT inputPatchNodeIdxBufferIndex;
                UINT inputPatchNodeIdxCounterIndex;
                UINT visiableIndexBufferIndex;
            };

            RootIndexBuffer rootIndexBuffer =
                RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                 RegGetTexDefSRVIdx(lastMinDepthPyramidHandle),
                                 RegGetBufDefSRVIdx(terrainPatchNodeHandle),
                                 RegGetBufDefSRVIdx(inputIndexBufferHandle),
                                 RegGetBufCounterSRVIdx(inputIndexBufferHandle),
                                 RegGetBufDefUAVIdx(visiableIndexBufferHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch1D(4096, 128);

        });

        cullPassInput.visiableIndexBufferHandle    = visiableIndexBufferHandle;
    }

    void IndirectTerrainCullPass::genCullCommandSignature(RHI::RenderGraph& graph, DepthCommandSigGenInput& cmdBufferInput)
    {
        RHI::RenderPass& cameraCullingPass = graph.AddRenderPass("TerrainCommandSignaturePass");

        RHI::RgResourceHandle visiableIndexBufferHandle = cmdBufferInput.inputIndexBufferHandle;
        RHI::RgResourceHandle terrainCommandSigBufHandle = cmdBufferInput.terrainCommandSigBufHandle;

        cameraCullingPass.Read(mainCommandSigUploadBufferHandle, true);
        cameraCullingPass.Read(visiableIndexBufferHandle, true);
        cameraCullingPass.Write(terrainCommandSigBufHandle, true);

        cameraCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBuf(mainCommandSigUploadBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ);
            pContext->TransitionBarrier(RegGetBufCounter(visiableIndexBufferHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);
            pContext->TransitionBarrier(RegGetBuf(terrainCommandSigBufHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();

            pContext->CopyBufferRegion(RegGetBuf(terrainCommandSigBufHandle), 0, RegGetBuf(mainCommandSigUploadBufferHandle), 0, sizeof(MoYu::TerrainCommandSignatureParams));
            pContext->CopyBufferRegion(RegGetBuf(terrainCommandSigBufHandle), terrainInstanceCountOffset, RegGetBufCounter(visiableIndexBufferHandle), 0, sizeof(int));
            
            pContext->TransitionBarrier(RegGetBuf(terrainCommandSigBufHandle), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            pContext->FlushResourceBarriers();

        });

        cmdBufferInput.terrainCommandSigBufHandle = terrainCommandSigBufHandle;
    }

    void IndirectTerrainCullPass::cullByLastFrameDepth(RHI::RenderGraph& graph, DepthCullIndexInput& input, DrawCallCommandBufferHandle& output)
    {
        RHI::RgResourceHandle terrainCommandSigBufHandle = GImport(graph, lastDepthCullRemainIndexBuffer.m_TerrainCommandSignatureBuffer.get()); 
        RHI::RgResourceHandle visiableIndexBufferHandle = GImport(graph, lastDepthCullRemainIndexBuffer.m_PatchNodeVisiableIndexBuffer.get());
        RHI::RgResourceHandle nonVisiableIndexBufferHandle = GImport(graph, lastDepthCullRemainIndexBuffer.m_PatchNodeNonVisiableIndexBuffer.get());

        //=================================================================================
        // 使用上一帧depth进行剪裁
        //=================================================================================
        DepthCullInput cullPassInput               = {};
        cullPassInput.perframeBufferHandle         = input.perframeBufferHandle;
        cullPassInput.minDepthPyramidHandle        = input.minDepthPyramidHandle;
        cullPassInput.terrainPatchNodeHandle       = terrainPatchNodeHandle;
        cullPassInput.inputIndexBufferHandle       = mainCamVisiableIndexHandle;
        cullPassInput.nonVisiableIndexBufferHandle = nonVisiableIndexBufferHandle;
        cullPassInput.visiableIndexBufferHandle    = visiableIndexBufferHandle;
        cullingPassLastFrame(graph, cullPassInput);

        //=================================================================================
        // 主相机地形CommandSignature准备
        //=================================================================================
        DepthCommandSigGenInput cmdBufferInput = {};
        cmdBufferInput.inputIndexBufferHandle  = cullPassInput.visiableIndexBufferHandle; 
        cmdBufferInput.terrainCommandSigBufHandle = terrainCommandSigBufHandle;
        genCullCommandSignature(graph, cmdBufferInput);

        // 保存不可见index，用于给当前帧depth再做一次剪裁
        lastFrameNonVisiableIndexBufferHandle = cullPassInput.nonVisiableIndexBufferHandle;

        // 输出index和commandsignature
        output.commandSigBufferHandle = cmdBufferInput.terrainCommandSigBufHandle;
        output.indirectIndexBufferHandle = cmdBufferInput.inputIndexBufferHandle;
    }

    void IndirectTerrainCullPass::cullByCurrentFrameDepth(RHI::RenderGraph& graph, DepthCullIndexInput& input, DrawCallCommandBufferHandle& output)
    {
        RHI::RgResourceHandle terrainCommandSigBufHandle = GImport(graph, curDepthCullRemainIndexBuffer.m_TerrainCommandSignatureBuffer.get()); 
        RHI::RgResourceHandle visiableIndexBufferHandle = GImport(graph, curDepthCullRemainIndexBuffer.m_PatchNodeVisiableIndexBuffer.get());
        
        //=================================================================================
        // 使用上一帧depth进行剪裁
        //=================================================================================
        DepthCullInput cullPassInput               = {};
        cullPassInput.perframeBufferHandle         = input.perframeBufferHandle;
        cullPassInput.minDepthPyramidHandle        = input.minDepthPyramidHandle;
        cullPassInput.terrainPatchNodeHandle       = terrainPatchNodeHandle;
        cullPassInput.inputIndexBufferHandle       = lastFrameNonVisiableIndexBufferHandle;
        cullPassInput.visiableIndexBufferHandle    = visiableIndexBufferHandle;
        cullingPassCurFrame(graph, cullPassInput);

        //=================================================================================
        // 主相机地形CommandSignature准备
        //=================================================================================
        DepthCommandSigGenInput cmdBufferInput = {};
        cmdBufferInput.inputIndexBufferHandle  = cullPassInput.visiableIndexBufferHandle; 
        cmdBufferInput.terrainCommandSigBufHandle = terrainCommandSigBufHandle;
        genCullCommandSignature(graph, cmdBufferInput);

        // 输出index和commandsignature
        output.commandSigBufferHandle = cmdBufferInput.terrainCommandSigBufHandle;
        output.indirectIndexBufferHandle = cmdBufferInput.inputIndexBufferHandle;
    }

    void IndirectTerrainCullPass::destroy()
    {

    }

    bool IndirectTerrainCullPass::initializeRenderTarget(RHI::RenderGraph& graph, TerrainCullOutput* drawPassOutput)
    {
        return true;
    }

    void IndirectTerrainCullPass::generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* _SrcTexture, bool genMin)
    {
        int numSubresource = _SrcTexture->GetNumSubresources();
        int iterCount = glm::ceil(numSubresource / 4.0f);
        for (int i = 0; i < iterCount; i++)
        {
            generateMipmapForTerrainHeightmap(context, _SrcTexture, 4 * i, genMin);
        }
    }

    void IndirectTerrainCullPass::generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* pContext, RHI::D3D12Texture* _SrcTexture, int srcIndex, bool genMin)
    {
        int numSubresource = _SrcTexture->GetNumSubresources();
        int outMipCount = glm::max(0, glm::min(numSubresource - 1 - srcIndex, 4));

        pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, srcIndex);
        for (int i = 1; i < outMipCount; i++)
        {
            pContext->TransitionBarrier(_SrcTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, i + srcIndex);
        }
        pContext->FlushResourceBarriers();

        auto _SrcDesc = _SrcTexture->GetDesc();
        auto _Mip0SRVDesc = RHI::D3D12ShaderResourceView::GetDesc(_SrcTexture, false, srcIndex, 1);
        glm::uint _SrcIndex = _SrcTexture->CreateSRV(_Mip0SRVDesc)->GetIndex();

        int _width  = _SrcDesc.Width >> (srcIndex + 1);
        int _height = _SrcDesc.Height >> (srcIndex + 1);

        glm::float2 _Mip1TexelSize = {1.0f / _width, 1.0f / _height};
        glm::uint   _SrcMipLevel   = srcIndex;
        glm::uint   _NumMipLevels  = outMipCount;

        MipGenInBuffer maxMipGenBuffer = {};

        maxMipGenBuffer.SrcMipLevel   = _SrcMipLevel;
        maxMipGenBuffer.NumMipLevels  = _NumMipLevels;
        maxMipGenBuffer.TexelSize     = _Mip1TexelSize;
        maxMipGenBuffer.SrcIndex      = _SrcIndex;

        for (int i = 0; i < outMipCount; i++)
        {
            auto _Mip1UAVDesc = RHI::D3D12UnorderedAccessView::GetDesc(_SrcTexture, 0, srcIndex + i + 1);
            glm::uint _OutMip1Index = _SrcTexture->CreateUAV(_Mip1UAVDesc)->GetIndex();
            *(&maxMipGenBuffer.OutMip1Index + i) = _OutMip1Index;
        }

        pContext->SetRootSignature(RootSignatures::pGenerateMipsLinearSignature.get());
        if (genMin)
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMinMipsLinearPSO.get());
        }
        else
        {
            pContext->SetPipelineState(PipelineStates::pGenerateMaxMipsLinearPSO.get());
        }

        pContext->SetConstantArray(0, sizeof(MipGenInBuffer) / sizeof(int), &maxMipGenBuffer);

        pContext->Dispatch2D(_width, _height, 8, 8);
    }

}
