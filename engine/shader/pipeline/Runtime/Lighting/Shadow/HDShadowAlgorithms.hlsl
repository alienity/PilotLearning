#ifndef HD_SHADOW_ALGORITHMS_INCLUDED
#define HD_SHADOW_ALGORITHMS_INCLUDED
// Configure which shadow algorithms to use per shadow level quality

// Since we use slope-scale bias, the constant bias is for now set as a small fixed value
#define FIXED_UNIFORM_BIAS (1.0f / 65536.0f)

// For non-fragment shaders we might skip the variant with the quality as shadows might not be used, if that's the case define something just to make the compiler happy in case the quality is not defined.
#ifndef SHADER_STAGE_FRAGMENT
    #if !defined(SHADOW_ULTRA_LOW) && !defined(SHADOW_LOW) && !defined(SHADOW_MEDIUM) && !defined(SHADOW_HIGH) // ultra low come from volumetricLighting.compute
        #define SHADOW_MEDIUM
    #endif
    #if !defined(AREA_SHADOW_LOW) && !defined(AREA_SHADOW_MEDIUM) && !defined(AREA_SHADOW_HIGH) // low come from volumetricLighting.compute
        #define AREA_SHADOW_MEDIUM
    #endif
#endif

// WARNINGS:
// Keep in sync with both HDShadowManager::GetDirectionalShadowAlgorithm() and GetPunctualFilterWidthInTexels() in C# as well!

#ifdef SHADOW_ULTRA_LOW
#define PUNCTUAL_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_PCF_Tent_3x3(sd.isInCachedAtlas ? _CachedShadowAtlasSize.zwxy : _ShadowAtlasSize.zwxy, posTC, tex, samp, bias)
#define DIRECTIONAL_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_Gather_PCF(_CascadeShadowAtlasSize.zwxy, posTC, tex, samp, bias)
#elif defined(SHADOW_LOW)
#define PUNCTUAL_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_PCF_Tent_3x3(sd.isInCachedAtlas ? _CachedShadowAtlasSize.zwxy : _ShadowAtlasSize.zwxy, posTC, tex, samp, bias)
#define DIRECTIONAL_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_PCF_Tent_5x5(_CascadeShadowAtlasSize.zwxy, posTC, tex, samp, bias)
#elif defined(SHADOW_MEDIUM)
#define PUNCTUAL_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_PCF_Tent_5x5(sd.isInCachedAtlas ? _CachedShadowAtlasSize.zwxy : _ShadowAtlasSize.zwxy, posTC, tex, samp, bias)
#define DIRECTIONAL_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_PCF_Tent_7x7(_CascadeShadowAtlasSize.zwxy, posTC, tex, samp, bias)
// Note: currently quality settings for PCSS need to be expose in UI and is control in HDLightUI.cs file IsShadowSettings
#elif defined(SHADOW_HIGH)
#define PUNCTUAL_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_PCSS(posTC, posSS, sd.shadowMapSize.xy * (sd.isInCachedAtlas ? _CachedShadowAtlasSize.zw : _ShadowAtlasSize.zw), sd.atlasOffset, sd.shadowFilterParams0.x, sd.shadowFilterParams0.w, asint(sd.shadowFilterParams0.y), asint(sd.shadowFilterParams0.z), tex, samp, s_point_clamp_sampler, bias, sd.zBufferParam, true, (sd.isInCachedAtlas ? _CachedShadowAtlasSize.xz : _ShadowAtlasSize.xz))
#define DIRECTIONAL_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_PCSS(posTC, posSS, sd.shadowMapSize.xy * _CascadeShadowAtlasSize.zw, sd.atlasOffset, sd.shadowFilterParams0.x, sd.shadowFilterParams0.w, asint(sd.shadowFilterParams0.y), asint(sd.shadowFilterParams0.z), tex, samp, s_point_clamp_sampler, bias, sd.zBufferParam, false, _CascadeShadowAtlasSize.xz)
#endif

#ifdef AREA_SHADOW_LOW
#define AREA_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_EVSM_1tap(posTC, sd.shadowFilterParams0.y, sd.shadowFilterParams0.z, sd.shadowFilterParams0.xx, false, tex, s_linear_clamp_sampler)
#define AREA_LIGHT_SHADOW_IS_EVSM 1
#elif defined(AREA_SHADOW_MEDIUM)
#define AREA_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_EVSM_1tap(posTC, sd.shadowFilterParams0.y, sd.shadowFilterParams0.z, sd.shadowFilterParams0.xx, false, tex, s_linear_clamp_sampler)
#define AREA_LIGHT_SHADOW_IS_EVSM 1
// Note: currently quality settings for PCSS need to be expose in UI and is control in HDLightUI.cs file IsShadowSettings
#elif defined(AREA_SHADOW_HIGH)
#define AREA_FILTER_ALGORITHM(sd, posSS, posTC, tex, samp, bias) SampleShadow_PCSS_Area(posTC, positionSS, sd.shadowMapSize.xy * texelSize, sd.atlasOffset, sd.shadowFilterParams0.x, sd.shadowFilterParams0.w, asint(sd.shadowFilterParams0.y), asint(sd.shadowFilterParams0.z), tex, s_linear_clamp_compare_sampler, s_point_clamp_sampler, FIXED_UNIFORM_BIAS, sd.zBufferParam, true, (sd.isInCachedAtlas ? _CachedAreaShadowAtlasSize.xz : _AreaShadowAtlasSize.xz))
#endif

#ifndef PUNCTUAL_FILTER_ALGORITHM
#error "Undefined punctual shadow filter algorithm"
#endif
#ifndef DIRECTIONAL_FILTER_ALGORITHM
#error "Undefined directional shadow filter algorithm"
#endif
#ifndef AREA_FILTER_ALGORITHM
#error "Undefined area shadow filter algorithm"
#endif

// Since we use slope-scale bias, the constant bias is for now set as a small fixed value
#define FIXED_UNIFORM_BIAS (1.0f / 65536.0f)

float4 EvalShadow_WorldToShadow(HDShadowData sd, float3 positionWS, bool perspProj)
{
    // Note: Due to high VGRP load we can't use the whole view projection matrix, instead we reconstruct it from
    // rotation, position and projection vectors (projection and position are stored in SGPR)
#if 0
    return mul(viewProjection, float4(positionWS, 1));
#else

    if(perspProj)
    {
        positionWS = positionWS - sd.pos;
        float3x3 view = { sd.rot0, sd.rot1, sd.rot2 };
        positionWS = mul(view, positionWS);
    }
    else
    {
        float3x4 view;
        view[0] = float4(sd.rot0, sd.pos.x);
        view[1] = float4(sd.rot1, sd.pos.y);
        view[2] = float4(sd.rot2, sd.pos.z);
        positionWS = mul(view, float4(positionWS, 1.0)).xyz;
    }

    float4x4 proj;
    proj = 0.0;
    proj._m00 = sd.proj[0];
    proj._m11 = sd.proj[1];
    proj._m22 = sd.proj[2];
    proj._m23 = sd.proj[3];
    if(perspProj)
        proj._m32 = -1.0;
    else
        proj._m33 = 1.0;

    return mul(proj, float4(positionWS, 1.0));
#endif
}

// function called by spot, point, area and directional eval routines to calculate shadow coordinates
float3 EvalShadow_GetTexcoordsAtlas(HDShadowData sd, float2 atlasSizeRcp, float3 positionWS, out float3 posNDC, bool perspProj)
{
    float4 posCS = EvalShadow_WorldToShadow(sd, positionWS, perspProj);
    // Avoid (0 / 0 = NaN).
    posNDC = (perspProj && posCS.w != 0) ? (posCS.xyz / posCS.w) : posCS.xyz;

    // calc TCs
    float3 posTC = float3(saturate(posNDC.xy * 0.5 + 0.5), posNDC.z);
    posTC.xy = posTC.xy * sd.shadowMapSize.xy * atlasSizeRcp;

    return posTC;
}

float3 EvalShadow_GetTexcoordsAtlas(HDShadowData sd, float2 atlasSizeRcp, float3 positionWS, bool perspProj)
{
    float3 ndc;
    return EvalShadow_GetTexcoordsAtlas(sd, atlasSizeRcp, positionWS, ndc, perspProj);
}

float2 EvalShadow_GetTexcoordsAtlas(HDShadowData sd, float2 atlasSizeRcp, float3 positionWS, out float2 closestSampleNDC, bool perspProj)
{
    float4 posCS = EvalShadow_WorldToShadow(sd, positionWS, perspProj);
    // Avoid (0 / 0 = NaN).
    float2 posNDC = (perspProj && posCS.w != 0) ? (posCS.xy / posCS.w) : posCS.xy;

    // calc TCs
    float2 posTC = posNDC * 0.5 + 0.5;
    closestSampleNDC = (floor(posTC * sd.shadowMapSize.xy) + 0.5) * sd.shadowMapSize.zw * 2.0 - float2(1.0f, 1.0f);
    return posTC * sd.shadowMapSize.xy * atlasSizeRcp + sd.atlasOffset;
}

uint2 EvalShadow_GetIntTexcoordsAtlas(HDShadowData sd, float4 atlasSize, float3 positionWS, out float2 closestSampleNDC, bool perspProj)
{
    float2 texCoords = EvalShadow_GetTexcoordsAtlas(sd, atlasSize.zw, positionWS, closestSampleNDC, perspProj);
    return uint2(texCoords * atlasSize.xy);
}

float3 EvalShadow_NormalBiasOrtho(float worldTexelSize, float normalBiasFactor, float3 normalWS)
{
    return normalWS * normalBiasFactor * worldTexelSize;
}

float3 EvalShadow_NormalBiasProj(float worldTexelSize, float normalBiasFactor, float3 normalWS, float L_dist)
{
    return EvalShadow_NormalBiasOrtho(worldTexelSize, normalBiasFactor, normalWS) * L_dist;
}

void EvalShadow_MinMaxCoords(HDShadowData sd, float2 texelSize, out float2 minCoord, out float2 maxCoord, int blurPassesScale = 1)
{
    maxCoord = (sd.shadowMapSize.xy - 0.5f * blurPassesScale) * texelSize + sd.atlasOffset;
    minCoord = sd.atlasOffset + texelSize * blurPassesScale;
}

bool EvalShadow_PositionTC(HDShadowData sd, float2 texelSize, float3 positionWS, float3 normalBias, bool perspective, out float3 posTC, int blurPassesScale = 1)
{
    positionWS += sd.cacheTranslationDelta.xyz;
    positionWS += normalBias;

    posTC = EvalShadow_GetTexcoordsAtlas(sd, texelSize, positionWS, perspective);

    float2 minCoord, maxCoord;
    EvalShadow_MinMaxCoords(sd, texelSize, minCoord, maxCoord, blurPassesScale);
    return !(any(posTC.xy > maxCoord) || any(posTC.xy < minCoord));
}

//
//  Point shadows
//

float EvalShadow_PunctualDepth(HDShadowData sd, Texture2D tex, SamplerComparisonState samp, float2 positionSS, float3 positionWS, float3 normalWS, float3 L, float L_dist, bool perspective)
{
    float2 texelSize = sd.isInCachedAtlas ? _CachedShadowAtlasSize.zw : _ShadowAtlasSize.zw;
    float3 normalBias = EvalShadow_NormalBiasProj(sd.worldTexelSize, sd.normalBias, normalWS, perspective ? L_dist : 1.0f);
    float3 posTC;

    return EvalShadow_PositionTC(sd, texelSize, positionWS, normalBias, perspective, posTC) ? PUNCTUAL_FILTER_ALGORITHM(sd, positionSS, posTC, tex, samp, FIXED_UNIFORM_BIAS) : 1.0f;
}

//
//  Area light shadows
//

float EvalShadow_AreaDepth(HDShadowData sd, Texture2D tex, float2 positionSS, float3 positionWS, float3 normalWS, float3 L, float L_dist, bool perspective)
{
#ifdef AREA_LIGHT_SHADOW_IS_EVSM
    // Needed as blurring might cause some leaks. It might be overclipping, but empirically is a good value.
    int blurPassesScale = (1 + min(4, sd.shadowFilterParams0.w) * 4.0f);
    float3 normalBias = 0;
#else
    int blurPassesScale = 1;
    float3 normalBias = EvalShadow_NormalBiasProj(sd.worldTexelSize, sd.normalBias, normalWS, L_dist);
#endif

    float2 texelSize = sd.isInCachedAtlas ? _CachedAreaShadowAtlasSize.zw : _AreaShadowAtlasSize.zw;
    float3 posTC;
    return EvalShadow_PositionTC(sd, texelSize, positionWS, normalBias, perspective, posTC, blurPassesScale) ? AREA_FILTER_ALGORITHM(sd, positionSS, posTC, tex, s_linear_clamp_compare_sampler, FIXED_UNIFORM_BIAS) : 1.0f;
}

//
//  Directional shadows (cascaded shadow map)
//

HDShadowData EvalShadow_GetHDShadowData(HDShadowContext shadowContext, int index, float3 positionWS, out int cascadeLevel) 
{
    // float4x4 viewMatrix, float3 worldPosition, uint4 shadow_bounds
    HDShadowData shadowData = shadowContext.shadowDatas[index];
    uint cascadeNumber = shadowData.cascadeNumber;
    uint currentLevel = shadowData.cascadeLevel;

    float3 positionVS = mul(shadowData.viewMatrix, float4(positionWS, 1)).xyz;
    float2 maxDistance = float2(abs(positionVS.x), abs(positionVS.y));

    cascadeLevel = -1;
    
    if (all(maxDistance < shadowData.shadowBounds.xy))
    {
        cascadeLevel = 0;
    }
    else
    {
        for (int i = currentLevel + 1; i < cascadeNumber; i++)
        {
            shadowData = shadowContext.shadowDatas[i];
            positionVS = mul(shadowData.viewMatrix, float4(positionWS, 1)).xyz;
            maxDistance = float2(abs(positionVS.x), abs(positionVS.y));
            if(all(maxDistance < shadowData.shadowBounds.xy))
            {
                cascadeLevel = i;
                break;
            }
        }
    }
    return shadowData;
}

float EvalShadow_CascadedDepth_Blend_SplitIndex(inout HDShadowContext shadowContext, Texture2D tex, SamplerComparisonState samp, float2 positionSS, float3 positionWS, float3 normalWS, int index, float3 L, out int shadowSplitIndex)
{
    float shadow = 1.0;
    HDShadowData sd = EvalShadow_GetHDShadowData(shadowContext, index, positionWS, shadowSplitIndex);

    float3 basePositionWS = positionWS;

    if (shadowSplitIndex >= 0)
    {
        HDShadowData sd = shadowContext.shadowDatas[index + shadowSplitIndex];
        positionWS = basePositionWS + sd.cacheTranslationDelta.xyz;

        /* normal based bias */
        float3 normalBias = EvalShadow_NormalBiasOrtho(sd.worldTexelSize, sd.normalBias, normalWS);
        positionWS += normalBias;

        /* get shadowmap texcoords */
        float3 posTC = EvalShadow_GetTexcoordsAtlas(sd, _CascadeShadowAtlasSize.zw, positionWS, false);
        /* evalute the first cascade */
        // shadow = DIRECTIONAL_FILTER_ALGORITHM(sd, positionSS, posTC, tex, samp, FIXED_UNIFORM_BIAS);
        shadow = SampleShadow_PCF_Tent_7x7(_CascadeShadowAtlasSize.zwxy, posTC, tex, samp, FIXED_UNIFORM_BIAS);
    }

    return shadow;
}

float EvalShadow_CascadedDepth_Blend(inout HDShadowContext shadowContext, Texture2D tex, SamplerComparisonState samp, float2 positionSS, float3 positionWS, float3 normalWS, int index, float3 L)
{
    int unusedSplitIndex;
    return EvalShadow_CascadedDepth_Blend_SplitIndex(shadowContext, tex, samp, positionSS, positionWS, normalWS, index, L, unusedSplitIndex);
}

float EvalShadow_CascadedDepth_Dither_SplitIndex(inout HDShadowContext shadowContext, SamplerComparisonState samp, float2 positionSS, float3 positionWS, float3 normalWS, int index, float3 L, int shadowSplitIndex)
{
    float shadow = 1.0;
    shadowSplitIndex = EvalShadow_GetSplitIndex(shadowContext, index, positionWS);

    float3 basePositionWS = positionWS;

    if (shadowSplitIndex >= 0.0)
    {
        HDShadowData sd = shadowContext.shadowDatas[index];
        LoadDirectionalShadowDatas(sd, shadowContext, index + shadowSplitIndex);
        positionWS = basePositionWS + sd.cacheTranslationDelta.xyz;

        /* normal based bias */
        float worldTexelSize = sd.worldTexelSize;
        float3 normalBias = EvalShadow_NormalBiasOrtho(worldTexelSize, sd.normalBias, normalWS);

        // /* We select what split we need to sample from */
        // float nextSplit = min(shadowSplitIndex + 1, cascadeCount - 1);
        // bool evalNextCascade = nextSplit != shadowSplitIndex;
        //
        // if (evalNextCascade)
        // {
        //     LoadDirectionalShadowDatas(sd, shadowContext, index + nextSplit);
        //     positionWS = basePositionWS + sd.cacheTranslationDelta.xyz;
        //     float biasModifier = (sd.worldTexelSize / worldTexelSize);
        //     normalBias *= biasModifier;
        // }

        positionWS += normalBias;
        float3 posTC = EvalShadow_GetTexcoordsAtlas(sd, _CascadeShadowAtlasSize.zw, positionWS, false);

        shadow = DIRECTIONAL_FILTER_ALGORITHM(sd, positionSS, posTC, tex, samp, FIXED_UNIFORM_BIAS);
        shadow = (shadowSplitIndex < cascadeCount - 1) ? shadow : lerp(shadow, 1.0, alpha);
    }

    return shadow;
}

float EvalShadow_CascadedDepth_Dither(inout HDShadowContext shadowContext, SamplerComparisonState samp, float2 positionSS, float3 positionWS, float3 normalWS, int index, float3 L)
{
    int unusedSplitIndex;
    return EvalShadow_CascadedDepth_Dither_SplitIndex(shadowContext, samp, positionSS, positionWS, normalWS, index, L, unusedSplitIndex);
}

// TODO: optimize this using LinearEyeDepth() to avoid having to pass the shadowToWorld matrix
float EvalShadow_SampleClosestDistance_Punctual(HDShadowData sd, Texture2D tex, SamplerState sampl, float3 positionWS, float3 L, float3 lightPositionWS, bool perspective)
{
    float2 texelSize = sd.isInCachedAtlas ? _CachedShadowAtlasSize.zw : _ShadowAtlasSize.zw;

    float4 closestNDC = { 0,0,0,1 };
    float2 texelIdx = EvalShadow_GetTexcoordsAtlas(sd, texelSize, positionWS, closestNDC.xy, perspective);

    // sample the shadow map
    closestNDC.z = SAMPLE_TEXTURE2D_LOD(tex, sampl, texelIdx, 0).x;

    // reconstruct depth position
    float4 closestWS = mul(closestNDC, sd.shadowToWorld);
    float3 occluderPosWS = closestWS.xyz / closestWS.w;

    return perspective ? distance(occluderPosWS, lightPositionWS) : dot(lightPositionWS - occluderPosWS , L);
}
#endif
