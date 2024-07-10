#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Material/Lit/LitProperties.hlsl"

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
};

struct VertexOutput
{
    float4 cs_pos : SV_POSITION;
};

VertexOutput VSMain(VertexInput input)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float4> terrainHeightmap = ResourceDescriptorHeap[terrainHeightmapIndex];
    StructuredBuffer<ClipmapTransform> clipmapTransformBuffers = ResourceDescriptorHeap[transformBufferIndex];
    
    ClipmapTransform clipTransform = clipmapTransformBuffers[clipmapIndex];
    
    float4x4 tLocalTransform = clipTransform.transform;
    
    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    
    HDDirectionalShadowData dirShadowData = mFrameUniforms.lightDataUniform.directionalShadowData;
    uint shadowCascadeIndex = dirShadowData.shadowDataIndex[cascadeLevel];
    HDShadowData shadowData = mFrameUniforms.lightDataUniform.shadowDatas[shadowCascadeIndex];
    float4x4 viewProjMatrix = shadowData.viewProjMatrix;
    
    float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;
    float terrainSize = mFrameUniforms.terrainUniform.terrainSize;
    
    float3 localPosition = input.position;
    localPosition = mul(tLocalTransform, float4(localPosition, 1)).xyz;
    float3 worldPosition = mul(localToWorldMatrix, float4(localPosition, 1)).xyz;
    
    float2 terrainUV = (worldPosition.xz) / float2(terrainSize, terrainSize);
    float curHeight = terrainHeightmap.SampleLevel(defaultSampler, terrainUV, 0).b;
    
    worldPosition.y += curHeight * terrainMaxHeight;
    
    VertexOutput output;
    output.cs_pos = mul(viewProjMatrix, float4(worldPosition, 1.0f));
    
    return output;
}
