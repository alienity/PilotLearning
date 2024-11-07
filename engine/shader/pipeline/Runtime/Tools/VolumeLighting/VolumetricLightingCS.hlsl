//--------------------------------------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------------------------------------

#define LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
// #define ENABLE_REPROJECTION
// #define ENABLE_ANISOTROPY
// #define VL_PRESET_OPTIMAL
// #define SUPPORT_LOCAL_LIGHTS


// Don't want contact shadows
#define LIGHT_EVALUATION_NO_CONTACT_SHADOWS // To define before LightEvaluation.hlsl
// #define LIGHT_EVALUATION_NO_HEIGHT_FOG

#ifndef LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
    #define USE_BIG_TILE_LIGHTLIST
#endif

#ifdef VL_PRESET_OPTIMAL
    // E.g. for 1080p: (1920/8)x(1080/8)x(64) = 2,073,600 voxels
    #define VBUFFER_VOXEL_SIZE 8
#endif

#define PREFER_HALF             0
#define GROUP_SIZE_1D           8
#define SHADOW_USE_DEPTH_BIAS   0 // Too expensive, not particularly effective
#define PUNCTUAL_SHADOW_ULTRA_LOW  // Different options are too expensive.
#define DIRECTIONAL_SHADOW_ULTRA_LOW  // Different options are too expensive.
#define AREA_SHADOW_LOW           // Different options are too expensive.
#define SHADOW_AUTO_FLIP_NORMAL 0 // No normal information, so no need to flip
#define SHADOW_VIEW_BIAS        1 // Prevents light leaking through thin geometry. Not as good as normal bias at grazing angles, but cheaper and independent from the geometry.
#define USE_DEPTH_BUFFER        1 // Accounts for opaque geometry along the camera ray

#define SHADOW_DATA_NOT_GUARANTEED_SCALAR // We are not looping over shadows in a scalarized fashion. If we will at some point, remove this define.

//--------------------------------------------------------------------------------------------------
// Included headers
//--------------------------------------------------------------------------------------------------

#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/GeometricTools.hlsl"
#include "../../ShaderLibrary/Color.hlsl"
#include "../../ShaderLibrary/Filtering.hlsl"
#include "../../ShaderLibrary/VolumeRendering.hlsl"
#include "../../ShaderLibrary/EntityLighting.hlsl"

#include "../../Material/Builtin/BuiltinData.hlsl"

#include "../../ShaderLibrary/ShaderVariables.hlsl"
#define SHADERPASS SHADERPASS_VOLUMETRIC_LIGHTING

#include "../../Tools/VolumeLighting/VolumetricLightingCommon.hlsl"

#include "../../ShaderLibrary/CommonLighting.hlsl"
#include "../../ShaderLibrary/VolumeRendering.hlsl"
#include "../../ShaderLibrary/Sampling/Sampling.hlsl"

#include "../../Lighting/Lighting.hlsl"
#include "../../Lighting/LightLoop/LightLoopDef.hlsl"
#include "../../Lighting/LightEvaluation.hlsl"

#include "../../Material/BuiltinGIUtilities.hlsl"

//--------------------------------------------------------------------------------------------------
// Inputs & outputs
//--------------------------------------------------------------------------------------------------

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint shaderVariablesVolumetricIndex;
    uint _VBufferDensityIndex;  // RGB = sqrt(scattering), A = sqrt(extinction)
    uint _DepthPyramidIndex;;
    uint _VBufferLightingIndex;
};

// RW_TEXTURE3D(float4, _VBufferLighting);
// RW_TEXTURE3D(float4, _VBufferFeedback);
// TEXTURE3D(_VBufferHistory);
// TEXTURE3D(_VBufferDensity);                     // RGB = scattering, A = extinction
// TEXTURE2D(_MaxZMaskTexture);
// // StructuredBuffer<float4> _VolumetricAmbientProbeBuffer;

SamplerState s_point_clamp_sampler      : register(s10);
SamplerState s_linear_clamp_sampler     : register(s11);
SamplerState s_linear_repeat_sampler    : register(s12);
SamplerState s_trilinear_clamp_sampler  : register(s13);
SamplerState s_trilinear_repeat_sampler : register(s14);
SamplerComparisonState s_linear_clamp_compare_sampler : register(s15);

//--------------------------------------------------------------------------------------------------
// Implementation
//--------------------------------------------------------------------------------------------------

SamplerStruct GetSamplerStruct()
{
    SamplerStruct mSamplerStruct;
    mSamplerStruct.SPointClampSampler = s_point_clamp_sampler;
    mSamplerStruct.SLinearClampSampler = s_linear_clamp_sampler;
    mSamplerStruct.SLinearRepeatSampler = s_linear_repeat_sampler;
    mSamplerStruct.STrilinearClampSampler = s_trilinear_clamp_sampler;
    mSamplerStruct.STrilinearRepeatSampler = s_trilinear_repeat_sampler;
    mSamplerStruct.SLinearClampCompareSampler = s_linear_clamp_compare_sampler;
    return mSamplerStruct;
}

// Ambient probe for volumetric contains a convolution with Cornette Shank phase function so it needs to sample a different buffer.
float3 EvaluateVolumetricAmbientProbe(float3 normalWS, float4 _VolumetricAmbientProbeBuffer[9])
{
    float4 SHCoefficients[7];
    SHCoefficients[0] = _VolumetricAmbientProbeBuffer[0];
    SHCoefficients[1] = _VolumetricAmbientProbeBuffer[1];
    SHCoefficients[2] = _VolumetricAmbientProbeBuffer[2];
    SHCoefficients[3] = _VolumetricAmbientProbeBuffer[3];
    SHCoefficients[4] = _VolumetricAmbientProbeBuffer[4];
    SHCoefficients[5] = _VolumetricAmbientProbeBuffer[5];
    SHCoefficients[6] = _VolumetricAmbientProbeBuffer[6];

    return SampleSH9(SHCoefficients, normalWS);
}

// Jittered ray with screen-space derivatives.
struct JitteredRay
{
    float3 originWS;
    float3 centerDirWS;
    float3 jitterDirWS;
    float3 xDirDerivWS;
    float3 yDirDerivWS;
    float  geomDist;

    float maxDist;
};

struct VoxelLighting
{
    float3 radianceComplete;
    float3 radianceNoPhase;
};

bool IsInRange(float x, float2 range)
{
    return clamp(x, range.x, range.y) == x;
}

float ComputeHistoryWeight()
{
    // Compute the exponential moving average over 'n' frames:
    // X = (1 - a) * ValueAtFrame[n] + a * AverageOverPreviousFrames.
    // We want each sample to be uniformly weighted by (1 / n):
    // X = (1 / n) * Sum{i from 1 to n}{ValueAtFrame[i]}.
    // Therefore, we get:
    // (1 - a) = (1 / n) => a = (1 - 1 / n) = (n - 1) / n,
    // X = (1 / n) * ValueAtFrame[n] + (1 - 1 / n) * AverageOverPreviousFrames.
    // Why does it work? We need to make the following assumption:
    // AverageOverPreviousFrames ≈ AverageOverFrames[n - 1].
    // AverageOverFrames[n - 1] = (1 / (n - 1)) * Sum{i from 1 to n - 1}{ValueAtFrame[i]}.
    // This implies that the reprojected (accumulated) value has mostly converged.
    // X = (1 / n) * ValueAtFrame[n] + ((n - 1) / n) * (1 / (n - 1)) * Sum{i from 1 to n - 1}{ValueAtFrame[i]}.
    // X = (1 / n) * ValueAtFrame[n] + (1 / n) * Sum{i from 1 to n - 1}{ValueAtFrame[i]}.
    // X = Sum{i from 1 to n}{ValueAtFrame[i] / n}.
    float numFrames     = 7;
    float frameWeight   = 1 / numFrames;
    float historyWeight = 1 - frameWeight;

    return historyWeight;
}

// Computes the light integral (in-scattered radiance) within the voxel.
// Multiplication by the scattering coefficient and the phase function is performed outside.
VoxelLighting EvaluateVoxelLightingDirectional(FrameUniforms frameUniforms, SamplerStruct samplerStruct,
                                               LightLoopContext context, uint featureFlags, PositionInputs posInput, float3 centerWS,
                                               JitteredRay ray, float t0, float t1, float dt, float rndVal, float extinction, float anisotropy, bool underWater)
{
    VoxelLighting lighting;
    ZERO_INITIALIZE(VoxelLighting, lighting);

    const float NdotL = 1;

    float tOffset, weight;
    ImportanceSampleHomogeneousMedium(rndVal, extinction, dt, tOffset, weight);

    float t = t0 + tOffset;
    posInput.positionWS = ray.originWS + t * ray.jitterDirWS;

    context.shadowValue = 1.0;

    // Evaluate sun shadows.
    // if (_DirectionalShadowIndex >= 0)
    if (1)
    {
        DirectionalLightData light = frameUniforms.lightDataUniform.directionalLightData;
        // DirectionalLightData light = _DirectionalLightDatas[_DirectionalShadowIndex];

        // Prep the light so that it works with non-volumetrics-aware code.
        light.contactShadowMask  = 0;
        light.shadowDimmer       = light.volumetricShadowDimmer;

        float3 L = -light.forward;

        // Is it worth sampling the shadow map?
        if ((light.volumetricLightDimmer > 0) && (light.volumetricShadowDimmer > 0))
        {
            #if SHADOW_VIEW_BIAS
                // Our shadows only support normal bias. Volumetrics has no access to the surface normal.
                // We fake view bias by invoking the normal bias code with the view direction.
                float3 shadowN = -ray.jitterDirWS;
            #else
                float3 shadowN = 0; // No bias
            #endif // SHADOW_VIEW_BIAS

            // context.shadowValue = GetDirectionalShadowAttenuation(context.shadowContext,
            //                         samplerStruct, posInput.positionSS, positionWS, GetNormalForShadowBias(bsdfData), light.shadowIndex, L);
            
            context.shadowValue = GetDirectionalShadowAttenuation(context.shadowContext, samplerStruct, 
                                                                  posInput.positionSS, posInput.positionWS, shadowN,
                                                                  light.shadowIndex, L);

            // // Apply the volumetric cloud shadow if relevant
            // if (_VolumetricCloudsShadowOriginToggle.w == 1.0)
            //     context.shadowValue *= EvaluateVolumetricCloudsShadows(light, posInput.positionWS);
        }
    }
    else
    {
        context.shadowValue = 1;
    }

    // for (uint i = 0; i < _DirectionalLightCount; ++i)
    {
        // DirectionalLightData light = _DirectionalLightDatas[i];
        DirectionalLightData light = frameUniforms.lightDataUniform.directionalLightData;

        // Prep the light so that it works with non-volumetrics-aware code.
        light.contactShadowMask  = 0;
        light.shadowDimmer       = light.volumetricShadowDimmer;

        float3 L = -light.forward;

        // Is it worth evaluating the light?
        float3 color; float attenuation;
        if (light.volumetricLightDimmer > 0)
        {
            float4 lightColor = EvaluateLight_Directional(context, posInput, light);

            // The volumetric light dimmer, unlike the regular light dimmer, is not pre-multiplied.
            lightColor.a *= light.volumetricLightDimmer;
            lightColor.rgb *= lightColor.a; // Composite

            #if SHADOW_VIEW_BIAS
                // Our shadows only support normal bias. Volumetrics has no access to the surface normal.
                // We fake view bias by invoking the normal bias code with the view direction.
                float3 shadowN = -ray.jitterDirWS;
            #else
                float3 shadowN = 0; // No bias
            #endif // SHADOW_VIEW_BIAS

            // This code works for both surface reflection and thin object transmission.
            BuiltinData unused;
            SHADOW_TYPE shadow = EvaluateShadow_Directional(context, posInput, light, unused, shadowN);
            lightColor.rgb *= ComputeShadowColor(shadow, light.shadowTint, light.penumbraTint);

            // Important:
            // Ideally, all scattering calculations should use the jittered versions
            // of the sample position and the ray direction. However, correct reprojection
            // of asymmetrically scattered lighting (affected by an anisotropic phase
            // function) is not possible. We work around this issue by reprojecting
            // lighting not affected by the phase function. This basically removes
            // the phase function from the temporal integration process. It is a hack.
            // The downside is that anisotropy no longer benefits from temporal averaging,
            // and any temporal instability of anisotropy causes causes visible jitter.
            // In order to stabilize the image, we use the voxel center for all
            // anisotropy-related calculations.
            float cosTheta = dot(L, ray.centerDirWS);
            float phase    = CornetteShanksPhasePartVarying(anisotropy, cosTheta);

            // Compute the amount of in-scattered radiance.
            // Note: the 'weight' accounts for transmittance from 't0' to 't'.
            lighting.radianceNoPhase += (weight * lightColor.rgb);
            lighting.radianceComplete += (weight * lightColor.rgb) * phase;
        }
    }

    return lighting;
}

/*
// Computes the light integral (in-scattered radiance) within the voxel.
// Multiplication by the scattering coefficient and the phase function is performed outside.
VoxelLighting EvaluateVoxelLightingLocal(FrameUniforms frameUniforms, SamplerStruct samplerStruct,
                                         LightLoopContext context, uint groupIdx, uint featureFlags, PositionInputs posInput,
                                         uint lightCount, uint lightStart, float3 centerWS,
                                         JitteredRay ray, float t0, float t1, float dt, float rndVal, float extinction, float anisotropy)
{
    VoxelLighting lighting;
    ZERO_INITIALIZE(VoxelLighting, lighting);

    const float NdotL = 1;

    if (featureFlags & LIGHTFEATUREFLAGS_PUNCTUAL)
    {
        uint lightOffset = 0; // This is used by two subsequent loops

        // This loop does not process box lights.
        for (; lightOffset < lightCount; lightOffset++)
        {
            uint lightIndex = FetchIndex(lightStart, lightOffset);
            LightData light = _LightDatas[lightIndex];

            if (light.lightType >= GPULIGHTTYPE_PROJECTOR_BOX) { break; }

            // Prep the light so that it works with non-volumetrics-aware code.
            light.contactShadowMask  = 0;
            light.shadowDimmer       = light.volumetricShadowDimmer;

            bool sampleLight = true;

            float tEntr = t0;
            float tExit = t1;

            // Perform ray-cone intersection for pyramid and spot lights.
            if (light.lightType != GPULIGHTTYPE_POINT)
            {
                float lenMul = 1;

                if (light.lightType == GPULIGHTTYPE_PROJECTOR_PYRAMID)
                {
                    // 'light.right' and 'light.up' vectors are pre-scaled on the CPU
                    // s.t. if you were to place them at the distance of 1 directly in front
                    // of the light, they would give you the "footprint" of the light.
                    // For spot lights, the cone fit is exact.
                    // For pyramid lights, however, this is the "inscribed" cone
                    // (contained within the pyramid), and we want to intersect
                    // the "escribed" cone (which contains the pyramid).
                    // Therefore, we have to scale the radii by the sqrt(2).
                    lenMul = rsqrt(2);
                }

                float3 coneAxisX = lenMul * light.right;
                float3 coneAxisY = lenMul * light.up;

                sampleLight = IntersectRayCone(ray.originWS, ray.jitterDirWS,
                                                              light.positionRWS, light.forward,
                                                              coneAxisX, coneAxisY,
                                                              t0, t1, tEntr, tExit);
            }

            // Is it worth evaluating the light?
            if (sampleLight)
            {
                float lightSqRadius = light.size.x;

                float t, distSq, rcpPdf;
                ImportanceSamplePunctualLight(rndVal, light.positionRWS, lightSqRadius,
                                              ray.originWS, ray.jitterDirWS,
                                              tEntr, tExit,
                                              t, distSq, rcpPdf);

                posInput.positionWS = ray.originWS + t * ray.jitterDirWS;

                float3 L;
                float4 distances; // {d, d^2, 1/d, d_proj}
                GetPunctualLightVectors(posInput.positionWS, light, L, distances);

                float4 lightColor = EvaluateLight_Punctual(context, posInput, light, L, distances);
                // The volumetric light dimmer, unlike the regular light dimmer, is not pre-multiplied.
                lightColor.a *= light.volumetricLightDimmer;
                lightColor.rgb *= lightColor.a; // Composite

            #if SHADOW_VIEW_BIAS
                // Our shadows only support normal bias. Volumetrics has no access to the surface normal.
                // We fake view bias by invoking the normal bias code with the view direction.
                float3 shadowN = -ray.jitterDirWS;
            #else
                float3 shadowN = 0; // No bias
            #endif // SHADOW_VIEW_BIAS

                SHADOW_TYPE shadow = EvaluateShadow_Punctual(context, posInput, light, unused, shadowN, L, distances);
                lightColor.rgb *= ComputeShadowColor(shadow, light.shadowTint, light.penumbraTint);


                // Important:
                // Ideally, all scattering calculations should use the jittered versions
                // of the sample position and the ray direction. However, correct reprojection
                // of asymmetrically scattered lighting (affected by an anisotropic phase
                // function) is not possible. We work around this issue by reprojecting
                // lighting not affected by the phase function. This basically removes
                // the phase function from the temporal integration process. It is a hack.
                // The downside is that anisotropy no longer benefits from temporal averaging,
                // and any temporal instability of anisotropy causes causes visible jitter.
                // In order to stabilize the image, we use the voxel center for all
                // anisotropy-related calculations.
                float3 centerL  = light.positionRWS - centerWS;
                float  cosTheta = dot(centerL, ray.centerDirWS) * rsqrt(dot(centerL, centerL));
                float  phase    = CornetteShanksPhasePartVarying(anisotropy, cosTheta);

                // Compute transmittance from 't0' to 't'.
                float weight = TransmittanceHomogeneousMedium(extinction, t - t0) * rcpPdf;

                // Compute the amount of in-scattered radiance.
                lighting.radianceNoPhase += (weight * lightColor.rgb);
                lighting.radianceComplete += (weight * lightColor.rgb) * phase;
            }
        }

        // This loop only processes box lights.
        for (; lightOffset < lightCount; lightOffset++)
        {
            uint lightIndex = FetchIndex(lightStart, lightOffset);
            LightData light = _LightDatas[lightIndex];

            if (light.lightType != GPULIGHTTYPE_PROJECTOR_BOX) { break; }

            // Prep the light so that it works with non-volumetrics-aware code.
            light.contactShadowMask  = 0;
            light.shadowDimmer       = light.volumetricShadowDimmer;

            bool sampleLight = true;

            // Convert the box light from OBB to AABB.
            // 'light.right' and 'light.up' vectors are pre-scaled on the CPU by (2/w) and (2/h).
            float3x3 rotMat = float3x3(light.right, light.up, light.forward);

            float3 o = mul(rotMat, ray.originWS - light.positionRWS);
            float3 d = mul(rotMat, ray.jitterDirWS);

            float3 boxPt0 = float3(-1, -1, 0);
            float3 boxPt1 = float3( 1,  1, light.range);

            float tEntr, tExit;
            sampleLight = IntersectRayAABB(o, d, boxPt0, boxPt1, t0, t1, tEntr, tExit);

            // Is it worth evaluating the light?
            if (sampleLight)
            {
                float tOffset, weight;
                ImportanceSampleHomogeneousMedium(rndVal, extinction, tExit - tEntr, tOffset, weight);

                // Compute transmittance from 't0' to 'tEntr'.
                weight *= TransmittanceHomogeneousMedium(extinction, tEntr - t0);

                float t = tEntr + tOffset;
                posInput.positionWS = ray.originWS + t * ray.jitterDirWS;

                float3 L             = -light.forward;
                float3 lightToSample = posInput.positionWS - light.positionRWS;
                float  distProj      = dot(lightToSample, light.forward);
                float4 distances     = float4(1, 1, 1, distProj);

                float4 lightColor = EvaluateLight_Punctual(context, posInput, light, L, distances);
                // The volumetric light dimmer, unlike the regular light dimmer, is not pre-multiplied.
                lightColor.a *= light.volumetricLightDimmer;
                lightColor.rgb *= lightColor.a; // Composite

            #if SHADOW_VIEW_BIAS
                // Our shadows only support normal bias. Volumetrics has no access to the surface normal.
                // We fake view bias by invoking the normal bias code with the view direction.
                float3 shadowN = -ray.jitterDirWS;
            #else
                float3 shadowN = 0; // No bias
            #endif // SHADOW_VIEW_BIAS

                SHADOW_TYPE shadow = EvaluateShadow_Punctual(context, posInput, light, unused, shadowN, L, distances);
                lightColor.rgb *= ComputeShadowColor(shadow, light.shadowTint, light.penumbraTint);


                // Important:
                // Ideally, all scattering calculations should use the jittered versions
                // of the sample position and the ray direction. However, correct reprojection
                // of asymmetrically scattered lighting (affected by an anisotropic phase
                // function) is not possible. We work around this issue by reprojecting
                // lighting not affected by the phase function. This basically removes
                // the phase function from the temporal integration process. It is a hack.
                // The downside is that anisotropy no longer benefits from temporal averaging,
                // and any temporal instability of anisotropy causes causes visible jitter.
                // In order to stabilize the image, we use the voxel center for all
                // anisotropy-related calculations.
                float3 centerL  = light.positionRWS - centerWS;
                float  cosTheta = dot(centerL, ray.centerDirWS) * rsqrt(dot(centerL, centerL));
                float  phase    = CornetteShanksPhasePartVarying(anisotropy, cosTheta);

                // Compute the amount of in-scattered radiance.
                // Note: the 'weight' accounts for transmittance from 't0' to 't'.
                lighting.radianceNoPhase += (weight * lightColor.rgb);
                lighting.radianceComplete += (weight * lightColor.rgb) * phase;
            }
        }
    }

    return lighting;
}
*/

// Computes the in-scattered radiance along the ray.
void FillVolumetricLightingBuffer(
    inout RWTexture3D<float4> _VBufferLighting, Texture3D<float4> _VBufferDensity, SamplerStruct samplerStruct,
    FrameUniforms frameUniforms, VolumetricLightingUniform volumetricLightingUniform, VBufferUniform vBufferUniform,
    LightLoopContext context, uint featureFlags, PositionInputs posInput, int groupIdx, JitteredRay ray, float tStart)
{
    float4 _VBufferDistanceDecodingParams = vBufferUniform._VBufferDistanceDecodingParams;
    float _VBufferRcpSliceCount = vBufferUniform._VBufferRcpSliceCount;
    uint _VBufferSliceCount = vBufferUniform._VBufferSliceCount;
    float _GlobalFogAnisotropy = volumetricLightingUniform._GlobalFogAnisotropy;
    int _PunctualLightCount = frameUniforms.lightDataUniform._PunctualLightCount;
    
    uint lightCount, lightStart;

    lightCount = _PunctualLightCount;
    lightStart = 0;

    float t0 = max(tStart, DecodeLogarithmicDepthGeneralized(0, _VBufferDistanceDecodingParams));
    float de = _VBufferRcpSliceCount; // Log-encoded distance between slices

    // // The contribution of the ambient probe does not depend on the position,
    // // only on the direction and the length of the interval.
    // // SampleSH9() evaluates the 3-band SH in a given direction.
    // // The probe is already pre-convolved with the phase function.
    // // Note: anisotropic, no jittering.
    // float3 probeInScatteredRadiance = EvaluateVolumetricAmbientProbe(ray.centerDirWS);

    float3 totalRadiance = 0;
    float  opticalDepth  = 0;
    uint slice = 0;
    for (; slice < _VBufferSliceCount; slice++)
    {
        uint3 voxelCoord = uint3(posInput.positionSS, slice);

        float e1 = slice * de + de; // (slice + 1) / sliceCount
        float t1 = max(tStart, DecodeLogarithmicDepthGeneralized(e1, _VBufferDistanceDecodingParams));
        float tNext = t1;

    #if USE_DEPTH_BUFFER
        bool containsOpaqueGeometry = IsInRange(ray.geomDist, float2(t0, t1));

        if (containsOpaqueGeometry)
        {
            // Only integrate up to the opaque surface (make the voxel shorter, but not completely flat).
            // Note that we can NOT completely stop integrating when the ray reaches geometry, since
            // otherwise we get flickering at geometric discontinuities if reprojection is enabled.
            // In this case, a temporally stable light leak is better than flickering.
            t1 = max(t0 * 1.0001, ray.geomDist);
        }
    #endif
        float dt = t1 - t0; // Is geometry-aware
        if(dt <= 0.0)
        {
            _VBufferLighting[voxelCoord] = 0;
#ifdef ENABLE_REPROJECTION
            _VBufferFeedback[voxelCoord] = 0;
#endif
            t0 = t1;
            continue;
        }

        // Accurately compute the center of the voxel in the log space. It's important to perform
        // the inversion exactly, since the accumulated value of the integral is stored at the center.
        // We will use it for participating media sampling, asymmetric scattering and reprojection.
        float  t = DecodeLogarithmicDepthGeneralized(e1 - 0.5 * de, _VBufferDistanceDecodingParams);
        float3 centerWS = ray.originWS + t * ray.centerDirWS;

        // Sample the participating medium at the center of the voxel.
        // We consider it to be constant along the interval [t0, t1] (within the voxel).
        float4 density = LOAD_TEXTURE3D(_VBufferDensity, voxelCoord);

        float3 scattering = density.rgb;
        float  extinction = density.a;
        float  anisotropy = _GlobalFogAnisotropy;

        // Perform per-pixel randomization by adding an offset and then sampling uniformly
        // (in the log space) in a vein similar to Stochastic Universal Sampling:
        // https://en.wikipedia.org/wiki/Stochastic_universal_sampling
        float perPixelRandomOffset = GenerateHashedRandomFloat(posInput.positionSS);

    #ifdef ENABLE_REPROJECTION
        // This is a time-based sequence of 7 equidistant numbers from 1/14 to 13/14.
        // Each of them is the centroid of the interval of length 2/14.
        float rndVal = frac(perPixelRandomOffset + _VBufferSampleOffset.z);
    #else
        float rndVal = frac(perPixelRandomOffset + 0.5);
    #endif

        VoxelLighting aggregateLighting;
        ZERO_INITIALIZE(VoxelLighting, aggregateLighting);

        // Prevent division by 0.
        extinction = max(extinction, FLT_MIN);

        // if (featureFlags & LIGHTFEATUREFLAGS_DIRECTIONAL)
        {
            VoxelLighting lighting = EvaluateVoxelLightingDirectional(frameUniforms, samplerStruct,
                                                                      context, featureFlags, posInput,
                                                                      centerWS, ray, t0, t1, dt, rndVal,
                                                                      extinction, anisotropy, false);

            aggregateLighting.radianceNoPhase  += lighting.radianceNoPhase;
            aggregateLighting.radianceComplete += lighting.radianceComplete;
        }

        #ifdef SUPPORT_LOCAL_LIGHTS
        {
            VoxelLighting lighting = EvaluateVoxelLightingLocal(context, groupIdx, featureFlags, posInput,
                                                                lightCount, lightStart,
                                                                centerWS, ray, t0, t1, dt, rndVal,
                                                                extinction, anisotropy);

            aggregateLighting.radianceNoPhase  += lighting.radianceNoPhase;
            aggregateLighting.radianceComplete += lighting.radianceComplete;
        }
        #endif

    #ifdef ENABLE_REPROJECTION
        // Clamp here to prevent generation of NaNs.
        float4 voxelValue           = float4(aggregateLighting.radianceNoPhase, extinction * dt);
        float4 linearizedVoxelValue = LinearizeRGBD(voxelValue);
        float4 normalizedVoxelValue = linearizedVoxelValue * rcp(dt);
        float4 normalizedBlendValue = normalizedVoxelValue;

        // Reproject the history at 'centerWS'.
        float4 reprojValue = SampleVBuffer(TEXTURE3D_ARGS(_VBufferHistory, s_linear_clamp_sampler),
                                           centerWS,
                                           _PrevCamPosRWS.xyz,
                                           UNITY_MATRIX_PREV_VP(frameUniforms.cameraUniform),
                                           _VBufferPrevViewportSize,
                                           _VBufferHistoryViewportScale.xyz,
                                           _VBufferHistoryViewportLimit.xyz,
                                           _VBufferPrevDistanceEncodingParams,
                                           _VBufferPrevDistanceDecodingParams,
                                           false, false, true) * float4(GetInversePreviousExposureMultiplier().xxx, 1);

        bool reprojSuccess = (_VBufferHistoryIsValid != 0) && (reprojValue.a != 0);

        if (reprojSuccess)
        {
            // Perform temporal blending in the log space ("Pixar blend").
            normalizedBlendValue = lerp(normalizedVoxelValue, reprojValue, ComputeHistoryWeight());
        }

        // Store the feedback for the voxel.
        // TODO: dynamic lights (which update their position, rotation, cookie or shadow at runtime)
        // do not support reprojection and should neither read nor write to the history buffer.
        // This will cause them to alias, but it is the only way to prevent ghosting.
        _VBufferFeedback[voxelCoord] = clamp(normalizedBlendValue * float4(GetCurrentExposureMultiplier().xxx, 1), 0, HALF_MAX);

        float4 linearizedBlendValue = normalizedBlendValue * dt;
        float4 blendValue = DelinearizeRGBD(linearizedBlendValue);

    #ifdef ENABLE_ANISOTROPY
        // Estimate the influence of the phase function on the results of the current frame.
        float3 phaseCurrFrame;

        phaseCurrFrame.r = SafeDiv(aggregateLighting.radianceComplete.r, aggregateLighting.radianceNoPhase.r);
        phaseCurrFrame.g = SafeDiv(aggregateLighting.radianceComplete.g, aggregateLighting.radianceNoPhase.g);
        phaseCurrFrame.b = SafeDiv(aggregateLighting.radianceComplete.b, aggregateLighting.radianceNoPhase.b);

        // Warning: in general, this does not work!
        // For a voxel with a single light, 'phaseCurrFrame' is monochromatic, and since
        // we don't jitter anisotropy, its value does not change from frame to frame
        // for a static camera/scene. This is fine.
        // If you have two lights per voxel, we compute:
        // phaseCurrFrame = (phaseA * lightingA + phaseB * lightingB) / (lightingA + lightingB).
        // 'phaseA' and 'phaseB' are still (different) constants for a static camera/scene.
        // 'lightingA' and 'lightingB' are jittered, so they change from frame to frame.
        // Therefore, 'phaseCurrFrame' becomes temporarily unstable and can cause flickering in practice. :-(
        blendValue.rgb *= phaseCurrFrame;
    #endif // ENABLE_ANISOTROPY

    #else // NO REPROJECTION

    #ifdef ENABLE_ANISOTROPY
        float4 blendValue = float4(aggregateLighting.radianceComplete, extinction * dt);
    #else
        float4 blendValue = float4(aggregateLighting.radianceNoPhase,  extinction * dt);
    #endif // ENABLE_ANISOTROPY

    #endif // ENABLE_REPROJECTION

        // Compute the transmittance from the camera to 't0'.
        float transmittance = TransmittanceFromOpticalDepth(opticalDepth);

    #ifdef ENABLE_ANISOTROPY
        float phase = _CornetteShanksConstant;
    #else
        float phase = IsotropicPhaseFunction();
    #endif // ENABLE_ANISOTROPY

        // // Integrate the contribution of the probe over the interval.
        // // Integral{a, b}{Transmittance(0, t) * L_s(t) dt} = Transmittance(0, a) * Integral{a, b}{Transmittance(0, t - a) * L_s(t) dt}.
        // float3 probeRadiance = probeInScatteredRadiance * TransmittanceIntegralHomogeneousMedium(extinction, dt);

        // Accumulate radiance along the ray.
        // totalRadiance += transmittance * scattering * (phase * blendValue.rgb + probeRadiance);
        totalRadiance += transmittance * scattering * (phase * blendValue.rgb);

        // Compute the optical depth up to the center of the interval.
        opticalDepth += 0.5 * blendValue.a;

        // Store the voxel data.
        // Note: for correct filtering, the data has to be stored in the perceptual space.
        // This means storing the tone mapped radiance and transmittance instead of optical depth.
        // See "A Fresh Look at Generalized Sampling", p. 51.
        // TODO: re-enable tone mapping after implementing pre-exposure.
        _VBufferLighting[voxelCoord] = max(0,
            LinearizeRGBD(float4(/*FastTonemap*/(totalRadiance), opticalDepth)) *
            float4(GetCurrentExposureMultiplier(frameUniforms).xxx, 1));

        // Compute the optical depth up to the end of the interval.
        opticalDepth += 0.5 * blendValue.a;

        if (t0 * 0.99 > ray.maxDist)
        {
            break;
        }

        t0 = tNext;
    }

    for (; slice < _VBufferSliceCount; slice++)
    {
        uint3 voxelCoord = uint3(posInput.positionSS, slice);
        _VBufferLighting[voxelCoord] = 0;
#ifdef ENABLE_REPROJECTION
        _VBufferFeedback[voxelCoord] = 0;
#endif

    }
}

[numthreads(GROUP_SIZE_1D, GROUP_SIZE_1D, 1)]
void VolumetricLighting(uint3 dispatchThreadId : SV_DispatchThreadID,
                        uint2 groupId          : SV_GroupID,
                        uint2 groupThreadId    : SV_GroupThreadID,
                        int   groupIndex       : SV_GroupIndex)
{
    uint2 groupOffset = groupId * GROUP_SIZE_1D;
    uint2 voxelCoord  = groupOffset + groupThreadId;

    // Reminder: our voxels are sphere-capped right frustums (truncated right pyramids).
    // The curvature of the front and back faces is quite gentle, so we can use
    // the right frustum approximation (thus the front and the back faces are squares).
    // Note, that since we still rely on the perspective camera model, pixels at the center
    // of the screen correspond to larger solid angles than those at the edges.
    // Basically, sizes of front and back faces depend on the XY coordinate.
    // https://www.desmos.com/calculator/i3rkesvidk

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<ShaderVariablesVolumetric> shaderVariablesVolumetric = ResourceDescriptorHeap[shaderVariablesVolumetricIndex];
    Texture3D<float4> _VBufferDensity = ResourceDescriptorHeap[_VBufferDensityIndex];
    Texture2D<float> _DepthPyramidRT = ResourceDescriptorHeap[_DepthPyramidIndex];
    RWTexture3D<float4> _VBufferLighting = ResourceDescriptorHeap[_VBufferLightingIndex];

    SamplerStruct samplerStruct = GetSamplerStruct();
    
    VolumetricLightingUniform volumetricLightingUniform = mFrameUniforms.volumetricLightingUniform;
    VBufferUniform volumeLightUniform = mFrameUniforms.vBufferUniform;

    float4 _VBufferViewportSize = volumeLightUniform._VBufferViewportSize;
    
    float4 _ZBufferParams = mFrameUniforms.cameraUniform._ZBufferParams;
    float g_fNearPlane = mFrameUniforms.cameraUniform._CameraNear;
    
    float4x4 _VBufferCoordToViewDirWS = shaderVariablesVolumetric._VBufferCoordToViewDirWS;
    float _VBufferUnitDepthTexelSpacing = shaderVariablesVolumetric._VBufferUnitDepthTexelSpacing;
    float4 _VBufferSampleOffset = shaderVariablesVolumetric._VBufferSampleOffset;
    float _VBufferVoxelSize = shaderVariablesVolumetric._VBufferVoxelSize;
    
    float3 F = GetViewForwardDir(mFrameUniforms);
    float3 U = GetViewUpDir(mFrameUniforms);
    float3 R = cross(F, U);

    float2 centerCoord = voxelCoord + float2(0.5, 0.5);

    uint voxelWidth, voxelHeight, voxelDepth;
    _VBufferDensity.GetDimensions(voxelWidth, voxelHeight, voxelDepth);
    float2 voxelUV = centerCoord / float2(voxelWidth, voxelHeight);
    float3 rayDirWS = mul(UNITY_MATRIX_I_P(mFrameUniforms.cameraUniform), float4(voxelUV * 2 - 1, 0, 1)).xyz;
    
    // Compute a ray direction s.t. ViewSpace(rayDirWS).z = 1.
    // float3 rayDirWS       = mul(-float4(centerCoord, 1, 1), _VBufferCoordToViewDirWS[unity_StereoEyeIndex]).xyz;
    float3 rightDirWS     = cross(rayDirWS, U);
    float  rcpLenRayDir   = rsqrt(dot(rayDirWS, rayDirWS));
    float  rcpLenRightDir = rsqrt(dot(rightDirWS, rightDirWS));

    JitteredRay ray;
    ray.originWS    = GetCurrentViewPosition(mFrameUniforms);
    ray.centerDirWS = rayDirWS * rcpLenRayDir; // Normalize

    float FdotD = dot(F, ray.centerDirWS);
    float unitDistFaceSize = _VBufferUnitDepthTexelSpacing * FdotD * rcpLenRayDir;

    ray.xDirDerivWS = rightDirWS * (rcpLenRightDir * unitDistFaceSize); // Normalize & rescale
    ray.yDirDerivWS = cross(ray.xDirDerivWS, ray.centerDirWS); // Will have the length of 'unitDistFaceSize' by construction

#ifdef ENABLE_REPROJECTION
    float2 sampleOffset = _VBufferSampleOffset.xy;
#else
    float2 sampleOffset = 0;
#endif

    ray.jitterDirWS = normalize(ray.centerDirWS + sampleOffset.x * ray.xDirDerivWS + sampleOffset.y * ray.yDirDerivWS);
    float tStart = g_fNearPlane / dot(ray.jitterDirWS, F);

    // We would like to determine the screen pixel (at the full resolution) which
    // the jittered ray corresponds to. The exact solution can be obtained by intersecting
    // the ray with the screen plane, e.i. (ViewSpace(jitterDirWS).z = 1). That's a little expensive.
    // So, as an approximation, we ignore the curvature of the frustum.
    uint2 pixelCoord = (uint2)((voxelCoord + 0.5 + sampleOffset) * _VBufferVoxelSize);

#ifdef VL_PRESET_OPTIMAL
    // The entire thread group is within the same light tile.
    uint2 tileCoord = groupOffset * VBUFFER_VOXEL_SIZE / TILE_SIZE_BIG_TILE;
#else
    // No compile-time optimizations, no scalarization.
    uint2 tileCoord = pixelCoord/* / TILE_SIZE_BIG_TILE*/;
#endif
    // uint  tileIndex = tileCoord.x + _NumTileBigTileX * tileCoord.y;
    // // This clamp is important as _VBufferVoxelSize can have float value which can cause en overflow (Crash on Vulkan and Metal)
    // tileIndex = min(tileIndex, _NumTileBigTileX * _NumTileBigTileY);

    // Do not jitter 'voxelCoord' else. It's expected to correspond to the center of the voxel.
    PositionInputs posInput = GetPositionInput(voxelCoord, _VBufferViewportSize.zw, tileCoord);

    ray.geomDist = FLT_INF;
    ray.maxDist = FLT_INF;
#if USE_DEPTH_BUFFER
    float deviceDepth = LoadCameraDepth(_DepthPyramidRT, pixelCoord);

    if (deviceDepth > 0) // Skip the skybox
    {
        // Convert it to distance along the ray. Doesn't work with tilt shift, etc.
        float linearDepth = LinearEyeDepth(deviceDepth, _ZBufferParams);
        ray.geomDist = linearDepth * rcp(dot(ray.jitterDirWS, F));

        // float2 UV = posInput.positionNDC;

        // This should really be using a max sampler here. This is a bit overdilating given that it is already dilated.
        // Better to be safer though.
        // float4 d = GATHER_RED_TEXTURE2D_X(_MaxZMaskTexture, s_point_clamp_sampler, UV) * rcp(dot(ray.jitterDirWS, F));
        float4 d = rcp(dot(ray.jitterDirWS, F));
        ray.maxDist = max(Max3(d.x, d.y, d.z), d.w);
    }
#endif

    // TODO
    LightLoopContext context;
    context.shadowContext = InitShadowContext(mFrameUniforms);
    uint featureFlags     = 0xFFFFFFFF;

    FillVolumetricLightingBuffer(
        _VBufferLighting, _VBufferDensity, samplerStruct,
        mFrameUniforms, volumetricLightingUniform, volumeLightUniform,
        context, featureFlags, posInput, groupIndex, ray, tStart);
}
