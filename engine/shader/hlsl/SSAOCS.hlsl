#include "SSAO.hlsli"
#include "InputTypes.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint normalIndex;
    uint depthIndex;
    uint ssaoIndex;
};
ConstantBuffer<FrameUniforms> g_FrameUniform : register(b1, space0);

SamplerState defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

[numthreads( 8, 8, 1 )]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalIndex];
    Texture2D<float4> depthTexture = ResourceDescriptorHeap[depthIndex];
    RWTexture2D<float4> ssaoTexture = ResourceDescriptorHeap[ssaoIndex];

    uint width, height;
    ssaoTexture.GetDimensions(width, height);

    float2 uv = float2(DTid.x + 0.5f, DTid.y + 0.5f)/float2(width, height);

    float ao = CustomSSAO(uv);
    ao = 1 - ao;
    float4 color = float4(ao, ao, ao, 1);

    outColorTexture[DTid.xy] = color;
}
