#include "runtime/function/render/renderer/indirect_cull_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/window_system.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{
    void IndirectCullPass::initialize(const RenderPassInitInfo& init_info)
    {
        // create default buffer
        pPerframeBuffer = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                             sizeof(HLSL::MeshPerframeStorageBufferObject),
                                                             sizeof(HLSL::MeshPerframeStorageBufferObject),
                                                             D3D12_HEAP_TYPE_DEFAULT,
                                                             D3D12_RESOURCE_FLAG_NONE);

        pMaterialBuffer = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                             sizeof(HLSL::MaterialInstance) * HLSL::MaterialLimit,
                                                             sizeof(HLSL::MaterialInstance),
                                                             D3D12_HEAP_TYPE_DEFAULT,
                                                             D3D12_RESOURCE_FLAG_NONE);

        pMeshBuffer = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                         sizeof(HLSL::MeshInstance) * HLSL::MeshLimit,
                                                         sizeof(HLSL::MeshInstance),
                                                         D3D12_HEAP_TYPE_DEFAULT,
                                                         D3D12_RESOURCE_FLAG_NONE);

        // create upload buffer
        pUploadPerframeBuffer = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                                   sizeof(HLSL::MeshPerframeStorageBufferObject),
                                                                   sizeof(HLSL::MeshPerframeStorageBufferObject),
                                                                   D3D12_HEAP_TYPE_UPLOAD,
                                                                   D3D12_RESOURCE_FLAG_NONE);
        pUploadMaterialBuffer = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                                   sizeof(HLSL::MaterialInstance) * HLSL::MaterialLimit,
                                                                   sizeof(HLSL::MaterialInstance),
                                                                   D3D12_HEAP_TYPE_UPLOAD,
                                                                   D3D12_RESOURCE_FLAG_NONE);
        pUploadMeshBuffer     = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                               sizeof(HLSL::MeshInstance) * HLSL::MeshLimit,
                                                               sizeof(HLSL::MeshInstance),
                                                               D3D12_HEAP_TYPE_UPLOAD,
                                                               D3D12_RESOURCE_FLAG_NONE);

        pPerframeObj = pUploadPerframeBuffer->GetCpuVirtualAddress<HLSL::MeshPerframeStorageBufferObject>();
        pMaterialObj = pUploadMaterialBuffer->GetCpuVirtualAddress<HLSL::MaterialInstance>();
        pMeshesObj   = pUploadMeshBuffer->GetCpuVirtualAddress<HLSL::MeshInstance>();

        // buffer for later draw
        commandBufferForDraw.p_IndirectCommandBuffer =
            std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                               HLSL::commandBufferCounterOffset + sizeof(uint64_t),
                                               sizeof(HLSL::CommandSignatureParams),
                                               D3D12_HEAP_TYPE_DEFAULT,
                                               D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        commandBufferForDraw.p_IndirectCommandBufferUav =
            std::make_shared<RHI::D3D12UnorderedAccessView>(m_Device->GetLinkedDevice(),
                                                            commandBufferForDraw.p_IndirectCommandBuffer.get(),
                                                            HLSL::MeshLimit,
                                                            HLSL::commandBufferCounterOffset);

    }

    void IndirectCullPass::prepareMeshData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        RenderResource* real_resource = (RenderResource*)render_resource.get();
        memcpy(pPerframeObj, &real_resource->m_mesh_perframe_storage_buffer_object, sizeof(HLSL::MeshPerframeStorageBufferObject));

        std::vector<RenderMeshNode>& renderMeshNodes = *m_visiable_nodes.p_all_mesh_nodes;

        uint32_t numMeshes = renderMeshNodes.size();
        assert(numMeshes < HLSL::MeshLimit);
        for (size_t i = 0; i < numMeshes; i++)
        {
            RenderMeshNode& temp_node = renderMeshNodes[i];

            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultWhiteView = real_resource->m_default_resource._white_texture2d_image_view;
            std::shared_ptr<RHI::D3D12ShaderResourceView> defaultBlackView = real_resource->m_default_resource._black_texture2d_image_view;

            std::shared_ptr<RHI::D3D12ShaderResourceView> uniformBufferView = temp_node.ref_material->material_uniform_buffer_view;
            std::shared_ptr<RHI::D3D12ShaderResourceView> baseColorView = temp_node.ref_material->base_color_image_view;
            std::shared_ptr<RHI::D3D12ShaderResourceView> metallicRoughnessView = temp_node.ref_material->metallic_roughness_image_view;
            std::shared_ptr<RHI::D3D12ShaderResourceView> normalView   = temp_node.ref_material->normal_image_view;
            std::shared_ptr<RHI::D3D12ShaderResourceView> emissionView = temp_node.ref_material->emissive_image_view;


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
            curMeshInstance.vertexBuffer           = temp_node.ref_mesh->mesh_vertex_buffer.GetVertexBufferView();
            curMeshInstance.indexBuffer            = temp_node.ref_mesh->mesh_index_buffer.GetIndexBufferView();
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

            if (dirShadowmapCommandBuffer.p_IndirectCommandBuffer == nullptr)
            {
                std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer =
                    std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                       HLSL::commandBufferCounterOffset + sizeof(uint64_t),
                                                       sizeof(HLSL::CommandSignatureParams),
                                                       D3D12_HEAP_TYPE_DEFAULT,
                                                       D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
                std::shared_ptr<RHI::D3D12UnorderedAccessView> p_IndirectCommandBufferUav =
                    std::make_shared<RHI::D3D12UnorderedAccessView>(m_Device->GetLinkedDevice(),
                                                                    p_IndirectCommandBuffer.get(),
                                                                    HLSL::MeshLimit,
                                                                    HLSL::commandBufferCounterOffset);

                dirShadowmapCommandBuffer.m_gobject_id    = m_visiable_nodes.p_directional_light->m_gobject_id;
                dirShadowmapCommandBuffer.m_gcomponent_id = m_visiable_nodes.p_directional_light->m_gcomponent_id;
                dirShadowmapCommandBuffer.p_IndirectCommandBuffer    = p_IndirectCommandBuffer;
                dirShadowmapCommandBuffer.p_IndirectCommandBufferUav = p_IndirectCommandBufferUav;
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

                    std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer =
                        std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                           HLSL::commandBufferCounterOffset + sizeof(uint64_t),
                                                           sizeof(HLSL::CommandSignatureParams),
                                                           D3D12_HEAP_TYPE_DEFAULT,
                                                           D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
                    std::shared_ptr<RHI::D3D12UnorderedAccessView> p_IndirectCommandBufferUav =
                        std::make_shared<RHI::D3D12UnorderedAccessView>(m_Device->GetLinkedDevice(),
                                                                        p_IndirectCommandBuffer.get(),
                                                                        HLSL::MeshLimit,
                                                                        HLSL::commandBufferCounterOffset);

                    spotShadowCommandBuffer.m_lightIndex               = i;
                    spotShadowCommandBuffer.m_gobject_id               = curSpotLightDesc.m_gobject_id;
                    spotShadowCommandBuffer.m_gcomponent_id            = curSpotLightDesc.m_gcomponent_id;
                    spotShadowCommandBuffer.p_IndirectCommandBuffer    = p_IndirectCommandBuffer;
                    spotShadowCommandBuffer.p_IndirectCommandBufferUav = p_IndirectCommandBufferUav;

                    spotShadowmapCommandBuffer.push_back(spotShadowCommandBuffer);
                }

            }
        }
    }

    void IndirectCullPass::cullMeshs(RHI::D3D12CommandContext& context,
                                     RHI::RenderGraphRegistry& registry,
                                     IndirectCullOutput&       indirectCullOutput)
    {
        indirectCullOutput.pPerframeBuffer = pPerframeBuffer;
        indirectCullOutput.pMaterialBuffer = pMaterialBuffer;
        indirectCullOutput.pMeshBuffer     = pMeshBuffer;
        indirectCullOutput.p_IndirectCommandBuffer    = commandBufferForDraw.p_IndirectCommandBuffer;
        indirectCullOutput.p_DirShadowmapCommandBuffer = dirShadowmapCommandBuffer.p_IndirectCommandBuffer;
        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
        {
            indirectCullOutput.p_SpotShadowmapCommandBuffers.push_back(
                spotShadowmapCommandBuffer[i].p_IndirectCommandBuffer);
        }

        int numMeshes = m_visiable_nodes.p_all_mesh_nodes->size();

        RHI::D3D12SyncHandle ComputeSyncHandle;
        if (numMeshes > 0)
        {
            RHI::D3D12CommandContext& copyContext = m_Device->GetLinkedDevice()->GetCopyContext1();
            copyContext.Open();
            {
                copyContext.ResetCounter(commandBufferForDraw.p_IndirectCommandBuffer.get(), HLSL::commandBufferCounterOffset);
                if (dirShadowmapCommandBuffer.p_IndirectCommandBuffer != nullptr)
                {
                    copyContext.ResetCounter(dirShadowmapCommandBuffer.p_IndirectCommandBuffer.get(), HLSL::commandBufferCounterOffset);
                }
                for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
                {
                    copyContext.ResetCounter(spotShadowmapCommandBuffer[i].p_IndirectCommandBuffer.get(), HLSL::commandBufferCounterOffset);
                }
                copyContext->CopyResource(pPerframeBuffer->GetResource(), pUploadPerframeBuffer->GetResource());
                copyContext->CopyResource(pMaterialBuffer->GetResource(), pUploadMaterialBuffer->GetResource());
                copyContext->CopyResource(pMeshBuffer->GetResource(), pUploadMeshBuffer->GetResource());
            }
            copyContext.Close();
            RHI::D3D12SyncHandle copySyncHandle = copyContext.Execute(false);

            RHI::D3D12CommandContext& asyncCompute = m_Device->GetLinkedDevice()->GetAsyncComputeCommandContext();
            asyncCompute.GetCommandQueue()->WaitForSyncHandle(copySyncHandle);

            asyncCompute.Open();
            {
                D3D12ScopedEvent(asyncCompute, "Gpu Frustum Culling for rendering");
                asyncCompute.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectCull));
                asyncCompute.SetComputeRootSignature(registry.GetRootSignature(RootSignatures::IndirectCull));

                asyncCompute->SetComputeRootConstantBufferView(0, pPerframeBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(1, pMeshBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootDescriptorTable(2, commandBufferForDraw.p_IndirectCommandBufferUav->GetGpuHandle());

                asyncCompute.Dispatch1D<128>(numMeshes);
            }
            if (dirShadowmapCommandBuffer.p_IndirectCommandBuffer != nullptr)
            {
                D3D12ScopedEvent(asyncCompute, "Gpu Frustum Culling direction light");
                asyncCompute.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectCullDirectionShadowmap));
                asyncCompute.SetComputeRootSignature(registry.GetRootSignature(RootSignatures::IndirectCull));
                
                asyncCompute->SetComputeRootConstantBufferView(0, pPerframeBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(1,pMeshBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootDescriptorTable(2, dirShadowmapCommandBuffer.p_IndirectCommandBufferUav->GetGpuHandle());

                asyncCompute.Dispatch1D<128>(numMeshes);
            }
            if (!spotShadowmapCommandBuffer.empty())
            {
                D3D12ScopedEvent(asyncCompute, "Gpu Frustum Culling spot light");
                for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
                {
                    asyncCompute.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectCullSpotShadowmap));
                    asyncCompute.SetComputeRootSignature(registry.GetRootSignature(RootSignatures::IndirectCullSpotShadowmap));

                    asyncCompute->SetComputeRootConstantBufferView(0, pPerframeBuffer->GetGpuVirtualAddress());
                    asyncCompute->SetComputeRoot32BitConstant(1, spotShadowmapCommandBuffer[i].m_lightIndex, 0);
                    asyncCompute->SetComputeRootShaderResourceView(2, pMeshBuffer->GetGpuVirtualAddress());
                    asyncCompute->SetComputeRootDescriptorTable(3, spotShadowmapCommandBuffer[i].p_IndirectCommandBufferUav->GetGpuHandle());

                    asyncCompute.Dispatch1D<128>(numMeshes);
                }
            }
            asyncCompute.Close();
            ComputeSyncHandle = asyncCompute.Execute(false);
        }

        context.GetCommandQueue()->WaitForSyncHandle(ComputeSyncHandle);

    }

    void IndirectCullPass::destroy()
    {
        pUploadPerframeBuffer = nullptr;
        pUploadMaterialBuffer = nullptr;
        pUploadMeshBuffer     = nullptr;

        pPerframeBuffer = nullptr;
        pMaterialBuffer = nullptr;
        pMeshBuffer     = nullptr;

        commandBufferForDraw.p_IndirectCommandBufferUav = nullptr;
        commandBufferForDraw.p_IndirectCommandBuffer    = nullptr;

        dirShadowmapCommandBuffer.Reset();
        for (size_t i = 0; i < spotShadowmapCommandBuffer.size(); i++)
            spotShadowmapCommandBuffer[i].Reset();
        spotShadowmapCommandBuffer.clear();

    }

}
