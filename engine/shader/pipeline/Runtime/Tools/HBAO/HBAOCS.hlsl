#include "HBAO.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint normalIndex;
    uint depthIndex;
    uint hbaoIndex;
};
ConstantBuffer<FrameUniforms> g_FrameUniform : register(b1, space0);

SamplerState defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

float toLinearZ(float fz, float m22, float m23)
{
    return -m23/(fz+m22);
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalIndex];
    Texture2D<float4> depthTexture = ResourceDescriptorHeap[depthIndex];
    RWTexture2D<float4> hbaoTexture = ResourceDescriptorHeap[hbaoIndex];

    uint width, height;
    hbaoTexture.GetDimensions(width, height);

    if(DTid.x >= width || DTid.y >= height)
        return;

    float2 uv = float2(DTid.x + 0.5f, DTid.y + 0.5f)/float2(width, height);

    HBAOInput _input;
    _input.frameUniforms = g_FrameUniform;
    _input.depthTex = depthTexture;
    _input.worldNormalMap = normalTexture;
    _input.defaultSampler = defaultSampler;

    float ao = CustomHBAO(_input, uv, uint2(width, height));
    ao = 1 - ao;
    float4 outAO = float4(ao, 0, 0, 1.0f);
    hbaoTexture[DTid.xy] = outAO;
}
