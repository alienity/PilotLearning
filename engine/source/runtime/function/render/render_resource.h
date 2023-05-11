#pragma once

#include "runtime/function/render/render_resource_base.h"
#include "runtime/function/render/render_type.h"

#include "runtime/function/render/render_common.h"

#include <array>
#include <cstdint>
#include <map>
#include <vector>
#include <tuple>

namespace Pilot
{
    class RenderCamera;

    struct IBLResource
    {
        std::shared_ptr<RHI::D3D12Texture> _brdfLUT_texture_image;
        std::shared_ptr<RHI::D3D12Texture> _irradiance_texture_image;
        std::shared_ptr<RHI::D3D12Texture> _specular_texture_image;
    };

    struct IBLResourceData
    {
        void*                _brdfLUT_texture_image_pixels;
        uint32_t             _brdfLUT_texture_image_width;
        uint32_t             _brdfLUT_texture_image_height;
        DXGI_FORMAT          _brdfLUT_texture_image_format;
        std::array<void*, 6> _irradiance_texture_image_pixels;
        uint32_t             _irradiance_texture_image_width;
        uint32_t             _irradiance_texture_image_height;
        DXGI_FORMAT          _irradiance_texture_image_format;
        std::array<void*, 6> _specular_texture_image_pixels;
        uint32_t             _specular_texture_image_width;
        uint32_t             _specular_texture_image_height;
        DXGI_FORMAT          _specular_texture_image_format;
    };

    struct ColorGradingResource
    {
        std::shared_ptr<RHI::D3D12Texture> _color_grading_LUT_texture_image;
    };

    struct ColorGradingResourceData
    {
        void*              _color_grading_LUT_texture_image_pixels;
        uint32_t           _color_grading_LUT_texture_image_width;
        uint32_t           _color_grading_LUT_texture_image_height;
        DXGI_FORMAT        _color_grading_LUT_texture_image_format;
    };

    struct GlobalRenderResource
    {
        IBLResource          _ibl_resource;
        ColorGradingResource _color_grading_resource;
    };

    //struct DefaultResource
    //{
    //    std::shared_ptr<RHI::D3D12Texture> _white_texture2d_image;
    //    std::shared_ptr<RHI::D3D12Texture> _black_texture2d_image;
    //};

    class RenderResource : public RenderResourceBase
    {
    public:
        RenderResource() = default;

        virtual void uploadGlobalRenderResource(LevelResourceDesc level_resource_desc) override final;
        virtual void uploadGameObjectRenderResource(RenderEntity render_entity, RenderMeshData mesh_data, RenderMaterialData material_data) override final;
        virtual void uploadGameObjectRenderResource(RenderEntity render_entity, RenderMeshData mesh_data) override final;
        virtual void uploadGameObjectRenderResource(RenderEntity render_entity, RenderMaterialData material_data) override final;

        virtual void updatePerFrameBuffer(std::shared_ptr<RenderScene>  render_scene, std::shared_ptr<RenderCamera> camera) override final;

        D3D12Mesh& getEntityMesh(RenderEntity entity);

        D3D12PBRMaterial& getEntityMaterial(RenderEntity entity);

        std::size_t hashTexture2D(uint32_t width, uint32_t height, uint32_t mip_levels, void* pixels_ptr, DXGI_FORMAT format);

        // global rendering resource, include IBL data, global storage buffer
        GlobalRenderResource m_global_render_resource;

        //// default texture resource
        //DefaultResource m_default_resource;

        // bindless objects
        HLSL::MeshPerframeStorageBufferObject                 m_mesh_perframe_storage_buffer_object;
        HLSL::MeshPointLightShadowPerframeStorageBufferObject m_mesh_point_light_shadow_perframe_storage_buffer_object;
        HLSL::MeshSpotLightShadowPerframeStorageBufferObject  m_mesh_spot_light_shadow_perframe_storage_buffer_object;
        HLSL::MeshDirectionalLightShadowPerframeStorageBufferObject m_mesh_directional_light_shadow_perframe_storage_buffer_object;
        HLSL::MeshInstance      m_all_mesh_buffer_object;

        // cached mesh and material
        std::map<size_t, D3D12Mesh>        m_d3d12_meshes;
        std::map<size_t, D3D12PBRMaterial> m_d3d12_pbr_materials;

    protected:
        std::shared_ptr<RHI::D3D12Buffer> createDynamicBuffer(void* buffer_data, uint32_t buffer_size, uint32_t buffer_stride);
        std::shared_ptr<RHI::D3D12Buffer> createDynamicBuffer(std::shared_ptr<BufferData>& buffer_data);
        
        std::shared_ptr<RHI::D3D12Buffer> createStaticBuffer(void* buffer_data, uint32_t buffer_size, uint32_t buffer_stride, bool raw, bool batch = false);
        std::shared_ptr<RHI::D3D12Buffer> createStaticBuffer(std::shared_ptr<BufferData>& buffer_data, bool raw, bool batch = false);

        std::shared_ptr<RHI::D3D12Texture> createTex2D(uint32_t width, uint32_t height, void* pixels, DXGI_FORMAT format, bool is_srgb, bool genMips = false, bool batch = false);
        std::shared_ptr<RHI::D3D12Texture> createTex2D(std::shared_ptr<TextureData>& tex2d_data, bool genMips = false, bool batch = false);

        std::shared_ptr<RHI::D3D12Texture> createCubeMap(std::array<std::shared_ptr<TextureData>, 6>& cube_maps, bool genMips = false, bool batch = false);

    private:

        D3D12Mesh&        getOrCreateD3D12Mesh(RenderEntity entity, RenderMeshData mesh_data);
        D3D12PBRMaterial& getOrCreateD3D12Material(RenderEntity entity, RenderMaterialData material_data);

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
    };
} // namespace Pilot
