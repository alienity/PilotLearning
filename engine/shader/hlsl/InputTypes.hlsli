#ifndef __INPUT_TYPE_HLSLI__
#define __INPUT_TYPE_HLSLI__

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
    �� the diffuse color of a non-metallic object
    �� the specular color of a metallic object
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

struct PerMaterialParametersBuffer
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

    float2 base_color_tilling;
    float2 metallic_roughness_tilling;
    float2 normal_tilling;
    float2 occlusion_tilling;
    float2 emissive_tilling;

    float2 _padding_uniform_01;

    float emissiveFactor;
    uint   is_blend;
    uint   is_double_sided;

    uint _padding_uniform_02;
};

struct PerMaterialViewIndexBuffer
{
    uint parametersBufferIndex;
    uint baseColorIndex;
    uint metallicRoughnessIndex;
    uint normalIndex;
    uint occlusionIndex;
    uint emissionIndex;

    uint2 _padding_material_0;
};

// ==================== Mesh ====================
struct PerRenderableMeshData
{
	// 16
    float enableVertexBlending;
    float3 _padding_enable_vertex_blending;
    
	// 64
    float4x4 worldFromModelMatrix;
    // 64
    float4x4 modelFromWorldMatrix;
	
	// 16
	D3D12_VERTEX_BUFFER_VIEW vertexBuffer;
	// 16
	D3D12_INDEX_BUFFER_VIEW indexBuffer;

	// 20
	D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;

    // 12
    float3 _padding_drawArguments;

    // 32
    BoundingBox boundingBox;

	// 16
    uint perMaterialViewIndexBufferIndex;
    uint3 _padding_materialIndex2;
};

// ------------------------------------------------------------------------------------------------------------------------

// =======================================
// Here we define all the UBOs known as C structs. It is used to fill the uniform values 
// and get the interface block names. It is also used by filabridge to get the interface block names.
// =======================================

struct CameraUniform
{
    float4x4 viewFromWorldMatrix; // clip    view <- world    : view matrix
    float4x4 worldFromViewMatrix; // clip    view -> world    : model matrix
    float4x4 clipFromViewMatrix; // clip <- view    world    : projection matrix
    float4x4 viewFromClipMatrix; // clip -> view    world    : inverse projection matrix
    float4x4 clipFromWorldMatrix; // clip <- view <- world
    float4x4 worldFromClipMatrix; // clip -> view -> world
    float4 clipTransform; // [sx, sy, tx, ty] only used by VERTEX_DOMAIN_DEVICE
    float3 cameraPosition; // camera position
    float _baseReserved0;
    
    float4 resolution; // physical viewport width, height, 1/width, 1/height
    float2 logicalViewportScale; // scale-factor to go from physical to logical viewport
    float2 logicalViewportOffset; // offset to go from physical to logical viewport
    
    // camera position in view space (when camera_at_origin is enabled), i.e. it's (0,0,0).
    float cameraNear;
    float cameraFar; // camera *culling* far-plane distance, always positive (projection far is at +inf)
    float exposure;
    float ev100;
};

struct BaseUniform
{
    float2 clipControl; // clip control
    float time; // time in seconds, with a 1-second period
    float temporalNoise; // noise [0,1] when TAA is used, 0 otherwise
    float4 userTime; // time(s), (double)time - (float)time, 0, 0    
    float needsAlphaChannel;
    float lodBias; // load bias to apply to user materials
    float refractionLodOffset;
    float baseReserved0;
};

struct AOUniform
{
    float aoSamplingQualityAndEdgeDistance; // <0: no AO, 0: bilinear, !0: bilateral edge distance
    float aoBentNormals; // 0: no AO bent normal, >0.0 AO bent normals
    float aoReserved0;
    float aoReserved1;
};

struct IBLUniform
{
    float4 iblSH[9]; // actually float3 entries (std140 requires float4 alignment)
    float iblLuminance;
    float iblRoughnessOneLevel; // level for roughness == 1
    float iblReserved0;
    float iblReserved1;
};

struct SSRUniform
{
    float4x4 ssrReprojection;
    float4x4 ssrUvFromViewMatrix;
    float ssrThickness; // ssr thickness, in world units
    float ssrBias; // ssr bias, in world units
    float ssrDistance; // ssr world raycast distance, 0 when ssr is off
    float ssrStride; // ssr texel stride, >= 1.0
};

struct DirectionalLightShadowmap
{
    uint shadowmap_srv_index; // shadowmap srv in descriptorheap index    
    uint cascadeCount; // how many cascade level, default 4
    float shadowmap_width; // shadowmap width
    float shadowmap_height; // shadowmap height
    uint4 shadow_bounds; // shadow bounds for each cascade level
    float4x4 light_view_matrix; // direction light view matrix
    float4x4 light_proj_matrix[4];
    float4x4 light_proj_view[4];
};

struct DirectionalLightStruct
{
    float4 lightColorIntensity; // directional light rgb - color, a - intensity
    float3 lightPosition; // directional light position
    float lightRadius; // sun radius
    float3 lightDirection; // directional light direction
    uint useShadowmap; // 1 use shadowmap
    
    DirectionalLightShadowmap directionalLightShadowmap;
};

struct PointLightStruct
{
    float3 lightPosition;
    float lightRadius;
    float4 lightIntensity; // point light rgb - color, a - intensity
};

struct PointLightUniform
{
    uint pointLightCounts;
    float3 _padding_0;
    PointLightStruct pointLightStructs[m_max_point_light_count];
};

struct SpotLightSgadowmap
{
    uint shadowmap_srv_index; // shadowmap srv in descriptorheap index
    float shadowmap_width;
    float2 _padding_shadowmap;
    float4x4 light_proj_view;
};

struct SpotLightStruct
{
    float3 lightPosition;
    float lightRadius;
    float4 lightIntensity; // spot light rgb - color, a - intensity
    float3 lightDirection;
    float inner_radians;
    float outer_radians;
    uint useShadowmap;
    float2 _padding_0;

    SpotLightSgadowmap spotLightShadowmap;
};

struct SpotLightUniform
{
    uint spotLightCounts;
    float3 _padding_0;
    SpotLightStruct spotLightStructs[m_max_spot_light_count];
};

struct MeshUniform
{
    uint totalMeshCount;
    float3 _padding_0;
};

struct FrameUniforms
{
    CameraUniform cameraUniform;
    BaseUniform baseUniform;
    MeshUniform meshUniform;
    AOUniform aoUniform;
    IBLUniform iblUniform;
    SSRUniform ssrUniform;
    DirectionalLightStruct directionalLight;
    PointLightUniform pointLightUniform;
    SpotLightUniform spotLightUniform;
};


// =======================================
// Samplers
// =======================================

struct SamplerStruct
{
    SamplerState defSampler;
    SamplerComparisonState sdSampler;
};


#endif