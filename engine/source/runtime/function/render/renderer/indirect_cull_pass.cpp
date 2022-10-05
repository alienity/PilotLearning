#include "runtime/function/render/renderer/indirect_cull_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/window_system.h"

#include "runtime/function/render/rhi/rhi_core.h"

#include <cassert>

namespace Pilot
{
    void IndirectCullPass::initialize(const RenderPassInitInfo& init_info)
    {
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
    }

    void IndirectCullPass::prepareMeshData(std::shared_ptr<RenderResourceBase> render_resource, uint32_t& numMeshes)
    {
        RenderResource* real_resource = (RenderResource*)render_resource.get();
        memcpy(pPerframeObj, &real_resource->m_mesh_perframe_storage_buffer_object, sizeof(HLSL::MeshPerframeStorageBufferObject));

        std::vector<RenderMeshNode>& renderMeshNodes = *m_visiable_nodes.p_all_mesh_nodes;

        numMeshes = renderMeshNodes.size();
        assert(numMeshes < HLSL::MeshLimit);
        for (size_t i = 0; i < numMeshes; i++)
        {
            RenderMeshNode& temp_node = renderMeshNodes[i];

            RHI::D3D12ShaderResourceView& defaultWhiteView = real_resource->m_default_resource._white_texture2d_image_view;
            RHI::D3D12ShaderResourceView& defaultBlackView = real_resource->m_default_resource._black_texture2d_image_view;

            RHI::D3D12ShaderResourceView& uniformBufferView     = temp_node.ref_material->material_uniform_buffer_view;
            RHI::D3D12ShaderResourceView& baseColorView         = temp_node.ref_material->base_color_image_view;
            RHI::D3D12ShaderResourceView& metallicRoughnessView = temp_node.ref_material->metallic_roughness_image_view;
            RHI::D3D12ShaderResourceView& normalView            = temp_node.ref_material->normal_image_view;
            RHI::D3D12ShaderResourceView& emissionView          = temp_node.ref_material->emissive_image_view;


            HLSL::MaterialInstance curMatInstance = {};
            curMatInstance.uniformBufferViewIndex = uniformBufferView.GetIndex();
            curMatInstance.baseColorViewIndex = baseColorView.IsValid() ? baseColorView.GetIndex() : defaultWhiteView.GetIndex();
            curMatInstance.metallicRoughnessViewIndex = metallicRoughnessView.IsValid() ? metallicRoughnessView.GetIndex() : defaultBlackView.GetIndex();
            curMatInstance.normalViewIndex = normalView.IsValid() ? normalView.GetIndex() : defaultWhiteView.GetIndex();
            curMatInstance.emissionViewIndex = emissionView.IsValid() ? emissionView.GetIndex() : defaultBlackView.GetIndex();

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

    }

    void IndirectCullPass::cullMeshs(RHI::D3D12CommandContext& context,
                                     RHI::RenderGraphRegistry& registry,
                                     IndirectCullParams&       indirectCullParams)
    {
        RHI::D3D12SyncHandle ComputeSyncHandle;
        if (indirectCullParams.numMeshes > 0)
        {
            RHI::D3D12CommandContext& copyContext = m_Device->GetLinkedDevice()->GetCopyContext1();
            copyContext.Open();
            {
                copyContext.ResetCounter(indirectCullParams.p_IndirectCommandBuffer.get(), indirectCullParams.commandBufferCounterOffset);
                copyContext.ResetCounter(indirectCullParams.p_IndirectShadowmapCommandBuffer.get(), indirectCullParams.commandBufferCounterOffset);
                copyContext->CopyResource(indirectCullParams.pPerframeBuffer->GetResource(), pUploadPerframeBuffer->GetResource());
                copyContext->CopyResource(indirectCullParams.pMaterialBuffer->GetResource(), pUploadMaterialBuffer->GetResource());
                copyContext->CopyResource(indirectCullParams.pMeshBuffer->GetResource(), pUploadMeshBuffer->GetResource());
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

                asyncCompute->SetComputeRootConstantBufferView(0, indirectCullParams.pPerframeBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(1, indirectCullParams.pMeshBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootDescriptorTable(2, indirectCullParams.p_IndirectCommandBufferUav->GetGpuHandle());

                asyncCompute.Dispatch1D<128>(indirectCullParams.numMeshes);
            }
            {
                D3D12ScopedEvent(asyncCompute, "Gpu Frustum Culling for direction light shadowmap");
                asyncCompute.SetPipelineState(registry.GetPipelineState(PipelineStates::IndirectCullShadowmap));
                asyncCompute.SetComputeRootSignature(registry.GetRootSignature(RootSignatures::IndirectCull));
                
                asyncCompute->SetComputeRootConstantBufferView(0, indirectCullParams.pPerframeBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootShaderResourceView(1, indirectCullParams.pMeshBuffer->GetGpuVirtualAddress());
                asyncCompute->SetComputeRootDescriptorTable(2, indirectCullParams.p_IndirectShadowmapCommandBufferUav->GetGpuHandle());

                asyncCompute.Dispatch1D<128>(indirectCullParams.numMeshes);
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
    }

}
