#include "IBLHelper.hlsli"

cbuffer Constants : register(b0)
{
    int _IBLSpecularIndex;
    int _LDOutputIndex;

    int _LodLevel;
    float _roughness;
};

void getCubemapFaceUV(float3 direction, out uint faceIndex, out float2 uv)
{
    float3 absDirection = abs(direction);

    if (absDirection.x >= absDirection.y && absDirection.x >= absDirection.z)
    {
        if (direction.x > 0)
            faceIndex = 0; // Positive X face
        else
            faceIndex = 1; // Negative X face

        uv.x = -direction.z / absDirection.x;
        uv.y = direction.y / absDirection.x;
        uv = 0.5 * uv + 0.5;
    }
    else if (absDirection.y >= absDirection.x && absDirection.y >= absDirection.z)
    {
        if (direction.y > 0)
            faceIndex = 2; // Positive Y face
        else
            faceIndex = 3; // Negative Y face

        uv.x = direction.x / absDirection.y;
        uv.y = direction.z / absDirection.y;
        uv = 0.5 * uv + 0.5;
    }
    else
    {
        if (direction.z > 0)
            faceIndex = 4; // Positive Z face
        else
            faceIndex = 5; // Negative Z face

        uv.x = direction.x / absDirection.z;
        uv.y = direction.y / absDirection.z;
        uv = 0.5 * uv + 0.5;
    }
}

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

    float4 ldVal = LD(m_IBLSpecular, defaultSampler, direction, _roughness);

    uint faceIndex;
    float2 faceUV;
    getCubemapFaceUV(direction, faceIndex, faceUV); 

    uint3 outputPos = uint3(faceUV.x * curWidth, DTid.y * curWidth, faceIndex);

    m_OutputLDTex[outputPos] = ldVal;
}