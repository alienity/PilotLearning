#pragma once

#include "runtime/function/render/render_resource_base.h"
#include "runtime/function/render/render_type.h"

#include "runtime/function/render/render_common.h"

#include <array>
#include <cstdint>
#include <map>
#include <vector>

namespace Pilot
{
    class RenderCamera;

    struct IBLResource
    {
        RHI::D3D12Texture            _brdfLUT_texture_image;
        RHI::D3D12ShaderResourceView _brdfLUT_texture_image_view;

        RHI::D3D12Texture            _irradiance_texture_image;
        RHI::D3D12ShaderResourceView _irradiance_texture_image_view;

        RHI::D3D12Texture            _specular_texture_image;
        RHI::D3D12ShaderResourceView _specular_texture_image_view;
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
        RHI::D3D12Texture            _color_grading_LUT_texture_image;
        RHI::D3D12ShaderResourceView _color_grading_LUT_texture_image_view;
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

    class RenderResource : public RenderResourceBase
    {
    public:
        virtual void uploadGlobalRenderResource(LevelResourceDesc level_resource_desc) override final;

        virtual void uploadGameObjectRenderResource(RenderEntity       render_entity,
                                                    RenderMeshData     mesh_data,
                                                    RenderMaterialData material_data) override final;

        virtual void uploadGameObjectRenderResource(RenderEntity   render_entity,
                                                    RenderMeshData mesh_data) override final;

        virtual void uploadGameObjectRenderResource(RenderEntity       render_entity,
                                                    RenderMaterialData material_data) override final;

        virtual void updatePerFrameBuffer(std::shared_ptr<RenderScene>  render_scene,
                                          std::shared_ptr<RenderCamera> camera) override final;

        D3D12Mesh& getEntityMesh(RenderEntity entity);

        D3D12PBRMaterial& getEntityMaterial(RenderEntity entity);

        // global rendering resource, include IBL data, global storage buffer
        GlobalRenderResource m_global_render_resource;

        // storage buffer objects
        MeshPerframeStorageBufferObject                 m_mesh_perframe_storage_buffer_object;
        MeshPointLightShadowPerframeStorageBufferObject m_mesh_point_light_shadow_perframe_storage_buffer_object;
        MeshDirectionalLightShadowPerframeStorageBufferObject
                                                       m_mesh_directional_light_shadow_perframe_storage_buffer_object;
        AxisStorageBufferObject                        m_axis_storage_buffer_object;
        MeshInefficientPickPerframeStorageBufferObject m_mesh_inefficient_pick_perframe_storage_buffer_object;
        ParticleBillboardPerframeStorageBufferObject   m_particlebillboard_perframe_storage_buffer_object;

        // cached mesh and material
        std::map<size_t, D3D12Mesh>        m_d3d12_meshes;
        std::map<size_t, D3D12PBRMaterial> m_d3d12_pbr_materials;

    protected:
        void createDynamicBuffer(void*             buffer_data,
                                 uint32_t          buffer_size,
                                 uint32_t          buffer_stride,
                                 RHI::D3D12Buffer& dynamicBuffer);
        void createDynamicBuffer(std::shared_ptr<BufferData>& buffer_data,
                                 RHI::D3D12Buffer&            dynamicBuffer);
        
        void createStaticBuffer(void*             buffer_data,
                                uint32_t          buffer_size,
                                uint32_t          buffer_stride,
                                RHI::D3D12Buffer& staticBuffer,
                                bool              batch = false);
        void createStaticBuffer(std::shared_ptr<BufferData>& buffer_data,
                                RHI::D3D12Buffer&            staticBuffer,
                                bool                         batch = false);

        void createTex2D(uint32_t                      width,
                         uint32_t                      height,
                         void*                         pixels,
                         DXGI_FORMAT                   format,
                         bool                          is_srgb,
                         RHI::D3D12Texture&            tex2d,
                         RHI::D3D12ShaderResourceView& tex2d_view,
                         bool                          batch = false);
        void createTex2D(std::shared_ptr<TextureData>& tex2d_data,
                         RHI::D3D12Texture&            tex2d,
                         RHI::D3D12ShaderResourceView& tex2d_view,
                         bool                          batch = false);

        void createCubeMap(std::array<std::shared_ptr<TextureData>, 6>& cube_maps,
                           RHI::D3D12Texture&                           cube_tex,
                           RHI::D3D12ShaderResourceView&                cube_tex_view,
                           bool                                         batch = false);

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
                                uint32_t                                      index_buffer_size,
                                uint16_t*                                     index_buffer_data,
                                D3D12Mesh&                                    now_mesh);
        void updateIndexBuffer(uint32_t index_buffer_size, void* index_buffer_data, D3D12Mesh& now_mesh);
    };
} // namespace Pilot
