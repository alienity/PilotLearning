#ifndef MOYU_SSAO_INCLUDED
#define MOYU_SSAO_INCLUDED

#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariablesGlobal.hlsl"

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

struct SSAOInput
{
    FrameUniforms frameUniforms;
    Texture2D<float4> depthTex;
    Texture2D<float4> worldNormalMap;
    SamplerState defaultSampler;
};

float CustomSSAO(SSAOInput ssaoInput, float2 uv, uint2 screenSize)
{
    // Parameters used in coordinate conversion
    float4x4 projMatInv = ssaoInput.frameUniforms.cameraUniform._InvProjMatrix;
    float4x4 projMatrix = ssaoInput.frameUniforms.cameraUniform._ProjMatrix;
    
    float3x3 normalMat = transpose((float3x3)ssaoInput.frameUniforms.cameraUniform._InvViewMatrix);

    float raw_depth_o = ssaoInput.depthTex.Sample(ssaoInput.defaultSampler, uv).r;
    float4 pos_ss = float4(uv.xy*2-1, raw_depth_o, 1.0f);
    float4 pos_view = mul(projMatInv, pos_ss);

    float3 vpos_o = pos_view.xyz / pos_view.w;
    float depth_o = vpos_o.z;

    // float3 norm_o = -normalize(cross(ddy(vpos_o), ddx(vpos_o)));
    float3 norm_o = ssaoInput.worldNormalMap.SampleLevel(ssaoInput.defaultSampler, uv, 0).rgb * 2 - 1;
    norm_o = mul(normalMat, norm_o);
    norm_o.y = -norm_o.y;

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

        v_s1 = faceforward(v_s1, -norm_o, v_s1);
        float3 vpos_s1 = vpos_o + v_s1;

        // Reproject the sample point
        float4 spos_s1 = mul(projMatrix, float4(vpos_s1, 1.0f));
        float2 uv_s1_01 = clamp((spos_s1.xy * rcp(spos_s1.w) + 1.0) * 0.5, 0.0, 1.0);

        // Depth at the sample point
        float raw_depth_s1 = ssaoInput.depthTex.Sample(ssaoInput.defaultSampler, uv_s1_01).r;
        
        // Relative position of the sample point
        float4 pos_ss_s1 = float4(uv_s1_01.xy*2.0f-1.0f, raw_depth_s1, 1.0f);
        // pos_ss_s1.y *= -1;
        float4 pos_view_s1 = mul(projMatInv, pos_ss_s1);

        float3 vpos_s2 = pos_view_s1.xyz / pos_view_s1.w;
        
        float3 v_s2 = vpos_s2 - vpos_o;

        // // Estimate the obscurance value
        // // float a1 = max(dot(v_s2, norm_o) - kBeta * depth_o, 0.0);
        // float a1 = max(dot(v_s2, norm_o), 0.0);
        // float a2 = dot(v_s2, v_s2) + EPSILON;
        // ao += a1 * rcp(a2);

        ao += step(0, v_s2.z);
    }

    // Intensity normalization
    ao *= RADIUS;

    // // Apply contrast
    // ao = pow(ao * INTENSITY * rcpSampleCount, kContrast);

    return ao;
}

#endif