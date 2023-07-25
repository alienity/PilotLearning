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

    enum CameraProjType
    {
        Perspective,
        Orthogonal
    };

    struct SceneCommonIdentifier
    {
        GObjectID    m_object_id {K_Invalid_Object_Id};
        GComponentID m_component_id {K_Invalid_Component_Id};

        bool operator==(const SceneCommonIdentifier& id) const
        {
            return m_object_id == id.m_object_id && m_component_id == id.m_component_id;
        }
        bool operator!=(const SceneCommonIdentifier& id) const
        {
            return m_object_id != id.m_object_id || m_component_id != id.m_component_id;
        }
    };

    extern SceneCommonIdentifier _UndefCommonIdentifier;
    #define UndefCommonIdentifier _UndefCommonIdentifier

    struct InternalVertexBuffer
    {
        InputDefinition input_element_definition;
        uint32_t vertex_count;
        std::shared_ptr<RHI::D3D12Buffer> vertex_buffer;
    };

    struct InternalIndexBuffer
    {
        uint32_t index_stride;
        uint32_t index_count;
        std::shared_ptr<RHI::D3D12Buffer> index_buffer;
    };

    struct InternalMesh
    {
        bool enable_vertex_blending;
        AxisAlignedBox axis_aligned_box;
        InternalIndexBuffer index_buffer;
        InternalVertexBuffer vertex_buffer;
    };

    struct InternalPBRMaterial
    {
        // Factors
        bool m_blend;
        bool m_double_sided;

        // 所有的值都设置在的uniformbuffer中
        /*
        Vector4 m_base_color_factor;
        float   m_metallic_factor;
        float   m_roughness_factor;
        float   m_normal_scale;
        float   m_occlusion_strength;
        Vector3 m_emissive_factor;
        */
        // Textures
        std::shared_ptr<RHI::D3D12Texture> base_color_texture_image;
        std::shared_ptr<RHI::D3D12Texture> metallic_roughness_texture_image;
        std::shared_ptr<RHI::D3D12Texture> normal_texture_image;
        std::shared_ptr<RHI::D3D12Texture> occlusion_texture_image;
        std::shared_ptr<RHI::D3D12Texture> emissive_texture_image;
        std::shared_ptr<RHI::D3D12Buffer>  material_uniform_buffer;
    };

    struct InternalMaterial
    {
        std::string m_shader_name;
        InternalPBRMaterial m_intenral_pbr_mat;
    };

    struct InternalMeshRenderer
    {
        SceneCommonIdentifier m_identifier;

        bool enable_vertex_blending = false;

        Matrix4x4 model_matrix;
        Matrix4x4 model_matrix_inverse;

        InternalMesh ref_mesh;
        InternalMaterial ref_material;
    };

    struct BaseAmbientLight
    {
        Color m_color;
    };

    struct BaseDirectionLight
    {
        Color   m_color;
        float   m_intensity {1.0f};

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
        Vector3   m_direction;
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

    struct InternalCamera
    {
        SceneCommonIdentifier m_identifier;

        CameraProjType m_projType;

        Vector3    m_position;
        Quaternion m_rotation;

        float m_width;
        float m_height;
        float m_znear;
        float m_zfar;
        float m_aspect;
        float m_fovY;

        Matrix4x4 m_ViewMatrix;
        Matrix4x4 m_ViewMatrixInv;

        Matrix4x4 m_ProjMatrix;
        Matrix4x4 m_ProjMatrixInv;

        Matrix4x4 m_ViewProjMatrix;
        Matrix4x4 m_ViewProjMatrixInv;
    };

    struct SkyboxConfigs
    {
        std::shared_ptr<RHI::D3D12Texture> m_skybox_irradiance_map;
        std::shared_ptr<RHI::D3D12Texture> m_skybox_specular_map;
    };

    //========================================================================
    // Render Desc
    //========================================================================

    enum ComponentType
    {
        C_Transform    = 0,
        C_Light        = 1,
        C_Camera       = 1 << 1,
        C_Mesh         = 1 << 2,
        C_Material     = 1 << 3,
        C_MeshRenderer = C_Mesh | C_Material
    };
    DEFINE_MOYU_ENUM_FLAG_OPERATORS(ComponentType)

    struct SceneMesh
    {
        bool m_is_mesh_data {false};
        std::string m_sub_mesh_file {""};
        std::string m_mesh_data_path {""};
    };

    inline bool operator==(const SceneMesh& lhs, const SceneMesh& rhs)
    {
        return lhs.m_is_mesh_data == rhs.m_is_mesh_data && lhs.m_sub_mesh_file == rhs.m_sub_mesh_file &&
               lhs.m_mesh_data_path == rhs.m_mesh_data_path;
    }

    struct SceneImage
    {
        bool m_is_srgb {false};
        bool m_auto_mips {false};
        int  m_mip_levels {1};
        std::string m_image_file {""};
    };

    inline bool operator==(const SceneImage& lhs, const SceneImage& rhs)
    {
        return lhs.m_is_srgb == rhs.m_is_srgb && lhs.m_auto_mips == rhs.m_auto_mips &&
               lhs.m_mip_levels == rhs.m_mip_levels && lhs.m_image_file == rhs.m_image_file;
    }

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

        SceneImage m_base_color_texture_file {};
        SceneImage m_metallic_roughness_texture_file {};
        SceneImage m_normal_texture_file {};
        SceneImage m_occlusion_texture_file {};
        SceneImage m_emissive_texture_file {};
    };

    inline bool operator==(const ScenePBRMaterial& lhs, const ScenePBRMaterial& rhs)
    {
#define CompareVal(Val) lhs.Val == rhs.Val

        return CompareVal(m_blend) && CompareVal(m_double_sided) && CompareVal(m_base_color_factor) &&
               CompareVal(m_metallic_factor) && CompareVal(m_roughness_factor) && CompareVal(m_normal_scale) &&
               CompareVal(m_occlusion_strength) && CompareVal(m_emissive_factor) &&
               CompareVal(m_base_color_texture_file) && CompareVal(m_metallic_roughness_texture_file) &&
               CompareVal(m_normal_texture_file) && CompareVal(m_occlusion_texture_file) &&
               CompareVal(m_emissive_texture_file);
    }

    struct MaterialRes;

    // MaterialRes是可以存所有材质的对象，而ScenePBRMaterial是PBR材质属性
    MaterialRes ToMaterialRes(const ScenePBRMaterial& pbrMaterial, const std::string shaderName);
    ScenePBRMaterial ToPBRMaterial(const MaterialRes& materialRes);

    extern ScenePBRMaterial _DefaultScenePBRMaterial;

    struct SceneMaterial
    {
        std::string m_shader_name;
        ScenePBRMaterial m_mat_data;
    };

    struct SceneMeshRenderer
    {
        static const ComponentType m_component_type {ComponentType::C_MeshRenderer};

        SceneCommonIdentifier m_identifier;

        SceneMesh m_scene_mesh;
        SceneMaterial m_material;
    };

    enum LightType
    {
        AmbientLight,
        DirectionLight,
        PointLight,
        SpotLight
    };

    struct SceneLight
    {
        static const ComponentType m_component_type {ComponentType::C_Light};

        SceneCommonIdentifier m_identifier;

        LightType m_light_type;

        BaseAmbientLight   ambient_light;
        BaseDirectionLight direction_light;
        BasePointLight     point_light;
        BaseSpotLight      spot_light;
    };

    struct SceneCamera
    {
        static const ComponentType m_component_type {ComponentType::C_Camera};
        
        SceneCommonIdentifier m_identifier;

        CameraProjType m_projType;

        float m_width;
        float m_height;
        float m_znear;
        float m_zfar;
        float m_fovY;
    };

    struct SceneTransform
    {
        static const ComponentType m_component_type {ComponentType::C_Transform};

        SceneCommonIdentifier m_identifier;

        Matrix4x4  m_transform_matrix {Matrix4x4::Identity};
        Vector3    m_position {Vector3::Zero};
        Quaternion m_rotation {Quaternion::Identity};
        Vector3    m_scale {Vector3::One};
    };

    struct GameObjectComponentDesc
    {
        ComponentType m_component_type {ComponentType::C_Transform};

        //SceneCommonIdentifier m_identifier {};

        SceneTransform    m_transform_desc {};
        SceneMeshRenderer m_mesh_renderer_desc {};
        SceneLight        m_scene_light {};
        SceneCamera       m_scene_camera {};
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
        bool     m_need_free {true};
        void*    m_pixels {nullptr};

        TextureData() = default;
        ~TextureData()
        {
            if (m_pixels && m_need_free)
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
        AxisAlignedBox m_axis_aligned_box;
        StaticMeshData m_static_mesh_data;
        std::shared_ptr<BufferData> m_skeleton_binding_buffer;
    };

} // namespace MoYu
