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
    // ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    // Texture2D<float4> terrainHeightmap = ResourceDescriptorHeap[terrainHeightmapIndex];
    // StructuredBuffer<ClipmapTransform> clipmapTransformBuffers = ResourceDescriptorHeap[transformBufferIndex];
    //
    // ClipmapTransform clipTransform = clipmapTransformBuffers[clipmapIndex];
    //
    // float4x4 tLocalTransform = clipTransform.transform;
    //
    // float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    // float4x4 projectionViewMatrix = mFrameUniforms.directionalLight.directionalLightShadowmap.light_proj_view[cascadeLevel];
    //
    // float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;
    // float terrainSize = mFrameUniforms.terrainUniform.terrainSize;
    //
    // float3 localPosition = input.position;
    // localPosition = mul(tLocalTransform, float4(localPosition, 1)).xyz;
    // float3 worldPosition = mul(localToWorldMatrix, float4(localPosition, 1)).xyz;
    //
    // float2 terrainUV = (worldPosition.xz) / float2(terrainSize, terrainSize);
    // float curHeight = terrainHeightmap.SampleLevel(defaultSampler, terrainUV, 0).b;
    //
    // worldPosition.y += curHeight * terrainMaxHeight;
    //
    // VertexOutput output;
    // output.cs_pos = mul(projectionViewMatrix, float4(worldPosition, 1.0f));

    VertexOutput output;
    output.cs_pos = float4(input.position.x, input.position.y, 0, 1);
    
    return output;
}
