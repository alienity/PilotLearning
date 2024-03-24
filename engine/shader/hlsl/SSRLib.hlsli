#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "BSDF.hlsli"

float3 getWorldPos(float3 screenPos, float4x4 viewProjectionMatrixInv)
{
    float4 worldPos = mul(viewProjectionMatrixInv, float4(screenPos, 1));
    return worldPos.xyz / worldPos.w;
}

float3 getViewPos(float3 screenPos, float4x4 projectionMatrixInv)
{
    float4 viewPos = mul(projectionMatrixInv, float4(screenPos, 1));
    return viewPos.xyz / viewPos.w;
}

float RayAttenBorder(float2 pos, float value)
{
    float borderDist = min(1.0 - max(pos.x, pos.y), min(pos.x, pos.y));
    return saturate(borderDist > value ? 1.0 : borderDist / value);
}

float Luminance(float3 linearRgb)
{
    return dot(linearRgb, float3(0.2126729, 0.7151522, 0.0721750));
}

float BRDF_Weight(float3 V, float3 L, float3 N, float Roughness)
{
    float3 H = normalize(L + V);

    float NdotH = saturate(dot(N, H));
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    half G = visibility(Roughness, NdotV, NdotL);
    half D = distribution(Roughness, NdotH);

    return (D * G) * (F_PI / 4.0);
}

static const int2 offset[9] =
{
    int2(0, 0),
	int2(0, 1),
	int2(1, -1),
	int2(-1, -1),
	int2(-1, 0),
	int2(0, -1),
	int2(1, 0),
	int2(-1, 1),
	int2(1, 1)
};