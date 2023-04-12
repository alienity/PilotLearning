#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/function/render/render_type.h"
#include "runtime/function/render/rhi/d3d12/d3d12_core.h"
#include "runtime/function/render/rhi/hlsl_data_types.h"

namespace Pilot
{
    // mesh
    struct D3D12Mesh
    {
        bool enable_vertex_blending;
        uint32_t mesh_vertex_count;
        uint32_t mesh_index_count;
        std::shared_ptr<RHI::D3D12Buffer> p_mesh_vertex_buffer;
        std::shared_ptr<RHI::D3D12Buffer> p_mesh_index_buffer;
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
        std::shared_ptr<RHI::D3D12Texture> base_color_texture_image;
        std::shared_ptr<RHI::D3D12Texture> metallic_roughness_texture_image;
        std::shared_ptr<RHI::D3D12Texture> normal_texture_image;
        std::shared_ptr<RHI::D3D12Texture> occlusion_texture_image;
        std::shared_ptr<RHI::D3D12Texture> emissive_texture_image;
        std::shared_ptr<RHI::D3D12Buffer>  material_uniform_buffer;
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

    // AmbientLight
    struct AmbientLightNode
    {
        glm::vec3 m_position;
        glm::vec4 m_color;
    };

    // DirectionalLight
    struct DirectionLightNode
    {
        glm::vec3   m_position;
        glm::vec4   m_color;
        float       m_intensity {1.0f};
        glm::vec3   m_direction;
        bool        m_shadowmap {false};
        glm::vec2   m_shadow_bounds;
        float       m_shadow_near_plane {0.1f};
        float       m_shadow_far_plane {500.0f};
        glm::vec2   m_shadowmap_size {512, 512};
        glm::mat4x4 m_shadow_view_proj_mat;
    };

    // PointLight
    struct PointLightNode
    {
        glm::vec3 m_position;
        glm::vec4 m_color;
        float     m_intensity {1.0f};
        float     m_radius;
    };

    // SpotLight
    struct SpotLightNode
    {
        glm::vec3   m_position;
        glm::vec3   m_direction;
        glm::vec4   m_color;
        float       m_intensity;
        float       m_radius;
        float       m_inner_radians;
        float       m_outer_radians;
        bool        m_shadowmap {false};
        glm::vec2   m_shadow_bounds;
        float       m_shadow_near_plane {0.1f};
        float       m_shadow_far_plane {500.0f};
        glm::vec2   m_shadowmap_size {512, 512};
        glm::mat4x4 m_shadow_view_proj_mat;
    };

    // inputlayout 
    struct MeshInputLayout
    {
        // Vertex struct holding position information.
        struct D3D12MeshVertexPosition
        {
            glm::vec3 position;

            static RHI::D3D12InputLayout InputLayout;

        private:
            #define InputElementCount 1
            static D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
        };

        // Vertex struct holding position and color information.
        struct D3D12MeshVertexPositionTexture
        {
            glm::vec3 position;
            glm::vec2 texcoord;

            static RHI::D3D12InputLayout InputLayout;

        private:
            #define InputElementCount 2
            static D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
        };

        // Vertex struct holding position and texture mapping information.

        struct D3D12MeshVertexPositionNormalTangentTexture
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec4 tangent;
            glm::vec2 texcoord;

            static RHI::D3D12InputLayout InputLayout;

        private:
            #define InputElementCount 4
            static D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
        };

        struct D3D12MeshVertexPositionNormalTangentTextureJointBinding
        {
            glm::vec3  position;
            glm::vec3  normal;
            glm::vec4  tangent;
            glm::vec2  texcoord;
            glm::ivec4 indices;
            glm::vec4  weights;

            static RHI::D3D12InputLayout InputLayout;

        private:
            #define InputElementCount 6
            static D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
        };
    };

} // namespace Pilot
