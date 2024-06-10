#ifndef __BUILTINGIUTILITIES_HLSL__
#define __BUILTINGIUTILITIES_HLSL__

// Include the IndirectDiffuseMode enum
#include "../Lighting/ScreenSpaceLighting/ScreenSpaceGlobalIllumination.hlsl"
#include "../Lighting/ScreenSpaceLighting/ScreenSpaceReflection.hlsl"

// // We need to define this before including ProbeVolume.hlsl as that file expects this function to be defined.
// // AmbientProbe Data is fetch directly from a compute buffer to remain on GPU and is preconvolved with clamped cosinus
// float3 EvaluateAmbientProbe(float3 normalWS)
// {
//     return SampleSH9(_AmbientProbeData, normalWS);
// }
//
// float3 EvaluateLightProbe(float3 normalWS)
// {
//     float4 SHCoefficients[7];
//     SHCoefficients[0] = unity_SHAr;
//     SHCoefficients[1] = unity_SHAg;
//     SHCoefficients[2] = unity_SHAb;
//     SHCoefficients[3] = unity_SHBr;
//     SHCoefficients[4] = unity_SHBg;
//     SHCoefficients[5] = unity_SHBb;
//     SHCoefficients[6] = unity_SHC;
//
//     return SampleSH9(SHCoefficients, normalWS);
// }

void EvaluateLightProbeBuiltin(float3 positionRWS, float3 normalWS, float3 backNormalWS, inout float3 bakeDiffuseLighting, inout float3 backBakeDiffuseLighting)
{
//     if (unity_ProbeVolumeParams.x == 0.0)
//     {
//         bakeDiffuseLighting += EvaluateLightProbe(normalWS);
//         backBakeDiffuseLighting += EvaluateLightProbe(backNormalWS);
//     }
//     else
//     {
//         // Note: Probe volume here refer to LPPV not APV
// #if SHADEROPTIONS_CAMERA_RELATIVE_RENDERING == 1
//         if (unity_ProbeVolumeParams.y == 0.0)
//             positionRWS += _WorldSpaceCameraPos;
// #endif
//         SampleProbeVolumeSH4(TEXTURE3D_ARGS(unity_ProbeVolumeSH, samplerunity_ProbeVolumeSH), positionRWS, normalWS, backNormalWS, GetProbeVolumeWorldToObject(),
//             unity_ProbeVolumeParams.y, unity_ProbeVolumeParams.z, unity_ProbeVolumeMin.xyz, unity_ProbeVolumeSizeInv.xyz, bakeDiffuseLighting, backBakeDiffuseLighting);
//     }
}

// No need to initialize bakeDiffuseLighting and backBakeDiffuseLighting must be initialize outside the function
void SampleBakedGI(
    PositionInputs posInputs,
    float3 normalWS,
    float3 backNormalWS,
    uint renderingLayers,
    float2 uvStaticLightmap,
    float2 uvDynamicLightmap,
    bool needToIncludeAPV,
    out float3 bakeDiffuseLighting,
    out float3 backBakeDiffuseLighting)
{
    bakeDiffuseLighting = float3(0, 0, 0);
    backBakeDiffuseLighting = float3(0, 0, 0);

    // If we have SSGI/RTGI enabled in which case we don't want to read Lightmaps/Lightprobe at all.
    // If we have Mixed RTR or GI enabled, we need to have the lightmap exported in the case of the gbuffer as they will be consumed and will be used if the pixel is ray marched.
    // This behavior only applies to opaque Materials as Transparent one don't receive SSGI/RTGI/Mixed lighting.
    // The check need to be here to work with both regular shader and shader graph
    // Note: With Probe volume the code is skip in the lightloop if any of those effects is enabled
    // We prevent to read GI only if we are not raytrace pass that are used to fill the RTGI/Mixed buffer need to be executed normaly
// #if !defined(_SURFACE_TYPE_TRANSPARENT) && (SHADERPASS != SHADERPASS_RAYTRACING_INDIRECT) && (SHADERPASS != SHADERPASS_RAYTRACING_GBUFFER)
//     if (_IndirectDiffuseMode != INDIRECTDIFFUSEMODE_OFF
// #if (SHADERPASS == SHADERPASS_GBUFER)
//         && _IndirectDiffuseMode != INDIRECTDIFFUSEMODE_MIXED && _ReflectionsMode != REFLECTIONSMODE_MIXED
// #endif
//         )
//         return;
// #endif

    float3 positionRWS = posInputs.positionWS;

    EvaluateLightProbeBuiltin(positionRWS, normalWS, backNormalWS, bakeDiffuseLighting, backBakeDiffuseLighting);

//     // We only want to apply the ray tracing ambient probe dimmer on the ambient probe
//     // and legacy light probes (and obviously only in the ray tracing shaders).
// #if defined(SHADER_STAGE_RAY_TRACING)
//     bakeDiffuseLighting *= _RayTracingAmbientProbeDimmer;
//     backBakeDiffuseLighting *= _RayTracingAmbientProbeDimmer;
// #endif
// #endif
}

void SampleBakedGI(
    PositionInputs posInputs,
    float3 normalWS,
    float3 backNormalWS,
    uint renderingLayers,
    float2 uvStaticLightmap,
    float2 uvDynamicLightmap,   
    out float3 bakeDiffuseLighting,
    out float3 backBakeDiffuseLighting)
{
    bool needToIncludeAPV = false;
    SampleBakedGI(posInputs, normalWS, backNormalWS, renderingLayers, uvStaticLightmap, uvDynamicLightmap, needToIncludeAPV, bakeDiffuseLighting, backBakeDiffuseLighting);
}

float3 SampleBakedGI(float3 positionRWS, float3 normalWS, float2 uvStaticLightmap, float2 uvDynamicLightmap, bool needToIncludeAPV = false)
{
    // Need PositionInputs for indexing probe volume clusters, but they are not availbile from the current SampleBakedGI() function signature.
    // Reconstruct.
    uint renderingLayers = 0;
    PositionInputs posInputs;
    ZERO_INITIALIZE(PositionInputs, posInputs);
    posInputs.positionWS = positionRWS;

    const float3 backNormalWSUnused = 0.0;
    float3 bakeDiffuseLighting;
    float3 backBakeDiffuseLightingUnused;
    SampleBakedGI(posInputs, normalWS, backNormalWSUnused, renderingLayers, uvStaticLightmap, uvDynamicLightmap, needToIncludeAPV, bakeDiffuseLighting, backBakeDiffuseLightingUnused);

    return bakeDiffuseLighting;
}


// float4 SampleShadowMask(float3 positionRWS, float2 uvStaticLightmap) // normalWS not use for now
// {
// #if defined(LIGHTMAP_ON)
//     float2 uv = uvStaticLightmap * unity_LightmapST.xy + unity_LightmapST.zw;
//     return SAMPLE_TEXTURE2D_LIGHTMAP(SHADOWMASK_NAME, SHADOWMASK_SAMPLER_NAME, SHADOWMASK_SAMPLE_EXTRA_ARGS); // Can't reuse sampler from Lightmap because with shader graph, the compile could optimize out the lightmaps if metal is 1
// #elif (defined(PROBE_VOLUMES_L1) || defined(PROBE_VOLUMES_L2))
//     return 1;
// #else
//     float4 rawOcclusionMask;
//     if (unity_ProbeVolumeParams.x == 1.0)
//     {
// #if SHADEROPTIONS_CAMERA_RELATIVE_RENDERING == 1
//         if (unity_ProbeVolumeParams.y == 0.0)
//             positionRWS += _WorldSpaceCameraPos;
// #endif
//         rawOcclusionMask = SampleProbeOcclusion(TEXTURE3D_ARGS(unity_ProbeVolumeSH, samplerunity_ProbeVolumeSH), positionRWS, GetProbeVolumeWorldToObject(),
//             unity_ProbeVolumeParams.y, unity_ProbeVolumeParams.z, unity_ProbeVolumeMin.xyz, unity_ProbeVolumeSizeInv.xyz);
//     }
//     else
//     {
//         // Note: Default value when the feature is not enabled is float(1.0, 1.0, 1.0, 1.0) in C++
//         rawOcclusionMask = unity_ProbesOcclusion;
//     }
//
//     return rawOcclusionMask;
// #endif
// }

#endif
