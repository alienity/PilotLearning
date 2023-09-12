#include "IBLHelper.hlsli"

cbuffer Constants : register(b0)
{
    int _IBLSpecularIndex;
    int _LDOutputIndex;

    uint _LodLevel;
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
    
    float3 direction = normalize(float3(DTid.x, DTid.y, DTid.z));

    float4 ldVal = roughnessFilter(m_IBLSpecular, defaultSampler, 1200, direction, _roughness);

    CubemapAddress addr = getAddressForCubemap(direction); 

    uint3 outputPos = uint3(addr.st.x * curWidth, addr.st.y * curWidth, addr.face);

    m_OutputLDTex[outputPos] = ldVal;
}