#pragma once
#include "Math.hlsli"
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
    float3    direction;
    float     _padding_direction;
    float3    color;
    float     intensity;
    uint      shadowmap;           // 1 - use shadowmap, 0 - do not use shadowmap
    uint      shadowmap_srv_index; // shadowmap srv in descriptorheap index
    float     shadowmap_width;
    float     _padding_shadowmap;
    float4x4  light_proj_view;
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

//-------------------------------------------------------------------------------------

struct MaterialInputs
{
    float4 baseColor;         // default: float4(1.0)
    float4 emissive;          // default: float4(0.0, 0.0, 0.0, 1.0)
    float4 postLightingColor; // default: float4(0.0)

    // no other field is available with the unlit shading model
    float roughness;        // default: 1.0
    float metallic;         // default: 0.0, not available with cloth or specularGlossiness
    float reflectance;      // default: 0.5, not available with cloth or specularGlossiness
    float ambientOcclusion; // default: 0.0

    // not available when the shading model is subsurface or cloth
    float3 sheenColor;          // default: float3(0.0)
    float  sheenRoughness;      // default: 0.0
    float  clearCoat;           // default: 1.0
    float  clearCoatRoughness;  // default: 0.0
    float3 clearCoatNormal;     // default: float3(0.0, 0.0, 1.0)
    float  anisotropy;          // default: 0.0
    float3 anisotropyDirection; // default: float3(1.0, 0.0, 0.0)

    // only available when the shading model is subsurface or refraction is enabled
    float thickness; // default: 0.5

    // only available when the shading model is subsurface
    float  subsurfacePower; // default: 12.234
    float3 subsurfaceColor; // default: float3(1.0)

    // only available when the shading model is cloth
    float3 clothSheenColor;      // default: sqrt(baseColor)
    float3 clothSubsurfaceColor; // default: float3(0.0)

    // only available when the shading model is specularGlossiness
    float3 specularColor; // default: float3(0.0)
    float  glossiness;    // default: 0.0

    // not available when the shading model is unlit
    // must be set before calling prepareMaterial()
    float3 normal; // default: float3(0.0, 0.0, 1.0)

    // only available when refraction is enabled
    float  transmission;   // default: 1.0
    float3 absorption;     // default float3(0.0, 0.0, 0.0)
    float  ior;            // default: 1.5
    float  microThickness; // default: 0.0, not available with refractionType "solid"
};

struct ShadingData
{
    // The material's diffuse color, as derived from baseColor and metallic.
    // This color is pre-multiplied by alpha and in the linear sRGB color space.
    float3 diffuseColor;

    // The material's specular color, as derived from baseColor and metallic.
    // This color is pre-multiplied by alpha and in the linear sRGB color space.
    float3 f0;

    // The perceptual roughness is the roughness value set in MaterialInputs,
    // with extra processing:
    // - Clamped to safe values
    // - Filtered if specularAntiAliasing is enabled
    // This value is between 0.0 and 1.0.
    float perceptualRoughness;

    // The roughness value expected by BRDFs. This value is the square of
    // perceptualRoughness. This value is between 0.0 and 1.0.
    float roughness;
};

struct LightData
{
    // The color (.rgb) and pre-exposed intensity (.w) of the light.
    // The color is an RGB value in the linear sRGB color space.
    // The pre-exposed intensity is the intensity of the light multiplied by
    // the camera's exposure value.
    float4 colorIntensity;

    // The normalized light vector, in world space (direction from the
    // current fragment's position to the light).
    float3 l;

    // The dot product of the shading normal (with normal mapping applied)
    // and the light vector. This value is equal to the result of
    // saturate(dot(getWorldSpaceNormal(), lightData.l)).
    // This value is always between 0.0 and 1.0. When the value is <= 0.0,
    // the current fragment is not visible from the light and lighting
    // computations can be skipped.
    float NdotL;

    // The position of the light in world space.
    float3 worldPosition;

    // Attenuation of the light based on the distance from the current
    // fragment to the light in world space. This value between 0.0 and 1.0
    // is computed differently for each type of light (it's always 1.0 for
    // directional lights).
    float attenuation;

    // Visibility factor computed from shadow maps or other occlusion data
    // specific to the light being evaluated. This value is between 0.0 and
    // 1.0.
    float visibility;
};
