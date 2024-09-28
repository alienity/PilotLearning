#ifndef TERRAIN_COMMON_INPUT
#define TERRAIN_COMMON_INPUT

#ifdef _CPP_MACRO_
#define uint glm::uint
#define uint2 glm::uvec2
#define uint3 glm::uvec3
#define uint4 glm::uvec4
#define int2 glm::int2
#define int3 glm::int3
#define int4 glm::int4
#define float2 glm::fvec2
#define float3 glm::fvec3
#define float4 glm::fvec4
#define float4x4 glm::float4x4
#define float2x4 glm::float2x4
#else
#include "../../ShaderLibrary/Common.hlsl"
#endif

//最大的LOD级别是5
#define MAX_TERRAIN_LOD 5
#define MAX_NODE_ID 34124

// 在 Max LOD 下，世界由5x5个Node组成
#define MAX_LOD_NODE_COUNT 5

//一个PatchMesh由16x16网格组成
#define PATCH_MESH_GRID_COUNT 16

//一个PatchMesh边长8米
#define PATCH_MESH_SIZE 8

//一个Node拆成8x8个Patch
#define PATCH_COUNT_PER_NODE 8

//PatchMesh一个格子的大小为0.5x0.5
#define PATCH_MESH_GRID_SIZE 0.5
#define SECTOR_COUNT_WORLD 160

struct TerrainRenderPatch
{
    float2 position;
    float minHeight;
    float maxHeight;
    uint4 lodTrans;
    uint lod;
};

struct TerrainPatchBounds
{
    float3 minPosition;
    float3 maxPosition;
};

struct TerrainRenderData
{
    float4x4 objectToWorldMatrix;
    float4x4 worldToObjectMatrix;
    float4x4 prevObjectToWorldMatrix;
    float4x4 prevWorldToObjectMatrix;
    float4 terrainSize;
};

struct TerrainPatchCmdSig
{
    
};

#ifdef _CPP_MACRO_
#undef uint
#undef uint2
#undef uint3
#undef uint4
#undef int2
#undef int3
#undef int4
#undef float2
#undef float3
#undef float4
#undef float4x4
#undef float2x4
#endif

#endif
