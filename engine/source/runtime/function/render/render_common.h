#pragma once

#include "runtime/core/math/moyu_math.h"

#include "runtime/function/render/render_type.h"

#include "runtime/function/render/rhi/hlsl_data_types.h"

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#include <glm/glm.hpp>

namespace Pilot
{
    // mesh
    struct D3D12Mesh
    {
        bool enable_vertex_blending;

        uint32_t mesh_vertex_count;

        RHI::D3D12Buffer mesh_vertex_buffer;

        uint32_t mesh_index_count;

        RHI::D3D12Buffer mesh_index_buffer;
    };

    // material
    struct D3D12PBRMaterial
    {
        RHI::D3D12Texture            base_color_texture_image;
        RHI::D3D12ShaderResourceView base_color_image_view;

        RHI::D3D12Texture            metallic_roughness_texture_image;
        RHI::D3D12ShaderResourceView metallic_roughness_image_view;

        RHI::D3D12Texture            normal_texture_image;
        RHI::D3D12ShaderResourceView normal_image_view;

        RHI::D3D12Texture            occlusion_texture_image;
        RHI::D3D12ShaderResourceView occlusion_image_view;

        RHI::D3D12Texture            emissive_texture_image;
        RHI::D3D12ShaderResourceView emissive_image_view;

        RHI::D3D12Buffer             material_uniform_buffer;
        RHI::D3D12ShaderResourceView material_uniform_buffer_view;
        
        //VkDescriptorSet material_descriptor_set;
    };

    // boundingbox
    struct MeshBoundingBox
    {
        glm::vec3 center;
        glm::vec3 extent;
    };

    // nodes
    struct RenderMeshNode
    {
        glm::mat4          model_matrix;
        glm::mat4          model_matrix_inverse;
        D3D12Mesh*         ref_mesh     = nullptr;
        D3D12PBRMaterial*  ref_material = nullptr;
        uint32_t           node_id;
        MeshBoundingBox    bounding_box;
        bool               enable_vertex_blending = false;
    };

    struct RenderAxisNode
    {
        glm::mat4  model_matrix {glm::mat4(1.0f)};
        D3D12Mesh* ref_mesh {nullptr};
        uint32_t   node_id;
        bool       enable_vertex_blending {false};
    };

    struct RenderParticleBillboardNode
    {
        std::vector<Vector4> positions;
    };

    struct TextureDataToUpdate
    {
        void*              base_color_image_pixels;
        uint32_t           base_color_image_width;
        uint32_t           base_color_image_height;
        DXGI_FORMAT        base_color_image_format;
        void*              metallic_roughness_image_pixels;
        uint32_t           metallic_roughness_image_width;
        uint32_t           metallic_roughness_image_height;
        DXGI_FORMAT        metallic_roughness_image_format;
        void*              normal_roughness_image_pixels;
        uint32_t           normal_roughness_image_width;
        uint32_t           normal_roughness_image_height;
        DXGI_FORMAT        normal_roughness_image_format;
        void*              occlusion_image_pixels;
        uint32_t           occlusion_image_width;
        uint32_t           occlusion_image_height;
        DXGI_FORMAT        occlusion_image_format;
        void*              emissive_image_pixels;
        uint32_t           emissive_image_width;
        uint32_t           emissive_image_height;
        DXGI_FORMAT        emissive_image_format;
        D3D12PBRMaterial* now_material;
    };
} // namespace Pilot
