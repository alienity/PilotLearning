#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

cbuffer PSConstants : register(b0)
{
    int   RadianceIBLTexIndex;
    float TextureLevel;
};

SamplerState cubeMapSampler : register(s10);

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 viewDir : TEXCOORD3;
};

float4 PSMain(VSOutput vsOutput) : SV_Target0
{
    TextureCube<float3> radianceIBLTexture = ResourceDescriptorHeap[RadianceIBLTexIndex];

    return float4(radianceIBLTexture.SampleLevel(cubeMapSampler, vsOutput.viewDir, 0), 1);
}
