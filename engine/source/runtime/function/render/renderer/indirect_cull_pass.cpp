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

        // 初始化绘制中用到的对象
        pPerframeBuffer = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                             sizeof(HLSL::MeshPerframeStorageBufferObject),
                                                             sizeof(HLSL::MeshPerframeStorageBufferObject),
                                                             D3D12_HEAP_TYPE_UPLOAD,
                                                             D3D12_RESOURCE_FLAG_NONE);
        pMaterialBuffer = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                             sizeof(HLSL::MaterialInstance) * HLSL::MaterialLimit,
                                                             sizeof(HLSL::MaterialInstance),
                                                             D3D12_HEAP_TYPE_UPLOAD,
                                                             D3D12_RESOURCE_FLAG_NONE);
        pMeshBuffer    = std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                                          sizeof(HLSL::MeshInstance) * HLSL::MeshLimit,
                                                          sizeof(HLSL::MeshInstance),
                                                          D3D12_HEAP_TYPE_UPLOAD,
                                                          D3D12_RESOURCE_FLAG_NONE);

        pPerframeObj = pPerframeBuffer->GetCpuVirtualAddress<HLSL::MeshPerframeStorageBufferObject>();
        pMaterialObj = pMaterialBuffer->GetCpuVirtualAddress<HLSL::MaterialInstance>();
        pMeshesObj   = pMeshBuffer->GetCpuVirtualAddress<HLSL::MeshInstance>();
    }

    void IndirectCullPass::prepareMeshData(std::shared_ptr<RenderResourceBase>& render_resource)
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

            HLSL::MaterialInstance curMatInstance;
            curMatInstance.uniformBufferViewIndex = temp_node.ref_material->material_uniform_buffer_view.GetIndex();
            curMatInstance.baseColorViewIndex     = temp_node.ref_material->base_color_image_view.GetIndex();
            curMatInstance.metallicRoughnessViewIndex =
                temp_node.ref_material->metallic_roughness_image_view.GetIndex();
            curMatInstance.normalViewIndex   = temp_node.ref_material->normal_image_view.GetIndex();
            curMatInstance.emissionViewIndex = temp_node.ref_material->emissive_image_view.GetIndex();

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

    void IndirectCullPass::cullMeshs(IndirectCullResultBuffer& indirectCullResult)
    {

    }

    void IndirectCullPass::destroy()
    {

    }

}
