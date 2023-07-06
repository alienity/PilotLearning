#include "runtime/function/render/renderer/indirect_cull_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/core/math/moyu_math.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsCommon.h"
#include "runtime/function/render/glm_wrapper.h"

#include <cassert>

namespace MoYu
{
#define RegGetBuf(h) registry->GetD3D12Buffer(h)
#define RegGetBufCounter(h) registry->GetD3D12Buffer(h)->GetCounterBuffer().get()
#define RegGetTex(h) registry->GetD3D12Texture(h)
#define RegGetBufDefCBVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultCBV()->GetIndex()
#define RegGetBufDefSRVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultSRV()->GetIndex()
#define RegGetBufDefUAVIdx(h) registry->GetD3D12Buffer(h)->GetDefaultUAV()->GetIndex()

#define CreateCullingBuffer(numElement, elementSize, bufferName) \
    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(), \
                             RHI::RHIBufferRandomReadWrite, \
                             numElement, \
                             elementSize, \
                             bufferName, \
                             RHI::RHIBufferModeImmutable, \
                             D3D12_RESOURCE_STATE_GENERIC_READ)

#define CreateUploadBuffer(numElement, elementSize, bufferName) \
    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(), \
                             RHI::RHIBufferTargetNone, \
                             numElement, \
                             elementSize, \
                             bufferName, \
                             RHI::RHIBufferModeDynamic, \
                             D3D12_RESOURCE_STATE_GENERIC_READ)

#define CreateArgBufferDesc(name, numElements) \
    RHI::RgBufferDesc(name) \
        .SetSize(numElements, sizeof(D3D12_DISPATCH_ARGUMENTS)) \
        .SetRHIBufferMode(RHI::RHIBufferMode::RHIBufferModeImmutable) \
        .SetRHIBufferTarget(RHI::RHIBufferTarget::RHIBufferTargetIndirectArgs | RHI::RHIBufferTarget::RHIBufferRandomReadWrite | RHI::RHIBufferTarget::RHIBufferTargetRaw)

#define CreateIndexBuffer(name) \
    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(), \
                             RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter, \
                             HLSL::MeshLimit, \
                             sizeof(HLSL::BitonicSortCommandSigParams), \
                             name)

#define CreateSortCommandBuffer(name) \
    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(), \
                             RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter, \
                             HLSL::MeshLimit, \
                             sizeof(HLSL::CommandSignatureParams), \
                             name)

    void IndirectCullPass::initialize(const RenderPassInitInfo& init_info)
    {
        // create default buffer
        pPerframeBuffer = CreateCullingBuffer(1, sizeof(HLSL::MeshPerframeStorageBufferObject), L"PerFrameBuffer");
        pMaterialBuffer = CreateCullingBuffer(HLSL::MaterialLimit, sizeof(HLSL::MaterialInstance), L"MaterialBuffer");
        pMeshBuffer     = CreateCullingBuffer(HLSL::MeshLimit, sizeof(HLSL::MeshInstance), L"MeshBuffer");

        // create upload buffer
        pUploadPerframeBuffer = CreateUploadBuffer(1, sizeof(HLSL::MeshPerframeStorageBufferObject), L"UploadPerFrameBuffer");
        pUploadMaterialBuffer = CreateUploadBuffer(HLSL::MaterialLimit, sizeof(HLSL::MaterialInstance), L"MaterialBuffer");
        pUploadMeshBuffer = CreateUploadBuffer(HLSL::MeshLimit, sizeof(HLSL::MeshInstance), L"MeshBuffer");

        sortDispatchArgsBufferDesc = CreateArgBufferDesc("SortDispatchArgs", 22 * 23 / 2);
        grabDispatchArgsBufferDesc = CreateArgBufferDesc("GrabDispatchArgs", 22 * 23 / 2);


        // buffer for opaque draw
        commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer = CreateIndexBuffer(L"OpaqueIndexBuffer");
        commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer  = CreateSortCommandBuffer(L"OpaqueBuffer");

        // buffer for transparent draw
        commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer = CreateIndexBuffer(L"TransparentIndexBuffer");
        commandBufferForTransparentDraw.p_IndirectSortCommandBuffer  = CreateSortCommandBuffer(L"TransparentBuffer");
    }

    void IndirectCullPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        // update per-frame buffer
        render_resource->updatePerFrameBuffer(m_render_scene, m_render_camera);

        memcpy(pUploadPerframeBuffer->GetCpuVirtualAddress<HLSL::MeshPerframeStorageBufferObject>(),
               &render_resource->m_mesh_perframe_storage_buffer_object,
               sizeof(HLSL::MeshPerframeStorageBufferObject));

        std::vector<CachedMeshRenderer>& _mesh_renderers = m_render_scene->m_mesh_renderers;

        HLSL::MaterialInstance* pMaterialObj = pUploadMaterialBuffer->GetCpuVirtualAddress<HLSL::MaterialInstance>();
        HLSL::MeshInstance* pMeshesObj = pUploadMeshBuffer->GetCpuVirtualAddress<HLSL::MeshInstance>();

        uint32_t numMeshes = _mesh_renderers.size();
        ASSERT(numMeshes < HLSL::MeshLimit);
        for (size_t i = 0; i < numMeshes; i++)
        {
            InternalMeshRenderer& temp_mesh_renderer = _mesh_renderers[i].internalMeshRenderer;

            InternalMesh& temp_ref_mesh = temp_mesh_renderer.ref_mesh;
            InternalMaterial& temp_ref_material = temp_mesh_renderer.ref_material;

            //std::shared_ptr<RHI::D3D12ShaderResourceView> defaultWhiteView = real_resource->m_default_resource._white_texture2d_image->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> defaultBlackView = real_resource->m_default_resource._black_texture2d_image->GetDefaultSRV();

            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultWhiteView = RHI::GetDefaultTexture(RHI::eDefaultTexture::kWhiteOpaque2D)->GetDefaultSRV();
            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultBlackView = RHI::GetDefaultTexture(RHI::eDefaultTexture::kBlackOpaque2D)->GetDefaultSRV();

            std::shared_ptr<RHI::D3D12ShaderResourceView> uniformBufferView = temp_ref_material.m_intenral_pbr_mat.material_uniform_buffer->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> baseColorView = temp_ref_material.m_intenral_pbr_mat.base_color_texture_image->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> metallicRoughnessView = temp_ref_material.m_intenral_pbr_mat.metallic_roughness_texture_image->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> normalView   = temp_ref_material.m_intenral_pbr_mat.normal_texture_image->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> emissionView = temp_ref_material.m_intenral_pbr_mat.emissive_texture_image->GetDefaultSRV();

            #define Tex2ViewIndex(toIndex, fromTex)\
            if (fromTex == nullptr)\
                toIndex = defaultWhiteView->GetIndex();\
            else\
                toIndex = fromTex->GetDefaultSRV()->GetIndex();\

            HLSL::MaterialInstance curMatInstance = {};
            curMatInstance.uniformBufferViewIndex = uniformBufferView->GetIndex();            
            Tex2ViewIndex(curMatInstance.baseColorViewIndex, temp_ref_material.m_intenral_pbr_mat.base_color_texture_image)
            Tex2ViewIndex(curMatInstance.metallicRoughnessViewIndex, temp_ref_material.m_intenral_pbr_mat.metallic_roughness_texture_image)
            Tex2ViewIndex(curMatInstance.normalViewIndex, temp_ref_material.m_intenral_pbr_mat.normal_texture_image)
            Tex2ViewIndex(curMatInstance.emissionViewIndex, temp_ref_material.m_intenral_pbr_mat.emissive_texture_image)

            //curMatInstance.baseColorViewIndex = baseColorView->IsValid() ? baseColorView->GetIndex() : defaultWhiteView->GetIndex();
            //curMatInstance.metallicRoughnessViewIndex = metallicRoughnessView->IsValid() ? metallicRoughnessView->GetIndex() : defaultBlackView->GetIndex();
            //curMatInstance.normalViewIndex = normalView->IsValid() ? normalView->GetIndex() : defaultWhiteView->GetIndex();
            //curMatInstance.emissionViewIndex = emissionView->IsValid() ? emissionView->GetIndex() : defaultBlackView->GetIndex();

            pMaterialObj[i] = curMatInstance;

            D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments = {};
            drawIndexedArguments.IndexCountPerInstance        = temp_ref_mesh.index_buffer.index_count; // temp_node.ref_mesh->mesh_index_count;
            drawIndexedArguments.InstanceCount                = 1;
            drawIndexedArguments.StartIndexLocation           = 0;
            drawIndexedArguments.BaseVertexLocation           = 0;
            drawIndexedArguments.StartInstanceLocation        = 0;

            HLSL::BoundingBox boundingBox = {};
            boundingBox.center = GLMUtil::fromVec3(temp_ref_mesh.axis_aligned_box.getCenter()); // temp_node.bounding_box.center;
            boundingBox.extents = GLMUtil::fromVec3(temp_ref_mesh.axis_aligned_box.getHalfExtent());//temp_node.bounding_box.extent;

            HLSL::MeshInstance curMeshInstance     = {};
            curMeshInstance.enable_vertex_blending = temp_ref_mesh.enable_vertex_blending; // temp_node.enable_vertex_blending;
            curMeshInstance.model_matrix           = GLMUtil::fromMat4x4(temp_mesh_renderer.model_matrix); // temp_node.model_matrix;
            curMeshInstance.model_matrix_inverse   = GLMUtil::fromMat4x4(temp_mesh_renderer.model_matrix_inverse);//temp_node.model_matrix_inverse;
            curMeshInstance.vertexBuffer           = temp_ref_mesh.vertex_buffer.vertex_buffer->GetVertexBufferView();//temp_node.ref_mesh->p_mesh_vertex_buffer->GetVertexBufferView();
            curMeshInstance.indexBuffer            = temp_ref_mesh.index_buffer.index_buffer->GetIndexBufferView();//temp_node.ref_mesh->p_mesh_index_buffer->GetIndexBufferView();
            curMeshInstance.drawIndexedArguments   = drawIndexedArguments;
            curMeshInstance.boundingBox            = boundingBox;
            curMeshInstance.materialIndex          = i;

            pMeshesObj[i] = curMeshInstance;
        }

        prepareBuffer();
    }

    void IndirectCullPass::prepareBuffer()
    {
        if (m_render_scene->m_directional_light.m_identifier != UndefCommonIdentifier)
        {
            if (dirShadowmapCommandBuffer.m_identifier != m_render_scene->m_directional_light.m_identifier)
            {
                dirShadowmapCommandBuffer.Reset();
            }

            if (dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer == nullptr)
            {
                RHI::RHIBufferTarget indirectCommandTarget = RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter;

                std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer =
                    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                             indirectCommandTarget,
                                             HLSL::MeshLimit,
                                             sizeof(HLSL::CommandSignatureParams),
                                             L"DirectionIndirectSortCommandBuffer");

                dirShadowmapCommandBuffer.m_identifier = m_render_scene->m_directional_light.m_identifier;
                dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer = p_IndirectCommandBuffer;
            }
        }
        else
        {
            dirShadowmapCommandBuffer.Reset();
        }

        if (!m_render_scene->m_spot_light_list.empty())
        {
            int spotLightCount = m_render_scene->m_spot_light_list.size();
            for (size_t i = 0; i < spotLightCount; i++)
            {
                MoYu::InternalSpotLight& curSpotLightDesc = m_render_scene->m_spot_light_list[i];

                bool curSpotLighBufferExist = false;
                int  curBufferIndex         = -1;
                for (size_t j = 0; j < spotShadowmapCommandBuffer.size(); j++)
                {
                    if (spotShadowmapCommandBuffer[j].m_identifier == curSpotLightDesc.m_identifier)
                    {
                        curBufferIndex         = j;
                        curSpotLighBufferExist = true;
                        break;
                    }
                }

                if (curSpotLighBufferExist)
                {
                    spotShadowmapCommandBuffer[curBufferIndex].Reset();
                    spotShadowmapCommandBuffer.erase(spotShadowmapCommandBuffer.begin() + curBufferIndex);
                }

                if (!curSpotLighBufferExist)
                {
                    ShadowmapCommandBuffer spotShadowCommandBuffer = {};

                    RHI::RHIBufferTarget indirectCommandTarget =
                        RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter;

                    std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer =
                        RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                                 indirectCommandTarget,
                                                 HLSL::MeshLimit,
                                                 sizeof(HLSL::CommandSignatureParams),
                                                 std::wstring(L"SpotIndirectSortCommandBuffer_" + i));

                    spotShadowCommandBuffer.m_lightIndex                = i;
                    spotShadowCommandBuffer.m_identifier                = curSpotLightDesc.m_identifier;
                    spotShadowCommandBuffer.p_IndirectSortCommandBuffer = p_IndirectCommandBuffer;
                    
                    spotShadowmapCommandBuffer.push_back(spotShadowCommandBuffer);
                }

            }
        }
    }

    void IndirectCullPass::bitonicSort(RHI::D3D12ComputeContext* context,
                                       RHI::D3D12Buffer*         keyIndexList,
                                       RHI::D3D12Buffer*         countBuffer,
                                       RHI::D3D12Buffer*         sortDispatchArgBuffer, // pSortDispatchArgs
                                       bool                      isPartiallyPreSorted,
                                       bool                      sortAscending)
    {
        const uint32_t ElementSizeBytes      = keyIndexList->GetStride();
        const uint32_t MaxNumElements        = 1024; //keyIndexList->GetSizeInBytes() / ElementSizeBytes;
        const uint32_t AlignedMaxNumElements = MoYu::AlignPowerOfTwo(MaxNumElements);
        const uint32_t MaxIterations         = MoYu::Log2(std::max(2048u, AlignedMaxNumElements)) - 10;

        ASSERT(ElementSizeBytes == 4 || ElementSizeBytes == 8); // , "Invalid key-index list for bitonic sort"

        context->SetRootSignature(RootSignatures::pBitonicSortRootSignature.get());

        // This controls two things.  It is a key that will sort to the end, and it is a mask used to
        // determine whether the current group should sort ascending or descending.
        //context.SetConstants(3, counterOffset, sortAscending ? 0xffffffff : 0);
        context->SetConstants(3, 0, sortAscending ? 0x7F7FFFFF : 0);
        
        // Generate execute indirect arguments
        context->SetPipelineState(PipelineStates::pBitonicIndirectArgsPSO.get());
        context->TransitionBarrier(countBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        context->TransitionBarrier(sortDispatchArgBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        context->SetConstants(0, MaxIterations);
        context->SetDescriptorTable(1, countBuffer->GetDefaultSRV()->GetGpuHandle());
        context->SetDescriptorTable(2, sortDispatchArgBuffer->GetDefaultUAV()->GetGpuHandle());
        context->Dispatch(1, 1, 1);

        // Pre-Sort the buffer up to k = 2048.  This also pads the list with invalid indices
        // that will drift to the end of the sorted list.
        context->TransitionBarrier(sortDispatchArgBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        context->TransitionBarrier(keyIndexList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        context->InsertUAVBarrier(keyIndexList);
        context->FlushResourceBarriers();

        //context->SetComputeRootUnorderedAccessView(2, keyIndexList->GetGpuVirtualAddress());
        context->SetDescriptorTable(2, keyIndexList->GetDefaultRawUAV()->GetGpuHandle());

        if (!isPartiallyPreSorted)
        {
            context->SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32PreSortPSO.get() :
                                                              PipelineStates::pBitonic64PreSortPSO.get());

            context->DispatchIndirect(sortDispatchArgBuffer, 0);
            context->InsertUAVBarrier(keyIndexList);
        }

        uint32_t IndirectArgsOffset = 12;

        // We have already pre-sorted up through k = 2048 when first writing our list, so
        // we continue sorting with k = 4096.  For unnecessarily large values of k, these
        // indirect dispatches will be skipped over with thread counts of 0.

        for (uint32_t k = 4096; k <= AlignedMaxNumElements; k *= 2)
        {
            context->SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32OuterSortPSO.get() :
                                                              PipelineStates::pBitonic64OuterSortPSO.get());

            for (uint32_t j = k / 2; j >= 2048; j /= 2)
            {
                context->SetConstant(0, j, k);
                context->DispatchIndirect(sortDispatchArgBuffer, IndirectArgsOffset);
                context->InsertUAVBarrier(keyIndexList);
                IndirectArgsOffset += 12;
            }

            context->SetPipelineState(ElementSizeBytes == 4 ? PipelineStates::pBitonic32InnerSortPSO.get() :
                                                             PipelineStates::pBitonic64InnerSortPSO.get());
            context->DispatchIndirect(sortDispatchArgBuffer, IndirectArgsOffset);
            context->InsertUAVBarrier(keyIndexList);
            IndirectArgsOffset += 12;
        }
    }

    void IndirectCullPass::grabObject(RHI::D3D12ComputeContext* context,
                                      RHI::D3D12Buffer*         meshBuffer,
                                      RHI::D3D12Buffer*         indirectIndexBuffer,
                                      RHI::D3D12Buffer*         indirectSortBuffer,
                                      RHI::D3D12Buffer*         grabDispatchArgBuffer) // pSortDispatchArgs

    {
        {
            context->TransitionBarrier(indirectIndexBuffer->GetCounterBuffer().get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            context->TransitionBarrier(grabDispatchArgBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            context->InsertUAVBarrier(grabDispatchArgBuffer);
            context->FlushResourceBarriers();

            context->SetPipelineState(PipelineStates::pIndirectCullArgs.get());
            context->SetRootSignature(RootSignatures::pIndirectCullArgs.get());

            context->SetBufferSRV(0, indirectIndexBuffer->GetCounterBuffer().get());
            context->SetBufferUAV(1, grabDispatchArgBuffer);

            context->Dispatch(1, 1, 1);
        }
        {
            context->TransitionBarrier(meshBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            context->TransitionBarrier(indirectIndexBuffer->GetCounterBuffer().get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            context->TransitionBarrier(indirectIndexBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            context->TransitionBarrier(grabDispatchArgBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            context->TransitionBarrier(indirectSortBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            context->InsertUAVBarrier(indirectSortBuffer);
            context->FlushResourceBarriers();

            context->SetPipelineState(PipelineStates::pIndirectCullGrab.get());
            context->SetRootSignature(RootSignatures::pIndirectCullGrab.get());

            context->SetBufferSRV(0, meshBuffer);
            context->SetBufferSRV(1, indirectIndexBuffer->GetCounterBuffer().get());
            context->SetBufferSRV(2, indirectIndexBuffer);
            context->SetDescriptorTable(3, indirectSortBuffer->GetDefaultUAV()->GetGpuHandle());

            context->DispatchIndirect(grabDispatchArgBuffer, 0);

            // Transition to indirect argument state
            context->TransitionBarrier(indirectSortBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        }
    }

    void IndirectCullPass::update(RHI::RenderGraph& graph, IndirectCullOutput& cullOutput)
    {
        RHI::RgResourceHandle uploadPerframeBufferHandle = GImport(graph, pUploadPerframeBuffer.get());
        RHI::RgResourceHandle uploadMaterialBufferHandle = GImport(graph, pUploadMaterialBuffer.get());
        RHI::RgResourceHandle uploadMeshBufferHandle     = GImport(graph, pUploadMeshBuffer.get());

        RHI::RgResourceHandle sortDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(sortDispatchArgsBufferDesc);
        RHI::RgResourceHandle grabDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(grabDispatchArgsBufferDesc);

        // import buffers
        cullOutput.perframeBufferHandle = GImport(graph, pPerframeBuffer.get());
        cullOutput.materialBufferHandle = GImport(graph, pMaterialBuffer.get());
        cullOutput.meshBufferHandle     = GImport(graph, pMeshBuffer.get());

        cullOutput.opaqueDrawHandle = DrawCallCommandBufferHandle {
            GImport(graph, commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer.get()),
            GImport(graph, commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer.get())};

        cullOutput.transparentDrawHandle = DrawCallCommandBufferHandle {
            GImport(graph, commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer.get()),
            GImport(graph, commandBufferForTransparentDraw.p_IndirectSortCommandBuffer.get())};

        bool hasDirShadowmap = false;
        if (dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer != nullptr)
        {
            hasDirShadowmap = true;
            cullOutput.dirShadowmapHandle = DrawCallCommandBufferHandle {
                GImport(graph, dirShadowmapCommandBuffer.p_IndirectIndexCommandBuffer.get()),
                GImport(graph, dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer.get())};
        }

        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
        {
            DrawCallCommandBufferHandle bufferHandle = {
                GImport(graph, spotShadowmapCommandBuffer[i].p_IndirectIndexCommandBuffer.get()),
                GImport(graph, spotShadowmapCommandBuffer[i].p_IndirectSortCommandBuffer.get())};
            cullOutput.spotShadowmapHandles.push_back(bufferHandle);
        }


        //int numMeshes = m_visiable_nodes.p_all_mesh_nodes->size();
        int numMeshes = 0;

        RHI::D3D12SyncHandle ComputeSyncHandle;
        if (numMeshes > 0)
        {
            auto mPerframeBufferHandle  = cullOutput.perframeBufferHandle;
            auto mMaterialBufferHandle  = cullOutput.materialBufferHandle;
            auto mMeshBufferHandle      = cullOutput.meshBufferHandle;
            auto mOpaqueDrawHandle      = cullOutput.opaqueDrawHandle;
            auto mTransparentDrawHandle = cullOutput.transparentDrawHandle;
            auto mDirShadowmapHandle    = cullOutput.dirShadowmapHandle;
            auto mSpotShadowmapHandles  = cullOutput.spotShadowmapHandles;

            {
                RHI::RenderPass& resetPass = graph.AddRenderPass("ResetPass");

                PassReadIg(resetPass, uploadPerframeBufferHandle);
                PassReadIg(resetPass, uploadMaterialBufferHandle);
                PassReadIg(resetPass, uploadMeshBufferHandle);
                
                PassWriteIg(resetPass, cullOutput.perframeBufferHandle);
                PassWriteIg(resetPass, cullOutput.materialBufferHandle);
                PassWriteIg(resetPass, cullOutput.meshBufferHandle);
                PassWriteIg(resetPass, cullOutput.opaqueDrawHandle.indirectIndexBufferHandle);
                PassWriteIg(resetPass, cullOutput.opaqueDrawHandle.indirectSortBufferHandle);
                PassWriteIg(resetPass, cullOutput.transparentDrawHandle.indirectIndexBufferHandle);
                PassWriteIg(resetPass, cullOutput.transparentDrawHandle.indirectSortBufferHandle);
                if (hasDirShadowmap)
                {
                    PassWriteIg(resetPass, cullOutput.dirShadowmapHandle.indirectIndexBufferHandle);
                    PassWriteIg(resetPass, cullOutput.dirShadowmapHandle.indirectSortBufferHandle);
                }
                for (size_t i = 0; i < cullOutput.spotShadowmapHandles.size(); i++)
                {
                    PassWriteIg(resetPass, cullOutput.spotShadowmapHandles[i].indirectIndexBufferHandle);
                    PassWriteIg(resetPass, cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle);
                }

                resetPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pCopyContext = context->GetComputeContext();

                    pCopyContext->ResetCounter(RegGetBufCounter(mOpaqueDrawHandle.indirectIndexBufferHandle));
                    pCopyContext->ResetCounter(RegGetBufCounter(mOpaqueDrawHandle.indirectSortBufferHandle));
                    pCopyContext->ResetCounter(RegGetBufCounter(mTransparentDrawHandle.indirectIndexBufferHandle));
                    pCopyContext->ResetCounter(RegGetBufCounter(mTransparentDrawHandle.indirectSortBufferHandle));

                    if (hasDirShadowmap)
                    {
                        pCopyContext->ResetCounter(RegGetBufCounter(mDirShadowmapHandle.indirectSortBufferHandle));
                    }

                    for (size_t i = 0; i < mSpotShadowmapHandles.size(); i++)
                    {
                        pCopyContext->ResetCounter(RegGetBufCounter(mSpotShadowmapHandles[i].indirectSortBufferHandle));
                    }

                    pCopyContext->CopyBuffer(RegGetBuf(mPerframeBufferHandle), RegGetBuf(uploadPerframeBufferHandle));
                    pCopyContext->CopyBuffer(RegGetBuf(mMaterialBufferHandle), RegGetBuf(uploadMaterialBufferHandle));
                    pCopyContext->CopyBuffer(RegGetBuf(mMeshBufferHandle), RegGetBuf(uploadMeshBufferHandle));

                    pCopyContext->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_GENERIC_READ);
                    pCopyContext->TransitionBarrier(RegGetBuf(mMaterialBufferHandle), D3D12_RESOURCE_STATE_GENERIC_READ);
                    pCopyContext->TransitionBarrier(RegGetBuf(mMeshBufferHandle), D3D12_RESOURCE_STATE_GENERIC_READ);
                });

            }

            {
                RHI::RenderPass& cullingPass = graph.AddRenderPass("OpaqueTransCullingPass");

                PassReadIg(cullingPass, cullOutput.perframeBufferHandle);
                PassReadIg(cullingPass, cullOutput.meshBufferHandle);
                PassReadIg(cullingPass, cullOutput.materialBufferHandle);

                PassWriteIg(cullingPass, cullOutput.opaqueDrawHandle.indirectIndexBufferHandle);
                PassWriteIg(cullingPass, cullOutput.transparentDrawHandle.indirectIndexBufferHandle);

                cullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();
                
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mMeshBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mMaterialBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mOpaqueDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mTransparentDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

                    pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCullForSort.get());
                    pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCullForSort.get());

                    struct RootIndexBuffer
                    {
                        UINT meshPerFrameBufferIndex;
                        UINT meshInstanceBufferIndex;
                        UINT materialIndexBufferIndex;
                        UINT opaqueSortIndexDisBufferIndex;
                        UINT transSortIndexDisBufferIndex;
                    };

                    RootIndexBuffer rootIndexBuffer = RootIndexBuffer {
                        RegGetBufDefCBVIdx(mPerframeBufferHandle),
                        RegGetBufDefSRVIdx(mMeshBufferHandle),
                        RegGetBufDefSRVIdx(mMaterialBufferHandle),
                        RegGetBufDefUAVIdx(mOpaqueDrawHandle.indirectIndexBufferHandle),
                        RegGetBufDefUAVIdx(mTransparentDrawHandle.indirectIndexBufferHandle)};

                    pAsyncCompute->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);
                    pAsyncCompute->Dispatch1D(numMeshes, 128);
                });
            }

            {
                RHI::RenderPass& opaqueSortPass = graph.AddRenderPass("OpaqueBitonicSortPass");

                PassReadIg(opaqueSortPass, cullOutput.opaqueDrawHandle.indirectIndexBufferHandle);

                PassWriteIg(opaqueSortPass, sortDispatchArgsHandle);
                PassWriteIg(opaqueSortPass, cullOutput.opaqueDrawHandle.indirectIndexBufferHandle);
            
                opaqueSortPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* bufferPtr = RegGetBuf(mOpaqueDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* bufferCouterPtr = bufferPtr->GetCounterBuffer().get();
                    RHI::D3D12Buffer* sortDispatchArgsPtr = RegGetBuf(sortDispatchArgsHandle);

                    bitonicSort(pAsyncCompute, bufferPtr, bufferCouterPtr, sortDispatchArgsPtr, false, true);
                });
            }

            {
                RHI::RenderPass& transSortPass = graph.AddRenderPass("TransparentBitonicSortPass");

                PassReadIg(transSortPass, cullOutput.transparentDrawHandle.indirectIndexBufferHandle);

                PassWriteIg(transSortPass, sortDispatchArgsHandle);
                PassWriteIg(transSortPass, cullOutput.transparentDrawHandle.indirectIndexBufferHandle);

                transSortPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* bufferPtr = RegGetBuf(mTransparentDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* bufferCouterPtr = bufferPtr->GetCounterBuffer().get();
                    RHI::D3D12Buffer* sortDispatchArgsPtr = RegGetBuf(sortDispatchArgsHandle);

                    bitonicSort(pAsyncCompute, bufferPtr, bufferCouterPtr, sortDispatchArgsPtr, false, false);
                });
            }

            {
                RHI::RenderPass& grabOpaquePass = graph.AddRenderPass("GrabOpaquePass");

                PassReadIg(grabOpaquePass, cullOutput.meshBufferHandle);
                PassReadIg(grabOpaquePass, cullOutput.opaqueDrawHandle.indirectIndexBufferHandle);

                PassWriteIg(grabOpaquePass, grabDispatchArgsHandle);
                PassWriteIg(grabOpaquePass, cullOutput.opaqueDrawHandle.indirectSortBufferHandle);

                grabOpaquePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();
                
                    RHI::D3D12Buffer* meshBufferPtr = RegGetBuf(mMeshBufferHandle);
                    RHI::D3D12Buffer* indirectIndexBufferPtr = RegGetBuf(mOpaqueDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* indirectSortBufferPtr = RegGetBuf(mOpaqueDrawHandle.indirectSortBufferHandle);
                    RHI::D3D12Buffer* grabDispatchArgsPtr = RegGetBuf(grabDispatchArgsHandle);

                    grabObject(pAsyncCompute, meshBufferPtr, indirectIndexBufferPtr, indirectSortBufferPtr, grabDispatchArgsPtr);
                });
            }

            {
                RHI::RenderPass& grabTransPass = graph.AddRenderPass("GrabTransPass");

                PassReadIg(grabTransPass, cullOutput.meshBufferHandle);
                PassReadIg(grabTransPass, cullOutput.transparentDrawHandle.indirectIndexBufferHandle);

                PassWriteIg(grabTransPass, grabDispatchArgsHandle);
                PassWriteIg(grabTransPass, cullOutput.transparentDrawHandle.indirectSortBufferHandle);

                grabTransPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* meshBufferPtr = RegGetBuf(mMeshBufferHandle);
                    RHI::D3D12Buffer* indirectIndexBufferPtr = RegGetBuf(mTransparentDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* indirectSortBufferPtr = RegGetBuf(mTransparentDrawHandle.indirectSortBufferHandle);
                    RHI::D3D12Buffer* grabDispatchArgsPtr = RegGetBuf(grabDispatchArgsHandle);

                    grabObject(pAsyncCompute, meshBufferPtr, indirectIndexBufferPtr, indirectSortBufferPtr, grabDispatchArgsPtr);
                });
            }

            {
                RHI::RenderPass& dirLightShadowCullPass = graph.AddRenderPass("DirectionLightShadowCullPass");

                PassReadIg(dirLightShadowCullPass, cullOutput.perframeBufferHandle);
                PassReadIg(dirLightShadowCullPass, cullOutput.meshBufferHandle);
                PassReadIg(dirLightShadowCullPass, cullOutput.materialBufferHandle);

                PassWriteIg(dirLightShadowCullPass, cullOutput.dirShadowmapHandle.indirectSortBufferHandle);

                dirLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    pAsyncCompute->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mMeshBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mMaterialBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mDirShadowmapHandle.indirectSortBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->InsertUAVBarrier(RegGetBuf(mDirShadowmapHandle.indirectSortBufferHandle));
                    pAsyncCompute->FlushResourceBarriers();

                    pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCullDirectionShadowmap.get());
                    pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCullDirectionShadowmap.get());

                    pAsyncCompute->SetConstantBuffer(0, RegGetBuf(mPerframeBufferHandle)->GetGpuVirtualAddress());
                    pAsyncCompute->SetBufferSRV(1, RegGetBuf(mMeshBufferHandle));
                    pAsyncCompute->SetBufferSRV(2, RegGetBuf(mMaterialBufferHandle));
                    pAsyncCompute->SetDescriptorTable(3, RegGetBuf(mDirShadowmapHandle.indirectSortBufferHandle)->GetDefaultUAV()->GetGpuHandle());
                
                    pAsyncCompute->Dispatch1D(numMeshes, 128);

                    pAsyncCompute->TransitionBarrier(RegGetBuf(mDirShadowmapHandle.indirectSortBufferHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                });
            }

            {
                RHI::RenderPass& spotLightShadowCullPass = graph.AddRenderPass("SpotLightShadowCullPass");

                PassReadIg(spotLightShadowCullPass, cullOutput.perframeBufferHandle);
                PassReadIg(spotLightShadowCullPass, cullOutput.meshBufferHandle);
                PassReadIg(spotLightShadowCullPass, cullOutput.materialBufferHandle);

                std::vector<uint32_t> _spotlightIndex;
                for (size_t i = 0; i < cullOutput.spotShadowmapHandles.size(); i++)
                {
                    PassWriteIg(spotLightShadowCullPass, cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle);
                    _spotlightIndex.push_back(spotShadowmapCommandBuffer[i].m_lightIndex);
                }

                spotLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    pAsyncCompute->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mMeshBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mMaterialBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    for (size_t i = 0; i < mSpotShadowmapHandles.size(); i++)
                    {
                        pAsyncCompute->TransitionBarrier(RegGetBuf(mSpotShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                        pAsyncCompute->InsertUAVBarrier(RegGetBuf(mSpotShadowmapHandles[i].indirectSortBufferHandle));
                    }
                    pAsyncCompute->FlushResourceBarriers();

                    for (size_t i = 0; i < mSpotShadowmapHandles.size(); i++)
                    {
                        pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCullSpotShadowmap.get());
                        pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCullSpotShadowmap.get());

                        pAsyncCompute->SetConstantBuffer(0, RegGetBuf(mPerframeBufferHandle)->GetGpuVirtualAddress());
                        pAsyncCompute->SetConstant(1, 0, _spotlightIndex[i]);
                        pAsyncCompute->SetBufferSRV(2, RegGetBuf(mMeshBufferHandle));
                        pAsyncCompute->SetBufferSRV(3, RegGetBuf(mMaterialBufferHandle));
                        pAsyncCompute->SetDescriptorTable(4, RegGetBuf(mSpotShadowmapHandles[i].indirectSortBufferHandle)->GetDefaultUAV()->GetGpuHandle());

                        pAsyncCompute->Dispatch1D(numMeshes, 128);

                        pAsyncCompute->TransitionBarrier(RegGetBuf(mSpotShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                    }
                });
            }
        }
    }

    void IndirectCullPass::destroy()
    {
        pUploadPerframeBuffer = nullptr;
        pUploadMaterialBuffer = nullptr;
        pUploadMeshBuffer     = nullptr;

        pPerframeBuffer = nullptr;
        pMaterialBuffer = nullptr;
        pMeshBuffer     = nullptr;

        commandBufferForOpaqueDraw.ResetBuffer();
        commandBufferForTransparentDraw.ResetBuffer();
        dirShadowmapCommandBuffer.Reset();
        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
            spotShadowmapCommandBuffer[i].Reset();
        spotShadowmapCommandBuffer.clear();

    }

}
