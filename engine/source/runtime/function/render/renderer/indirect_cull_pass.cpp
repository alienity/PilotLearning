#include "runtime/function/render/renderer/indirect_cull_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/window_system.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{
    void IndirectCullPass::initialize(const RenderPassInitInfo& init_info)
    {
        p_IndirectCommandBuffer = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                                     commandBufferCounterOffset + sizeof(uint64_t),
                                                                     sizeof(HLSL::CommandSignatureParams),
                                                                     D3D12_HEAP_TYPE_DEFAULT,
                                                                     D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        p_IndirectCommandBufferUav = std::make_shared<RHI::D3D12UnorderedAccessView>(
            m_Device->GetLinkedDevice(), p_IndirectCommandBuffer.get(), HLSL::MeshLimit, commandBufferCounterOffset);

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
        pPerframeObj          = pUploadPerframeBuffer->GetCpuVirtualAddress<HLSL::MeshPerframeStorageBufferObject>();
        pMaterialObj          = pUploadMaterialBuffer->GetCpuVirtualAddress<HLSL::MaterialInstance>();
        pMeshesObj            = pUploadMeshBuffer->GetCpuVirtualAddress<HLSL::MeshInstance>();

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
        pMeshBuffer     = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                         sizeof(HLSL::MeshInstance) * HLSL::MeshLimit,
                                                         sizeof(HLSL::MeshInstance),
                                                         D3D12_HEAP_TYPE_DEFAULT,
                                                         D3D12_RESOURCE_FLAG_NONE);


    }

    void IndirectCullPass::prepareMeshData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        RenderResource* real_resource = (RenderResource*)render_resource.get();
        memcpy(pPerframeObj,
               &real_resource->m_mesh_perframe_storage_buffer_object,
               sizeof(HLSL::MeshPerframeStorageBufferObject));

        std::vector<RenderMeshNode>& renderMeshNodes = *m_visiable_nodes.p_all_mesh_nodes;

        numMeshes = renderMeshNodes.size();
        assert(numMeshes < HLSL::MeshLimit);
        for (size_t i = 0; i < numMeshes; i++)
        {
            RenderMeshNode& temp_node = renderMeshNodes[i];

            RHI::D3D12ShaderResourceView& defaultWhiteView =
                real_resource->m_default_resource._white_texture2d_image_view;
            RHI::D3D12ShaderResourceView& defaultBlackView =
                real_resource->m_default_resource._white_texture2d_image_view;

            RHI::D3D12ShaderResourceView& uniformBufferView  = temp_node.ref_material->material_uniform_buffer_view;
            RHI::D3D12ShaderResourceView& baseColorView = temp_node.ref_material->base_color_image_view;
            RHI::D3D12ShaderResourceView& metallicRoughnessView = temp_node.ref_material->metallic_roughness_image_view;
            RHI::D3D12ShaderResourceView& normalView            = temp_node.ref_material->normal_image_view;
            RHI::D3D12ShaderResourceView& emissionView          = temp_node.ref_material->emissive_image_view;


            HLSL::MaterialInstance curMatInstance = {};
            curMatInstance.uniformBufferViewIndex =
                uniformBufferView.IsValid() ? uniformBufferView.GetIndex() : defaultWhiteView.GetIndex();
            curMatInstance.baseColorViewIndex =
                baseColorView.IsValid() ? baseColorView.GetIndex() : defaultWhiteView.GetIndex();
            curMatInstance.metallicRoughnessViewIndex =
                metallicRoughnessView.IsValid() ? metallicRoughnessView.GetIndex() : defaultBlackView.GetIndex();
            curMatInstance.normalViewIndex = normalView.IsValid() ? normalView.GetIndex() : defaultWhiteView.GetIndex();
            curMatInstance.emissionViewIndex =
                emissionView.IsValid() ? emissionView.GetIndex() : defaultBlackView.GetIndex();

            pMaterialObj[i] = curMatInstance;

            HLSL::MeshInstance curMeshInstance;
            curMeshInstance.enable_vertex_blending = temp_node.enable_vertex_blending;
            curMeshInstance.model_matrix           = temp_node.model_matrix;
            curMeshInstance.vertexBuffer           = temp_node.ref_mesh->mesh_vertex_buffer.GetVertexBufferView();
            curMeshInstance.indexBuffer            = temp_node.ref_mesh->mesh_index_buffer.GetIndexBufferView();
            curMeshInstance.bounding_box_min       = temp_node.bounding_box_min;
            curMeshInstance.bounding_box_max       = temp_node.bounding_box_max;
            curMeshInstance.materialIndex          = i;

            pMeshesObj[i] = curMeshInstance;
        }

    }

    void IndirectCullPass::cullMeshs(RHI::D3D12CommandContext& context,
                                     RHI::RenderGraphRegistry& registry,
                                     IndirectCullResultBuffer& indirectCullResult)
    {
        RHI::D3D12SyncHandle ComputeSyncHandle;
        if (numMeshes > 0)
        {
            RHI::D3D12CommandContext& copyContext = m_Device->GetLinkedDevice()->GetCopyContext1();
            copyContext.Open();
            {
                copyContext.ResetCounter(p_IndirectCommandBuffer.get(), commandBufferCounterOffset);
                copyContext->CopyResource(pPerframeBuffer->GetResource(), pUploadPerframeBuffer->GetResource());
                copyContext->CopyResource(pMaterialBuffer->GetResource(), pUploadMaterialBuffer->GetResource());
                copyContext->CopyResource(pMeshBuffer->GetResource(), pMeshBuffer->GetResource());
            }
            copyContext.Close();
            RHI::D3D12SyncHandle copySyncHandle = copyContext.Execute(false);

            RHI::D3D12CommandContext& asyncCompute = m_Device->GetLinkedDevice()->GetAsyncComputeCommandContext();
            asyncCompute.GetCommandQueue()->WaitForSyncHandle(copySyncHandle);

            asyncCompute.Open();
            {
                D3D12ScopedEvent(AsyncCompute, "Gpu Frustum Culling");
                asyncCompute.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectCull));
                asyncCompute.SetComputeRootSignature(registry.GetRootSignature(RootSignatures::IndirectCull));

                asyncCompute->SetComputeRootConstantBufferView(0, pPerframeBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(1, WorldRenderView->Meshes.GetGpuVirtualAddress());
                asyncCompute->SetComputeRootDescriptorTable(2, IndirectCommandBufferUav.GetGpuHandle());

                asyncCompute.Dispatch1D<128>(numMeshes);
            }
            asyncCompute.Close();
            ComputeSyncHandle = asyncCompute.Execute(false);
        }

        context.GetCommandQueue()->WaitForSyncHandle(ComputeSyncHandle);

    }

    void IndirectCullPass::destroy()
    {

    }

}
