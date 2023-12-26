#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "SSRLib.hlsli"

#define KERNEL_SIZE 16
#define FLT_EPS 0.00000001f;

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint worldNormalIndex;
    uint worldTangentIndex;
    uint matNormalIndex;
    uint metallic_Roughness_Reflectance_AO_Index;
    uint minDepthBufferIndex;
    uint blueNoiseIndex;
};

SamplerState defaultSampler : register(s10);

float clampDelta(float2 start, float2 end, float2 delta)
{
    float2 dir = abs(end - start);
    return length(float2(min(dir.x, delta.x), min(dir.y, delta.y)));
}

float4 rayMatch(float3 viewPos, float3 viewDir, float3 screenPos, float2 uv, int numSteps, float thickness, 
    float4x4 projectionMatrix, float2 screenDelta2, float4 zBufferParams, Texture2D<float4> minDepthTex)
{
    float4 rayProj = mul(projectionMatrix, float4(viewPos + viewDir, 1.0f));
    float3 rayDir = normalize(rayProj.xyz / rayProj.w - screenPos);
    rayDir.xy *= 0.5;
    float3 rayStart = float3(uv, screenPos.z);
    float d = clampDelta(rayStart.xy, rayStart.xy + rayDir.xy, screenDelta2);
    float3 samplePos = rayStart + rayDir * d;
    int level = 0;
    
    float mask = 0;
    [loop]
    for (int i = 0; i < numSteps; i++)
    {
        float2 currentDelta = screenDelta2 * exp2(level + 1);
        float distance = clampDelta(samplePos.xy, samplePos.xy + rayDir.xy, currentDelta);
        float3 nextSamplePos = samplePos + rayDir * distance;
        float sampleMinDepth = minDepthTex.SampleLevel(defaultSampler, nextSamplePos.xy, level);
        float nextDepth = nextSamplePos.z;
        
        [flatten]
        if (sampleMinDepth < nextDepth)
        {
            level = min(level + 1, 6);
            samplePos = nextSamplePos;
        }
        else
        {
            level--;
        }
        
        [branch]
        if (level < 0)
        {
            float delta = (-LinearEyeDepth(sampleMinDepth, zBufferParams)) - (-LinearEyeDepth(samplePos.z, zBufferParams));
            mask = delta <= thickness && i > 0;
            return float4(samplePos, mask);
        }
    }
}

float3 getWorldPosition(float3 screenPos, float4x4 viewProjectionMatrixInv)
{
    float4 worldPos = mul(viewProjectionMatrixInv, float4(screenPos, 1));
    return worldPos.xyz / worldPos.w;
}

[numthreads(KERNEL_SIZE, KERNEL_SIZE, 1)]
void CSRaycast(uint3 id : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float4> worldNormalMap = ResourceDescriptorHeap[worldNormalIndex];
    Texture2D<float4> worldTangentMap = ResourceDescriptorHeap[worldTangentIndex];
    Texture2D<float4> matNormalMap = ResourceDescriptorHeap[matNormalIndex];
    Texture2D<float4> mrraMap = ResourceDescriptorHeap[metallic_Roughness_Reflectance_AO_Index];
    Texture2D<float4> minDepthMap = ResourceDescriptorHeap[minDepthBufferIndex];
    Texture2D<float4> blueNoiseMap = ResourceDescriptorHeap[blueNoiseIndex];

    float4x4 worldToCameraMatrix = mFrameUniforms.cameraUniform.curFrameUniform.viewFromWorldMatrix;

    float4 rayCastSize = mFrameUniforms.cameraUniform.resolution;    
    float2 uv = (id.xy + 0.5) * rayCastSize.zw;

    float3 worldNormal = worldNormalMap.SampleLevel(defaultSampler, uv, 0).xyz * 2 - 1;
    float3 viewNormal = normalize(mul((float3x3)worldToCameraMatrix, worldNormal));
    float roughness = mrraMap.SampleLevel(defaultSampler, uv, 0).y;

    float depth = minDepthMap.SampleLevel(defaultSampler, uv, 0).r;
    float3 screenPos = float3(uv * 2 - 1, depth);
    float3 worldPos = getWorldPosition();
    
    
}
