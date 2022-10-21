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
        // Factors
        bool m_blend;
        bool m_double_sided;

        Vector4 m_base_color_factor;
        float   m_metallic_factor;
        float   m_roughness_factor;
        float   m_normal_scale;
        float   m_occlusion_strength;
        Vector3 m_emissive_factor;

        // Textures

        std::shared_ptr<RHI::D3D12Texture>            base_color_texture_image;
        std::shared_ptr<RHI::D3D12ShaderResourceView> base_color_image_view;

        std::shared_ptr<RHI::D3D12Texture>            metallic_roughness_texture_image;
        std::shared_ptr<RHI::D3D12ShaderResourceView> metallic_roughness_image_view;

        std::shared_ptr<RHI::D3D12Texture>            normal_texture_image;
        std::shared_ptr<RHI::D3D12ShaderResourceView> normal_image_view;

        std::shared_ptr<RHI::D3D12Texture>            occlusion_texture_image;
        std::shared_ptr<RHI::D3D12ShaderResourceView> occlusion_image_view;

        std::shared_ptr<RHI::D3D12Texture>            emissive_texture_image;
        std::shared_ptr<RHI::D3D12ShaderResourceView> emissive_image_view;

        std::shared_ptr<RHI::D3D12Buffer>             material_uniform_buffer;
        std::shared_ptr<RHI::D3D12ShaderResourceView> material_uniform_buffer_view;
        
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
        MeshBoundingBox    bounding_box;
        bool               enable_vertex_blending = false;
    };

    struct RenderParticleBillboardNode
    {
        std::vector<Vector4> positions;
    };

} // namespace Pilot
