#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "SSRLib.hlsli"

#define KERNEL_SIZE 16
#define RESOLVE_RAD 4
#define RESOLVE_RAD2 8

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint raycastStructIndex;
    uint worldNormalIndex;
    uint worldTangentIndex;
    uint matNormalIndex;
    uint metallic_Roughness_Reflectance_AO_Index;
    uint minDepthBufferIndex;
    uint ScreenInputIndex;
    uint RaycastInputIndex;
    uint MaskInputIndex;
    uint ResolveResultIndex;
};

SamplerState defaultSampler : register(s10);

groupshared uint rr_cacheR[(KERNEL_SIZE + RESOLVE_RAD2) * (KERNEL_SIZE + RESOLVE_RAD2)];
groupshared uint rr_cacheG[(KERNEL_SIZE + RESOLVE_RAD2) * (KERNEL_SIZE + RESOLVE_RAD2)];
groupshared uint rr_cacheB[(KERNEL_SIZE + RESOLVE_RAD2) * (KERNEL_SIZE + RESOLVE_RAD2)];
groupshared uint rr_cacheA[(KERNEL_SIZE + RESOLVE_RAD2) * (KERNEL_SIZE + RESOLVE_RAD2)];

void StoreResolveData(uint index, float4 raycast,float4 mask)
{
    rr_cacheR[index] = f32tof16(raycast.r) | f32tof16(mask.r) << 16;
    rr_cacheG[index] = f32tof16(raycast.g) | f32tof16(mask.g) << 16;
    rr_cacheB[index] = f32tof16(raycast.b) | f32tof16(mask.b) << 16;
    rr_cacheA[index] = f32tof16(raycast.a) | f32tof16(mask.a) << 16;
}

void LoadResolveData(uint index, out float4 raycast, out float4 mask)
{
    uint rr = rr_cacheR[index];
    uint gg = rr_cacheG[index];
    uint bb = rr_cacheB[index];
    uint aa = rr_cacheA[index];
    raycast = float4(f16tof32(rr), f16tof32(gg), f16tof32(bb), f16tof32(aa));
    mask = float4(f16tof32(rr >> 16), f16tof32(gg >> 16), f16tof32(bb >> 16), f16tof32(aa >> 16));
}

[numthreads( 8, 8, 1 )]
void CSResolve(uint3 groupId : SV_GroupId, uint groupIndex : SV_GroupIndex, uint3 groupThreadId : SV_GroupThreadID)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float4> worldNormalMap = ResourceDescriptorHeap[worldNormalIndex];
    Texture2D<float4> worldTangentMap = ResourceDescriptorHeap[worldTangentIndex];
    Texture2D<float4> matNormalMap = ResourceDescriptorHeap[matNormalIndex];
    Texture2D<float4> mrraMap = ResourceDescriptorHeap[metallic_Roughness_Reflectance_AO_Index];
    Texture2D<float4> minDepthMap = ResourceDescriptorHeap[minDepthBufferIndex];

    Texture2D<float4> ScreenInput = ResourceDescriptorHeap[ScreenInputIndex];
    Texture2D<float4> RaycastInput = ResourceDescriptorHeap[RaycastInputIndex];
    Texture2D<float4> MaskInput = ResourceDescriptorHeap[MaskInputIndex];
    
    RWTexture2D<float4> ResolveResult = ResourceDescriptorHeap[ResolveResultIndex];

    SSRUniform ssrUniform = mFrameUniforms.ssrUniform;

    float4 ResolveSize = ssrUniform.ResolveSize;
    float4 RayCastSize = ssrUniform.RayCastSize;

    uint2 uvInt = (groupId.xy * KERNEL_SIZE) + groupThreadId.xy - RESOLVE_RAD;
    float2 uv = (uvInt.xy + 0.5) * ResolveSize.zw;

    float4x4 worldToCameraMatrix = mFrameUniforms.cameraUniform.curFrameUniform.viewFromWorldMatrix;
    float4x4 viewProjectionMatrixInv = mFrameUniforms.cameraUniform.curFrameUniform.worldFromClipMatrix;
    float4x4 projectionMatrixInv = mFrameUniforms.cameraUniform.curFrameUniform.viewFromClipMatrix;

    float3 cameraPosition = mFrameUniforms.cameraUniform.curFrameUniform.cameraPosition;

    float3 n = worldNormalMap.Sample(defaultSampler, uv).rgb * 2.0f - 1.0f;
    float4 vertex_worldTangent = worldTangentMap.Sample(defaultSampler, uv).xyzw * 2.0f - 1.0f;
    float3 t = vertex_worldTangent.xyz;
    float3 b = cross(n, t) * vertex_worldTangent.w;
    float3x3 tangentToWorld = transpose(float3x3(t, b, n));
    float3 matNormal = matNormalMap.Sample(defaultSampler, uv).rgb * 2.0f - 1.0f;
    float3 worldNormal = normalize(mul(tangentToWorld, matNormal));

    float roughness = mrraMap.SampleLevel(defaultSampler, uv, 0).y;

    float depth = minDepthMap.SampleLevel(defaultSampler, uv, 0).r;
    float3 screenPos = float3(uv * 2 - 1, depth);
    float3 worldPos = getWorldPos(screenPos, viewProjectionMatrixInv);
    float3 viewPos = getViewPos(screenPos, projectionMatrixInv);
    float3 viewDir = normalize(worldPos - cameraPosition);

    float NdotV = saturate(dot(worldNormal.xyz, -viewDir));
    float coneTangent = lerp(0.0, roughness * (1.0 - ssrUniform.BRDFBias), NdotV * sqrt(roughness));
    coneTangent *= lerp(saturate(NdotV * 2), 1, sqrt(roughness));

    float4 ScreenSize = ssrUniform.ScreenSize;

    float maxMipLevel = (float)ssrUniform.MaxMipMap - 1.0;
    float4 hitSourcePacked = RaycastInput.SampleLevel(defaultSampler, uv, 0);
    float sourceMip = clamp(log2(coneTangent * length(hitSourcePacked.xy - uv) * max(ScreenSize.x, ScreenSize.y)), 0.0, maxMipLevel);
    float4 maskAndColorPacked = float4(ScreenInput.SampleLevel(defaultSampler, hitSourcePacked.xy, sourceMip).rgb, MaskInput.SampleLevel(defaultSampler, uv, 0).r);
    StoreResolveData(groupIndex, hitSourcePacked, maskAndColorPacked);

    GroupMemoryBarrierWithGroupSync();

    bool border = groupThreadId.x < RESOLVE_RAD | groupThreadId.y < RESOLVE_RAD | groupThreadId.x >= KERNEL_SIZE + RESOLVE_RAD || groupThreadId.y >= KERNEL_SIZE + RESOLVE_RAD;
    if (border) return;

    float3 viewNormal = normalize(mul((float3x3)worldToCameraMatrix, worldNormal));

    float4 result = 0.0;
    float weightSum = 0.0;
    float mul = ResolveSize.x / RayCastSize.x;

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        int2 offsetUV = offset[i] * mul * 2;

        // "uv" is the location of the current (or "local") pixel. We want to resolve the local pixel using
        // intersections spawned from neighboring pixels. The neighboring pixel is this one:
        int2 neighborGroupThreadId = groupThreadId.xy + offsetUV;
        uint neighborIndex = neighborGroupThreadId.y * (KERNEL_SIZE + RESOLVE_RAD2) + neighborGroupThreadId.x;

        // Now we fetch the intersection point and the PDF that the neighbor's ray hit.
        float4 hitPacked;
        float4 maskScreenPacked;
        LoadResolveData(neighborIndex, hitPacked, maskScreenPacked);
        float2 hitUV = hitPacked.xy;
        float hitZ = hitPacked.z;
        float hitPDF = hitPacked.w;
        float hitMask = maskScreenPacked.a;

        float3 screenPos = float3(hitUV * 2 - 1, hitZ);
        float3 hitViewPos = getViewPos(screenPos, projectionMatrixInv);

		// We assume that the hit point of the neighbor's ray is also visible for our ray, and we blindly pretend
		// that the current pixel shot that ray. To do that, we treat the hit point as a tiny light source. To calculate
		// a lighting contribution from it, we evaluate the BRDF. Finally, we need to account for the probability of getting
		// this specific position of the "light source", and that is approximately 1/PDF, where PDF comes from the neighbor.
		// Finally, the weight is BRDF/PDF. BRDF uses the local pixel's normal and roughness, but PDF comes from the neighbor.
        float weight = BRDF_Weight(normalize(-viewPos) /*V*/, normalize(hitViewPos - viewPos) /*L*/, viewNormal /*N*/, roughness) / max(1e-5, hitPDF);

        float4 sampleColor;
        sampleColor.rgb = maskScreenPacked.rgb;
        sampleColor.a = RayAttenBorder(hitUV, ssrUniform.EdgeFactor) * hitMask;

        //fireflies
        sampleColor.rgb /= 1 + Luminance(sampleColor.rgb);

        result += sampleColor * weight;
        weightSum += weight;
    }

    result /= weightSum;

    //fireflies
    result.rgb /= 1 - Luminance(result.rgb);

    ResolveResult[uvInt.xy] = max(1e-5, result);
}

