#ifndef ENTITY_LIGHTING_INCLUDED
#define ENTITY_LIGHTING_INCLUDED

#include "../ShaderLibrary/Common.hlsl"
#include "../ShaderLibrary/Color.hlsl"
#include "../ShaderLibrary/SphericalHarmonics.hlsl"

// This sample a 3D volume storing SH
// Volume is store as 3D texture with 4 R, G, B, Occ set of 4 coefficient store atlas in same 3D texture. Occ is use for occlusion.
// TODO: the packing here is inefficient as we will fetch values far away from each other and they may not fit into the cache - Suggest we pack RGB continuously
// TODO: The calcul of texcoord could be perform with a single matrix multicplication calcualted on C++ side that will fold probeVolumeMin and probeVolumeSizeInv into it and handle the identity case, no reasons to do it in C++ (ask Ionut about it)
// It should also handle the camera relative path (if the render pipeline use it)
// bakeDiffuseLighting and backBakeDiffuseLighting must be initialize outside the function
void SampleProbeVolumeSH4(TEXTURE3D_PARAM(SHVolumeTexture, SHVolumeSampler), float3 positionWS, float3 normalWS, float3 backNormalWS, float4x4 WorldToTexture,
                            float transformToLocal, float texelSizeX, float3 probeVolumeMin, float3 probeVolumeSizeInv,
                            inout float3 bakeDiffuseLighting, inout float3 backBakeDiffuseLighting)
{
    float3 position = (transformToLocal == 1.0) ? mul(WorldToTexture, float4(positionWS, 1.0)).xyz : positionWS;
    float3 texCoord = (position - probeVolumeMin) * probeVolumeSizeInv.xyz;
    // Each component is store in the same texture 3D. Each use one quater on the x axis
    // Here we get R component then increase by step size (0.25) to get other component. This assume 4 component
    // but last one is not used.
    // Clamp to edge of the "internal" texture, as R is from half texel to size of R texture minus half texel.
    // This avoid leaking
    texCoord.x = clamp(texCoord.x * 0.25, 0.5 * texelSizeX, 0.25 - 0.5 * texelSizeX);

    float4 shAr = SAMPLE_TEXTURE3D_LOD(SHVolumeTexture, SHVolumeSampler, texCoord, 0);
    texCoord.x += 0.25;
    float4 shAg = SAMPLE_TEXTURE3D_LOD(SHVolumeTexture, SHVolumeSampler, texCoord, 0);
    texCoord.x += 0.25;
    float4 shAb = SAMPLE_TEXTURE3D_LOD(SHVolumeTexture, SHVolumeSampler, texCoord, 0);

    bakeDiffuseLighting += SHEvalLinearL0L1(normalWS, shAr, shAg, shAb);
    backBakeDiffuseLighting += SHEvalLinearL0L1(backNormalWS, shAr, shAg, shAb);
}

// Just a shortcut that call function above
float3 SampleProbeVolumeSH4(TEXTURE3D_PARAM(SHVolumeTexture, SHVolumeSampler), float3 positionWS, float3 normalWS, float4x4 WorldToTexture,
                                float transformToLocal, float texelSizeX, float3 probeVolumeMin, float3 probeVolumeSizeInv)
{
    float3 backNormalWSUnused = 0.0;
    float3 bakeDiffuseLighting = 0.0;
    float3 backBakeDiffuseLightingUnused = 0.0;
    SampleProbeVolumeSH4(TEXTURE3D_ARGS(SHVolumeTexture, SHVolumeSampler), positionWS, normalWS, backNormalWSUnused, WorldToTexture,
                            transformToLocal, texelSizeX, probeVolumeMin, probeVolumeSizeInv,
                            bakeDiffuseLighting, backBakeDiffuseLightingUnused);
    return bakeDiffuseLighting;
}

// The SphericalHarmonicsL2 coefficients are packed into 7 coefficients per color channel instead of 9.
// The packing from 9 to 7 is done from engine code and will use the alpha component of the pixel to store an additional SH coefficient.
// The 3D atlas texture will contain 7 SH coefficient parts.
// bakeDiffuseLighting and backBakeDiffuseLighting must be initialize outside the function
void SampleProbeVolumeSH9(TEXTURE3D_PARAM(SHVolumeTexture, SHVolumeSampler), float3 positionWS, float3 normalWS, float3 backNormalWS, float4x4 WorldToTexture,
                                           float transformToLocal, float texelSizeX, float3 probeVolumeMin, float3 probeVolumeSizeInv,
                                           inout float3 bakeDiffuseLighting, inout float3 backBakeDiffuseLighting)
{
    float3 position = (transformToLocal == 1.0f) ? mul(WorldToTexture, float4(positionWS, 1.0)).xyz : positionWS;
    float3 texCoord = (position - probeVolumeMin) * probeVolumeSizeInv;

    const uint shCoeffCount = 7;
    const float invShCoeffCount = 1.0f / float(shCoeffCount);

    // We need to compute proper X coordinate to sample into the atlas.
    texCoord.x = texCoord.x / shCoeffCount;

    // Clamp the x coordinate otherwise we'll have leaking between RGB coefficients.
    float texCoordX = clamp(texCoord.x, 0.5f * texelSizeX, invShCoeffCount - 0.5f * texelSizeX);

    float4 SHCoefficients[7];

    for (uint i = 0; i < shCoeffCount; i++)
    {
        texCoord.x = texCoordX + i * invShCoeffCount;
        SHCoefficients[i] = SAMPLE_TEXTURE3D_LOD(SHVolumeTexture, SHVolumeSampler, texCoord, 0);
    }

    bakeDiffuseLighting += SampleSH9(SHCoefficients, normalize(normalWS));
    backBakeDiffuseLighting += SampleSH9(SHCoefficients, normalize(backNormalWS));
}

// Just a shortcut that call function above
float3 SampleProbeVolumeSH9(TEXTURE3D_PARAM(SHVolumeTexture, SHVolumeSampler), float3 positionWS, float3 normalWS, float4x4 WorldToTexture,
                                float transformToLocal, float texelSizeX, float3 probeVolumeMin, float3 probeVolumeSizeInv)
{
    float3 backNormalWSUnused = 0.0;
    float3 bakeDiffuseLighting = 0.0;
    float3 backBakeDiffuseLightingUnused = 0.0;
    SampleProbeVolumeSH9(TEXTURE3D_ARGS(SHVolumeTexture, SHVolumeSampler), positionWS, normalWS, backNormalWSUnused, WorldToTexture,
                            transformToLocal, texelSizeX, probeVolumeMin, probeVolumeSizeInv,
                            bakeDiffuseLighting, backBakeDiffuseLightingUnused);
    return bakeDiffuseLighting;
}

float4 SampleProbeOcclusion(TEXTURE3D_PARAM(SHVolumeTexture, SHVolumeSampler), float3 positionWS, float4x4 WorldToTexture,
                            float transformToLocal, float texelSizeX, float3 probeVolumeMin, float3 probeVolumeSizeInv)
{
    float3 position = (transformToLocal == 1.0) ? mul(WorldToTexture, float4(positionWS, 1.0)).xyz : positionWS;
    float3 texCoord = (position - probeVolumeMin) * probeVolumeSizeInv.xyz;

    // Sample fourth texture in the atlas
    // We need to compute proper U coordinate to sample.
    // Clamp the coordinate otherwize we'll have leaking between ShB coefficients and Probe Occlusion(Occ) info
    texCoord.x = max(texCoord.x * 0.25 + 0.75, 0.75 + 0.5 * texelSizeX);

    return SAMPLE_TEXTURE3D(SHVolumeTexture, SHVolumeSampler, texCoord);
}

#endif // ENTITY_LIGHTING_INCLUDED
