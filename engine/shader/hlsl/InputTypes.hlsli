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
    float2 _padding0;

    float2 base_color_tilling;
    float2 metallic_roughness_tilling;
    float2 normal_tilling;
    float2 occlusion_tilling;
    float2 emissive_tilling;
    
    uint   is_blend;
    uint   is_double_sided;
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

	// 64
    float4x4 prevWorldFromModelMatrix;
    // 64
    float4x4 prevModelFromWorldMatrix;
	
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

struct FrameCameraUniform
{
    float4x4 viewFromWorldMatrix; // clip    view <- world    : view matrix
    float4x4 worldFromViewMatrix; // clip    view -> world    : model matrix
    float4x4 clipFromViewMatrix;  // clip <- view    world    : projection matrix
    float4x4 viewFromClipMatrix;  // clip -> view    world    : inverse projection matrix
    float4x4 unJitterProjectionMatrix; // Unjitter projection matrix
    float4x4 unJitterProjectionMatrixInv; // Unjitter projection matrix inverse
    float4x4 clipFromWorldMatrix; // clip <- view <- world
    float4x4 worldFromClipMatrix; // clip -> view -> world
    float4   clipTransform;       // [sx, sy, tx, ty] only used by VERTEX_DOMAIN_DEVICE
    float3   cameraPosition;      // camera world position
    uint     jitterIndex;         // jitter index
};

struct CameraUniform
{
    FrameCameraUniform curFrameUniform;
    FrameCameraUniform lastFrameUniform;
    
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

struct TAAUniform
{
    float4 projectionExtents;  // xy = frustum extents at distance 1, zw = jitter at distance 1
    float4 jitterUV;
    float feedbackMin;
    float feedbackMax;
    float motionScale;
    float __Reserved0;
};

struct IBLUniform
{
    float4 iblSH[7]; // actually float3 entries (std140 requires float4 alignment)
    float iblLuminance;
    float iblRoughnessOneLevel; // level for roughness == 1
    int   dfg_lut_srv_index;    // specular lut dfg
    int   ld_lut_srv_index;     // specular lut ld
    int   radians_srv_index;    // diffuse
    float3 __Reserved0;
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
    uint2 shadowmap_size; // shadowmap size
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
    uint shadowType; // 1 use shadowmap
    float2 lightFarAttenuationParams; // a, a/far (a=1/pct-of-far)
    float2 _padding_0;
    
    DirectionalLightShadowmap directionalLightShadowmap;
};

struct PointLightShadowmap
{
    uint shadowmap_srv_index; // shadowmap srv in descriptorheap index
    uint2 shadowmap_size;
    float _padding_shadowmap;
    float4x4 light_proj_view[6];
};

struct PointLightStruct
{
    float3 lightPosition;
    float lightRadius;
    float4 lightIntensity; // point light rgb - color, a - intensity
    uint shadowType;
    float falloff; // default 1.0f
    uint2 _padding_0;

    PointLightShadowmap pointLightShadowmap;
};

struct PointLightUniform
{
    uint pointLightCounts;
    float3 _padding_0;
    PointLightStruct pointLightStructs[m_max_point_light_count];
};

struct SpotLightShadowmap
{
    uint shadowmap_srv_index; // shadowmap srv in descriptorheap index
    uint2 shadowmap_size;
    float _padding_0;
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
    uint shadowType;
    float falloff; // default 1.0f
    float _padding_0;

    SpotLightShadowmap spotLightShadowmap;
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

struct TerrainUniform
{
    uint terrainSize;      // 1024*1024
    uint terrainMaxHeight; // 1024
    uint heightMapIndex;
    uint normalMapIndex;
    float4x4 local2WorldMatrix;
    float4x4 world2LocalMatrix;
    float4x4 prevLocal2WorldMatrix;
    float4x4 prevWorld2LocalMatrix;
};

struct FrameUniforms
{
    CameraUniform cameraUniform;
    BaseUniform baseUniform;
    TerrainUniform terrainUniform;
    MeshUniform meshUniform;
    AOUniform aoUniform;
    TAAUniform taaUniform;
    IBLUniform iblUniform;
    SSRUniform ssrUniform;
    DirectionalLightStruct directionalLight;
    PointLightUniform pointLightUniform;
    SpotLightUniform spotLightUniform;
};

// =======================================
// Terrain
// =======================================

struct ClipmapIndexInstance
{
    float4x4 transform;
    int mesh_type;
};

struct TerrainPatchNode
{
    float2 patchMinPos; // node的左下角顶点
    float maxHeight; // 当前node最大高度
    float minHeight; // 当前node最小高度
    float nodeWidth; // patchnode的宽度
    int mipLevel; // 当前node的mip等级
    uint neighbor; // 更高一级mip作为邻居的标识
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