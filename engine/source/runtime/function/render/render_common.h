#pragma once

#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/framework/object/object_id_allocator.h"
#include "runtime/function/render/rhi/hlsl_data_types.h"

#include "DirectXTex.h"
#include "DirectXTex.inl"

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#include <glm/glm.hpp>

// https://developer.nvidia.com/nvidia-texture-tools-exporter

namespace MoYu
{

    //========================================================================
    // ScratchBuffer ScratchImage
    //========================================================================

    typedef DirectX::Blob         MoYuScratchBuffer;
    typedef DirectX::TexMetadata  MoYuTexMetadata;
    typedef DirectX::Image        MoYuImage;
    typedef DirectX::ScratchImage MoYuScratchImage;

    //************************************************************
    // InputLayout
    //************************************************************

    enum InputDefinition : uint64_t
    {
        POSITION  = 0,
        NORMAL    = 1 << 0,
        TANGENT   = 1 << 1,
        TEXCOORD  = 1 << 2,
        TEXCOORD0 = 1 << 2,
        TEXCOORD1 = 1 << 3,
        TEXCOORD2 = 1 << 4,
        TEXCOORD3 = 1 << 5,
        INDICES   = 1 << 6,
        WEIGHTS   = 1 << 7,
        COLOR     = 1 << 8
    };
    DEFINE_MOYU_ENUM_FLAG_OPERATORS(InputDefinition)

    // Vertex struct holding position information.
    struct D3D12MeshVertexPosition
    {
        glm::float3 position;

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition = InputDefinition::POSITION;

    private:
        static constexpr unsigned int         InputElementCount = 1;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    // Vertex struct holding position and color information.
    struct D3D12MeshVertexPositionTexture
    {
        glm::float3 position;
        glm::float2 texcoord;

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition = InputDefinition::POSITION | InputDefinition::TEXCOORD;

    private:
        static constexpr unsigned int         InputElementCount = 2;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    // Vertex struct holding position and texture mapping information.

    struct D3D12MeshVertexPositionNormalTangentTexture
    {
        glm::float3 position;
        glm::float3 normal;
        glm::float4 tangent;
        glm::float2 texcoord;

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition =
            InputDefinition::POSITION | InputDefinition::NORMAL | InputDefinition::TANGENT | InputDefinition::TEXCOORD;

    private:
        static constexpr unsigned int         InputElementCount = 4;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    struct D3D12MeshVertexPositionNormalTangentTextureJointBinding
    {
        glm::float3 position;
        glm::float3 normal;
        glm::float4 tangent;
        glm::float2 texcoord;
        glm::uvec4  indices;
        glm::float4 weights;

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition = InputDefinition::POSITION | InputDefinition::NORMAL |
                                                              InputDefinition::TANGENT | InputDefinition::TEXCOORD |
                                                              InputDefinition::INDICES | InputDefinition::WEIGHTS;

    private:
        static constexpr unsigned int         InputElementCount = 6;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    struct D3D12MeshVertexStandard
    {
        glm::float3 position;
        glm::float3 normal;
        glm::float4 tangent;
        glm::float2 uv0;
        glm::float4 color;

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition = 
            InputDefinition::POSITION | InputDefinition::NORMAL |
            InputDefinition::TANGENT | InputDefinition::TEXCOORD0 | InputDefinition::COLOR;

    private:
        static constexpr unsigned int         InputElementCount = 5;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    //************************************************************
    // TerrainPatchCluster
    //************************************************************
    struct D3D12TerrainPatch
    {
        glm::float3 position;
        glm::float3 normal;
        glm::float4 tangent;
        glm::float2 texcoord0;
        glm::float4 color;

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition = InputDefinition::POSITION | InputDefinition::NORMAL |
                                                              InputDefinition::TANGENT | InputDefinition::TEXCOORD |
                                                              InputDefinition::COLOR;
    private:
        static constexpr unsigned int         InputElementCount = 5;
        static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    //************************************************************
    // Scene
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
        AABB axis_aligned_box;
        InternalIndexBuffer index_buffer;
        InternalVertexBuffer vertex_buffer;
    };

    struct InternalStandardLightMaterial
    {
#define InteralMaterialImageMacro(TexName) std::shared_ptr<RHI::D3D12Texture> TexName = nullptr;

        glm::float4 _BaseColor{ 1, 1, 1, 1 };
        InteralMaterialImageMacro(_BaseColorMap)

        InteralMaterialImageMacro(_MaskMap) // MaskMap is RGBA: Metallic, Ambient Occlusion (Optional), detail Mask (Optional), Smoothness
        float _Metallic{ 0 };
        float _Smoothness{ 0.5f };
        float _MetallicRemapMin{ 0.0f };
        float _MetallicRemapMax{ 1.0f };
        float _SmoothnessRemapMin{ 0.0f };
        float _SmoothnessRemapMax{ 1.0f };
        float _AlphaRemapMin{ 0.0f };
        float _AlphaRemapMax{ 1.0f };
        float _AORemapMin{ 0.0f };
        float _AORemapMax{ 1.0f };

        InteralMaterialImageMacro(_NormalMap)   // Tangent space normal map
        InteralMaterialImageMacro(_NormalMapOS) // Object space normal map - no good default value
        float _NormalScale{ 1.0f };

        InteralMaterialImageMacro(_BentNormalMap)
        InteralMaterialImageMacro(_BentNormalMapOS)

        InteralMaterialImageMacro(_HeightMap)
        float _HeightAmplitude{ 0.02f };
        float _HeightCenter{ 0.5f };

        InteralMaterialImageMacro(_DetailMap)
        float _DetailAlbedoScale{ 1.0f };
        float _DetailNormalScale{ 1.0f };
        float _DetailSmoothnessScale{ 1.0f };

        InteralMaterialImageMacro(_TangentMap)
        InteralMaterialImageMacro(_TangentMapOS)
        float _Anisotropy{ 0 };
        InteralMaterialImageMacro(_AnisotropyMap)

        float _SubsurfaceMask{ 1.0f };
        InteralMaterialImageMacro(_SubsurfaceMaskMap)
        float _TransmissionMask{ 1.0f };
        InteralMaterialImageMacro(_TransmissionMaskMap)
        float _Thickness{ 1.0f };
        InteralMaterialImageMacro(_ThicknessMap)
        glm::float4 _ThicknessRemap{ 0,1,0,0 };

        float _IridescenceThickness{ 1.0f };
        InteralMaterialImageMacro(_IridescenceThicknessMap)
        glm::float4 _IridescenceThicknessRemap{ 0, 1, 0, 0 };
        float _IridescenceMask{ 1.0f };
        InteralMaterialImageMacro(_IridescenceMaskMap)

        float _CoatMask{ 0.0f };
        InteralMaterialImageMacro(_CoatMaskMap)

        float _EnergyConservingSpecularColor{ 1.0f };
        glm::float4 _SpecularColor{ 1,1,1,1 };
        InteralMaterialImageMacro(_SpecularColorMap)

        int _SpecularOcclusionMode{ 1 };

        glm::float3 _EmissiveColor{ 0,0,0 };
        InteralMaterialImageMacro(_EmissiveColorMap)

        float _AlbedoAffectEmissive{ 0.0f };
        float _EmissiveExposureWeight{ 1.0f };

        float _UseShadowThreshold{ 0.0f };
        float _AlphaCutoff{ 0.5f };
        float _AlphaCutoffShadow{ 0.5f };
        float _AlphaCutoffPrepass{ 0.5f };
        float _AlphaCutoffPostpass{ 0.5f };

        // Transparency
        float _Ior{ 1.5f };
        glm::float3 _TransmittanceColor{ 1.0, 1.0, 1.0 };
        InteralMaterialImageMacro(_TransmittanceColorMap)
        float _ATDistance{ 1.0f };

        // Blending state
        float _SurfaceType{ 0.0f };
        float _BlendMode{ 0.0f };

        float _EnableBlendModePreserveSpecularLighting{ 1.0 };

        float _DoubleSidedEnable{ 0.0f };

        float _ObjectSpaceUVMapping{ 0.0f };
        float _InvTilingScale{ 1.0f };
        float _TexWorldScale{ 1.0f };
        glm::float4 _UVMappingMask{ 1,0,0,0 };
        glm::float4 _UVDetailsMappingMask{ 1,0,0,0 };

        float _PPDMinSamples{ 5 };
        float _PPDMaxSamples{ 15 };
        float _PPDLodThreshold{ 5 };
        float _PPDPrimitiveLength{ 1 };
        float _PPDPrimitiveWidth{ 1 };
        glm::float4 _InvPrimScale{ 1, 1, 0, 0 };
    };

    struct InternalMaterial
    {
        std::string m_shader_name;
        InternalStandardLightMaterial m_intenral_light_mat;
    };

    /*
    struct InternalPBRMaterial
    {
        // Factors
        bool m_blend;
        bool m_double_sided;

        // 
        glm::float4 m_base_color_factor;
        float   m_metallic_factor;
        float   m_roughness_factor;
        float   m_normal_scale;
        float   m_occlusion_strength;
        glm::float3 m_emissive_factor;

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
    */

    struct InternalMeshRenderer
    {
        SceneCommonIdentifier m_identifier;

        glm::float4x4 model_matrix;
        glm::float4x4 model_matrix_inverse;
        glm::float4x4 prev_model_matrix;
        glm::float4x4 prev_model_matrix_inverse;

        InternalMesh ref_mesh;
        InternalMaterial ref_material;
    };
    
    struct InternalScratchIndexBuffer
    {
        uint32_t index_stride;
        uint32_t index_count;
        std::shared_ptr<MoYuScratchBuffer> index_buffer;
    };

    struct InternalScratchVertexBuffer
    {
        InputDefinition input_element_definition;
        uint32_t vertex_count;
        std::shared_ptr<MoYuScratchBuffer> vertex_buffer;
    };

    struct InternalScratchMesh
    {
        AABB              axis_aligned_box;
        InternalScratchIndexBuffer  scratch_index_buffer;
        InternalScratchVertexBuffer scratch_vertex_buffer;
    };

    struct ClipmapTransformScratchBuffer
    {
        int clip_transform_counts;
        std::shared_ptr<MoYuScratchBuffer> clip_transform_buffer; // ClipmapTransform array
        int clip_mesh_count[5]; // GeoClipMap::MeshType 5 types
    };

    struct InternalScratchClipmap
    {
        ClipmapTransformScratchBuffer instance_scratch_buffer;
        InternalScratchMesh clipmap_scratch_mesh[5];
    };

    struct ClipmapTransformBuffer
    {
        int clip_transform_counts;
        std::shared_ptr<RHI::D3D12Buffer> clip_transform_buffer; // ClipmapTransform array
        std::shared_ptr<RHI::D3D12Buffer> clip_mesh_count_buffer; // GeoClipMap::MeshType type count
        std::shared_ptr<RHI::D3D12Buffer> clip_base_mesh_command_buffer; // GeoClipMap::MeshType size, struct element is HLSL::ClipMeshCommandSigParams
    };

    struct InternalClipmap
    {
        ClipmapTransformBuffer instance_buffer;
        InternalMesh clipmap_mesh[5];
    };

    struct InternalTerrainBaseTexture
    {
        std::shared_ptr<RHI::D3D12Texture> albedo;
        std::shared_ptr<RHI::D3D12Texture> ao_roughness_metallic;
        std::shared_ptr<RHI::D3D12Texture> displacement;
        std::shared_ptr<RHI::D3D12Texture> normal;

        glm::float2 albedo_tilling;
        glm::float2 ao_roughness_metallic_tilling;
        glm::float2 displacement_tilling;
        glm::float2 normal_tilling;
    };

    struct InternalTerrain
    {
        glm::int2 terrain_size;
        int terrain_max_height;

        int mesh_size;
        int mesh_lods;

        InternalScratchClipmap terrain_scratch_clipmap;
        InternalClipmap terrain_clipmap;

        std::shared_ptr<MoYu::MoYuScratchImage> terrain_heightmap_scratch;
        std::shared_ptr<MoYu::MoYuScratchImage> terrain_normalmap_scratch;

        std::shared_ptr<RHI::D3D12Texture> terrain_heightmap;
        std::shared_ptr<RHI::D3D12Texture> terrain_normalmap;
    };

    struct InternalTerrainMaterial
    {
        InternalTerrainBaseTexture terrain_base_textures[2];
    };

    struct InternalTerrainRenderer
    {
        SceneCommonIdentifier m_identifier;

        glm::float4x4 model_matrix;
        glm::float4x4 model_matrix_inverse;
        glm::float4x4 prev_model_matrix;
        glm::float4x4 prev_model_matrix_inverse;

        InternalTerrain ref_terrain;
        InternalTerrainMaterial ref_material;
    };

    struct BaseAmbientLight
    {
        Color m_color;
    };

    struct BaseDirectionLight
    {
        Color   m_color {1.0f, 1.0f, 1.0f, 1.0f};
        float   m_intensity {1.0f};

        bool    m_shadowmap {false};
        int     m_cascade {4};
        glm::float2 m_shadow_bounds {32, 32}; // cascade level 0
        float   m_shadow_near_plane {0.1f};
        float   m_shadow_far_plane {200.0f};
        glm::float2 m_shadowmap_size {2048, 2048};
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
        float m_inner_degree;
        float m_outer_degree;

        bool    m_shadowmap {false};
        glm::float2 m_shadow_bounds {128, 128};
        float   m_shadow_near_plane {0.1f};
        float   m_shadow_far_plane {200.0f};
        glm::float2 m_shadowmap_size {512, 512};
    };

    struct InternalAmbientLight : public BaseAmbientLight
    {
        SceneCommonIdentifier m_identifier;

        glm::float3 m_position;
    };

    struct InternalDirectionLight : public BaseDirectionLight
    {
        SceneCommonIdentifier m_identifier;

        glm::float3   m_position;
        glm::float3   m_direction;
        glm::float4x4 m_shadow_view_mat;
        glm::float4x4 m_shadow_proj_mats[4];
        glm::float4x4 m_shadow_view_proj_mats[4];
    };

    struct InternalPointLight : public BasePointLight
    {
        SceneCommonIdentifier m_identifier;

        glm::float3 m_position;
    };

    struct InternalSpotLight : public BaseSpotLight
    {
        SceneCommonIdentifier m_identifier;

        glm::float3   m_position;
        glm::float3   m_direction;
        glm::float4x4 m_shadow_view_proj_mat;
    };

    struct InternalCamera
    {
        SceneCommonIdentifier m_identifier;

        CameraProjType m_projType;

        glm::float3 m_position;
        glm::quat   m_rotation;

        float m_width;
        float m_height;
        float m_znear;
        float m_zfar;
        float m_aspect;
        float m_fovY;

        glm::float4x4 m_ViewMatrix;
        glm::float4x4 m_ViewMatrixInv;

        glm::float4x4 m_ProjMatrix;
        glm::float4x4 m_ProjMatrixInv;

        glm::float4x4 m_ViewProjMatrix;
        glm::float4x4 m_ViewProjMatrixInv;
    };

    struct SkyboxConfigs
    {
        std::shared_ptr<RHI::D3D12Texture> m_skybox_irradiance_map;
        std::shared_ptr<RHI::D3D12Texture> m_skybox_specular_map;
    };

    struct CloudConfigs
    {
        std::shared_ptr<RHI::D3D12Texture> m_weather2D;
        std::shared_ptr<RHI::D3D12Texture> m_cloud3D;
        std::shared_ptr<RHI::D3D12Texture> m_worley3D;
    };

    struct IBLConfigs
    {
        std::shared_ptr<RHI::D3D12Texture> m_dfg;
        std::shared_ptr<RHI::D3D12Texture> m_ld;
        std::shared_ptr<RHI::D3D12Texture> m_radians;

        std::vector<glm::float4> m_SH;
    };

    struct BlueNoiseConfigs
    {
        std::shared_ptr<RHI::D3D12Texture> m_bluenoise_64x64_uni;
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
        C_Terrain      = 1 << 4,
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

    inline bool operator<(const SceneImage& l, const SceneImage& r)
    {
        return l.m_is_srgb < r.m_is_srgb || (l.m_is_srgb == r.m_is_srgb && l.m_auto_mips < r.m_auto_mips) ||
               ((l.m_is_srgb == r.m_is_srgb && l.m_auto_mips == r.m_auto_mips) && l.m_image_file < r.m_image_file) ||
               ((l.m_is_srgb == r.m_is_srgb && l.m_auto_mips == r.m_auto_mips && l.m_image_file == r.m_image_file) && l.m_mip_levels < r.m_mip_levels);
    }


    struct SceneTerrainMesh
    {
        glm::int2  terrain_size {glm::int2(1024, 1024)};
        int        terrian_max_height {1024};
        SceneImage m_terrain_height_map {};
        SceneImage m_terrain_normal_map {};
    };

    inline bool operator==(const SceneTerrainMesh& lhs, const SceneTerrainMesh& rhs)
    {
        return lhs.terrain_size == rhs.terrain_size && 
               lhs.terrian_max_height == rhs.terrian_max_height &&
               lhs.m_terrain_height_map == rhs.m_terrain_height_map &&
               lhs.m_terrain_normal_map == rhs.m_terrain_normal_map;
    }

    struct MaterialImage
    {
        SceneImage m_image {};
        glm::float2 m_tilling {1.0f, 1.0f};
    };

    inline bool operator==(const MaterialImage& lhs, const MaterialImage& rhs)
    {
        return lhs.m_tilling == rhs.m_tilling && lhs.m_image == rhs.m_image;
    }
    
    extern SceneImage DefaultSceneImageWhite;
    extern SceneImage DefaultSceneImageBlack;
    extern SceneImage DefaultSceneImageGrey;
    extern SceneImage DefaultSceneImageRed;
    extern SceneImage DefaultSceneImageGreen;
    extern SceneImage DefaultSceneImageBlue;

    extern MaterialImage DefaultMaterialImageWhite;
    extern MaterialImage DefaultMaterialImageBlack;
    extern MaterialImage DefaultMaterialImageGrey;
    extern MaterialImage DefaultMaterialImageRed;
    extern MaterialImage DefaultMaterialImageGreen;
    extern MaterialImage DefaultMaterialImageBlue;

    struct StandardLightMaterial
    {
        glm::float4 _BaseColor{ 1, 1, 1, 1 };
        MaterialImage _BaseColorMap{ DefaultMaterialImageWhite };

        MaterialImage _MaskMap{ DefaultMaterialImageWhite };
        float _Metallic{ 0 };
        float _Smoothness{ 0.5f };
        float _MetallicRemapMin{ 0.0f };
        float _MetallicRemapMax{ 1.0f };
        float _SmoothnessRemapMin{ 0.0f };
        float _SmoothnessRemapMax{ 1.0f };
        float _AlphaRemapMin{ 0.0f };
        float _AlphaRemapMax{ 1.0f };
        float _AORemapMin{ 0.0f };
        float _AORemapMax{ 1.0f };

        MaterialImage _NormalMap{ DefaultMaterialImageGrey }; // Tangent space normal map
        MaterialImage _NormalMapOS{ DefaultMaterialImageWhite }; // Object space normal map - no good default value
        float _NormalScale{ 1.0f };

        MaterialImage _BentNormalMap{ DefaultMaterialImageGrey };
        MaterialImage _BentNormalMapOS{ DefaultMaterialImageWhite };

        MaterialImage _HeightMap{ DefaultMaterialImageBlack };
        float _HeightAmplitude{ 0.02f };
        float _HeightCenter{ 0.5f };

        MaterialImage _DetailMap{ DefaultMaterialImageGrey };
        float _DetailAlbedoScale{ 1.0f };
        float _DetailNormalScale{ 1.0f };
        float _DetailSmoothnessScale{ 1.0f };

        MaterialImage _TangentMap{ DefaultMaterialImageGrey };
        MaterialImage _TangentMapOS{ DefaultMaterialImageWhite };
        float _Anisotropy{ 0 };
        MaterialImage _AnisotropyMap{ DefaultMaterialImageWhite };

        float _SubsurfaceMask{ 1.0f };
        MaterialImage _SubsurfaceMaskMap{ DefaultMaterialImageWhite };
        float _TransmissionMask{ 1.0f };
        MaterialImage _TransmissionMaskMap{ DefaultMaterialImageWhite };
        float _Thickness{ 1.0f };
        MaterialImage _ThicknessMap{ DefaultMaterialImageWhite };
        glm::float4 _ThicknessRemap{ 0,1,0,0 };

        float _IridescenceThickness{ 1.0f };
        MaterialImage _IridescenceThicknessMap{ DefaultMaterialImageWhite };
        glm::float4 _IridescenceThicknessRemap{ 0, 1, 0, 0 };
        float _IridescenceMask{ 1.0f };
        MaterialImage _IridescenceMaskMap{ DefaultMaterialImageWhite };

        float _CoatMask{ 0.0f };
        MaterialImage _CoatMaskMap{ DefaultMaterialImageWhite };

        float _EnergyConservingSpecularColor{1.0f};
        glm::float4 _SpecularColor{ 1,1,1,1 };
        MaterialImage _SpecularColorMap{ DefaultMaterialImageWhite };

        int _SpecularOcclusionMode{ 1 };

        glm::float3 _EmissiveColor{ 0,0,0 };
        MaterialImage _EmissiveColorMap{ DefaultMaterialImageBlack };
        float _AlbedoAffectEmissive{ 0.0f };
        float _EmissiveExposureWeight{ 1.0f };

        float _UseShadowThreshold{ 0.0f };
        float _AlphaCutoff{ 0.5f };
        float _AlphaCutoffShadow{ 0.5f };
        float _AlphaCutoffPrepass { 0.5f };
        float _AlphaCutoffPostpass{ 0.5f };

        // Transparency
        float _Ior{ 1.5f };
        glm::float3 _TransmittanceColor{ 1.0, 1.0, 1.0 };
        MaterialImage _TransmittanceColorMap{ DefaultMaterialImageWhite };
        float _ATDistance{ 1.0f };

        // Blending state
        float _SurfaceType{ 0.0f };
        float _BlendMode{ 0.0f };

        float _EnableBlendModePreserveSpecularLighting{ 1.0 };

        float _DoubleSidedEnable{ 0.0f };

        float _ObjectSpaceUVMapping{ 0.0f };
        float _InvTilingScale{ 1.0f };
        float _TexWorldScale{ 1.0f };
        glm::float4 _UVMappingMask{ 1,0,0,0 };
        glm::float4 _UVDetailsMappingMask{ 1,0,0,0 };

        float _PPDMinSamples{ 5 };
        float _PPDMaxSamples{ 15 };
        float _PPDLodThreshold{ 5 };
        float _PPDPrimitiveLength{ 1 };
        float _PPDPrimitiveWidth{ 1 };
        glm::float4 _InvPrimScale{ 1, 1, 0, 0 };
    };

    inline bool operator==(const StandardLightMaterial& lhs, const StandardLightMaterial& rhs)
    {
        #define CompareVal(Val) lhs.Val == rhs.Val

        return
            CompareVal(_BaseColor) &&
            CompareVal(_BaseColorMap) &&
            CompareVal(_MaskMap) &&
            CompareVal(_Metallic) &&
            CompareVal(_Smoothness) &&
            CompareVal(_MetallicRemapMin) &&
            CompareVal(_MetallicRemapMax) &&
            CompareVal(_SmoothnessRemapMin) &&
            CompareVal(_SmoothnessRemapMax) &&
            CompareVal(_AlphaRemapMin) &&
            CompareVal(_AlphaRemapMax) &&
            CompareVal(_AORemapMin) &&
            CompareVal(_AORemapMax) &&
            CompareVal(_NormalMap) &&
            CompareVal(_NormalMapOS) &&
            CompareVal(_NormalScale) &&
            CompareVal(_BentNormalMap) &&
            CompareVal(_BentNormalMapOS) &&
            CompareVal(_HeightMap) &&
            CompareVal(_HeightAmplitude) &&
            CompareVal(_HeightCenter) &&
            CompareVal(_DetailMap) &&
            CompareVal(_DetailAlbedoScale) &&
            CompareVal(_DetailNormalScale) &&
            CompareVal(_DetailSmoothnessScale) &&
            CompareVal(_TangentMap) &&
            CompareVal(_TangentMapOS) &&
            CompareVal(_Anisotropy) &&
            CompareVal(_AnisotropyMap) &&
            CompareVal(_SubsurfaceMask) &&
            CompareVal(_SubsurfaceMaskMap) &&
            CompareVal(_TransmissionMask) &&
            CompareVal(_TransmissionMaskMap) &&
            CompareVal(_Thickness) &&
            CompareVal(_ThicknessMap) &&
            CompareVal(_ThicknessRemap) &&
            CompareVal(_IridescenceThickness) &&
            CompareVal(_IridescenceThicknessMap) &&
            CompareVal(_IridescenceThicknessRemap) &&
            CompareVal(_IridescenceMask) &&
            CompareVal(_IridescenceMaskMap) &&
            CompareVal(_CoatMask) &&
            CompareVal(_CoatMaskMap) &&
            CompareVal(_EnergyConservingSpecularColor) &&
            CompareVal(_SpecularColor) &&
            CompareVal(_SpecularColorMap) &&
            CompareVal(_SpecularOcclusionMode) &&
            CompareVal(_EmissiveColor) &&
            CompareVal(_EmissiveColorMap) &&
            CompareVal(_AlbedoAffectEmissive) &&
            CompareVal(_EmissiveExposureWeight) &&
            CompareVal(_UseShadowThreshold) &&
            CompareVal(_AlphaCutoff) &&
            CompareVal(_AlphaCutoffShadow) &&
            CompareVal(_AlphaCutoffPrepass) &&
            CompareVal(_AlphaCutoffPostpass) &&
            CompareVal(_Ior) &&
            CompareVal(_TransmittanceColor) &&
            CompareVal(_TransmittanceColorMap) &&
            CompareVal(_ATDistance) &&
            CompareVal(_SurfaceType) &&
            CompareVal(_BlendMode) &&
            CompareVal(_EnableBlendModePreserveSpecularLighting) &&
            CompareVal(_DoubleSidedEnable) &&
            CompareVal(_InvTilingScale) &&
            CompareVal(_TexWorldScale) &&
            CompareVal(_UVMappingMask) &&
            CompareVal(_UVDetailsMappingMask) &&
            CompareVal(_PPDMinSamples) &&
            CompareVal(_PPDMaxSamples) &&
            CompareVal(_PPDLodThreshold) &&
            CompareVal(_PPDPrimitiveLength) &&
            CompareVal(_PPDPrimitiveWidth) &&
            CompareVal(_InvPrimScale);
;
    }

    struct MaterialRes;

    MaterialRes ToMaterialRes(const StandardLightMaterial& pbrMaterial, const std::string shaderName);
    StandardLightMaterial ToPBRMaterial(const MaterialRes& materialRes);

    extern StandardLightMaterial _DefaultScenePBRMaterial;

    struct SceneMaterial
    {
        std::string m_shader_name;
        StandardLightMaterial m_mat_data;
    };

    struct SceneMeshRenderer
    {
        static const ComponentType m_component_type {ComponentType::C_MeshRenderer};

        SceneCommonIdentifier m_identifier;

        SceneMesh m_scene_mesh;
        SceneMaterial m_material;
    };

    struct TerrainBaseTex
    {
        MaterialImage m_albedo_file {};
        MaterialImage m_ao_roughness_metallic_file {};
        MaterialImage m_displacement_file {};
        MaterialImage m_normal_file {};
    };

    inline bool operator==(const TerrainBaseTex& lhs, const TerrainBaseTex& rhs)
    {
    #define CompareVal(Val) lhs.Val == rhs.Val

        return CompareVal(m_albedo_file) && CompareVal(m_ao_roughness_metallic_file) &&
               CompareVal(m_displacement_file) && CompareVal(m_normal_file);
    }

    struct TerrainMaterial
    {
        TerrainBaseTex m_base_texs[2];
    };
    inline bool operator==(const TerrainMaterial& lhs, const TerrainMaterial& rhs)
    {
    #define CompareVal(Val) lhs.Val == rhs.Val
        return CompareVal(m_base_texs[0]) && CompareVal(m_base_texs[1]);
    }

    struct SceneTerrainRenderer
    {
        static const ComponentType m_component_type {ComponentType::C_Terrain};

        SceneCommonIdentifier m_identifier;

        SceneTerrainMesh m_scene_terrain_mesh;
        TerrainMaterial  m_terrain_material;
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

        glm::float4x4  m_transform_matrix {MYMatrix4x4::Identity};
        glm::float3     m_position {MYFloat3::Zero};
        glm::quat m_rotation {MYQuaternion::Identity};
        glm::float3     m_scale {MYFloat3::One};
    };

    struct GameObjectComponentDesc
    {
        ComponentType m_component_type {ComponentType::C_Transform};

        //SceneCommonIdentifier m_identifier {};

        SceneTransform    m_transform_desc {};
        SceneMeshRenderer m_mesh_renderer_desc {};
        SceneTerrainRenderer m_terrain_mesh_renderer_desc {};
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
    // 
    //========================================================================


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
        std::shared_ptr<MoYuScratchBuffer> m_vertex_buffer;
        std::shared_ptr<MoYuScratchBuffer> m_index_buffer;
    };

    struct RenderMeshData
    {
        AABB m_axis_aligned_box;
        StaticMeshData m_static_mesh_data;
    };

} // namespace MoYu
