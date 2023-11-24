#include "d3d12.hlsli"
// #include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

#define EAST 1
#define SOUTH 2
#define WEST 4
#define NORTH 8

cbuffer RootConstants : register(b0, space0) 
{ 
    uint cascadeLevel;
    uint terrainPatchNodeIndex;
    uint meshPerFrameBufferIndex;
    uint terrainHeightmapIndex;
    uint terrainDrawIdxBufferIndex;
};

SamplerState defaultSampler : register(s10);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
    float4 color    : COLOR;
    
    uint instanceID : SV_InstanceID;
};

struct VertexOutput
{
    float4 vertex_position : SV_POSITION;
    float4 vertex_worldPosition : TEXCOORD0;
    float2 vertex_uv01 : TEXCOORD1;
};

VertexOutput VSMain(VertexInput input)
{
    StructuredBuffer<TerrainPatchNode> mTerrainPatchNodes = ResourceDescriptorHeap[terrainPatchNodeIndex];
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[meshPerFrameBufferIndex];

    Texture2D<float4> terrainHeightmap = ResourceDescriptorHeap[terrainHeightmapIndex];

    StructuredBuffer<uint> mDrawIdxBuffer = ResourceDescriptorHeap[terrainDrawIdxBufferIndex];

    uint patchNodeIndex = mDrawIdxBuffer[input.instanceID];

    TerrainPatchNode _terrainPatchNode = mTerrainPatchNodes[patchNodeIndex];

    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    float4x4 localToWorldMatrixInv = mFrameUniforms.terrainUniform.world2LocalMatrix;

    float4x4 projectionViewMatrix = mFrameUniforms.directionalLight.directionalLightShadowmap.light_proj_view[cascadeLevel];

    float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;
    float terrainSize = mFrameUniforms.terrainUniform.terrainSize;

    float3 localPosition = input.position;

    if(_terrainPatchNode.neighbor & EAST)
    {
        localPosition += -float3(input.color.a, 0, 0);
    }
    if(_terrainPatchNode.neighbor & SOUTH)
    {
        localPosition += float3(0, 0, input.color.g);
    }
    if(_terrainPatchNode.neighbor & WEST)
    {
        localPosition += float3(input.color.b, 0, 0);
    }
    if(_terrainPatchNode.neighbor & NORTH)
    {
        localPosition += -float3(0, 0, input.color.g);
    }

    localPosition = localPosition * _terrainPatchNode.nodeWidth + 
        float3(_terrainPatchNode.patchMinPos.x, 0, _terrainPatchNode.patchMinPos.y);

    float2 terrainUV = float2(localPosition.x, localPosition.z) / float2(terrainSize + 1, terrainSize + 1);
    float curHeight = terrainHeightmap.SampleLevel(defaultSampler, terrainUV, 0).b;

    localPosition.y = curHeight * terrainMaxHeight;

    // localPosition.xyz *= 0.01f;
    localPosition.y += 1.5f;

    VertexOutput output;
    output.vertex_worldPosition = mul(localToWorldMatrix, float4(localPosition, 1.0f));
    output.vertex_position = mul(projectionViewMatrix, output.vertex_worldPosition);
    output.vertex_uv01 = terrainUV;

    return output;
}
