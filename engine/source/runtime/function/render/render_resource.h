#pragma once

#include "runtime/function/render/render_resource_loader.h"
#include "runtime/function/render/render_common.h"

#include <array>
#include <cstdint>
#include <map>
#include <vector>
#include <tuple>

namespace MoYu
{
    class RenderCamera;

    class RenderResource : public RenderResourceBase
    {
    public:
        RenderResource() = default;

        void uploadGlobalRenderResource(LevelResourceDesc level_resource_desc);

        void uploadGameObjectRenderResource(RenderEntity render_entity, RenderMeshData mesh_data, RenderMaterialData material_data);

        void uploadGameObjectRenderResource(RenderEntity render_entity, RenderMeshData mesh_data);

        void uploadGameObjectRenderResource(RenderEntity render_entity, RenderMaterialData material_data);

        void updatePerFrameBuffer(std::shared_ptr<RenderScene> render_scene, std::shared_ptr<RenderCamera> camera);

        // bindless objects
        HLSL::MeshPerframeStorageBufferObject                 m_mesh_perframe_storage_buffer_object;
        HLSL::MeshPointLightShadowPerframeStorageBufferObject m_mesh_point_light_shadow_perframe_storage_buffer_object;
        HLSL::MeshSpotLightShadowPerframeStorageBufferObject  m_mesh_spot_light_shadow_perframe_storage_buffer_object;
        HLSL::MeshDirectionalLightShadowPerframeStorageBufferObject m_mesh_directional_light_shadow_perframe_storage_buffer_object;
        HLSL::MeshInstance m_all_mesh_buffer_object;

    protected:
        std::shared_ptr<RHI::D3D12Buffer> createDynamicBuffer(void* buffer_data, uint32_t buffer_size, uint32_t buffer_stride);
        std::shared_ptr<RHI::D3D12Buffer> createDynamicBuffer(std::shared_ptr<BufferData>& buffer_data);
        
        std::shared_ptr<RHI::D3D12Buffer> createStaticBuffer(void* buffer_data, uint32_t buffer_size, uint32_t buffer_stride, bool raw, bool batch = false);
        std::shared_ptr<RHI::D3D12Buffer> createStaticBuffer(std::shared_ptr<BufferData>& buffer_data, bool raw, bool batch = false);

        std::shared_ptr<RHI::D3D12Texture> createTex2D(uint32_t width, uint32_t height, void* pixels, DXGI_FORMAT format, bool is_srgb, bool genMips = false, bool batch = false);
        std::shared_ptr<RHI::D3D12Texture> createTex2D(std::shared_ptr<TextureData>& tex2d_data, bool genMips = false, bool batch = false);

        std::shared_ptr<RHI::D3D12Texture> createCubeMap(std::array<std::shared_ptr<TextureData>, 6>& cube_maps, bool genMips = false, bool batch = false);

        /*
        void updateMeshData(bool                                          enable_vertex_blending,
                            uint32_t                                      index_buffer_size,
                            void*                                         index_buffer_data,
                            uint32_t                                      vertex_buffer_size,
                            struct MeshVertexDataDefinition const*        vertex_buffer_data,
                            uint32_t                                      joint_binding_buffer_size,
                            struct MeshVertexBindingDataDefinition const* joint_binding_buffer_data,
                            D3D12Mesh&                                    now_mesh);
        void updateVertexBuffer(bool                                          enable_vertex_blending,
                                uint32_t                                      vertex_buffer_size,
                                struct MeshVertexDataDefinition const*        vertex_buffer_data,
                                uint32_t                                      joint_binding_buffer_size,
                                struct MeshVertexBindingDataDefinition const* joint_binding_buffer_data,
                                D3D12Mesh&                                    now_mesh);
        void updateIndexBuffer(uint32_t index_buffer_size, void* index_buffer_data, D3D12Mesh& now_mesh);
        */
    };
} // namespace MoYu
