#include "IBLHelper.hlsli"

cbuffer Constants : register(b0)
{
    int _DFGOutputIndex;
};

SamplerState defaultSampler : register(s10);

[numthreads(8, 8, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID) {

    RWTexture2D<float4> m_OutputLDTex = ResourceDescriptorHeap[_DFGOutputIndex];

    uint width, height;
    m_OutputLDTex.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;
    
    int2 dstSize = int2(width, height);
    int2 inTexPos = int2(DTid.x, DTid.y);

    float3 dfgVal = DFG(dstSize, inTexPos, false);

    m_OutputLDTex[inTexPos] = float4(dfgVal.rgb, 0);
}