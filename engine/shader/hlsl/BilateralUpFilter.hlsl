#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint depthIndex;
    uint volume2DIndex;
}

SamplerState defaultSampler : register(s10);
SamplerState depthSampler : register(s11);

// https://github.com/SlightlyMad/VolumetricLights

// Z buffer to linear depth
float LinearEyeDepth(float z, float4 ZBufferParams)
{
    return 1.0 / (ZBufferParams.z * z + ZBufferParams.w);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float4> depthBuffer = ResourceDescriptorHeap[depthIndex];
    RWTexture2D<float4> volume2DTexture = ResourceDescriptorHeap[volume2DIndex];

    uint width, height;
    volume2DTexture.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    float4 zBufferParams = mFrameUniforms.cameraUniform.curFrameUniform.zBufferParams;

    float2 uv = (DTid.xy + 0.5) / float2(width, height);

    const float depth_delta = 0.00001f;
    float deviceDepth00 = saturate(LinearEyeDepth(depthBuffer.Sample(depthSampler, uv, 0).x)+depth_delta);
    float deviceDepth01 = saturate(LinearEyeDepth(depthBuffer.Sample(depthSampler, uv, 0).x)+0.00001f);
    float deviceDepth10 = saturate(LinearEyeDepth(depthUV+uint2(1,0))+0.00001f);
    float deviceDepth11 = saturate(LinearEyeDepth(depthUV+uint2(1,1))+0.00001f);



    m_OutputMapTex[DTid.xy] = ApplyFXAA(m_BaseMapTex, defaultSampler, baseMapTexelSize, uv);
}

