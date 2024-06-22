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

        static const RHI::D3D12InputLayout InputLayout;

        static const InputDefinition InputElementDefinition = 
            InputDefinition::POSITION | InputDefinition::NORMAL |
            InputDefinition::TANGENT | InputDefinition::TEXCOORD0;

    private:
        static constexpr unsigned int         InputElementCount = 4;
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
        //=======================Texture=======================
        std::shared_ptr<RHI::D3D12Texture> _BaseColorMap { nullptr }; // white
        std::shared_ptr<RHI::D3D12Texture> _MaskMap { nullptr }; // MaskMap is RGBA: Metallic, Ambient Occlusion (Optional), detail Mask (Optional), Smoothness
        std::shared_ptr<RHI::D3D12Texture> _NormalMap { nullptr }; // bump. Tangent space normal map
        std::shared_ptr<RHI::D3D12Texture> _NormalMapOS { nullptr }; // white. Object space normal map - no good default value    
        std::shared_ptr<RHI::D3D12Texture> _BentNormalMap { nullptr }; // bump. Tangent space normal map
        std::shared_ptr<RHI::D3D12Texture> _BentNormalMapOS { nullptr }; // white. Object space normal map - no good default value
        std::shared_ptr<RHI::D3D12Texture> _HeightMap { nullptr }; // black
        std::shared_ptr<RHI::D3D12Texture> _DetailMap { nullptr }; // linearGrey
        std::shared_ptr<RHI::D3D12Texture> _TangentMap { nullptr }; // bump
        std::shared_ptr<RHI::D3D12Texture> _TangentMapOS { nullptr }; // white
        std::shared_ptr<RHI::D3D12Texture> _AnisotropyMap { nullptr }; // white
        std::shared_ptr<RHI::D3D12Texture> _SubsurfaceMaskMap { nullptr }; // white. Subsurface Radius Map
        std::shared_ptr<RHI::D3D12Texture> _TransmissionMaskMap { nullptr }; // white. Transmission Mask Map
        std::shared_ptr<RHI::D3D12Texture> _ThicknessMap { nullptr }; // white. Thickness Map
        std::shared_ptr<RHI::D3D12Texture> _IridescenceThicknessMap { nullptr }; // white. Iridescence Thickness Map
        std::shared_ptr<RHI::D3D12Texture> _IridescenceMaskMap { nullptr }; // white. Iridescence Mask Map
        std::shared_ptr<RHI::D3D12Texture> _CoatMaskMap { nullptr }; // white
        std::shared_ptr<RHI::D3D12Texture> _SpecularColorMap { nullptr }; // white
        std::shared_ptr<RHI::D3D12Texture> _EmissiveColorMap { nullptr }; // white
        std::shared_ptr<RHI::D3D12Texture> _TransmittanceColorMap{ nullptr }; // white
        //=====================================================

        //=======================Properties====================
        glm::float4 _BaseColor{ 1, 1, 1, 1 };
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
        float _NormalScale{ 1.0f };
        float _HeightAmplitude{ 0.02f }; // Height Amplitude. In world units. Default value of _HeightAmplitude must be (_HeightMax - _HeightMin) * 0.01
        float _HeightCenter{ 0.5f }; // Height Center. In texture space
        int _HeightMapParametrization{ 0 }; // Heightmap Parametrization. MinMax, 0, Amplitude, 1
        // These parameters are for vertex displacement/Tessellation
        float _HeightOffset{ 0 }; // Height Offset
        // MinMax mode
        float _HeightMin{ -1 }; // Heightmap Min
        float _HeightMax{ 1 }; // Heightmap Max
        // Amplitude mode
        float _HeightTessAmplitude{ 2.0f }; // in Centimeters
        float _HeightTessCenter{ 0.5f }; // In texture space
        // These parameters are for pixel displacement
        float _HeightPoMAmplitude{ 2.0f }; // Height Amplitude. In centimeters
        float _DetailAlbedoScale{ 1.0f };
        float _DetailNormalScale{ 1.0f };
        float _DetailSmoothnessScale{ 1.0f };
        float _Anisotropy{ 0 };
        float _DiffusionProfileHash{ 0 };
        float _SubsurfaceMask{ 1.0f }; // Subsurface Radius
        float _TransmissionMask{ 1.0f }; // Transmission Mask
        float _Thickness{ 1.0f };
        glm::float4 _ThicknessRemap{ 0, 1, 0, 0 };
        glm::float4 _IridescenceThicknessRemap{ 0, 1, 0, 0 };
        float _IridescenceThickness{ 1.0f };
        float _IridescenceMask{ 1.0f };
        float _CoatMask{ 0 };
        float _EnergyConservingSpecularColor{ 1.0f };
        glm::float4 _SpecularColor{ 1, 1, 1, 1 };
        int _SpecularOcclusionMode{ 1 }; // Off, 0, From Ambient Occlusion, 1, From AO and Bent Normals, 2
        glm::float3 _EmissiveColor{ 0, 0, 0 };
        float _AlbedoAffectEmissive{ 0 }; // Albedo Affect Emissive
        int _EmissiveIntensityUnit{ 0 }; // Emissive Mode
        int _UseEmissiveIntensity{ 0 }; // Use Emissive Intensity
        float _EmissiveIntensity{ 1.0f }; // Emissive Intensity
        float _EmissiveExposureWeight{ 1.0f }; // Emissive Pre Exposure
        float _UseShadowThreshold{ 0 };
        float _AlphaCutoffEnable{ 0 };
        float _AlphaCutoff{ 0.5f };
        float _AlphaCutoffShadow{ 0.5f };
        float _AlphaCutoffPrepass{ 0.5f };
        float _AlphaCutoffPostpass{ 0.5f };
        float _TransparentDepthPrepassEnable{ 0 };
        float _TransparentBackfaceEnable{ 0 };
        float _TransparentDepthPostpassEnable{ 0 };
        float _TransparentSortPriority{ 0 };
        // Transparency
        int _RefractionModel{ 0 }; // None, 0, Planar, 1, Sphere, 2, Thin, 3
        float _Ior{ 1.5f };
        glm::float3 _TransmittanceColor{ 1.0f, 1.0f, 1.0f };
        float _ATDistance{ 1.0f };
        float _TransparentWritingMotionVec{ 0 };
        // Blending state
        float _SurfaceType{ 0 };
        float _BlendMode{ 0 };
        float _SrcBlend{ 1.0f };
        float _DstBlend{ 0 };
        float _AlphaSrcBlend{ 1.0f };
        float _AlphaDstBlend{ 0 };
        float _EnableFogOnTransparent{ 1.0f }; // Enable Fog
        float _EnableBlendModePreserveSpecularLighting{ 1.0f }; // Enable Blend Mode Preserve Specular Lighting
        float _DoubleSidedEnable{ 0 }; // Double sided enable
        float _DoubleSidedNormalMode{ 1 }; // Flip, 0, Mirror, 1, None, 2
        glm::float4 _DoubleSidedConstants{ 1, 1, -1, 0 };
        float _DoubleSidedGIMode{ 0 }; // Auto, 0, On, 1, Off, 2
        float _UVBase{ 0 }; // UV0, 0, UV1, 1, UV2, 2, UV3, 3, Planar, 4, Triplanar, 5
        float _ObjectSpaceUVMapping{ 0 }; // WorldSpace, 0, ObjectSpace, 1
        float _TexWorldScale{ 1 }; // Scale to apply on world coordinate
        glm::float4 _UVMappingMask{ 1, 0, 0, 0 };
        float _NormalMapSpace{ 0 }; // TangentSpace, 0, ObjectSpace, 1
        int _MaterialID{ 1 }; // Subsurface Scattering, 0, Standard, 1, Anisotropy, 2, Iridescence, 3, Specular Color, 4, Translucent, 5
        float _TransmissionEnable{ 1 };
        float _PPDMinSamples{ 5 }; // Min sample for POM
        float _PPDMaxSamples{ 15 }; // Max sample for POM
        float _PPDLodThreshold{ 5 }; // Start lod to fade out the POM effect
        float _PPDPrimitiveLength{ 1 }; // Primitive length for POM
        float _PPDPrimitiveWidth{ 1 }; // Primitive width for POM
        glm::float4 _InvPrimScale{ 1, 1, 0, 0 }; // Inverse primitive scale for non-planar POM
        glm::float4 _UVDetailsMappingMask{ 1, 0, 0, 0 };
        float _UVDetail{ 0 }; // UV0, 0, UV1, 1, UV2, 2, UV3, 3. UV Set for detail
        float _LinkDetailsWithBase{ 1.0f };
        float _EmissiveColorMode{ 1 }; // Use Emissive Color, 0, Use Emissive Mask, 1. Emissive color mode
        float _UVEmissive{ 0 }; // UV0, 0, UV1, 1, UV2, 2, UV3, 3, Planar, 4, Triplanar, 5, Same as Base, 6. UV Set for emissive
        //=====================================================
    };

    struct InternalMaterial
    {
        std::string m_shader_name;
        InternalStandardLightMaterial m_intenral_light_mat;
    };

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
        //=======================Texture=======================
        MaterialImage _BaseColorMap{ DefaultMaterialImageWhite }; // white
        MaterialImage _MaskMap{ DefaultMaterialImageWhite }; // MaskMap is RGBA: Metallic, Ambient Occlusion (Optional), detail Mask (Optional), Smoothness
        MaterialImage _NormalMap{ DefaultMaterialImageGrey }; // bump. Tangent space normal map
        MaterialImage _NormalMapOS{ DefaultMaterialImageWhite }; // white. Object space normal map - no good default value    
        MaterialImage _BentNormalMap{ DefaultMaterialImageGrey }; // bump. Tangent space normal map
        MaterialImage _BentNormalMapOS{ DefaultMaterialImageWhite }; // white. Object space normal map - no good default value
        MaterialImage _HeightMap{ DefaultMaterialImageBlack }; // black
        MaterialImage _DetailMap{ DefaultMaterialImageGrey }; // linearGrey
        MaterialImage _TangentMap{ DefaultMaterialImageGrey }; // bump
        MaterialImage _TangentMapOS{ DefaultMaterialImageWhite }; // white
        MaterialImage _AnisotropyMap{ DefaultMaterialImageWhite }; // white
        MaterialImage _SubsurfaceMaskMap{ DefaultMaterialImageWhite }; // white. Subsurface Radius Map
        MaterialImage _TransmissionMaskMap{ DefaultMaterialImageWhite }; // white. Transmission Mask Map
        MaterialImage _ThicknessMap{ DefaultMaterialImageWhite }; // white. Thickness Map
        MaterialImage _IridescenceThicknessMap{ DefaultMaterialImageWhite }; // white. Iridescence Thickness Map
        MaterialImage _IridescenceMaskMap{ DefaultMaterialImageWhite }; // white. Iridescence Mask Map
        MaterialImage _CoatMaskMap{ DefaultMaterialImageWhite }; // white
        MaterialImage _SpecularColorMap{ DefaultMaterialImageWhite }; // white
        MaterialImage _EmissiveColorMap{ DefaultMaterialImageBlack }; // white
        MaterialImage _TransmittanceColorMap{ DefaultMaterialImageWhite }; // white
        glm::float4 _BaseColor{ 1, 1, 1, 1 };
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
        float _NormalScale{ 1.0f };
        float _HeightAmplitude{ 0.02f }; // Height Amplitude. In world units. Default value of _HeightAmplitude must be (_HeightMax - _HeightMin) * 0.01
        float _HeightCenter{ 0.5f }; // Height Center. In texture space
        int _HeightMapParametrization{ 0 }; // Heightmap Parametrization. MinMax, 0, Amplitude, 1
        // These parameters are for vertex displacement/Tessellation
        float _HeightOffset{ 0 }; // Height Offset
        // MinMax mode
        float _HeightMin{ -1 }; // Heightmap Min
        float _HeightMax{ 1 }; // Heightmap Max
        // Amplitude mode
        float _HeightTessAmplitude{ 2.0f }; // in Centimeters
        float _HeightTessCenter{ 0.5f }; // In texture space
        // These parameters are for pixel displacement
        float _HeightPoMAmplitude{ 2.0f }; // Height Amplitude. In centimeters
        float _DetailAlbedoScale{ 1.0f };
        float _DetailNormalScale{ 1.0f };
        float _DetailSmoothnessScale{ 1.0f };
        float _Anisotropy{ 0 };
        float _DiffusionProfileHash{ 0 };
        float _SubsurfaceMask{ 1.0f }; // Subsurface Radius
        float _TransmissionMask{ 1.0f }; // Transmission Mask
        float _Thickness{ 1.0f };
        glm::float4 _ThicknessRemap{ 0, 1, 0, 0 };
        glm::float4 _IridescenceThicknessRemap{ 0, 1, 0, 0 };
        float _IridescenceThickness{ 1.0f };
        float _IridescenceMask{ 1.0f };
        float _CoatMask{ 0 };
        float _EnergyConservingSpecularColor{ 1.0f };
        glm::float4 _SpecularColor{ 1, 1, 1, 1 };
        int _SpecularOcclusionMode{ 1 }; // Off, 0, From Ambient Occlusion, 1, From AO and Bent Normals, 2
        glm::float3 _EmissiveColor{ 0, 0, 0 };
        float _AlbedoAffectEmissive{ 0 }; // Albedo Affect Emissive
        int _EmissiveIntensityUnit{ 0 }; // Emissive Mode
        int _UseEmissiveIntensity{ 0 }; // Use Emissive Intensity
        float _EmissiveIntensity{ 1.0f }; // Emissive Intensity
        float _EmissiveExposureWeight{ 1.0f }; // Emissive Pre Exposure
        float _UseShadowThreshold{ 0 };
        float _AlphaCutoffEnable{ 0 };
        float _AlphaCutoff{ 0.5f };
        float _AlphaCutoffShadow{ 0.5f };
        float _AlphaCutoffPrepass{ 0.5f };
        float _AlphaCutoffPostpass{ 0.5f };
        float _TransparentDepthPrepassEnable{ 0 };
        float _TransparentBackfaceEnable{ 0 };
        float _TransparentDepthPostpassEnable{ 0 };
        float _TransparentSortPriority{ 0 };
        // Transparency
        int _RefractionModel{ 0 }; // None, 0, Planar, 1, Sphere, 2, Thin, 3
        float _Ior{ 1.5f };
        glm::float3 _TransmittanceColor{ 1.0f, 1.0f, 1.0f };
        float _ATDistance{ 1.0f };
        float _TransparentWritingMotionVec{ 0 };
        // Blending state
        float _SurfaceType{ 0 };
        float _BlendMode{ 0 };
        float _SrcBlend{ 1.0f };
        float _DstBlend{ 0 };
        float _AlphaSrcBlend{ 1.0f };
        float _AlphaDstBlend{ 0 };
        float _EnableFogOnTransparent{ 1.0f }; // Enable Fog
        float _EnableBlendModePreserveSpecularLighting{ 1.0f }; // Enable Blend Mode Preserve Specular Lighting
        float _DoubleSidedEnable{ 0 }; // Double sided enable
        float _DoubleSidedNormalMode{ 1 }; // Flip, 0, Mirror, 1, None, 2
        glm::float4 _DoubleSidedConstants{ 1, 1, -1, 0 };
        float _DoubleSidedGIMode{ 0 }; // Auto, 0, On, 1, Off, 2
        float _UVBase{ 0 }; // UV0, 0, UV1, 1, UV2, 2, UV3, 3, Planar, 4, Triplanar, 5
        float _ObjectSpaceUVMapping{ 0 }; // WorldSpace, 0, ObjectSpace, 1
        float _TexWorldScale{ 1 }; // Scale to apply on world coordinate
        glm::float4 _UVMappingMask{ 1, 0, 0, 0 };
        float _NormalMapSpace{ 0 }; // TangentSpace, 0, ObjectSpace, 1
        int _MaterialID{ 1 }; // Subsurface Scattering, 0, Standard, 1, Anisotropy, 2, Iridescence, 3, Specular Color, 4, Translucent, 5
        float _TransmissionEnable{ 1 };
        float _PPDMinSamples{ 5 }; // Min sample for POM
        float _PPDMaxSamples{ 15 }; // Max sample for POM
        float _PPDLodThreshold{ 5 }; // Start lod to fade out the POM effect
        float _PPDPrimitiveLength{ 1 }; // Primitive length for POM
        float _PPDPrimitiveWidth{ 1 }; // Primitive width for POM
        glm::float4 _InvPrimScale{ 1, 1, 0, 0 }; // Inverse primitive scale for non-planar POM
        glm::float4 _UVDetailsMappingMask{ 1, 0, 0, 0 };
        float _UVDetail{ 0 }; // UV0, 0, UV1, 1, UV2, 2, UV3, 3. UV Set for detail
        float _LinkDetailsWithBase{ 1.0f };
        float _EmissiveColorMode{ 1 }; // Use Emissive Color, 0, Use Emissive Mask, 1. Emissive color mode
        float _UVEmissive{ 0 }; // UV0, 0, UV1, 1, UV2, 2, UV3, 3, Planar, 4, Triplanar, 5, Same as Base, 6. UV Set for emissive
    };

    inline bool operator==(const StandardLightMaterial& lhs, const StandardLightMaterial& rhs)
    {
        return
            lhs._BaseColorMap == rhs._BaseColorMap &&
            lhs._MaskMap == rhs._MaskMap &&
            lhs._NormalMap == rhs._NormalMap &&
            lhs._NormalMapOS == rhs._NormalMapOS &&
            lhs._BentNormalMap == rhs._BentNormalMap &&
            lhs._BentNormalMapOS == rhs._BentNormalMapOS &&
            lhs._HeightMap == rhs._HeightMap &&
            lhs._DetailMap == rhs._DetailMap &&
            lhs._TangentMap == rhs._TangentMap &&
            lhs._TangentMapOS == rhs._TangentMapOS &&
            lhs._AnisotropyMap == rhs._AnisotropyMap &&
            lhs._SubsurfaceMaskMap == rhs._SubsurfaceMaskMap &&
            lhs._TransmissionMaskMap == rhs._TransmissionMaskMap &&
            lhs._ThicknessMap == rhs._ThicknessMap &&
            lhs._IridescenceThicknessMap == rhs._IridescenceThicknessMap &&
            lhs._IridescenceMaskMap == rhs._IridescenceMaskMap &&
            lhs._CoatMaskMap == rhs._CoatMaskMap &&
            lhs._SpecularColorMap == rhs._SpecularColorMap &&
            lhs._EmissiveColorMap == rhs._EmissiveColorMap &&
            lhs._TransmittanceColorMap == rhs._TransmittanceColorMap &&
            lhs._BaseColor == rhs._BaseColor &&
            lhs._Metallic == rhs._Metallic &&
            lhs._Smoothness == rhs._Smoothness &&
            lhs._MetallicRemapMin == rhs._MetallicRemapMin &&
            lhs._MetallicRemapMax == rhs._MetallicRemapMax &&
            lhs._SmoothnessRemapMin == rhs._SmoothnessRemapMin &&
            lhs._SmoothnessRemapMax == rhs._SmoothnessRemapMax &&
            lhs._AlphaRemapMin == rhs._AlphaRemapMin &&
            lhs._AlphaRemapMax == rhs._AlphaRemapMax &&
            lhs._AORemapMin == rhs._AORemapMin &&
            lhs._AORemapMax == rhs._AORemapMax &&
            lhs._NormalScale == rhs._NormalScale &&
            lhs._HeightAmplitude == rhs._HeightAmplitude &&
            lhs._HeightCenter == rhs._HeightCenter &&
            lhs._HeightMapParametrization == rhs._HeightMapParametrization &&
            lhs._HeightOffset == rhs._HeightOffset &&
            lhs._HeightMin == rhs._HeightMin &&
            lhs._HeightMax == rhs._HeightMax &&
            lhs._HeightTessAmplitude == rhs._HeightTessAmplitude &&
            lhs._HeightTessCenter == rhs._HeightTessCenter &&
            lhs._HeightPoMAmplitude == rhs._HeightPoMAmplitude &&
            lhs._DetailAlbedoScale == rhs._DetailAlbedoScale &&
            lhs._DetailNormalScale == rhs._DetailNormalScale &&
            lhs._DetailSmoothnessScale == rhs._DetailSmoothnessScale &&
            lhs._Anisotropy == rhs._Anisotropy &&
            lhs._DiffusionProfileHash == rhs._DiffusionProfileHash &&
            lhs._SubsurfaceMask == rhs._SubsurfaceMask &&
            lhs._TransmissionMask == rhs._TransmissionMask &&
            lhs._Thickness == rhs._Thickness &&
            lhs._ThicknessRemap == rhs._ThicknessRemap &&
            lhs._IridescenceThicknessRemap == rhs._IridescenceThicknessRemap &&
            lhs._IridescenceThickness == rhs._IridescenceThickness &&
            lhs._IridescenceMask == rhs._IridescenceMask &&
            lhs._CoatMask == rhs._CoatMask &&
            lhs._EnergyConservingSpecularColor == rhs._EnergyConservingSpecularColor &&
            lhs._SpecularColor == rhs._SpecularColor &&
            lhs._SpecularOcclusionMode == rhs._SpecularOcclusionMode &&
            lhs._EmissiveColor == rhs._EmissiveColor &&
            lhs._AlbedoAffectEmissive == rhs._AlbedoAffectEmissive &&
            lhs._EmissiveIntensityUnit == rhs._EmissiveIntensityUnit &&
            lhs._UseEmissiveIntensity == rhs._UseEmissiveIntensity &&
            lhs._EmissiveIntensity == rhs._EmissiveIntensity &&
            lhs._EmissiveExposureWeight == rhs._EmissiveExposureWeight &&
            lhs._UseShadowThreshold == rhs._UseShadowThreshold &&
            lhs._AlphaCutoffEnable == rhs._AlphaCutoffEnable &&
            lhs._AlphaCutoff == rhs._AlphaCutoff &&
            lhs._AlphaCutoffShadow == rhs._AlphaCutoffShadow &&
            lhs._AlphaCutoffPrepass == rhs._AlphaCutoffPrepass &&
            lhs._AlphaCutoffPostpass == rhs._AlphaCutoffPostpass &&
            lhs._TransparentDepthPrepassEnable == rhs._TransparentDepthPrepassEnable &&
            lhs._TransparentBackfaceEnable == rhs._TransparentBackfaceEnable &&
            lhs._TransparentDepthPostpassEnable == rhs._TransparentDepthPostpassEnable &&
            lhs._TransparentSortPriority == rhs._TransparentSortPriority &&
            lhs._RefractionModel == rhs._RefractionModel &&
            lhs._Ior == rhs._Ior &&
            lhs._TransmittanceColor == rhs._TransmittanceColor &&
            lhs._ATDistance == rhs._ATDistance &&
            lhs._TransparentWritingMotionVec == rhs._TransparentWritingMotionVec &&
            lhs._SurfaceType == rhs._SurfaceType &&
            lhs._BlendMode == rhs._BlendMode &&
            lhs._SrcBlend == rhs._SrcBlend &&
            lhs._DstBlend == rhs._DstBlend &&
            lhs._AlphaSrcBlend == rhs._AlphaSrcBlend &&
            lhs._AlphaDstBlend == rhs._AlphaDstBlend &&
            lhs._EnableFogOnTransparent == rhs._EnableFogOnTransparent &&
            lhs._EnableBlendModePreserveSpecularLighting == rhs._EnableBlendModePreserveSpecularLighting &&
            lhs._DoubleSidedEnable == rhs._DoubleSidedEnable &&
            lhs._DoubleSidedNormalMode == rhs._DoubleSidedNormalMode &&
            lhs._DoubleSidedConstants == rhs._DoubleSidedConstants &&
            lhs._DoubleSidedGIMode == rhs._DoubleSidedGIMode &&
            lhs._UVBase == rhs._UVBase &&
            lhs._ObjectSpaceUVMapping == rhs._ObjectSpaceUVMapping &&
            lhs._TexWorldScale == rhs._TexWorldScale &&
            lhs._UVMappingMask == rhs._UVMappingMask &&
            lhs._NormalMapSpace == rhs._NormalMapSpace &&
            lhs._MaterialID == rhs._MaterialID &&
            lhs._TransmissionEnable == rhs._TransmissionEnable &&
            lhs._PPDMinSamples == rhs._PPDMinSamples &&
            lhs._PPDMaxSamples == rhs._PPDMaxSamples &&
            lhs._PPDLodThreshold == rhs._PPDLodThreshold &&
            lhs._PPDPrimitiveLength == rhs._PPDPrimitiveLength &&
            lhs._PPDPrimitiveWidth == rhs._PPDPrimitiveWidth &&
            lhs._InvPrimScale == rhs._InvPrimScale &&
            lhs._UVDetailsMappingMask == rhs._UVDetailsMappingMask &&
            lhs._UVDetail == rhs._UVDetail &&
            lhs._LinkDetailsWithBase == rhs._LinkDetailsWithBase &&
            lhs._EmissiveColorMode == rhs._EmissiveColorMode &&
            lhs._UVEmissive == rhs._UVEmissive;
    }

    struct MaterialRes;

    MaterialRes ToMaterialRes(const StandardLightMaterial& pbrMaterial, const std::string shaderName);
    StandardLightMaterial ToStandardMaterial(const MaterialRes& materialRes);

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
        return
            lhs.m_albedo_file == rhs.m_albedo_file &&
            lhs.m_ao_roughness_metallic_file == rhs.m_ao_roughness_metallic_file &&
            lhs.m_displacement_file == rhs.m_displacement_file &&
            lhs.m_normal_file == rhs.m_normal_file;
    }

    struct TerrainMaterial
    {
        TerrainBaseTex m_base_texs[2];
    };
    inline bool operator==(const TerrainMaterial& lhs, const TerrainMaterial& rhs)
    {
        return
            lhs.m_base_texs[0] == rhs.m_base_texs[0] &&
            lhs.m_base_texs[1] == rhs.m_base_texs[1];
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
