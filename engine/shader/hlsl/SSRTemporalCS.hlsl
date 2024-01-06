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
    uint screenInputIndex;
    uint raycastInputIndex;
    uint maxDepthBufferIndex;
    uint previousTemporalInputIndex;
    uint temporalResultIndex;
};

SamplerState defaultSampler : register(s10);
SamplerState minDepthSampler : register(s11);

groupshared uint t_cacheR[(KERNEL_SIZE + 2) * (KERNEL_SIZE + 2)];
groupshared uint t_cacheG[(KERNEL_SIZE + 2) * (KERNEL_SIZE + 2)];
groupshared uint t_cacheB[(KERNEL_SIZE + 2) * (KERNEL_SIZE + 2)];
groupshared uint t_cacheA[(KERNEL_SIZE + 2) * (KERNEL_SIZE + 2)];

void Store1TPixel(uint index, float4 pixel)
{
    t_cacheR[index] = asuint(pixel.r);
    t_cacheG[index] = asuint(pixel.g);
    t_cacheB[index] = asuint(pixel.b);
    t_cacheA[index] = asuint(pixel.a);
}

void Load1TPixel(uint index, out float4 pixel)
{
    pixel = asfloat(uint4(t_cacheR[index], t_cacheG[index], t_cacheB[index], t_cacheA[index]));
}

float4 clip_aabb(float3 aabb_min, float3 aabb_max, float4 p, float4 q)
{
    float3 p_clip = 0.5 * (aabb_max + aabb_min);
    float3 e_clip = 0.5 * (aabb_max - aabb_min) + FLT_EPS;

    float4 v_clip = q - float4(p_clip, p.w);
    float3 v_unit = v_clip.xyz / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.0)
        return float4(p_clip, p.w) + v_clip / ma_unit;
    else
        return q; // point inside aabb
}

/*
float3 GetViewRay(float2 uv)
{
    float4 _CamScreenDir = float4(1.0 / ProjectionMatrix[0][0], 1.0 / ProjectionMatrix[1][1], 1, 1);
    float3 ray = float3(uv.x * 2 - 1, uv.y * 2 - 1, 1);
    ray *= _CamScreenDir.xyz;
    ray = ray * (ProjectionParams.z / ray.z);
    return ray;
}

float2 CalculateMotion(float rawDepth, float2 inUV, float4 zBufferParams)
{
    float depth = Linear01Depth(rawDepth, zBufferParams);
    float3 ray = GetViewRay(inUV);
    float3 vPos = ray * depth;
    float4 worldPos = mul(CameraToWorldMatrix, float4(vPos, 1.0));

    float4 prevClipPos = mul(PrevViewProjectionMatrix, worldPos);
    float4 curClipPos = mul(ViewProjectionMatrix, worldPos);

    float2 prevHPos = prevClipPos.xy / prevClipPos.w;
    float2 curHPos = curClipPos.xy / curClipPos.w;

            // V is the viewport position at this pixel in the range 0 to 1.
    float2 vPosPrev = (prevHPos.xy + 1.0f) / 2.0f;
    float2 vPosCur = (curHPos.xy + 1.0f) / 2.0f;
    return vPosCur - vPosPrev;
}
*/

[numthreads(KERNEL_SIZE + 2, KERNEL_SIZE + 2, 1)]
void CSTemporal(uint3 groupId : SV_GroupId, uint3 groupThreadId : SV_GroupThreadID)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    SSRUniform ssrUniform = mFrameUniforms.ssrUniform;
    float4 resolveSize = ssrUniform.ResolveSize;
    float4 screenSize = ssrUniform.ScreenSize;
    float TResponseMin = ssrUniform.TResponseMin;
    float TResponseMax = ssrUniform.TResponseMax;

    Texture2D<float4> screenInput = ResourceDescriptorHeap[screenInputIndex];
    Texture2D<float4> raycastInput = ResourceDescriptorHeap[raycastInputIndex];
    Texture2D<float4> maxDepthMap = ResourceDescriptorHeap[maxDepthBufferIndex];
    Texture2D<float4> previousTemporalInput = ResourceDescriptorHeap[previousTemporalInputIndex];
    RWTexture2D<float4> temporalResult = ResourceDescriptorHeap[temporalResultIndex];

    uint2 uvInt = (groupId.xy * KERNEL_SIZE) + (groupThreadId.xy - 1);
    float2 uv = saturate((uvInt.xy + 0.5f) * resolveSize.zw);
    uint groupIndex = groupThreadId.y * (KERNEL_SIZE + 2) + groupThreadId.x;
    float4 current = screenInput.SampleLevel(defaultSampler, uv, 0);
    Store1TPixel(groupIndex, current);
    GroupMemoryBarrierWithGroupSync();

    bool isborder = groupThreadId.x == 0 | groupThreadId.y == 0 | groupThreadId.x == KERNEL_SIZE + 1 || groupThreadId.y == KERNEL_SIZE + 1;
    if (isborder) return;

    float4 hit = raycastInput.SampleLevel(defaultSampler,uv,0);

    float depth = maxDepthMap.SampleLevel(minDepthSampler,uv,0).x;
    float hitDepth = maxDepthMap.SampleLevel(minDepthSampler,hit.xy,0).x;

    // float2 reflectionCameraVelocity = CalculateMotion(hitDepth, uv);
    // float2 hitCameraVelocity = CalculateMotion(hitDepth, hit.xy);
    // float2 cameraVelocity = CalculateMotion(depth, uv);

    float2 reflectionCameraVelocity = float2(0,0);
    float2 hitCameraVelocity = float2(0,0);
    float2 cameraVelocity = float2(0,0);

    float2 velocityDiff = cameraVelocity;
    float2 hitVelocityDiff = hitCameraVelocity;

    float objectVelocityMask = saturate(dot(velocityDiff, velocityDiff) * screenSize.x * 100);
    float hitObjectVelocityMask = saturate(dot(hitVelocityDiff, hitVelocityDiff) * screenSize.x * 100);

    float2 reflectVelocity = (reflectionCameraVelocity * (1 - hitObjectVelocityMask)) * (1 - objectVelocityMask);
    float2 velocity = reflectVelocity;
    float2 prevUV = uv - velocity;
		
    float4 previous = previousTemporalInput.SampleLevel(defaultSampler, prevUV,0);

    float4 currentMin = 100;
    float4 currentMax = 0;
    float4 currentAvarage = 0;

	[unroll]
    for (int i = 0; i < 9; i++)
    {
		uint2 uv = groupThreadId.xy + offset[i];
        float4 val;
        Load1TPixel(uv.y * (KERNEL_SIZE + 2) + uv.x, val);
        currentMin = min(currentMin, val);
        currentMax = max(currentMax, val);
        currentAvarage += val;
    }

    currentAvarage /= 9.0;

    previous = clip_aabb(currentMin.xyz, currentMax.xyz, clamp(currentAvarage, currentMin, currentMax), previous);

    float lum0 = Luminance(current.rgb);
    float lum1 = Luminance(previous.rgb);

    float unbiased_diff = abs(lum0 - lum1) / max(lum0, max(lum1, 0.2));
    float unbiased_weight = 1.0 - unbiased_diff;
    float unbiased_weight_sqr = unbiased_weight * unbiased_weight;
    float k_feedback = lerp(TResponseMin, TResponseMax, unbiased_weight_sqr);

    float4 tResult = lerp(current, previous, k_feedback);

    temporalResult[uvInt.xy] = tResult;
}