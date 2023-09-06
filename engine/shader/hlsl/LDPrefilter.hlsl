#include "IBLHelper.hlsli"

cbuffer Constants : register(b0)
{
    int _IBLSpecularIndex;
    int _LDOutputIndex;

    int _LodLevel;
    float _roughness;
};

SamplerState defaultSampler : register(s10);

[numthreads(8, 8, 8)]
void CSMain(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID) {

    TextureCube<float4> m_IBLSpecular = ResourceDescriptorHeap[_IBLSpecularIndex];
    RWTexture2DArray<float4> m_OutputLDTex = ResourceDescriptorHeap[_LDOutputIndex];

    uint width, height;
    m_IBLSpecular.GetDimensions(width, height);

    uint curWidth = width >> _LodLevel;

    if (DTid.x >= curWidth || DTid.y >= curWidth || DTid.z >= curWidth)
        return;
    
    float3 localPos = float3(DTid.x, DTid.y, DTid.z);

    float4 ldVal = LD(m_IBLSpecular, defaultSampler, localPos, _roughness);

    



    


    float2 baseMapTexelSize = 1.0f / float2(width, height);
    float2 uv = DTid.xy * baseMapTexelSize;

    m_OutputMapTex[DTid.xy] = ApplyFXAA(m_BaseMapTex, defaultSampler, baseMapTexelSize, uv);
}

