#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/function/framework/object/object_id_allocator.h"
#include "runtime/function/render/rhi/hlsl_data_types.h"

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#include <glm/glm.hpp>

namespace MoYu
{
    //************************************************************
    // InputLayout类型相关
    //************************************************************

    enum InputDefinition
    {
        POSITION = 0,
        NORMAL   = 1 << 0,
        TANGENT  = 1 << 1,
        TEXCOORD = 1 << 2,
        INDICES  = 1 << 3,
        WEIGHTS  = 1 << 4
    };
    DEFINE_MOYU_ENUM_FLAG_OPERATORS(InputDefinition)

    // Vertex struct holding position information.
    struct D3D12MeshVertexPosition
    {
        glm::vec3 position;

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition = InputDefinition::POSITION;

    private:
        static constexpr unsigned int         InputElementCount = 1;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    // Vertex struct holding position and color information.
    struct D3D12MeshVertexPositionTexture
    {
        glm::vec3 position;
        glm::vec2 texcoord;

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition = InputDefinition::POSITION | InputDefinition::TEXCOORD;

    private:
        static constexpr unsigned int         InputElementCount = 2;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    // Vertex struct holding position and texture mapping information.

    struct D3D12MeshVertexPositionNormalTangentTexture
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec4 tangent;
        glm::vec2 texcoord;

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition =
            InputDefinition::POSITION | InputDefinition::NORMAL | InputDefinition::TANGENT | InputDefinition::TEXCOORD;

    private:
        static constexpr unsigned int         InputElementCount = 4;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    struct D3D12MeshVertexPositionNormalTangentTextureJointBinding
    {
        glm::vec3  position;
        glm::vec3  normal;
        glm::vec4  tangent;
        glm::vec2  texcoord;
        glm::ivec4 indices;
        glm::vec4  weights;

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition = InputDefinition::POSITION | InputDefinition::NORMAL |
                                                              InputDefinition::TANGENT | InputDefinition::TEXCOORD |
                                                              InputDefinition::INDICES | InputDefinition::WEIGHTS;

    private:
        static constexpr unsigned int         InputElementCount = 6;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    //************************************************************
    // 场景对应到的渲染对象
    //************************************************************

    struct SceneCommonIdentifier
    {
        GObjectID    m_object_id {K_Invalid_Object_Id};
        GComponentID m_component_id {K_Invalid_Component_Id};
    };

    struct InternalMesh
    {
        bool enable_vertex_blending;
        int mesh_vertex_count;
        int mesh_index_count;
        std::shared_ptr<RHI::D3D12Buffer> p_mesh_vertex_buffer;
        std::shared_ptr<RHI::D3D12Buffer> p_mesh_index_buffer;
    };

    struct InternalPBRMaterial
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

    struct InternalMeshRenderer
    {
        SceneCommonIdentifier m_identifier;

        bool enable_vertex_blending = false;

        Matrix4x4 model_matrix;
        Matrix4x4 model_matrix_inverse;
        AxisAlignedBox m_bounding_box;

        std::shared_ptr<InternalMesh>        ref_mesh;
        std::shared_ptr<InternalPBRMaterial> ref_material;
    };

    struct BaseAmbientLight
    {
        Color m_color;
    };

    struct BaseDirectionLight
    {
        Color   m_color;
        float   m_intensity {1.0f};
        Vector3 m_direction;
        bool    m_shadowmap {false};
        Vector2 m_shadow_bounds;
        float   m_shadow_near_plane {0.1f};
        float   m_shadow_far_plane {500.0f};
        Vector2 m_shadowmap_size {512, 512};
    };

    struct BasePointLight
    {
        Color m_color;
        float m_intensity {1.0f};
        float m_radius;
    };

    struct BaseSpotLight
    {
        Color m_color;
        float m_intensity;
        float m_radius;
        float m_inner_radians;
        float m_outer_radians;

        bool    m_shadowmap {false};
        Vector2 m_shadow_bounds;
        float   m_shadow_near_plane {0.1f};
        float   m_shadow_far_plane {500.0f};
        Vector2 m_shadowmap_size {512, 512};
    };

    struct InternalAmbientLight : public BaseAmbientLight
    {
        SceneCommonIdentifier m_identifier;

        Vector3 m_position;
    };

    struct InternalDirectionLight : public BaseDirectionLight
    {
        SceneCommonIdentifier m_identifier;

        Vector3   m_position;
        Matrix4x4 m_shadow_view_proj_mat;
    };

    struct InternalPointLight : public BasePointLight
    {
        SceneCommonIdentifier m_identifier;

        Vector3 m_position;
    };

    struct InternalSpotLight : public BaseSpotLight
    {
        SceneCommonIdentifier m_identifier;

        Vector3   m_position;
        Vector3   m_direction;
        Matrix4x4 m_shadow_view_proj_mat;
    };

    //========================================================================
    // Render Desc
    //========================================================================

    struct SceneMesh
    {
        std::string m_mesh_file;
    };

    struct SceneImage
    {
        bool m_is_srgb;
        bool m_auto_mips;
        int m_mip_levels;
        std::string m_image_file;
    };

    struct ScenePBRMaterial
    {
        bool m_blend {false};
        bool m_double_sided {false};

        Vector4 m_base_color_factor {1.0f, 1.0f, 1.0f, 1.0f};
        float   m_metallic_factor {1.0f};
        float   m_roughness_factor {1.0f};
        float   m_normal_scale {1.0f};
        float   m_occlusion_strength {1.0f};
        Vector3 m_emissive_factor {0.0f, 0.0f, 0.0f};

        SceneImage m_base_color_texture_file;
        SceneImage m_metallic_roughness_texture_file;
        SceneImage m_normal_texture_file;
        SceneImage m_occlusion_texture_file;
        SceneImage m_emissive_texture_file;
    };

    struct SceneMeshRenderer
    {
        SceneCommonIdentifier m_identifier;
        SceneMesh m_scene_mesh;
        ScenePBRMaterial m_material;

        Matrix4x4 model_matrix;
        Matrix4x4 model_matrix_inverse;
        AxisAlignedBox m_bounding_box;
    };

    struct SceneAmbientLight : public BaseAmbientLight
    {
        SceneCommonIdentifier m_identifier;
    };

    struct SceneDirectionLight : public BaseDirectionLight
    {
        SceneCommonIdentifier m_identifier;
    };

    struct ScenePointLight : public BasePointLight
    {
        SceneCommonIdentifier m_identifier;
    };

    struct SceneSpotLight : public BaseSpotLight
    {
        SceneCommonIdentifier m_identifier;
    };

    struct SceneTransform
    {
        SceneCommonIdentifier m_identifier;

        Matrix4x4  m_transform_matrix {Matrix4x4::Identity};
        Vector3    m_position {Vector3::Zero};
        Quaternion m_rotation {Quaternion::Identity};
        Vector3    m_scale {Vector3::One};
    };

    struct GameObjectComponentDesc
    {
        SceneCommonIdentifier m_identifier;

        SceneTransform      m_transform_desc;
        SceneMeshRenderer   m_mesh_renderer_desc;
        SceneAmbientLight   m_ambeint_light_desc;
        SceneDirectionLight m_direction_light_desc;
        ScenePointLight     m_point_light_desc;
        SceneSpotLight      m_spot_light_desc;
    };

    class GameObjectDesc
    {
    public:
        GameObjectDesc() : m_go_id(0) {}
        GameObjectDesc(size_t go_id, const std::vector<GameObjectComponentDesc>& parts) :
            m_go_id(go_id), m_object_components(parts)
        {}

        GObjectID getId() const { return m_go_id; }
        const std::vector<GameObjectComponentDesc>& getObjectParts() const { return m_object_components; }

    private:
        GObjectID m_go_id {K_Invalid_Object_Id};
        std::vector<GameObjectComponentDesc> m_object_components;
    };

    //========================================================================
    // 准备上传的Buffer和Texture数据
    //========================================================================

    class BufferData
    {
    public:
        size_t m_size {0};
        void*  m_data {nullptr};

        BufferData() = delete;
        BufferData(size_t size)
        {
            m_size = size;
            m_data = malloc(size);
        }
        BufferData(void* data, size_t size)
        {
            m_size = size;
            m_data = malloc(size);
            memcpy(m_data, data, size);
        }
        ~BufferData()
        {
            if (m_data)
            {
                free(m_data);
            }
        }
        bool isValid() const { return m_data != nullptr; }
    };

    struct TextureData
    {
    public:
        uint32_t m_width {1};
        uint32_t m_height {1};
        uint32_t m_depth {0};
        uint32_t m_mip_levels {0};
        uint32_t m_array_layers {0};
        uint32_t m_channels {4};
        bool     m_is_hdr {false};
        void*    m_pixels {nullptr};

        TextureData() = default;
        ~TextureData()
        {
            if (m_pixels)
            {
                free(m_pixels);
            }
        }
        bool isValid() const { return m_pixels != nullptr; }
    };

    struct SkeletonDefinition
    {
        int m_index0 {0};
        int m_index1 {0};
        int m_index2 {0};
        int m_index3 {0};

        float m_weight0 {0.f};
        float m_weight1 {0.f};
        float m_weight2 {0.f};
        float m_weight3 {0.f};
    };

    struct StaticMeshData
    {
        InputDefinition m_InputElementDefinition;
        std::shared_ptr<BufferData> m_vertex_buffer;
        std::shared_ptr<BufferData> m_index_buffer;
    };

    struct RenderMeshData
    {
        StaticMeshData m_static_mesh_data;
        std::shared_ptr<BufferData> m_skeleton_binding_buffer;
    };

} // namespace MoYu
