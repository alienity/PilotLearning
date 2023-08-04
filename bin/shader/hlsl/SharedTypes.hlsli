#pragma once
#include "CommonMath.hlsli"
#include "d3d12.hlsli"

#define m_max_point_light_count 16
#define m_max_spot_light_count 16

// ==================== Material ====================
/*
struct Material
{
	uint   BSDFType;
	float3 baseColor;
	float  metallic;
	float  subsurface;
	float  specular;
	float  roughness;
	float  specularTint;
	float  anisotropic;
	float  sheen;
	float  sheenTint;
	float  clearcoat;
	float  clearcoatGloss;

	// Used by Glass BxDF
	float3 T;
	float  etaA, etaB;

	// Texture indices
	int Albedo;
};
*/

// https://google.github.io/filament/Material%20Properties.pdf
/*
> BASE COLOR/SRGB
    Defines the perceived color of an object (sometimes called albedo). More precisely:
    ¡ú the diffuse color of a non-metallic object
    ¡ú the specular color of a metallic object
> METALLIC/GRAYSCALE
    Defines whether a surface is dielectric (0.0, non-metal) or conductor (1.0, metal).
    Pure, unweathered surfaces are rare and will be either 0.0 or 1.0.
    Rust is not a conductor.
> ROUGHNESS/GRAYSCALE
    Defines the perceived smoothness (0.0) or roughness (1.0).
    It is sometimes called glossiness.
> REFLECTANCE/GRAYSCALE
    Specular intensity for non-metals. The default is 0.5, or 4% reflectance.
> CLEAR COAT/GRAYSCALE
    Strength of the clear coat layer on top of a base dielectric or conductor layer.
    The clear coat layer will commonly be set to 0.0 or 1.0.
    This layer has a fixed index of refraction of 1.5.
> CLEAR COAT ROUGHNESS/GRAYSCALE
    Defines the perceived smoothness (0.0) or roughness (1.0) of the clear coat layer.
    It is sometimes called glossiness.
    This may affect the roughness of the base layer.
> ANISOTROPY/GRAYSCALE
    Defines whether the material appearance is directionally dependent, that is isotropic (0.0)
    or anisotropic (1.0). Brushed metals are anisotropic.
    Values can be negative to change the orientation of the specular reflections.
*/

struct PerMaterialUniformBuffer
{
    float4 baseColorFactor;

    float metallicFactor;
    float roughnessFactor;
    float reflectanceFactor;
    float clearCoatFactor;
    float clearCoatRoughnessFactor;
    float anisotropyFactor;
    float normalScale;
    float occlusionStrength;

    float3 emissiveFactor;
    uint   is_blend;
    uint   is_double_sided;

    uint _padding_uniform_1;
    uint _padding_uniform_2;
    uint _padding_uniform_3;
};

struct MaterialInstance
{
    uint uniformBufferIndex;
    uint baseColorIndex;
    uint metallicRoughnessIndex;
    uint normalIndex;
    uint occlusionIndex;
    uint emissionIndex;

    uint _padding_material_1;
    uint _padding_material_2;
};

// ==================== Light ====================
struct ScenePointLight
{
    float3 position;
    float  radius;
    float3 color;
    float intensity;
};

struct SceneSpotLight
{
    float3   position;
    float    radius;
    float3   color;
    float    intensity;
    float3   direction;
    float    _padding_direction;
    float    inner_radians;
    float    outer_radians;
    float2   _padding_radians;
    uint     shadowmap;           // 1 - use shadowmap, 0 - do not use shadowmap
    uint     shadowmap_srv_index; // shadowmap srv in descriptorheap index
    float    shadowmap_width;
    float    _padding_shadowmap;
    float4x4 light_proj_view;
};

struct SceneDirectionalLight
{
    float3   direction;
    float    _padding_0;
    float3   color;
    float    intensity;
    uint     shadowmap;           // 1 - use shadowmap, 0 - do not use shadowmap
    uint     cascade;             // how many cascade level, default 4
    float    shadowmap_width;     // shadowmap width and height
    uint     shadowmap_srv_index; // shadowmap srv in descriptorheap index
    uint4    shadow_bounds;       // shadow bounds for each cascade level
    float4x4 light_view_matrix;   // direction light view matrix
    float4x4 light_proj[4];
    float4x4 light_proj_view[4];
};

// ==================== Mesh ====================
struct MeshInstance
{
	// 16
    float enableVertexBlending;
    float _padding_enable_vertex_blending_1;
    float _padding_enable_vertex_blending_2;
    float _padding_enable_vertex_blending_3;

	// 64
	float4x4 localToWorldMatrix;
    // 64
    float4x4 localToWorldMatrixInverse;
	
	// 16
	D3D12_VERTEX_BUFFER_VIEW vertexBuffer;
	// 16
	D3D12_INDEX_BUFFER_VIEW indexBuffer;

	// 20
	D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;

    // 12
    float _padding_drawArguments0;
    float _padding_drawArguments1;
    float _padding_drawArguments2;

    // 32
    BoundingBox boundingBox;

	// 16
    uint materialIndex;
    uint vertexView;
    uint indexView;
    uint _padding_materialIndex2;
};

// ==================== Camera ====================
struct CameraInstance
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 projViewMatrix;
    float4x4 viewMatrixInverse;
    float4x4 projMatrixInverse;
    float4x4 projViewMatrixInverse;
    float3   cameraPosition;
    float    _padding_cameraPosition;
};

struct MeshPerframeBuffer
{
    CameraInstance        cameraInstance;
    float3                ambient_light;
    float                 _padding_ambient_light;
    uint                  point_light_num;
    uint                  spot_light_num;
    uint                  total_mesh_num;
    uint                  _padding_point_light_num_3;
    ScenePointLight       scene_point_lights[m_max_point_light_count];
    SceneSpotLight        scene_spot_lights[m_max_spot_light_count];
    SceneDirectionalLight scene_directional_light;
};
