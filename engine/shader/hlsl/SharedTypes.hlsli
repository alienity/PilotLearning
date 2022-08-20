#pragma once
#include "Math.hlsli"
#include "d3d12.hlsli"

#define m_max_point_light_count 16

// ==================== Material ====================
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

// ==================== Light ====================
struct ScenePointLight
{
    float3 position;
    float  radius;
    float3 intensity;
    float  _padding_intensity;
};

struct SceneDirectionalLight
{
    float3 direction;
    float  _padding_direction;
    float3 color;
    float  _padding_color;
};

// ==================== Mesh ====================
struct Mesh
{
	// 64
	float4x4 Transform;
	// 64
	float4x4 PreviousTransform;

	// 16
	D3D12_VERTEX_BUFFER_VIEW VertexBuffer;
	// 16
	D3D12_INDEX_BUFFER_VIEW IndexBuffer;
	// 16
	D3D12_GPU_VIRTUAL_ADDRESS Meshlets;
	D3D12_GPU_VIRTUAL_ADDRESS UniqueVertexIndices;
	// 16
	D3D12_GPU_VIRTUAL_ADDRESS PrimitiveIndices;
	D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure;

	// 24
	BoundingBox BoundingBox;
	// 20
	D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;

	// 20
	unsigned int MaterialIndex;
	unsigned int NumMeshlets;
	unsigned int VertexView;
	unsigned int IndexView;
	unsigned int DEADBEEF2;
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
    unsigned int          point_light_num;
    unsigned int          total_mesh_num;
    unsigned int          _padding_point_light_num_2;
    unsigned int          _padding_point_light_num_3;
    ScenePointLight       scene_point_lights[m_max_point_light_count];
    SceneDirectionalLight scene_directional_light;
    float4x4              directional_light_proj_view;
};
