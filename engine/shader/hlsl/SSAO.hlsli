#ifndef MOYU_SSAO_INCLUDED
#define MOYU_SSAO_INCLUDED

#include "CommonMath.hlsli"
#include "Random.hlsli"
#include "InputTypes.hlsli"

#define INTENSITY 1
#define SAMPLE_COUNT 8
#define RADIUS 0.05

static const float kContrast = 0.6;
static const float kBeta = 0.002;
#define EPSILON 1.0e-4

float2 GetScreenSpacePosition(float2 uv, uint2 screenSize)
{
    return uv * screenSize;
}

// Trigonometric function utility
float2 CosSin(float theta)
{
    float sn, cs;
    sincos(theta, sn, cs);
    return float2(cs, sn);
}

// Pseudo random number generator with 2D coordinates
float UVRandom(float u, float v)
{
    float f = dot(float2(12.9898, 78.233), float2(u, v));
    return frac(43758.5453 * sin(f));
}

// Sample point picker
float3 PickSamplePoint(float2 uv, uint2 screenSize, float randAddon, int index)
{
    float2 positionSS = GetScreenSpacePosition(uv, screenSize);
    float gn = InterleavedGradientNoise(positionSS, index);
    float u = frac(UVRandom(0.0, index + randAddon) + gn) * 2.0 - 1.0;
    float theta = (UVRandom(1.0, index + randAddon) + gn) * F_TAU;
    return float3(CosSin(theta) * sqrt(1.0 - u * u), u);
}

/*
float RawToLinearDepth(float rawDepth)
{
    return LinearEyeDepth(rawDepth, _ZBufferParams);
}

float SampleAndGetLinearDepth(float2 uv)
{
    float rawDepth = SampleSceneDepth(uv.xy).r;
    return RawToLinearDepth(rawDepth);
}

float3 ReconstructViewPos(float2 uv, float depth, float2 p11_22, float2 p13_31)
{
    float3 viewPos = float3(depth * ((uv.xy * 2.0 - 1.0 - p13_31) * p11_22), depth);
    return viewPos;
}

void SampleDepthNormalView(float2 uv, float2 p11_22, float2 p13_31, out float depth, out float3 normal, out float3 vpos)
{
    depth  = SampleAndGetLinearDepth(uv);
    vpos = ReconstructViewPos(uv, depth, p11_22, p13_31);
    normal = normalize(cross(ddy(vpos), ddx(vpos)));;
}

float3x3 GetCoordinateConversionParameters(out float2 p11_22, out float2 p13_31)
{
    float3x3 camProj = (float3x3)unity_CameraProjection;

    p11_22 = rcp(float2(camProj._11, camProj._22));
    p13_31 = float2(camProj._13, camProj._23);

    return camProj;
}
*/

struct SSAOInput
{
    FrameUniforms frameUniforms;
    Texture2D<float4> depthTex;
    Texture2D<float4> normalMap;
    Texture2D<float4> worldNormalMap;
    Texture2D<float4> worldTangentMap;
    SamplerState defaultSampler;
};

float CustomSSAO(SSAOInput ssaoInput, float2 uv, uint2 screenSize)
{
    // Parameters used in coordinate conversion
    float4x4 projMatrix = ssaoInput.frameUniforms.cameraUniform.clipFromViewMatrix;

    // Get the depth, normal and view position for this fragment
    float3 norm_o = ssaoInput.worldNormalMap.Sample(ssaoInput.defaultSampler, uv).rgb;

    float4x4 projMatInv = ssaoInput.frameUniforms.cameraUniform.viewFromClipMatrix;
    float depth = ssaoInput.depthTex.Sample(ssaoInput.defaultSampler, uv).r;
    float4 pos_ss = float3(uv.xy*2-1, depth);
    float4 pos_view = mul(projMatInv, pos_ss);

    float3 vpos_o = pos_view.xyz / pos_view.w;

    // This was added to avoid a NVIDIA driver issue.
    float randAddon = uv.x * 1e-10;

    float rcpSampleCount = rcp(SAMPLE_COUNT);
    float ao = 0.0;
    for (int s = 0; s < int(SAMPLE_COUNT); s++)
    {
        // Sample point
        float3 v_s1 = PickSamplePoint(uv, screenSize, randAddon, s);

        // Make it distributed between [0, _Radius]
        v_s1 *= sqrt((s + 1.0) * rcpSampleCount) * RADIUS;

        v_s1 = v_s1 + norm_o;
        float3 vpos_s1 = vpos_o + v_s1;

        // Reproject the sample point
        float3 spos_s1 = mul(projMatrix, vpos_s1);
        float2 uv_s1_01 = clamp((spos_s1.xy * rcp(vpos_s1.z) + 1.0) * 0.5, 0.0, 1.0);

        // Depth at the sample point
        float depth_s1 = SampleAndGetLinearDepth(uv_s1_01) + 0.0001;

        // Relative position of the sample point
        float3 vpos_s2 = ReconstructViewPos(uv_s1_01, depth_s1, p11_22, p13_31);
        float3 v_s2 = vpos_s2 - vpos_o;

        // Estimate the obscurance value
        float a1 = max(dot(v_s2, norm_o) - kBeta * depth_o, 0.0);
        float a2 = dot(v_s2, v_s2) + EPSILON;
        ao += a1 * rcp(a2);
    }

    // Intensity normalization
    ao *= RADIUS;

    // Apply contrast
    ao = pow(ao * INTENSITY * rcpSampleCount, kContrast);

    return ao;
}

#endif