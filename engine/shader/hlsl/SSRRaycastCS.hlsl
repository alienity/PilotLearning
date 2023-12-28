#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "SSRLib.hlsli"

#define KERNEL_SIZE 16

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint raycastStructIndex;
    uint worldNormalIndex;
    uint worldTangentIndex;
    uint matNormalIndex;
    uint metallic_Roughness_Reflectance_AO_Index;
    uint minDepthBufferIndex;
    uint blueNoiseIndex;
    uint RaycastResultIndex;
    uint MaskResultIndex;
};

struct RaycastStruct
{
    float4 ScreenSize;
    float4 ResolveSize;
    float4 RayCastSize;
    float4 JitterSizeAndOffset;
    float4 NoiseSize;
    float BRDFBias;
    float TResponseMin;
    float TResponseMax;
    float TScale;
    float EdgeFactor;
    float Thickness;
    int NumSteps;
    int MaxMipMap;
};

SamplerState defaultSampler : register(s10);

float clampDelta(float2 start, float2 end, float2 delta)
{
    float2 dir = abs(end - start);
    return length(float2(min(dir.x, delta.x), min(dir.y, delta.y)));
}

float4 rayMarch(float3 viewPos, float3 viewDir, float3 screenPos, float2 uv, int numSteps, float thickness, 
    float4x4 projectionMatrix, float4 rayCastSize, float4 zBufferParams, Texture2D<float4> minDepthTex)
{
    float4 rayProj = mul(projectionMatrix, float4(viewPos + viewDir, 1.0f));
    float3 rayDir = normalize(rayProj.xyz / rayProj.w - screenPos);
    rayDir.xy *= 0.5;
    float3 rayStart = float3(uv, screenPos.z);
    float2 screenDelta2 = rayCastSize.zw;
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
        float sampleMinDepth = minDepthTex.SampleLevel(defaultSampler, nextSamplePos.xy, level).r;
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
            float delta = (LinearEyeDepth(sampleMinDepth, zBufferParams)) - (LinearEyeDepth(samplePos.z, zBufferParams));
            mask = delta <= thickness && i > 0;
            return float4(samplePos, mask);
        }
    }
    return float4(samplePos, mask);
}

// Brian Karis, Epic Games "Real Shading in Unreal Engine 4"
float4 importanceSampleGGX(float2 Xi, float Roughness)
{
    float m = Roughness * Roughness;
    float m2 = m * m;
		
    float Phi = 2 * F_PI * Xi.x;
				 
    float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (m2 - 1.0) * Xi.y));
    float SinTheta = sqrt(max(1e-5, 1.0 - CosTheta * CosTheta));
				 
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;
		
    float d = (CosTheta * m2 - CosTheta) * CosTheta + 1;
    float D = m2 / (F_PI * d * d);
    float pdf = D * CosTheta;

    return float4(H, pdf);
}

[numthreads(KERNEL_SIZE, KERNEL_SIZE, 1)]
void CSRaycast(uint3 id : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<RaycastStruct> mRaycastStruct = ResourceDescriptorHeap[raycastStructIndex];
    Texture2D<float4> worldNormalMap = ResourceDescriptorHeap[worldNormalIndex];
    Texture2D<float4> worldTangentMap = ResourceDescriptorHeap[worldTangentIndex];
    Texture2D<float4> matNormalMap = ResourceDescriptorHeap[matNormalIndex];
    Texture2D<float4> mrraMap = ResourceDescriptorHeap[metallic_Roughness_Reflectance_AO_Index];
    Texture2D<float4> minDepthMap = ResourceDescriptorHeap[minDepthBufferIndex];
    Texture2D<float4> blueNoiseMap = ResourceDescriptorHeap[blueNoiseIndex];

    RWTexture2D<float4> RaycastResult = ResourceDescriptorHeap[RaycastResultIndex];
    RWTexture2D<float> MaskResult = ResourceDescriptorHeap[MaskResultIndex];

    float4x4 worldToCameraMatrix = mFrameUniforms.cameraUniform.curFrameUniform.viewFromWorldMatrix;
    float4x4 viewProjectionMatrixInv = mFrameUniforms.cameraUniform.curFrameUniform.worldFromClipMatrix;
    float4x4 projectionMatrix = mFrameUniforms.cameraUniform.curFrameUniform.clipFromViewMatrix;
    float4x4 projectionMatrixInv = mFrameUniforms.cameraUniform.curFrameUniform.viewFromClipMatrix;

    float4 zBufferParams = mFrameUniforms.cameraUniform.curFrameUniform.zBufferParams;

    float4 rayCastSize = mRaycastStruct.RayCastSize;    
    float2 uv = (id.xy + 0.5) * rayCastSize.zw;

    float3 n = worldNormalMap.Sample(defaultSampler, uv).rgb * 2.0f - 1.0f;
    float4 vertex_worldTangent = worldTangentMap.Sample(defaultSampler, uv).xyzw * 2.0f - 1.0f;
    float3 t = vertex_worldTangent.xyz;
    float3 b = cross(n, t) * vertex_worldTangent.w;
    float3x3 tangentToWorld = transpose(float3x3(t, b, n));
    float3 matNormal = matNormalMap.Sample(defaultSampler, uv).rgb * 2.0f - 1.0f;
    float3 worldNormal = normalize(mul(tangentToWorld, matNormal));

    float3 viewNormal = normalize(mul((float3x3)worldToCameraMatrix, worldNormal));
    float roughness = mrraMap.SampleLevel(defaultSampler, uv, 0).y;

    float depth = minDepthMap.SampleLevel(defaultSampler, uv, 0).r;
    float3 screenPos = float3(uv * 2 - 1, depth);
    float3 worldPos = getWorldPos(screenPos, viewProjectionMatrixInv);
    float3 viewPos = getViewPos(screenPos, projectionMatrixInv);

    // Blue noise generated by https://github.com/bartwronski/BlueNoiseGenerator/
    float2 blueNoiseUV = (uv + mRaycastStruct.JitterSizeAndOffset.zw) * mRaycastStruct.ScreenSize.xy * mRaycastStruct.NoiseSize.zw;
    float2 jitter = blueNoiseMap.SampleLevel(defaultSampler, blueNoiseUV, 0).xy;

    float2 Xi = jitter;

    Xi.y = lerp(Xi.y, 0.0, mRaycastStruct.BRDFBias);

    float4 H = importanceSampleGGX(Xi, roughness) * float4(viewNormal.xyz, 1.0f);
    float3 dir = reflect(normalize(viewPos), H.xyz);

    int numSteps = mRaycastStruct.NumSteps;
    float thickness = mRaycastStruct.Thickness;

    float4 rayTrace = rayMarch(viewPos, dir, screenPos, uv, numSteps, thickness, 
        projectionMatrix, rayCastSize, zBufferParams, minDepthMap);
    
    float2 rayTraceHit = rayTrace.xy;
    float rayTraceZ = rayTrace.z;
    float rayPDF = H.w;
    float rayMask = rayTrace.w;
    
    RaycastResult[id.xy] = float4(rayTraceHit, rayTraceZ, rayPDF);
    MaskResult[id.xy] = rayMask;
}
