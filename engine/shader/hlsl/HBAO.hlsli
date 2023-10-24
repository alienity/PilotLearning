#ifndef MOYU_HBAO_INCLUDED
#define MOYU_HBAO_INCLUDED

#include "CommonMath.hlsli"
#include "Random.hlsli"
#include "InputTypes.hlsli"

#define INTENSITY 1
#define RADIUS    0.05
#define NDotVBias 0.1

#define NUM_STEPS 4
#define NUM_DIRECTIONS 8

#define kContrast 0.6
#define kBeta 0.002
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

float Falloff(float DistanceSquare)
{
  // 1 scalar mad instruction
  return DistanceSquare / (RADIUS * RADIUS) + 1.0;
}

//----------------------------------------------------------------------------------
// P = view-space position at the kernel center
// N = view-space normal at the kernel center
// S = view-space position of the current sample
//----------------------------------------------------------------------------------
float ComputeAO(float3 P, float3 N, float3 S)
{
  float3 V = S - P;
  float VdotV = dot(V, V);
  float NdotV = dot(N, V) * 1.0/sqrt(VdotV);

  // Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
  return clamp(NdotV-NDotVBias,0,1) * clamp(Falloff(VdotV),0,1);
}

float2 RotateDirection(float2 Dir, float2 CosSin)
{
  return float2(Dir.x*CosSin.x - Dir.y*CosSin.y,
                Dir.x*CosSin.y + Dir.y*CosSin.x);
}

struct HBAOInput
{
    FrameUniforms frameUniforms;
    Texture2D<float4> depthTex;
    Texture2D<float4> worldNormalMap;
    SamplerState defaultSampler;
};

float CustomHBAO(HBAOInput hbaoInput, float2 uv, uint2 screenSize)
{
    // Parameters used in coordinate conversion
    float4x4 projMatInv = hbaoInput.frameUniforms.cameraUniform.viewFromClipMatrix;
    float4x4 projMatrix = hbaoInput.frameUniforms.cameraUniform.clipFromViewMatrix;

    // float4x4 viewMatInv = hbaoInput.frameUniforms.cameraUniform.worldFromViewMatrix;
    
    float3x3 normalMat = transpose((float3x3)hbaoInput.frameUniforms.cameraUniform.worldFromViewMatrix);

    float raw_depth_o = hbaoInput.depthTex.Sample(hbaoInput.defaultSampler, uv).r;
    float4 pos_ss = float4(uv.xy*2-1, raw_depth_o, 1.0f);
    float4 pos_view = mul(projMatInv, pos_ss);

    float3 vpos_o = pos_view.xyz / pos_view.w;
    float depth_o = vpos_o.z;

    // float3 norm_o = normalize(cross(ddx(vpos_o), ddy(vpos_o)));
    float3 norm_o = hbaoInput.worldNormalMap.SampleLevel(hbaoInput.defaultSampler, uv, 0).rgb * 2 - 1;
    norm_o = mul(normalMat, norm_o);
    norm_o.y = -norm_o.y;

    // This was added to avoid a NVIDIA driver issue.
    float randAddon = uv.x * 1e-10;

    float RadiusPixels = RADIUS / depth_o * projMatrix._m11 * screenSize.y;

    // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
    float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);

    const float Alpha = 2.0 * F_PI / NUM_DIRECTIONS;
    float AO = 0.5;

    for(int directionIndex = 0; directionIndex < NUM_DIRECTIONS; directionIndex++)
    {
        float Angle = Alpha * directionIndex;

        float3 Rand = PickSamplePoint(uv, screenSize, randAddon, directionIndex);
        Rand = faceforward(Rand, -norm_o, Rand);

        // Compute normalized 2D direction
        float2 Direction = RotateDirection(float2(cos(Angle), sin(Angle)), Rand.xy);

        // Jitter starting sample within the first step
        float RayPixels = (Rand.z * StepSizePixels + 1.0);

        for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex)
        {
            float2 SnappedUV = round(RayPixels * Direction) / screenSize.xy + uv;

            float depth_snapped = hbaoInput.depthTex.Sample(hbaoInput.defaultSampler, SnappedUV).r;
            float4 pos_ss_s1 = float4(SnappedUV.xy*2.0f-1.0f, depth_snapped, 1.0f);
            float4 pos_view_s1 = mul(projMatInv, pos_ss_s1);
            float3 S = pos_view_s1.xyz/pos_view_s1.w;
            S.z -= 0.00001f;
            RayPixels += StepSizePixels;

            AO += ComputeAO(vpos_o, norm_o, S);
        }
    }

    float rcpSampleCount = rcp(NUM_DIRECTIONS * NUM_STEPS);
    // Apply contrast
    AO = pow(abs(AO * INTENSITY * rcpSampleCount), kContrast);

    return AO;
}

#endif