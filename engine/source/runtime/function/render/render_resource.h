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
    class RenderScene;

    enum DefaultTexType
    {
        White,
        Black,
        Red,
        Green,
        Blue,
        BaseColor,
        MetallicAndRoughness,
        TangentNormal,
    };

    class RenderResource : public RenderResourceBase
    {
    public:
        RenderResource() = default;
        ~RenderResource();

        void updateFrameUniforms(RenderScene* render_scene, RenderCamera* camera);
        
        bool updateGlobalRenderResource(RenderScene* m_render_scene, GlobalRenderingRes level_resource_desc);

        bool updateInternalMaterial(SceneMaterial scene_material, SceneMaterial& cached_material, InternalMaterial& internal_material, bool has_initialized = false);
        bool updateInternalMesh(SceneMesh scene_mesh, SceneMesh& cached_mesh, InternalMesh& internal_mesh, bool has_initialized = false);
        bool updateInternalMeshRenderer(SceneMeshRenderer scene_mesh_renderer, 
            SceneMeshRenderer& cached_mesh_renderer, InternalMeshRenderer& internal_mesh_renderer, bool has_initialized = false);
        bool updateInternalTerrainRenderer(
            SceneTerrainRenderer scene_terrain_renderer, 
            SceneTerrainRenderer& cached_terrain_renderer, 
            InternalTerrainRenderer& internal_terrain_renderer, 
            glm::float4x4 model_matrix,
            glm::float4x4 model_matrix_inv,
            bool has_initialized = false);

        // bindless objects
        HLSL::FrameUniforms m_FrameUniforms;
        //HLSL::MeshPointLightShadowPerframeStorageBufferObject m_mesh_point_light_shadow_perframe_storage_buffer_object;
        //HLSL::MeshSpotLightShadowPerframeStorageBufferObject  m_mesh_spot_light_shadow_perframe_storage_buffer_object;
        //HLSL::MeshDirectionalLightShadowPerframeStorageBufferObject m_mesh_directional_light_shadow_perframe_storage_buffer_object;
        //HLSL::MeshInstance m_all_mesh_buffer_object;

    public:
        void InitDefaultTextures();
        void ReleaseAllTextures();

        std::map<SceneImage, std::shared_ptr<RHI::D3D12Texture>> _Image2TexMap;
        std::map<DefaultTexType, std::shared_ptr<RHI::D3D12Texture>> _Default2TexMap;

    protected:
        std::shared_ptr<RHI::D3D12Buffer> createDynamicBuffer(void* buffer_data, uint32_t buffer_size, uint32_t buffer_stride);
        std::shared_ptr<RHI::D3D12Buffer> createDynamicBuffer(std::shared_ptr<MoYu::MoYuScratchBuffer>& buffer_data);
        
        std::shared_ptr<RHI::D3D12Buffer> createStaticBuffer(void* buffer_data, uint32_t buffer_size, uint32_t buffer_stride, bool raw, bool batch = false);
        std::shared_ptr<RHI::D3D12Buffer> createStaticBuffer(std::shared_ptr<MoYu::MoYuScratchBuffer>& buffer_data, bool raw, bool batch = false);

        // MoYu::MoYuScratchImage 如果对应的是一整个对象就可以直接用于创建纹理或者buffer对象
        std::shared_ptr<RHI::D3D12Texture> createTex(std::shared_ptr<MoYu::MoYuScratchImage> scratch_image, bool batch = false);

        std::shared_ptr<RHI::D3D12Texture> createTex2D(uint32_t width, uint32_t height, void* pixels, DXGI_FORMAT format, bool is_srgb, bool genMips = false, bool batch = false);
        std::shared_ptr<RHI::D3D12Texture> createTex2D(std::shared_ptr<MoYu::MoYuScratchImage>& tex2d_data, DXGI_FORMAT format, bool is_srgb, bool genMips = false, bool batch = false);

        std::shared_ptr<RHI::D3D12Texture> createCubeMap(std::array<std::shared_ptr<MoYu::MoYuScratchImage>, 6>& cube_maps, DXGI_FORMAT format, bool is_srgb, bool isReadWrite = true, bool genMips = false, bool batch = false);

        std::shared_ptr<RHI::D3D12Texture> SceneImageToTexture(const SceneImage& normal_image);

        InternalMesh createInternalMesh(SceneMesh scene_mesh);
        InternalMesh createInternalMesh(RenderMeshData mesh_data);
        InternalVertexBuffer createVertexBuffer(InputDefinition input_definition, std::shared_ptr<MoYu::MoYuScratchBuffer> vertex_buffer);
        InternalIndexBuffer createIndexBuffer(std::shared_ptr<MoYu::MoYuScratchBuffer> index_buffer);
    };
} // namespace MoYu
