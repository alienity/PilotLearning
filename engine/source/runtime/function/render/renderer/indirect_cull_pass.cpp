#include "runtime/function/render/renderer/indirect_cull_pass.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/rhi/rhi_core.h"
#include "runtime/core/math/moyu_math.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsCommon.h"

#include <cassert>

namespace Pilot
{
#define GImport(b) graph.Import(b)
#define PassRead(p, b) p.Read(b)
#define PassWrite(p, b) p.Write(b)
#define PassReadIg(p, b) p.Read(b, true)
#define PassWriteIg(p, b) p.Write(b, true)
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
        commandBufferForTransparentDraw.p_IndirectSortCommandBuffer  = CreateSortCommandBuffer(L"TransparentIndexBuffer");
    }

    void IndirectCullPass::prepareMeshData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        RenderResource* real_resource = (RenderResource*)render_resource.get();
        memcpy(pUploadPerframeBuffer->GetCpuVirtualAddress<HLSL::MeshPerframeStorageBufferObject>(),
               &real_resource->m_mesh_perframe_storage_buffer_object,
               sizeof(HLSL::MeshPerframeStorageBufferObject));

        std::vector<RenderMeshNode>* renderMeshNodes = m_visiable_nodes.p_all_mesh_nodes;

        HLSL::MaterialInstance* pMaterialObj = pUploadMaterialBuffer->GetCpuVirtualAddress<HLSL::MaterialInstance>();
        HLSL::MeshInstance* pMeshesObj = pUploadMeshBuffer->GetCpuVirtualAddress<HLSL::MeshInstance>();

        uint32_t numMeshes = renderMeshNodes->size();
        ASSERT(numMeshes < HLSL::MeshLimit);
        for (size_t i = 0; i < numMeshes; i++)
        {
            RenderMeshNode& temp_node = renderMeshNodes->at(i);

            //std::shared_ptr<RHI::D3D12ShaderResourceView> defaultWhiteView = real_resource->m_default_resource._white_texture2d_image->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> defaultBlackView = real_resource->m_default_resource._black_texture2d_image->GetDefaultSRV();

            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultWhiteView = RHI::GetDefaultTexture(RHI::eDefaultTexture::kWhiteOpaque2D)->GetDefaultSRV();
            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultBlackView = RHI::GetDefaultTexture(RHI::eDefaultTexture::kBlackOpaque2D)->GetDefaultSRV();

            std::shared_ptr<RHI::D3D12ShaderResourceView> uniformBufferView = temp_node.ref_material->material_uniform_buffer->GetDefaultSRV();
            std::shared_ptr<RHI::D3D12ShaderResourceView> baseColorView = temp_node.ref_material->base_color_texture_image->GetDefaultSRV();
            std::shared_ptr<RHI::D3D12ShaderResourceView> metallicRoughnessView = temp_node.ref_material->metallic_roughness_texture_image->GetDefaultSRV();
            std::shared_ptr<RHI::D3D12ShaderResourceView> normalView   = temp_node.ref_material->normal_texture_image->GetDefaultSRV();
            std::shared_ptr<RHI::D3D12ShaderResourceView> emissionView = temp_node.ref_material->emissive_texture_image->GetDefaultSRV();

            HLSL::MaterialInstance curMatInstance = {};
            curMatInstance.uniformBufferViewIndex = uniformBufferView->GetIndex();
            curMatInstance.baseColorViewIndex = baseColorView->IsValid() ? baseColorView->GetIndex() : defaultWhiteView->GetIndex();
            curMatInstance.metallicRoughnessViewIndex = metallicRoughnessView->IsValid() ? metallicRoughnessView->GetIndex() : defaultBlackView->GetIndex();
            curMatInstance.normalViewIndex = normalView->IsValid() ? normalView->GetIndex() : defaultWhiteView->GetIndex();
            curMatInstance.emissionViewIndex = emissionView->IsValid() ? emissionView->GetIndex() : defaultBlackView->GetIndex();

            pMaterialObj[i] = curMatInstance;

            D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments = {};
            drawIndexedArguments.IndexCountPerInstance        = temp_node.ref_mesh->mesh_index_count;
            drawIndexedArguments.InstanceCount                = 1;
            drawIndexedArguments.StartIndexLocation           = 0;
            drawIndexedArguments.BaseVertexLocation           = 0;
            drawIndexedArguments.StartInstanceLocation        = 0;

            HLSL::BoundingBox boundingBox = {};
            boundingBox.center            = temp_node.bounding_box.center;
            boundingBox.extents           = temp_node.bounding_box.extent;

            HLSL::MeshInstance curMeshInstance     = {};
            curMeshInstance.enable_vertex_blending = temp_node.enable_vertex_blending;
            curMeshInstance.model_matrix           = temp_node.model_matrix;
            curMeshInstance.model_matrix_inverse   = temp_node.model_matrix_inverse;
            curMeshInstance.vertexBuffer           = temp_node.ref_mesh->p_mesh_vertex_buffer->GetVertexBufferView();
            curMeshInstance.indexBuffer            = temp_node.ref_mesh->p_mesh_index_buffer->GetIndexBufferView();
            curMeshInstance.drawIndexedArguments   = drawIndexedArguments;
            curMeshInstance.boundingBox            = boundingBox;
            curMeshInstance.materialIndex          = i;

            pMeshesObj[i] = curMeshInstance;
        }

        prepareBuffer();
    }

    void IndirectCullPass::prepareBuffer()
    {
        if (m_visiable_nodes.p_directional_light != nullptr && m_visiable_nodes.p_directional_light->m_is_active)
        {
            if (dirShadowmapCommandBuffer.m_gobject_id != m_visiable_nodes.p_directional_light->m_gobject_id ||
                dirShadowmapCommandBuffer.m_gcomponent_id != m_visiable_nodes.p_directional_light->m_gcomponent_id)
            {
                dirShadowmapCommandBuffer.Reset();
            }

            if (dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer == nullptr)
            {
                RHI::RHIBufferTarget indirectCommandTarget =
                    RHI::RHIBufferRandomReadWrite | RHI::RHIBufferTargetStructured | RHI::RHIBufferTargetCounter;

                std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer =
                    RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                             indirectCommandTarget,
                                             HLSL::MeshLimit,
                                             sizeof(HLSL::CommandSignatureParams),
                                             L"DirectionIndirectSortCommandBuffer");

                dirShadowmapCommandBuffer.m_gobject_id    = m_visiable_nodes.p_directional_light->m_gobject_id;
                dirShadowmapCommandBuffer.m_gcomponent_id = m_visiable_nodes.p_directional_light->m_gcomponent_id;
                dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer = p_IndirectCommandBuffer;
            }
        }
        else
        {
            dirShadowmapCommandBuffer.Reset();
        }

        if (m_visiable_nodes.p_spot_light_list != nullptr)
        {
            int spotLightCount = m_visiable_nodes.p_spot_light_list->size();
            for (size_t i = 0; i < spotLightCount; i++)
            {
                Pilot::SpotLightDesc curSpotLightDesc = m_visiable_nodes.p_spot_light_list->at(i);

                bool curSpotLighBufferExist = false;
                int  curBufferIndex         = -1;
                for (size_t j = 0; j < spotShadowmapCommandBuffer.size(); j++)
                {
                    if (spotShadowmapCommandBuffer[j].m_gobject_id == curSpotLightDesc.m_gobject_id &&
                        spotShadowmapCommandBuffer[j].m_gcomponent_id == curSpotLightDesc.m_gcomponent_id)
                    {
                        curBufferIndex         = j;
                        curSpotLighBufferExist = true;
                        break;
                    }
                }

                if (!curSpotLightDesc.m_is_active && curSpotLighBufferExist)
                {
                    spotShadowmapCommandBuffer[curBufferIndex].Reset();
                    spotShadowmapCommandBuffer.erase(spotShadowmapCommandBuffer.begin() + curBufferIndex);
                }

                if (curSpotLightDesc.m_is_active && !curSpotLighBufferExist)
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
                    spotShadowCommandBuffer.m_gobject_id                = curSpotLightDesc.m_gobject_id;
                    spotShadowCommandBuffer.m_gcomponent_id             = curSpotLightDesc.m_gcomponent_id;
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
        const uint32_t AlignedMaxNumElements = Pilot::AlignPowerOfTwo(MaxNumElements);
        const uint32_t MaxIterations         = Pilot::Log2(std::max(2048u, AlignedMaxNumElements)) - 10;

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
            context->TransitionBarrier(indirectIndexBuffer->GetCounterBuffer().get(),
                                             D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
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
            context->TransitionBarrier(indirectIndexBuffer->GetCounterBuffer().get(),
                                             D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            context->TransitionBarrier(indirectIndexBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            context->TransitionBarrier(grabDispatchArgBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
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
        RHI::RgResourceHandle uploadPerframeBufferHandle = GImport(pUploadPerframeBuffer.get());
        RHI::RgResourceHandle uploadMaterialBufferHandle = GImport(pUploadMaterialBuffer.get());
        RHI::RgResourceHandle uploadMeshBufferHandle     = GImport(pUploadMeshBuffer.get());

        RHI::RgResourceHandle sortDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(sortDispatchArgsBufferDesc);
        RHI::RgResourceHandle grabDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(grabDispatchArgsBufferDesc);

        // import buffers
        cullOutput.perframeBufferHandle = GImport(pPerframeBuffer.get());
        cullOutput.materialBufferHandle = GImport(pMaterialBuffer.get());
        cullOutput.meshBufferHandle     = GImport(pMeshBuffer.get());

        cullOutput.opaqueDrawHandle = DrawCallCommandBufferHandle {
            GImport(commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer.get()),
            GImport(commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer.get())};

        cullOutput.transparentDrawHandle = DrawCallCommandBufferHandle {
            GImport(commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer.get()),
            GImport(commandBufferForTransparentDraw.p_IndirectSortCommandBuffer.get())};

        bool hasDirShadowmap = false;
        if (dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer != nullptr)
        {
            hasDirShadowmap = true;
            cullOutput.dirShadowmapHandle = DrawCallCommandBufferHandle {
                GImport(dirShadowmapCommandBuffer.p_IndirectIndexCommandBuffer.get()),
                GImport(dirShadowmapCommandBuffer.p_IndirectSortCommandBuffer.get())};
        }

        bool hasSpotShadowmap = false;
        if (spotShadowmapCommandBuffer.size() != 0)
            hasSpotShadowmap = true;
        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
        {
            DrawCallCommandBufferHandle bufferHandle = {
                GImport(spotShadowmapCommandBuffer[i].p_IndirectIndexCommandBuffer.get()),
                GImport(spotShadowmapCommandBuffer[i].p_IndirectSortCommandBuffer.get())};
            cullOutput.spotShadowmapHandles.push_back(bufferHandle);
        }


        int numMeshes = m_visiable_nodes.p_all_mesh_nodes->size();

        RHI::D3D12SyncHandle ComputeSyncHandle;
        if (numMeshes > 0)
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
            for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
            {
                PassWriteIg(resetPass, cullOutput.spotShadowmapHandles[i].indirectIndexBufferHandle);
                PassWriteIg(resetPass, cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle);
            }

            resetPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pCopyContext = context->GetComputeContext();

                pCopyContext->ResetCounter(RegGetBuf(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle));
                pCopyContext->ResetCounter(RegGetBuf(cullOutput.opaqueDrawHandle.indirectSortBufferHandle));
                pCopyContext->ResetCounter(RegGetBuf(cullOutput.transparentDrawHandle.indirectIndexBufferHandle));
                pCopyContext->ResetCounter(RegGetBuf(cullOutput.transparentDrawHandle.indirectSortBufferHandle));

                if (hasDirShadowmap)
                {
                    pCopyContext->ResetCounter(RegGetBufCounter(cullOutput.dirShadowmapHandle.indirectSortBufferHandle));
                }

                for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
                {
                    pCopyContext->ResetCounter(RegGetBufCounter(cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle));
                }

                pCopyContext->CopyBuffer(RegGetBuf(cullOutput.perframeBufferHandle), RegGetBuf(uploadPerframeBufferHandle));
                pCopyContext->CopyBuffer(RegGetBuf(cullOutput.materialBufferHandle), RegGetBuf(uploadMaterialBufferHandle));
                pCopyContext->CopyBuffer(RegGetBuf(cullOutput.meshBufferHandle), RegGetBuf(uploadMeshBufferHandle));

                pCopyContext->TransitionBarrier(RegGetBuf(cullOutput.perframeBufferHandle), D3D12_RESOURCE_STATE_GENERIC_READ);
                pCopyContext->TransitionBarrier(RegGetBuf(cullOutput.materialBufferHandle), D3D12_RESOURCE_STATE_GENERIC_READ);
                pCopyContext->TransitionBarrier(RegGetBuf(cullOutput.meshBufferHandle), D3D12_RESOURCE_STATE_GENERIC_READ);
            });


            RHI::RenderPass& cullingPass = graph.AddRenderPass("OpaqueTransCullingPass");

            PassReadIg(cullingPass, cullOutput.perframeBufferHandle);
            PassReadIg(cullingPass, cullOutput.meshBufferHandle);
            PassReadIg(cullingPass, cullOutput.materialBufferHandle);
            PassWriteIg(cullingPass, cullOutput.opaqueDrawHandle.indirectIndexBufferHandle);
            PassWriteIg(cullingPass, cullOutput.transparentDrawHandle.indirectIndexBufferHandle);

            cullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();
                
                pAsyncCompute->TransitionBarrier(RegGetBuf(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                pAsyncCompute->TransitionBarrier(RegGetBuf(cullOutput.transparentDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                
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

                RootIndexBuffer rootIndexBuffer =
                    RootIndexBuffer {RegGetBufDefCBVIdx(cullOutput.perframeBufferHandle),
                                     RegGetBufDefSRVIdx(cullOutput.meshBufferHandle),
                                     RegGetBufDefSRVIdx(cullOutput.materialBufferHandle),
                                     RegGetBufDefUAVIdx(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle),
                                     RegGetBufDefUAVIdx(cullOutput.transparentDrawHandle.indirectIndexBufferHandle)};

                pAsyncCompute->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);
                pAsyncCompute->Dispatch1D(numMeshes, 128);
            });


            RHI::RenderPass& opaqueSortPass = graph.AddRenderPass("OpaqueBitonicSortPass");

            PassWriteIg(opaqueSortPass, sortDispatchArgsHandle);
            PassWriteIg(opaqueSortPass, cullOutput.opaqueDrawHandle.indirectIndexBufferHandle);
            
            opaqueSortPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                RHI::D3D12Buffer* bufferPtr = RegGetBuf(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle);
                RHI::D3D12Buffer* bufferCouterPtr = bufferPtr->GetCounterBuffer().get();
                RHI::D3D12Buffer* sortDispatchArgsPtr = RegGetBuf(sortDispatchArgsHandle);

                bitonicSort(pAsyncCompute, bufferPtr, bufferCouterPtr, sortDispatchArgsPtr, false, true);
            });


            RHI::RenderPass& transSortPass = graph.AddRenderPass("TransparentBitonicSortPass");

            PassWriteIg(transSortPass, sortDispatchArgsHandle);
            PassWriteIg(transSortPass, cullOutput.transparentDrawHandle.indirectIndexBufferHandle);

            transSortPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                RHI::D3D12Buffer* bufferPtr       = RegGetBuf(cullOutput.transparentDrawHandle.indirectIndexBufferHandle);
                RHI::D3D12Buffer* bufferCouterPtr = bufferPtr->GetCounterBuffer().get();
                RHI::D3D12Buffer* sortDispatchArgsPtr = RegGetBuf(sortDispatchArgsHandle);

                bitonicSort(pAsyncCompute, bufferPtr, bufferCouterPtr, sortDispatchArgsPtr, false, false);
            });


            RHI::RenderPass& grabOpaquePass = graph.AddRenderPass("GrabOpaquePass");

            PassReadIg(grabOpaquePass, cullOutput.meshBufferHandle);
            PassReadIg(grabOpaquePass, cullOutput.opaqueDrawHandle.indirectIndexBufferHandle);
            PassWriteIg(grabOpaquePass, grabDispatchArgsHandle);
            PassWriteIg(grabOpaquePass, cullOutput.opaqueDrawHandle.indirectSortBufferHandle);

            grabOpaquePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();
                
                RHI::D3D12Buffer* meshBufferPtr = RegGetBuf(cullOutput.meshBufferHandle);
                RHI::D3D12Buffer* indirectIndexBufferPtr = RegGetBuf(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle);
                RHI::D3D12Buffer* indirectSortBufferPtr = RegGetBuf(cullOutput.opaqueDrawHandle.indirectSortBufferHandle);
                RHI::D3D12Buffer* grabDispatchArgsPtr = RegGetBuf(grabDispatchArgsHandle);

                grabObject(pAsyncCompute, meshBufferPtr, indirectIndexBufferPtr, indirectSortBufferPtr, grabDispatchArgsPtr);
            });


            RHI::RenderPass& grabTransPass = graph.AddRenderPass("GrabTransPass");

            PassReadIg(grabOpaquePass, cullOutput.meshBufferHandle);
            PassReadIg(grabOpaquePass, cullOutput.transparentDrawHandle.indirectIndexBufferHandle);
            PassWriteIg(grabTransPass, grabDispatchArgsHandle);
            PassWriteIg(grabTransPass, cullOutput.transparentDrawHandle.indirectSortBufferHandle);

            grabTransPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                RHI::D3D12Buffer* meshBufferPtr = RegGetBuf(cullOutput.meshBufferHandle);
                RHI::D3D12Buffer* indirectIndexBufferPtr = RegGetBuf(cullOutput.transparentDrawHandle.indirectIndexBufferHandle);
                RHI::D3D12Buffer* indirectSortBufferPtr = RegGetBuf(cullOutput.transparentDrawHandle.indirectSortBufferHandle);
                RHI::D3D12Buffer* grabDispatchArgsPtr = RegGetBuf(grabDispatchArgsHandle);

                grabObject(pAsyncCompute, meshBufferPtr, indirectIndexBufferPtr, indirectSortBufferPtr, grabDispatchArgsPtr);
            });


            RHI::RenderPass& dirLightShadowCullPass = graph.AddRenderPass("DirectionLightShadowCullPass");

            PassReadIg(dirLightShadowCullPass, cullOutput.perframeBufferHandle);
            PassReadIg(dirLightShadowCullPass, cullOutput.meshBufferHandle);
            PassReadIg(dirLightShadowCullPass, cullOutput.materialBufferHandle);
            PassWriteIg(dirLightShadowCullPass, cullOutput.dirShadowmapHandle.indirectSortBufferHandle);

            dirLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCullDirectionShadowmap.get());
                pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCullDirectionShadowmap.get());

                RHI::D3D12Buffer* indirectSortBufferPtr = RegGetBuf(cullOutput.dirShadowmapHandle.indirectSortBufferHandle);

                pAsyncCompute->SetConstantBuffer(0, RegGetBuf(cullOutput.perframeBufferHandle)->GetGpuVirtualAddress());
                pAsyncCompute->SetBufferSRV(1, RegGetBuf(cullOutput.meshBufferHandle));
                pAsyncCompute->SetBufferSRV(2, RegGetBuf(cullOutput.materialBufferHandle));
                pAsyncCompute->SetDescriptorTable(3, indirectSortBufferPtr->GetDefaultUAV()->GetGpuHandle());
                
                pAsyncCompute->Dispatch1D(numMeshes, 128);

                pAsyncCompute->TransitionBarrier(RegGetBuf(cullOutput.dirShadowmapHandle.indirectSortBufferHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            });


            RHI::RenderPass& spotLightShadowCullPass = graph.AddRenderPass("SpotLightShadowCullPass");

            PassReadIg(spotLightShadowCullPass, cullOutput.perframeBufferHandle);
            PassReadIg(spotLightShadowCullPass, cullOutput.meshBufferHandle);
            PassReadIg(spotLightShadowCullPass, cullOutput.materialBufferHandle);
            for (size_t i = 0; i < cullOutput.spotShadowmapHandles.size(); i++)
            {
                PassWriteIg(spotLightShadowCullPass, cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle);
            }

            spotLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                for (size_t i = 0; i < cullOutput.spotShadowmapHandles.size(); i++)
                {
                    pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCullSpotShadowmap.get());
                    pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCullSpotShadowmap.get());

                    pAsyncCompute->SetConstantBuffer(0, RegGetBuf(cullOutput.perframeBufferHandle)->GetGpuVirtualAddress());
                    pAsyncCompute->SetConstant(1, 0, spotShadowmapCommandBuffer[i].m_lightIndex);
                    pAsyncCompute->SetBufferSRV(2, RegGetBuf(cullOutput.meshBufferHandle));
                    pAsyncCompute->SetBufferSRV(3, RegGetBuf(cullOutput.materialBufferHandle));
                    pAsyncCompute->SetDescriptorTable(4, RegGetBuf(cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle)->GetDefaultUAV()->GetGpuHandle());

                    pAsyncCompute->Dispatch1D(numMeshes, 128);

                    pAsyncCompute->TransitionBarrier(RegGetBuf(cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                }
            });
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
