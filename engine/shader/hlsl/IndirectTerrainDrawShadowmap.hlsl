#include "d3d12.hlsli"
// #include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

cbuffer RootConstants : register(b0, space0) 
{
    uint clipmapIndex;
    
    uint cascadeLevel;
    uint transformBufferIndex;
    uint perFrameBufferIndex;
    uint terrainHeightmapIndex;
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
    float4 cs_pos : SV_POSITION;
    float4 ws_position : TEXCOORD0;
    float2 vertex_uv01 : TEXCOORD1;
};

VertexOutput VSMain(VertexInput input)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float4> terrainHeightmap = ResourceDescriptorHeap[terrainHeightmapIndex];
    StructuredBuffer<ClipmapTransform> clipmapTransformBuffers = ResourceDescriptorHeap[transformBufferIndex];

    ClipmapTransform clipTransform = clipmapTransformBuffers[clipmapIndex];

    float4x4 tLocalTransform = clipTransform.transform;
    // int tMeshType = clipTransform.mesh_type;

    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    float4x4 localToWorldMatrixInv = mFrameUniforms.terrainUniform.world2LocalMatrix;
    float4x4 projectionViewMatrix = mFrameUniforms.directionalLight.directionalLightShadowmap.light_proj_view[cascadeLevel];

    float4x4 projectionMatrix         = mFrameUniforms.cameraUniform.curFrameUniform.clipFromViewMatrix;
    float4x4 lastviewFromWorldMatrix  = mFrameUniforms.cameraUniform.lastFrameUniform.viewFromWorldMatrix;
    float4x4 projectionViewMatrixPrev = mul(projectionMatrix, lastviewFromWorldMatrix);

    float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;
    float terrainSize = mFrameUniforms.terrainUniform.terrainSize;

    float3 localPosition = input.position;
    localPosition = mul(tLocalTransform, float4(localPosition, 1)).xyz;
    float3 worldPosition = mul(localToWorldMatrix, float4(localPosition, 1)).xyz;
    
    float2 terrainUV = (worldPosition.xz) / float2(terrainSize, terrainSize);
    float curHeight = terrainHeightmap.SampleLevel(defaultSampler, terrainUV, 0).b;

    worldPosition.y += curHeight * terrainMaxHeight;

    VertexOutput output;
    output.ws_position = float4(worldPosition, 1.0f);
    output.cs_pos = mul(projectionViewMatrix, output.ws_position);
    output.vertex_uv01 = terrainUV;

    return output;
}
