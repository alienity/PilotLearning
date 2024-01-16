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
    void IndirectTerrainCullPass::initialize(const TerrainCullInitInfo& init_info)
    {
        ShaderCompiler*       m_ShaderCompiler = init_info.m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath = init_info.m_ShaderRootPath;

        //--------------------------------------------------------------------------
        {
            mainCamVisClipmapCS =
                m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                m_ShaderRootPath / "hlsl/ClipmapCullingForMainCamera.hlsl",
                                                ShaderCompileOptions(L"CSMain"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pMainCamVisClipmapSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pMainCamVisClipmapSignature.get());
            psoStream.CS                    = &mainCamVisClipmapCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pMainCamVisClipmapGenPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"MainCamVisClipmapGenPSO", psoDesc);
        }
        //--------------------------------------------------------------------------

        //--------------------------------------------------------------------------
        {
            dirVisClipmapCS = m_ShaderCompiler->CompileShader(RHI_SHADER_TYPE::Compute,
                                                              m_ShaderRootPath / "hlsl/ClipmapCullingForDirShadow.hlsl",
                                                              ShaderCompileOptions(L"CSMain"));

            RHI::RootSignatureDesc rootSigDesc =
                RHI::RootSignatureDesc()
                    .Add32BitConstants<0, 0>(12)
                    .AddStaticSampler<10, 0>(D3D12_FILTER::D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8)
                    .AllowInputLayout()
                    .AllowResourceDescriptorHeapIndexing()
                    .AllowSampleDescriptorHeapIndexing();

            pDirVisClipmapSignature = std::make_shared<RHI::D3D12RootSignature>(m_Device, rootSigDesc);

            struct PsoStream
            {
                PipelineStateStreamRootSignature RootSignature;
                PipelineStateStreamCS            CS;
            } psoStream;
            psoStream.RootSignature         = PipelineStateStreamRootSignature(pDirVisClipmapSignature.get());
            psoStream.CS                    = &dirVisClipmapCS;
            PipelineStateStreamDesc psoDesc = {sizeof(PsoStream), &psoStream};

            pDirVisClipmapGenPSO =
                std::make_shared<RHI::D3D12PipelineState>(m_Device, L"DirVisClipmapGenPSO", psoDesc);
        }
        //--------------------------------------------------------------------------

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
                                            10,
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
                                            10,
                                            heightmapDesc.Format,
                                            RHI::RHISurfaceCreateRandomWrite | RHI::RHISurfaceCreateMipmap,
                                            1,
                                            L"TerrainMaxHeightmap",
                                            D3D12_RESOURCE_STATE_COMMON,
                                            std::nullopt);
        }

        if (clipmapBaseCommandSigBuffer == nullptr)
        {
            clipmapBaseCommandSigBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                             RHI::RHIBufferTargetStructured,
                                                             HLSL::GeoClipMeshType,
                                                             sizeof(HLSL::ClipMeshCommandSigParams),
                                                             L"ClipmapBaseCommandSigBuffer",
                                                             RHI::RHIBufferModeImmutable,
                                                             D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        {
            std::vector<CachedTerrainRenderer>& _terrain_renderers = m_render_scene->m_terrain_renderers;
            InternalTerrainRenderer& _internalTerrainRenderer      = _terrain_renderers[0].internalTerrainRenderer;

            uint32_t clip_transform_counts =
                _internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_transform_counts;

            // Clipmap Transform Buffer
            if (clipmapTransformBuffer == nullptr)
            {
                clipmapTransformBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                                  RHI::RHIBufferTargetStructured,
                                                                  clip_transform_counts,
                                                                  sizeof(HLSL::ClipmapTransform),
                                                                  L"ClipmapTransformBuffer",
                                                                  RHI::RHIBufferModeImmutable,
                                                                  D3D12_RESOURCE_STATE_GENERIC_READ);
            }

            if (clipmapMeshCountBuffer == nullptr)
            {
                clipmapMeshCountBuffer = RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                                  RHI::RHIBufferTargetStructured,
                                                                  1,
                                                                  MoYu::AlignUp(sizeof(HLSL::TerrainPatchNode), 256),
                                                                  L"ClipmapMeshCountBuffer",
                                                                  RHI::RHIBufferModeImmutable,
                                                                  D3D12_RESOURCE_STATE_GENERIC_READ);
            }

            // 主相机会使用到的CommandSignatureBuffer
            if (camVisableClipmapBuffer == nullptr)
            {
                camVisableClipmapBuffer = RHI::D3D12Buffer::Create(
                    m_Device->GetLinkedDevice(),
                    RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                    clip_transform_counts,
                    sizeof(HLSL::ToDrawCommandSignatureParams),
                    L"CamVisableClipmapBuffer",
                    RHI::RHIBufferModeImmutable,
                    D3D12_RESOURCE_STATE_GENERIC_READ);
            }

            // 主光源会使用到的CommandSignatureBuffer
            if (m_render_scene->m_directional_light.m_identifier != UndefCommonIdentifier)
            {
                if (dirVisableClipmapBuffers.m_identifier != m_render_scene->m_directional_light.m_identifier)
                {
                    dirVisableClipmapBuffers.Reset();
                }

                if (dirVisableClipmapBuffers.m_VisableClipmapBuffers.size() !=
                    m_render_scene->m_directional_light.m_cascade)
                {
                    dirVisableClipmapBuffers.m_VisableClipmapBuffers.clear();

                    dirVisableClipmapBuffers.m_identifier = m_render_scene->m_directional_light.m_identifier;

                    for (size_t i = 0; i < m_render_scene->m_directional_light.m_cascade; i++)
                    {
                        std::shared_ptr<RHI::D3D12Buffer> shadowClipNodeVisiableBuffer =
                            RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                     RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter,
                                                     clip_transform_counts,
                                                    sizeof(HLSL::ToDrawCommandSignatureParams),
                                                     fmt::format(L"DirectionVisableClipmapBuffer_Cascade_{}", i));

                        dirVisableClipmapBuffers.m_VisableClipmapBuffers.push_back(shadowClipNodeVisiableBuffer);
                    }
                }
            }
            else
            {
                dirVisableClipmapBuffers.Reset();
            }
        }
    }

    void IndirectTerrainCullPass::update(RHI::RenderGraph& graph, TerrainCullInput& passInput, TerrainCullOutput& passOutput)
    {
        bool needClearRenderTarget = initializeRenderTarget(graph, &passOutput);

        InternalTerrainRenderer& internalTerrainRenderer = m_render_scene->m_terrain_renderers[0].internalTerrainRenderer;
        std::shared_ptr<RHI::D3D12Texture> terrainHeightmap = internalTerrainRenderer.ref_terrain.terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrainNormalmap = internalTerrainRenderer.ref_terrain.terrain_normalmap;

        std::shared_ptr<RHI::D3D12Buffer> uploadTransformBuffer = internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_transform_buffer;
        std::shared_ptr<RHI::D3D12Buffer> uploadMeshCountBuffer = internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_mesh_count_buffer;
        std::shared_ptr<RHI::D3D12Buffer> uploadBaseMeshCommandSigBuffer = internalTerrainRenderer.ref_terrain.terrain_clipmap.instance_buffer.clip_base_mesh_command_buffer;

        RHI::RgResourceHandle terrainHeightmapHandle = GImport(graph, terrainHeightmap.get());
        RHI::RgResourceHandle terrainNormalmapHandle = GImport(graph, terrainNormalmap.get());

        RHI::RgResourceHandle terrainMinHeightMapHandle = GImport(graph, terrainMinHeightMap.get());
        RHI::RgResourceHandle terrainMaxHeightMapHandle = GImport(graph, terrainMaxHeightMap.get());

        RHI::RgResourceHandle uploadBaseMeshCommandSigHandle = GImport(graph, uploadBaseMeshCommandSigBuffer.get());
        RHI::RgResourceHandle uploadClipmapTransformHandle   = GImport(graph, uploadTransformBuffer.get());
        RHI::RgResourceHandle uploadMeshCountHandle          = GImport(graph, uploadMeshCountBuffer.get());

        RHI::RgResourceHandle clipmapBaseCommandSighandle = GImport(graph, clipmapBaseCommandSigBuffer.get());
        RHI::RgResourceHandle clipmapTransformHandle      = GImport(graph, clipmapTransformBuffer.get());
        RHI::RgResourceHandle clipmapMeshCountHandle      = GImport(graph, clipmapMeshCountBuffer.get());

        RHI::RgResourceHandle camVisableClipmapHandle = GImport(graph, camVisableClipmapBuffer.get());
        
        std::vector<RHI::RgResourceHandle> dirVisableClipmapHandles {};
        int dirVisableClipmapBufferCount = dirVisableClipmapBuffers.m_VisableClipmapBuffers.size();
        for (int i = 0; i < dirVisableClipmapBufferCount; i++)
        {
            dirVisableClipmapHandles.push_back(GImport(graph, dirVisableClipmapBuffers.m_VisableClipmapBuffers[i].get()));
        }


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
        // 主相机准备CommandSigBuffer数据
        //=================================================================================
        RHI::RenderPass& terrainCommandSigPreparePass = graph.AddRenderPass("TerrainClipmapPreparePass");

        terrainCommandSigPreparePass.Read(uploadBaseMeshCommandSigHandle, true);
        terrainCommandSigPreparePass.Read(uploadClipmapTransformHandle, true);
        terrainCommandSigPreparePass.Read(uploadMeshCountHandle, true);
        terrainCommandSigPreparePass.Write(clipmapBaseCommandSighandle, true);
        terrainCommandSigPreparePass.Write(clipmapTransformHandle, true);
        terrainCommandSigPreparePass.Write(clipmapMeshCountHandle, true);

        terrainCommandSigPreparePass.Execute(
            [=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                pContext->TransitionBarrier(RegGetBuf(clipmapBaseCommandSighandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->TransitionBarrier(RegGetBuf(clipmapTransformHandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->TransitionBarrier(RegGetBuf(clipmapMeshCountHandle), D3D12_RESOURCE_STATE_COPY_DEST);
                pContext->FlushResourceBarriers();

                pContext->CopyBuffer(RegGetBuf(clipmapBaseCommandSighandle), RegGetBuf(uploadBaseMeshCommandSigHandle));
                pContext->CopyBuffer(RegGetBuf(clipmapTransformHandle), RegGetBuf(uploadClipmapTransformHandle));
                pContext->CopyBuffer(RegGetBuf(clipmapMeshCountHandle), RegGetBuf(uploadMeshCountHandle));

                pContext->FlushResourceBarriers();
            });

        //=================================================================================
        // 主相机剪裁clipmap节点
        //=================================================================================
        RHI::RenderPass& terrainMainCullingPass = graph.AddRenderPass("TerrainMainCullingPass");

        terrainMainCullingPass.Read(perframeBufferHandle, true);
        terrainMainCullingPass.Read(clipmapBaseCommandSighandle, true);
        terrainMainCullingPass.Read(clipmapTransformHandle, true);
        terrainMainCullingPass.Read(clipmapMeshCountHandle, true);
        terrainMainCullingPass.Read(terrainMinHeightMapHandle, true);
        terrainMainCullingPass.Read(terrainMaxHeightMapHandle, true);
        terrainMainCullingPass.Write(camVisableClipmapHandle, true);

        terrainMainCullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
            RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

            pContext->TransitionBarrier(RegGetBufCounter(camVisableClipmapHandle), D3D12_RESOURCE_STATE_COPY_DEST);
            pContext->FlushResourceBarriers();
            pContext->ResetCounter(RegGetBufCounter(camVisableClipmapHandle));

            pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(clipmapMeshCountHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            pContext->TransitionBarrier(RegGetBuf(clipmapBaseCommandSighandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(clipmapTransformHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            pContext->TransitionBarrier(RegGetBuf(camVisableClipmapHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->TransitionBarrier(RegGetBufCounter(camVisableClipmapHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pContext->FlushResourceBarriers();

            pContext->SetRootSignature(pMainCamVisClipmapSignature.get());
            pContext->SetPipelineState(pMainCamVisClipmapGenPSO.get());

            struct RootIndexBuffer
            {
                uint32_t perFrameBufferIndex;
                uint32_t clipmapMeshCountIndex;
                uint32_t transformBufferIndex;
                uint32_t clipmapBaseCommandSigIndex;
                uint32_t terrainMinHeightmapIndex;
                uint32_t terrainMaxHeightmapIndex;
                uint32_t clipmapVisableBufferIndex;
            };

            RootIndexBuffer rootIndexBuffer = RootIndexBuffer {RegGetBufDefCBVIdx(perframeBufferHandle),
                                                               RegGetBufDefCBVIdx(clipmapMeshCountHandle),
                                                               RegGetBufDefSRVIdx(clipmapTransformHandle),
                                                               RegGetBufDefSRVIdx(clipmapBaseCommandSighandle),
                                                               RegGetTexDefSRVIdx(terrainMinHeightMapHandle),
                                                               RegGetTexDefSRVIdx(terrainMaxHeightMapHandle),
                                                               RegGetBufDefUAVIdx(camVisableClipmapHandle)};

            pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

            pContext->Dispatch1D(256, 8);
        });

        //=================================================================================
        // 光源Shadow剪裁
        //=================================================================================
        int dirShadowmapVisiableBufferCount = dirVisableClipmapBuffers.m_VisableClipmapBuffers.size();
        if (dirShadowmapVisiableBufferCount != 0)
        {
            RHI::RenderPass& dirLightShadowCullPass = graph.AddRenderPass("DirectionLightTerrainCullingPass");

            dirLightShadowCullPass.Read(perframeBufferHandle, true);
            dirLightShadowCullPass.Read(clipmapBaseCommandSighandle, true);
            dirLightShadowCullPass.Read(clipmapTransformHandle, true);
            dirLightShadowCullPass.Read(clipmapMeshCountHandle, true);
            dirLightShadowCullPass.Read(terrainMinHeightMapHandle, true);
            dirLightShadowCullPass.Read(terrainMaxHeightMapHandle, true);  
            for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
            {
                dirLightShadowCullPass.Write(dirVisableClipmapHandles[i], true);
            }
            dirLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pContext = context->GetComputeContext();

                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBufCounter(dirVisableClipmapHandles[i]), D3D12_RESOURCE_STATE_COPY_DEST);
                }
                pContext->FlushResourceBarriers();
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->ResetCounter(RegGetBufCounter(dirVisableClipmapHandles[i]));
                }
                pContext->TransitionBarrier(RegGetBuf(perframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pContext->TransitionBarrier(RegGetBuf(clipmapMeshCountHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pContext->TransitionBarrier(RegGetBuf(clipmapBaseCommandSighandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetBuf(clipmapTransformHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetTex(terrainMinHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pContext->TransitionBarrier(RegGetTex(terrainMaxHeightMapHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                for (int i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->TransitionBarrier(RegGetBuf(dirVisableClipmapHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pContext->TransitionBarrier(RegGetBufCounter(dirVisableClipmapHandles[i]), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                }
                pContext->FlushResourceBarriers();

                for (uint32_t i = 0; i < dirShadowmapVisiableBufferCount; i++)
                {
                    pContext->SetRootSignature(pDirVisClipmapSignature.get());
                    pContext->SetPipelineState(pDirVisClipmapGenPSO.get());

                    struct RootIndexBuffer
                    {
                        uint32_t cascadeLevel;
                        uint32_t perFrameBufferIndex;
                        uint32_t clipmapMeshCountIndex;
                        uint32_t transformBufferIndex;
                        uint32_t clipmapBaseCommandSigIndex;
                        uint32_t terrainMinHeightmapIndex;
                        uint32_t terrainMaxHeightmapIndex;
                        uint32_t clipmapVisableBufferIndex;
                    };

                    RootIndexBuffer rootIndexBuffer = RootIndexBuffer {i,
                                                                       RegGetBufDefCBVIdx(perframeBufferHandle),
                                                                       RegGetBufDefCBVIdx(clipmapMeshCountHandle),
                                                                       RegGetBufDefSRVIdx(clipmapTransformHandle),
                                                                       RegGetBufDefSRVIdx(clipmapBaseCommandSighandle),
                                                                       RegGetTexDefSRVIdx(terrainMinHeightMapHandle),
                                                                       RegGetTexDefSRVIdx(terrainMaxHeightMapHandle),
                                                                       RegGetBufDefUAVIdx(dirVisableClipmapHandles[i])};

                    pContext->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);

                    pContext->Dispatch1D(256, 8);
                }
            });
        }



        /*
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
        */
        //=================================================================================
        // 输出Pass结果
        //=================================================================================

        passOutput.terrainHeightmapHandle    = terrainHeightmapHandle;
        passOutput.terrainNormalmapHandle    = terrainNormalmapHandle;
        passOutput.maxHeightmapPyramidHandle = terrainMaxHeightMapHandle;
        passOutput.minHeightmapPyramidHandle = terrainMinHeightMapHandle;

        passOutput.transformBufferHandle     = clipmapTransformHandle;

        passOutput.mainCamVisCommandSigHandle = camVisableClipmapHandle;
        passOutput.dirVisCommandSigHandles.assign(dirVisableClipmapHandles.begin(), dirVisableClipmapHandles.end());

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
