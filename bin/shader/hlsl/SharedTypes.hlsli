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

struct PerMaterialUniformBufferObject
{
    float4 baseColorFactor;

    float metallicFactor;
    float roughnessFactor;
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
    float3   cameraPosition;
    float    _padding_cameraPosition;
};

struct MeshPerframeStorageBufferObject
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
