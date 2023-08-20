#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "InputTypes.hlsli"
#include "d3d12.hlsli"

// Convert rgb to luminance
// with rgb in linear space with sRGB primaries and D65 white point
float Luminance(float3 linearRgb) { return dot(linearRgb, float3(0.2126729, 0.7151522, 0.0721750)); }

cbuffer Constants : register(b0)
{
    uint _BaseTexIndex;
    uint _RWOutputTexIndex;
};

[numthreads(8, 8, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    Texture2D<float4> m_BaseMapTex = ResourceDescriptorHeap[_BaseTexIndex];
    RWTexture2D<float4> m_OutputMapTex = ResourceDescriptorHeap[_RWOutputTexIndex];

    uint width, height;
    m_BaseMapTex.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    float4 texColor = m_BaseMapTex[DTid.xy];
    texColor.a = Luminance(saturate(texColor.rgb));

    m_OutputMapTex[DTid.xy] = texColor;
}
