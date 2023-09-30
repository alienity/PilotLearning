#include "IBLHelper.hlsli"

cbuffer Constants : register(b0)
{
    int _IBLSpecularIndex;
    int _LDOutputIndex;
};

SamplerState defaultSampler : register(s10);

[numthreads(8, 8, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID) {

    TextureCube<float4> m_IBLSpecular = ResourceDescriptorHeap[_IBLSpecularIndex];
    RWTexture2DArray<float4> m_OutputLDTex = ResourceDescriptorHeap[_LDOutputIndex];

    uint width, height, elements;
    m_OutputLDTex.GetDimensions(width, height, elements);

    if (DTid.x >= width || DTid.y >= height || DTid.z >= elements)
        return;

    float2 texXY = float2((DTid.x + 0.5f)/(float)width, (DTid.y + 0.5f)/(float)height) * 2.0f - 1.0f;
    uint face = DTid.z;

    float3 direction = getDirectionForCubemap(face, texXY);
    direction = normalize(direction);

    float3 ldVal = diffuseIrradiance(m_IBLSpecular, defaultSampler, 4096, direction).rgb;

    uint3 outputPos = uint3(DTid.xyz);

    m_OutputLDTex[outputPos] = float4(ldVal, 1.0);
}