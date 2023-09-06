#include "IBLHelper.hlsli"

cbuffer Constants : register(b0)
{
    int _IBLSpecularIndex;
    int _LDOutputIndex;
};

SamplerState defaultSampler : register(s10);

[numthreads(8, 8, 8)]
void CSMain(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID) {

    TextureCube<float4> m_IBLSpecular = ResourceDescriptorHeap[_IBLSpecularIndex];
    RWTexture2DArray<float4> m_OutputLDTex = ResourceDescriptorHeap[_LDOutputIndex];

    uint width, height, numberOfLevels;
    m_IBLSpecular.GetDimensions(0, width, height, numberOfLevels);

    if (DTid.x >= width || DTid.y >= height || DTid.z >= numberOfLevels)
        return;
    
    uint 




    


    float2 baseMapTexelSize = 1.0f / float2(width, height);
    float2 uv = DTid.xy * baseMapTexelSize;

    m_OutputMapTex[DTid.xy] = ApplyFXAA(m_BaseMapTex, defaultSampler, baseMapTexelSize, uv);
}

