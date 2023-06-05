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

        void updatePerFrameBuffer(std::shared_ptr<RenderScene> render_scene, std::shared_ptr<RenderCamera> camera);
        
        bool updateInternalMaterial(SceneMaterial scene_material, SceneMaterial& cached_material, InternalMaterial& internal_material);
        bool updateInternalMesh(SceneMesh scene_mesh, SceneMesh& cached_mesh, InternalMesh& internal_mesh);
        bool updateInternalMeshRenderer(SceneMeshRenderer scene_mesh_renderer, SceneMeshRenderer& cached_mesh_renderer, InternalMeshRenderer& internal_mesh_renderer);

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
        std::shared_ptr<RHI::D3D12Texture> createTex2D(std::shared_ptr<TextureData>& tex2d_data, DXGI_FORMAT format, bool is_srgb, bool genMips = false, bool batch = false);

        std::shared_ptr<RHI::D3D12Texture> createCubeMap(std::array<std::shared_ptr<TextureData>, 6>& cube_maps, DXGI_FORMAT format, bool is_srgb, bool genMips = false, bool batch = false);

        InternalMesh createInternalMesh(SceneMesh scene_mesh);
        InternalMesh createInternalMesh(RenderMeshData mesh_data);
        InternalVertexBuffer createVertexBuffer(InputDefinition input_definition, std::shared_ptr<BufferData> vertex_buffer);
        InternalIndexBuffer createIndexBuffer(std::shared_ptr<BufferData> index_buffer);
    };
} // namespace MoYu
