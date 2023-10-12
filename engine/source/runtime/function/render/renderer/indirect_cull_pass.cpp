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
        pFrameUniformBuffer = CreateCullingBuffer(1, sizeof(HLSL::FrameUniforms), L"FrameUniformBuffer");
        pMaterialViewIndexBuffer = CreateCullingBuffer(HLSL::MaterialLimit, sizeof(HLSL::PerMaterialViewIndexBuffer), L"MaterialViewIndexBuffer");
        pRenderableMeshBuffer = CreateCullingBuffer(HLSL::MeshLimit, sizeof(HLSL::PerRenderableMeshData), L"PerRenderableMeshBuffer");

        // create upload buffer
        pUploadFrameUniformBuffer = CreateUploadBuffer(1, sizeof(HLSL::FrameUniforms), L"UploadFrameUniformBuffer");
        pUploadMaterialViewIndexBuffer = CreateUploadBuffer(HLSL::MaterialLimit, sizeof(HLSL::PerMaterialViewIndexBuffer), L"UploadMaterialViewIndexBuffer");
        pUploadRenderableMeshBuffer = CreateUploadBuffer(HLSL::MeshLimit, sizeof(HLSL::PerRenderableMeshData), L"UploadPerRenderableMeshBuffer");

        sortDispatchArgsBufferDesc = CreateArgBufferDesc("SortDispatchArgs", 22 * 23 / 2);
        grabDispatchArgsBufferDesc = CreateArgBufferDesc("GrabDispatchArgs", 22 * 23 / 2);


        // buffer for opaque draw
        commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer = CreateIndexBuffer(L"OpaqueIndexBuffer");
        commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer  = CreateSortCommandBuffer(L"OpaqueBuffer");

        // buffer for transparent draw
        commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer = CreateIndexBuffer(L"TransparentIndexBuffer");
        commandBufferForTransparentDraw.p_IndirectSortCommandBuffer  = CreateSortCommandBuffer(L"TransparentBuffer");
    }

    void IndirectCullPass::inflatePerframeBuffer(std::shared_ptr<RenderResource> render_resource)
    {
        //// update per-frame buffer
        //render_resource->updatePerFrameBuffer(m_render_scene, m_render_camera);

        memcpy(pUploadFrameUniformBuffer->GetCpuVirtualAddress<HLSL::FrameUniforms>(),
               &render_resource->m_FrameUniforms,
               sizeof(HLSL::FrameUniforms));
    }

    void IndirectCullPass::prepareMeshData(std::shared_ptr<RenderResource> render_resource)
    {
        std::vector<CachedMeshRenderer>& _mesh_renderers = m_render_scene->m_mesh_renderers;

        HLSL::PerMaterialViewIndexBuffer* pMaterialViewIndexBuffer = pUploadMaterialViewIndexBuffer->GetCpuVirtualAddress<HLSL::PerMaterialViewIndexBuffer>();
        HLSL::PerRenderableMeshData* pRenderableMeshBuffer = pUploadRenderableMeshBuffer->GetCpuVirtualAddress<HLSL::PerRenderableMeshData>();

        uint32_t numMeshes = _mesh_renderers.size();
        ASSERT(numMeshes < HLSL::MeshLimit);
        for (size_t i = 0; i < numMeshes; i++)
        {
            InternalMeshRenderer& temp_mesh_renderer = _mesh_renderers[i].internalMeshRenderer;

            InternalMesh& temp_ref_mesh = temp_mesh_renderer.ref_mesh;
            InternalMaterial& temp_ref_material = temp_mesh_renderer.ref_material;

            //std::shared_ptr<RHI::D3D12ShaderResourceView> defaultWhiteView = real_resource->m_default_resource._white_texture2d_image->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> defaultBlackView = real_resource->m_default_resource._black_texture2d_image->GetDefaultSRV();

            //std::shared_ptr<RHI::D3D12ShaderResourceView> defaultWhiteView = RHI::GetDefaultTexture(RHI::eDefaultTexture::kWhiteOpaque2D)->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> defaultBlackView = RHI::GetDefaultTexture(RHI::eDefaultTexture::kBlackOpaque2D)->GetDefaultSRV();

            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultWhiteView =
                render_resource->_Default2TexMap[DefaultTexType::White]->GetDefaultSRV();
            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultBlackView =
                render_resource->_Default2TexMap[DefaultTexType::Black]->GetDefaultSRV();
            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultNormalView =
                render_resource->_Default2TexMap[DefaultTexType::TangentNormal]->GetDefaultSRV();


            std::shared_ptr<RHI::D3D12ShaderResourceView> uniformBufferView = temp_ref_material.m_intenral_pbr_mat.material_uniform_buffer->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> baseColorView = temp_ref_material.m_intenral_pbr_mat.base_color_texture_image->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> metallicRoughnessView = temp_ref_material.m_intenral_pbr_mat.metallic_roughness_texture_image->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> normalView   = temp_ref_material.m_intenral_pbr_mat.normal_texture_image->GetDefaultSRV();
            //std::shared_ptr<RHI::D3D12ShaderResourceView> emissionView = temp_ref_material.m_intenral_pbr_mat.emissive_texture_image->GetDefaultSRV();

            #define Tex2ViewIndex(toIndex, replaceTexView, fromTex)\
            if (fromTex == nullptr)\
                toIndex = replaceTexView->GetIndex();\
            else\
                toIndex = fromTex->GetDefaultSRV()->GetIndex();\

            HLSL::PerMaterialViewIndexBuffer curMatViewIndexBuffer = {};
            curMatViewIndexBuffer.parametersBufferIndex = uniformBufferView->GetIndex();            
            Tex2ViewIndex(curMatViewIndexBuffer.baseColorIndex, defaultWhiteView, temp_ref_material.m_intenral_pbr_mat.base_color_texture_image)
            Tex2ViewIndex(curMatViewIndexBuffer.metallicRoughnessIndex, defaultWhiteView, temp_ref_material.m_intenral_pbr_mat.metallic_roughness_texture_image)
            Tex2ViewIndex(curMatViewIndexBuffer.normalIndex, defaultNormalView, temp_ref_material.m_intenral_pbr_mat.normal_texture_image)
            Tex2ViewIndex(curMatViewIndexBuffer.occlusionIndex, defaultWhiteView, temp_ref_material.m_intenral_pbr_mat.occlusion_texture_image)
            Tex2ViewIndex(curMatViewIndexBuffer.emissionIndex, defaultBlackView, temp_ref_material.m_intenral_pbr_mat.emissive_texture_image)

            //curMatInstance.baseColorViewIndex = baseColorView->IsValid() ? baseColorView->GetIndex() : defaultWhiteView->GetIndex();
            //curMatInstance.metallicRoughnessViewIndex = metallicRoughnessView->IsValid() ? metallicRoughnessView->GetIndex() : defaultBlackView->GetIndex();
            //curMatInstance.normalViewIndex = normalView->IsValid() ? normalView->GetIndex() : defaultWhiteView->GetIndex();
            //curMatInstance.emissionViewIndex = emissionView->IsValid() ? emissionView->GetIndex() : defaultBlackView->GetIndex();

            pMaterialViewIndexBuffer[i] = curMatViewIndexBuffer;

            D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments = {};
            drawIndexedArguments.IndexCountPerInstance        = temp_ref_mesh.index_buffer.index_count; // temp_node.ref_mesh->mesh_index_count;
            drawIndexedArguments.InstanceCount                = 1;
            drawIndexedArguments.StartIndexLocation           = 0;
            drawIndexedArguments.BaseVertexLocation           = 0;
            drawIndexedArguments.StartInstanceLocation        = 0;

            HLSL::BoundingBox boundingBox = {};
            boundingBox.center = GLMUtil::fromVec3(temp_ref_mesh.axis_aligned_box.getCenter()); // temp_node.bounding_box.center;
            boundingBox.extents = GLMUtil::fromVec3(temp_ref_mesh.axis_aligned_box.getHalfExtent());//temp_node.bounding_box.extent;

            HLSL::PerRenderableMeshData curRenderableMeshData = {};
            curRenderableMeshData.enableVertexBlending = temp_ref_mesh.enable_vertex_blending; // temp_node.enable_vertex_blending;
            curRenderableMeshData.worldFromModelMatrix = GLMUtil::fromMat4x4(temp_mesh_renderer.model_matrix); // temp_node.model_matrix;
            curRenderableMeshData.modelFromWorldMatrix = GLMUtil::fromMat4x4(temp_mesh_renderer.model_matrix_inverse);//temp_node.model_matrix_inverse;
            curRenderableMeshData.vertexBuffer         = temp_ref_mesh.vertex_buffer.vertex_buffer->GetVertexBufferView();//temp_node.ref_mesh->p_mesh_vertex_buffer->GetVertexBufferView();
            curRenderableMeshData.indexBuffer          = temp_ref_mesh.index_buffer.index_buffer->GetIndexBufferView();//temp_node.ref_mesh->p_mesh_index_buffer->GetIndexBufferView();
            curRenderableMeshData.drawIndexedArguments = drawIndexedArguments;
            curRenderableMeshData.boundingBox          = boundingBox;
            curRenderableMeshData.perMaterialViewIndexBufferIndex = i;

            pRenderableMeshBuffer[i] = curRenderableMeshData;
        }

        prepareBuffer();
    }

    void IndirectCullPass::prepareBuffer()
    {
        if (m_render_scene->m_directional_light.m_identifier != UndefCommonIdentifier)
        {
            if (dirShadowmapCommandBuffers.m_identifier != m_render_scene->m_directional_light.m_identifier)
            {
                dirShadowmapCommandBuffers.Reset();
            }

            if (dirShadowmapCommandBuffers.m_DrawCallCommandBuffer.size() != m_render_scene->m_directional_light.m_cascade)
            {
                dirShadowmapCommandBuffers.m_identifier = m_render_scene->m_directional_light.m_identifier;

                for (size_t i = 0; i < m_render_scene->m_directional_light.m_cascade; i++)
                {
                    std::wstring _name = std::wstring(L"DirectionIndirectSortCommandBuffer_Cascade_" + i);

                    DrawCallCommandBuffer _CommandBuffer = {};
                    _CommandBuffer.p_IndirectIndexCommandBuffer = CreateIndexBuffer(_name);
                    _CommandBuffer.p_IndirectSortCommandBuffer  = CreateSortCommandBuffer(_name);

                    dirShadowmapCommandBuffers.m_DrawCallCommandBuffer.push_back(_CommandBuffer);
                }
            }
        }
        else
        {
            dirShadowmapCommandBuffers.Reset();
        }

        {
            std::vector<SpotShadowmapCommandBuffer> _SpotLightToGPU = {};

            for (size_t i = 0; i < m_render_scene->m_spot_light_list.size(); i++)
            {
                InternalSpotLight& _internalSpotLight = m_render_scene->m_spot_light_list[i];
                if (_internalSpotLight.m_shadowmap)
                {
                    std::wstring _name = std::wstring(L"SpotIndirectSortCommandBuffer_" + i);

                    bool isFindInOldCommandBuffer = false;
                    for (size_t j = 0; j < spotShadowmapCommandBuffer.size(); j++)
                    {
                        if (_internalSpotLight.m_identifier == spotShadowmapCommandBuffer[j].m_identifier)
                        {
                            SpotShadowmapCommandBuffer _SpotShadowCommandBuffer = spotShadowmapCommandBuffer[j];
                            _SpotShadowCommandBuffer.m_lightIndex = i;
                            _SpotShadowCommandBuffer.m_DrawCallCommandBuffer.p_IndirectIndexCommandBuffer->SetResourceName(_name);
                            _SpotShadowCommandBuffer.m_DrawCallCommandBuffer.p_IndirectSortCommandBuffer->SetResourceName(_name);
                            _SpotLightToGPU.push_back(_SpotShadowCommandBuffer);
                            spotShadowmapCommandBuffer.erase(spotShadowmapCommandBuffer.begin() + j);
                            isFindInOldCommandBuffer = true;
                            break;
                        }
                    }
                    if (!isFindInOldCommandBuffer)
                    {
                        SpotShadowmapCommandBuffer _SpotShadowCommandBuffer = {};
                        _SpotShadowCommandBuffer.m_lightIndex = i;
                        _SpotShadowCommandBuffer.m_identifier = _internalSpotLight.m_identifier;
                        _SpotShadowCommandBuffer.m_DrawCallCommandBuffer.p_IndirectIndexCommandBuffer = CreateIndexBuffer(_name);
                        _SpotShadowCommandBuffer.m_DrawCallCommandBuffer.p_IndirectSortCommandBuffer = CreateSortCommandBuffer(_name);
                        _SpotLightToGPU.push_back(_SpotShadowCommandBuffer);
                    }
                }
            }
            spotShadowmapCommandBuffer.clear();
            spotShadowmapCommandBuffer = _SpotLightToGPU;
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

        context->TransitionBarrier(countBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        context->TransitionBarrier(sortDispatchArgBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        context->FlushResourceBarriers();

        context->SetRootSignature(RootSignatures::pBitonicSortRootSignature.get());
        // Generate execute indirect arguments
        context->SetPipelineState(PipelineStates::pBitonicIndirectArgsPSO.get());

        // This controls two things.  It is a key that will sort to the end, and it is a mask used to
        // determine whether the current group should sort ascending or descending.
        //context.SetConstants(3, counterOffset, sortAscending ? 0xffffffff : 0);
        context->SetConstants(0, MaxIterations);
        context->SetDescriptorTable(1, countBuffer->GetDefaultSRV()->GetGpuHandle());
        context->SetDescriptorTable(2, sortDispatchArgBuffer->GetDefaultUAV()->GetGpuHandle());
        context->SetConstants(3, 0, sortAscending ? 0x7F7FFFFF : 0);
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
        RHI::RgResourceHandle uploadFrameUniformHandle = GImport(graph, pUploadFrameUniformBuffer.get());
        RHI::RgResourceHandle uploadMaterialViewIndexHandle = GImport(graph, pUploadMaterialViewIndexBuffer.get());
        RHI::RgResourceHandle uploadMeshBufferHandle     = GImport(graph, pUploadRenderableMeshBuffer.get());

        RHI::RgResourceHandle sortDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(sortDispatchArgsBufferDesc);
        RHI::RgResourceHandle grabDispatchArgsHandle = graph.Create<RHI::D3D12Buffer>(grabDispatchArgsBufferDesc);

        // import buffers
        cullOutput.perframeBufferHandle = GImport(graph, pFrameUniformBuffer.get());
        cullOutput.materialBufferHandle = GImport(graph, pMaterialViewIndexBuffer.get());
        cullOutput.meshBufferHandle     = GImport(graph, pRenderableMeshBuffer.get());

        cullOutput.opaqueDrawHandle = DrawCallCommandBufferHandle {
            GImport(graph, commandBufferForOpaqueDraw.p_IndirectIndexCommandBuffer.get()),
            GImport(graph, commandBufferForOpaqueDraw.p_IndirectSortCommandBuffer.get())};

        cullOutput.transparentDrawHandle = DrawCallCommandBufferHandle {
            GImport(graph, commandBufferForTransparentDraw.p_IndirectIndexCommandBuffer.get()),
            GImport(graph, commandBufferForTransparentDraw.p_IndirectSortCommandBuffer.get())};

        for (size_t i = 0; i < dirShadowmapCommandBuffers.m_DrawCallCommandBuffer.size(); i++)
        {
            DrawCallCommandBufferHandle bufferHandle = {
                GImport(graph, dirShadowmapCommandBuffers.m_DrawCallCommandBuffer[i].p_IndirectIndexCommandBuffer.get()),
                GImport(graph, dirShadowmapCommandBuffers.m_DrawCallCommandBuffer[i].p_IndirectSortCommandBuffer.get())};
            cullOutput.directionShadowmapHandles.push_back(bufferHandle);
        }

        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
        {
            DrawCallCommandBufferHandle bufferHandle = {
                GImport(graph, spotShadowmapCommandBuffer[i].m_DrawCallCommandBuffer.p_IndirectIndexCommandBuffer.get()),
                GImport(graph, spotShadowmapCommandBuffer[i].m_DrawCallCommandBuffer.p_IndirectSortCommandBuffer.get())};
            cullOutput.spotShadowmapHandles.push_back(bufferHandle);
        }

        // reset pass
        auto mPerframeBufferHandle  = cullOutput.perframeBufferHandle;
        auto mMaterialBufferHandle  = cullOutput.materialBufferHandle;
        auto mMeshBufferHandle      = cullOutput.meshBufferHandle;
        auto mOpaqueDrawHandle      = cullOutput.opaqueDrawHandle;
        auto mTransparentDrawHandle = cullOutput.transparentDrawHandle;
        auto mDirShadowmapHandles(cullOutput.directionShadowmapHandles);
        auto mSpotShadowmapHandles(cullOutput.spotShadowmapHandles);

        {
            RHI::RenderPass& resetPass = graph.AddRenderPass("ResetPass");

            resetPass.Read(uploadFrameUniformHandle, true);
            resetPass.Read(uploadMaterialViewIndexHandle, true);
            resetPass.Read(uploadMeshBufferHandle, true);

            resetPass.Write(cullOutput.perframeBufferHandle, true);
            resetPass.Write(cullOutput.materialBufferHandle, true);
            resetPass.Write(cullOutput.meshBufferHandle, true);
            resetPass.Write(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle, true);
            resetPass.Write(cullOutput.opaqueDrawHandle.indirectSortBufferHandle, true);
            resetPass.Write(cullOutput.transparentDrawHandle.indirectIndexBufferHandle, true);
            resetPass.Write(cullOutput.transparentDrawHandle.indirectSortBufferHandle, true);
            
            for (size_t i = 0; i < cullOutput.directionShadowmapHandles.size(); i++)
            {
                resetPass.Write(cullOutput.directionShadowmapHandles[i].indirectIndexBufferHandle, true);
                resetPass.Write(cullOutput.directionShadowmapHandles[i].indirectSortBufferHandle, true);
            }
            for (size_t i = 0; i < cullOutput.spotShadowmapHandles.size(); i++)
            {
                resetPass.Write(cullOutput.spotShadowmapHandles[i].indirectIndexBufferHandle, true);
                resetPass.Write(cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle, true);
            }

            resetPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pCopyContext = context->GetComputeContext();

                pCopyContext->ResetCounter(RegGetBufCounter(mOpaqueDrawHandle.indirectIndexBufferHandle));
                pCopyContext->ResetCounter(RegGetBufCounter(mOpaqueDrawHandle.indirectSortBufferHandle));
                pCopyContext->ResetCounter(RegGetBufCounter(mTransparentDrawHandle.indirectIndexBufferHandle));
                pCopyContext->ResetCounter(RegGetBufCounter(mTransparentDrawHandle.indirectSortBufferHandle));

                for (size_t i = 0; i < mDirShadowmapHandles.size(); i++)
                {
                    pCopyContext->ResetCounter(RegGetBufCounter(mDirShadowmapHandles[i].indirectSortBufferHandle));
                }

                for (size_t i = 0; i < mSpotShadowmapHandles.size(); i++)
                {
                    pCopyContext->ResetCounter(RegGetBufCounter(mSpotShadowmapHandles[i].indirectSortBufferHandle));
                }

                pCopyContext->CopyBuffer(RegGetBuf(mPerframeBufferHandle), RegGetBuf(uploadFrameUniformHandle));
                pCopyContext->CopyBuffer(RegGetBuf(mMaterialBufferHandle), RegGetBuf(uploadMaterialViewIndexHandle));
                pCopyContext->CopyBuffer(RegGetBuf(mMeshBufferHandle), RegGetBuf(uploadMeshBufferHandle));

                pCopyContext->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_GENERIC_READ);
                pCopyContext->TransitionBarrier(RegGetBuf(mMaterialBufferHandle), D3D12_RESOURCE_STATE_GENERIC_READ);
                pCopyContext->TransitionBarrier(RegGetBuf(mMeshBufferHandle), D3D12_RESOURCE_STATE_GENERIC_READ);

                pCopyContext->FlushResourceBarriers();
            });
        }

        int numMeshes = m_render_scene->m_mesh_renderers.size();
        if (numMeshes > 0)
        {
            {
                RHI::RenderPass& cullingPass = graph.AddRenderPass("OpaqueTransCullingPass");

                cullingPass.Read(cullOutput.perframeBufferHandle, true);
                cullingPass.Read(cullOutput.meshBufferHandle, true);
                cullingPass.Read(cullOutput.materialBufferHandle, true);

                cullingPass.Write(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle, true);
                cullingPass.Write(cullOutput.transparentDrawHandle.indirectIndexBufferHandle, true);

                cullingPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    pAsyncCompute->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mMeshBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mMaterialBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mOpaqueDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->TransitionBarrier(RegGetBufCounter(mOpaqueDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mTransparentDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->TransitionBarrier(RegGetBufCounter(mTransparentDrawHandle.indirectIndexBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

                    /**/
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
                        RootIndexBuffer {RegGetBufDefCBVIdx(mPerframeBufferHandle),
                                         RegGetBufDefSRVIdx(mMeshBufferHandle),
                                         RegGetBufDefSRVIdx(mMaterialBufferHandle),
                                         RegGetBufDefUAVIdx(mOpaqueDrawHandle.indirectIndexBufferHandle),
                                         RegGetBufDefUAVIdx(mTransparentDrawHandle.indirectIndexBufferHandle)};

                    pAsyncCompute->SetConstantArray(0, sizeof(RootIndexBuffer) / sizeof(UINT), &rootIndexBuffer);
                    

                    /*
                    pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCull.get());
                    pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCull.get());

                    pAsyncCompute->SetConstantBuffer(0, RegGetBuf(mPerframeBufferHandle)->GetGpuVirtualAddress());
                    pAsyncCompute->SetBufferSRV(1, RegGetBuf(mMeshBufferHandle));
                    pAsyncCompute->SetBufferSRV(2, RegGetBuf(mMaterialBufferHandle));
                    pAsyncCompute->SetDescriptorTable(3, RegGetBuf(mOpaqueDrawHandle.indirectIndexBufferHandle)->GetDefaultUAV()->GetGpuHandle());
                    pAsyncCompute->SetDescriptorTable(4, RegGetBuf(mTransparentDrawHandle.indirectIndexBufferHandle)->GetDefaultUAV()->GetGpuHandle());
                    */

                    pAsyncCompute->Dispatch1D(numMeshes, 128);
                });
            }

            {
                RHI::RenderPass& opaqueSortPass = graph.AddRenderPass("OpaqueBitonicSortPass");

                opaqueSortPass.Read(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle, true);

                opaqueSortPass.Write(sortDispatchArgsHandle, true);
                opaqueSortPass.Write(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle, true);

                opaqueSortPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* bufferPtr           = RegGetBuf(mOpaqueDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* bufferCouterPtr     = bufferPtr->GetCounterBuffer().get();
                    RHI::D3D12Buffer* sortDispatchArgsPtr = RegGetBuf(sortDispatchArgsHandle);

                    bitonicSort(pAsyncCompute, bufferPtr, bufferCouterPtr, sortDispatchArgsPtr, false, true);
                });
            }

            {
                RHI::RenderPass& transSortPass = graph.AddRenderPass("TransparentBitonicSortPass");

                transSortPass.Read(cullOutput.transparentDrawHandle.indirectIndexBufferHandle, true);

                transSortPass.Write(sortDispatchArgsHandle, true);
                transSortPass.Write(cullOutput.transparentDrawHandle.indirectIndexBufferHandle, true);

                transSortPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* bufferPtr           = RegGetBuf(mTransparentDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* bufferCouterPtr     = bufferPtr->GetCounterBuffer().get();
                    RHI::D3D12Buffer* sortDispatchArgsPtr = RegGetBuf(sortDispatchArgsHandle);

                    bitonicSort(pAsyncCompute, bufferPtr, bufferCouterPtr, sortDispatchArgsPtr, false, false);
                });
            }

            {
                RHI::RenderPass& grabOpaquePass = graph.AddRenderPass("GrabOpaquePass");

                grabOpaquePass.Read(cullOutput.meshBufferHandle, true);
                grabOpaquePass.Read(cullOutput.opaqueDrawHandle.indirectIndexBufferHandle, true);

                grabOpaquePass.Write(grabDispatchArgsHandle, true);
                grabOpaquePass.Write(cullOutput.opaqueDrawHandle.indirectSortBufferHandle, true);

                grabOpaquePass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* meshBufferPtr          = RegGetBuf(mMeshBufferHandle);
                    RHI::D3D12Buffer* indirectIndexBufferPtr = RegGetBuf(mOpaqueDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* indirectSortBufferPtr  = RegGetBuf(mOpaqueDrawHandle.indirectSortBufferHandle);
                    RHI::D3D12Buffer* grabDispatchArgsPtr    = RegGetBuf(grabDispatchArgsHandle);

                    grabObject(pAsyncCompute,
                               meshBufferPtr,
                               indirectIndexBufferPtr,
                               indirectSortBufferPtr,
                               grabDispatchArgsPtr);
                });
            }

            {
                RHI::RenderPass& grabTransPass = graph.AddRenderPass("GrabTransPass");

                grabTransPass.Read(cullOutput.meshBufferHandle, true);
                grabTransPass.Read(cullOutput.transparentDrawHandle.indirectIndexBufferHandle, true);

                grabTransPass.Write(grabDispatchArgsHandle, true);
                grabTransPass.Write(cullOutput.transparentDrawHandle.indirectSortBufferHandle, true);

                grabTransPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                    RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                    RHI::D3D12Buffer* meshBufferPtr = RegGetBuf(mMeshBufferHandle);
                    RHI::D3D12Buffer* indirectIndexBufferPtr =
                        RegGetBuf(mTransparentDrawHandle.indirectIndexBufferHandle);
                    RHI::D3D12Buffer* indirectSortBufferPtr =
                        RegGetBuf(mTransparentDrawHandle.indirectSortBufferHandle);
                    RHI::D3D12Buffer* grabDispatchArgsPtr = RegGetBuf(grabDispatchArgsHandle);

                    grabObject(pAsyncCompute,
                               meshBufferPtr,
                               indirectIndexBufferPtr,
                               indirectSortBufferPtr,
                               grabDispatchArgsPtr);
                });
            }
        }

        if (mDirShadowmapHandles.size() != 0 && numMeshes > 0)
        {
            RHI::RenderPass& dirLightShadowCullPass = graph.AddRenderPass("DirectionLightShadowCullPass");

            dirLightShadowCullPass.Read(cullOutput.perframeBufferHandle, true);
            dirLightShadowCullPass.Read(cullOutput.meshBufferHandle, true);
            dirLightShadowCullPass.Read(cullOutput.materialBufferHandle, true);

            for (size_t i = 0; i < cullOutput.directionShadowmapHandles.size(); i++)
            {
                dirLightShadowCullPass.Write(cullOutput.directionShadowmapHandles[i].indirectSortBufferHandle, true);
            }

            dirLightShadowCullPass.Execute([=](RHI::RenderGraphRegistry* registry, RHI::D3D12CommandContext* context) {
                RHI::D3D12ComputeContext* pAsyncCompute = context->GetComputeContext();

                pAsyncCompute->TransitionBarrier(RegGetBuf(mPerframeBufferHandle), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                pAsyncCompute->TransitionBarrier(RegGetBuf(mMeshBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                pAsyncCompute->TransitionBarrier(RegGetBuf(mMaterialBufferHandle), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                for (size_t i = 0; i < mDirShadowmapHandles.size(); i++)
                {
                    pAsyncCompute->TransitionBarrier(RegGetBuf(mDirShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->InsertUAVBarrier(RegGetBuf(mDirShadowmapHandles[i].indirectSortBufferHandle));
                    pAsyncCompute->TransitionBarrier(RegGetBufCounter(mDirShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->InsertUAVBarrier(RegGetBufCounter(mDirShadowmapHandles[i].indirectSortBufferHandle));
                }
                pAsyncCompute->FlushResourceBarriers();

                for (int i = 0; i < mDirShadowmapHandles.size(); i++)
                {
                    pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCullDirectionShadowmap.get());
                    pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCullDirectionShadowmap.get());


                    pAsyncCompute->SetConstant(0, 0, RHI::DWParam(i));
                    pAsyncCompute->SetConstantBuffer(1, RegGetBuf(mPerframeBufferHandle)->GetGpuVirtualAddress());
                    pAsyncCompute->SetBufferSRV(2, RegGetBuf(mMeshBufferHandle));
                    pAsyncCompute->SetBufferSRV(3, RegGetBuf(mMaterialBufferHandle));
                    pAsyncCompute->SetDescriptorTable(4, RegGetBuf(mDirShadowmapHandles[i].indirectSortBufferHandle)->GetDefaultUAV()->GetGpuHandle());
                
                    pAsyncCompute->Dispatch1D(numMeshes, 128);

                    pAsyncCompute->TransitionBarrier(RegGetBuf(mDirShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                }
            });
        }

        if (spotShadowmapCommandBuffer.size() != 0 && numMeshes > 0)
        {
            RHI::RenderPass& spotLightShadowCullPass = graph.AddRenderPass("SpotLightShadowCullPass");

            spotLightShadowCullPass.Read(cullOutput.perframeBufferHandle, true);
            spotLightShadowCullPass.Read(cullOutput.meshBufferHandle, true);
            spotLightShadowCullPass.Read(cullOutput.materialBufferHandle, true);

            std::vector<uint32_t> _spotlightIndex;
            for (size_t i = 0; i < cullOutput.spotShadowmapHandles.size(); i++)
            {
                spotLightShadowCullPass.Write(cullOutput.spotShadowmapHandles[i].indirectSortBufferHandle, true);
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
                    pAsyncCompute->TransitionBarrier(RegGetBufCounter(mSpotShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    pAsyncCompute->InsertUAVBarrier(RegGetBufCounter(mSpotShadowmapHandles[i].indirectSortBufferHandle));
                }
                pAsyncCompute->FlushResourceBarriers();

                for (size_t i = 0; i < mSpotShadowmapHandles.size(); i++)
                {
                    pAsyncCompute->SetPipelineState(PipelineStates::pIndirectCullSpotShadowmap.get());
                    pAsyncCompute->SetRootSignature(RootSignatures::pIndirectCullSpotShadowmap.get());

                    pAsyncCompute->SetConstant(0, 0, _spotlightIndex[i]);
                    pAsyncCompute->SetConstantBuffer(1, RegGetBuf(mPerframeBufferHandle)->GetGpuVirtualAddress());
                    pAsyncCompute->SetBufferSRV(2, RegGetBuf(mMeshBufferHandle));
                    pAsyncCompute->SetBufferSRV(3, RegGetBuf(mMaterialBufferHandle));
                    pAsyncCompute->SetDescriptorTable(4, RegGetBuf(mSpotShadowmapHandles[i].indirectSortBufferHandle)->GetDefaultUAV()->GetGpuHandle());

                    pAsyncCompute->Dispatch1D(numMeshes, 128);

                    pAsyncCompute->TransitionBarrier(RegGetBuf(mSpotShadowmapHandles[i].indirectSortBufferHandle), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                }
            });
        }
    }

    void IndirectCullPass::destroy()
    {
        pUploadFrameUniformBuffer      = nullptr;
        pUploadMaterialViewIndexBuffer = nullptr;
        pUploadRenderableMeshBuffer    = nullptr;

        pFrameUniformBuffer      = nullptr;
        pMaterialViewIndexBuffer = nullptr;
        pRenderableMeshBuffer    = nullptr;

        commandBufferForOpaqueDraw.ResetBuffer();
        commandBufferForTransparentDraw.ResetBuffer();
        dirShadowmapCommandBuffers.Reset(); 
        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
            spotShadowmapCommandBuffer[i].Reset();
        spotShadowmapCommandBuffer.clear();

    }

}
