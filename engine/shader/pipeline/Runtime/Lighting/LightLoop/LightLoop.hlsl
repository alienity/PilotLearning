#include "../../ShaderLibrary/Macros.hlsl"
#include "../../Material/BuiltinUtilities.hlsl"

#ifndef LIGHTLOOP_HLSL
#define LIGHTLOOP_HLSL

#define _DISABLE_SSR
#define _DISABLE_SSR_TRANSPARENT

#define LIGHTLOOP_DISABLE_TILE_AND_CLUSTER

//-----------------------------------------------------------------------------
// LightLoop
// ----------------------------------------------------------------------------

void LightLoop(
    FrameUniforms frameUniform, SamplerStruct samplerStruct,
    float3 V, PositionInputs posInput, PreLightData preLightData, BSDFData bsdfData, BuiltinData builtinData, uint featureFlags,
    out LightLoopOutput lightLoopOutput)
{
    // Init LightLoop output structure
    ZERO_INITIALIZE(LightLoopOutput, lightLoopOutput);

    LightLoopContext context;

    context.shadowContext    = InitShadowContext(frameUniform);
    context.shadowValue      = 1;
    context.sampleReflection = 0;
#ifdef APPLY_FOG_ON_SKY_REFLECTIONS
    context.positionWS       = posInput.positionWS;
#endif
    
    // First of all we compute the shadow value of the directional light to reduce the VGPR pressure
    if (featureFlags & LIGHTFEATUREFLAGS_DIRECTIONAL)
    {
        {
            DirectionalLightData light = frameUniform.lightDataUniform.directionalLightData;

            {
                // TODO: this will cause us to load from the normal buffer first. Does this cause a performance problem?
                float3 L = -light.forward;

                // Is it worth sampling the shadow map?
                if ((light.lightDimmer > 0) && (light.shadowDimmer > 0) && // Note: Volumetric can have different dimmer, thus why we test it here
                    IsNonZeroBSDF(V, L, preLightData, bsdfData))
                {
                    float3 positionWS = posInput.positionWS;

                    context.shadowValue = GetDirectionalShadowAttenuation(context.shadowContext,
                        samplerStruct, posInput.positionSS, positionWS, GetNormalForShadowBias(bsdfData), light.shadowIndex, L);
                }
            }
        }
    }
    
    // This struct is define in the material. the Lightloop must not access it
    // PostEvaluateBSDF call at the end will convert Lighting to diffuse and specular lighting
    AggregateLighting aggregateLighting;
    ZERO_INITIALIZE(AggregateLighting, aggregateLighting); // LightLoop is in charge of initializing the struct

    if (featureFlags & LIGHTFEATUREFLAGS_PUNCTUAL)
    {
        uint lightCount, lightStart;
    
        lightCount = frameUniform.lightDataUniform._PunctualLightCount;
        lightStart = 0;
        
        // Scalarized loop. All lights that are in a tile/cluster touched by any pixel in the wave are loaded (scalar load), only the one relevant to current thread/pixel are processed.
        // For clarity, the following code will follow the convention: variables starting with s_ are meant to be wave uniform (meant for scalar register),
        // v_ are variables that might have different value for each thread in the wave (meant for vector registers).
        // This will perform more loads than it is supposed to, however, the benefits should offset the downside, especially given that light data accessed should be largely coherent.
        // Note that the above is valid only if wave intriniscs are supported.
        uint v_lightListOffset = 0;
        uint v_lightIdx = lightStart;
    
        while (v_lightListOffset < lightCount)
        {
            v_lightIdx = lightStart + v_lightListOffset;
            uint s_lightIdx = v_lightIdx;
            if (s_lightIdx == -1)
                break;
    
            LightData s_lightData = frameUniform.lightDataUniform.lightData[s_lightIdx];
    
            // If current scalar and vector light index match, we process the light. The v_lightListOffset for current thread is increased.
            // Note that the following should really be ==, however, since helper lanes are not considered by WaveActiveMin, such helper lanes could
            // end up with a unique v_lightIdx value that is smaller than s_lightIdx hence being stuck in a loop. All the active lanes will not have this problem.
            if (s_lightIdx >= v_lightIdx)
            {
                v_lightListOffset++;
                if (IsMatchingLightLayer(s_lightData.lightLayers, builtinData.renderingLayers))
                {
                    DirectLighting lighting = EvaluateBSDF_Punctual(samplerStruct, context, V, posInput, preLightData, s_lightData, bsdfData, builtinData);
                    AccumulateDirectLighting(lighting, aggregateLighting);
                }
            }
        }
    }

    // Define macro for a better understanding of the loop
    // TODO: this code is now much harder to understand...
#define EVALUATE_BSDF_ENV_SKY(envLightData, TYPE, type) \
        IndirectLighting lighting = EvaluateBSDF_Env(context, samplerStruct, V, posInput, preLightData, envLightData, bsdfData, envLightData.influenceShapeType, MERGE_NAME(GPUIMAGEBASEDLIGHTINGTYPE_, TYPE), MERGE_NAME(type, HierarchyWeight)); \
        AccumulateIndirectLighting(lighting, aggregateLighting);

// Environment cubemap test lightlayers, sky don't test it
#define EVALUATE_BSDF_ENV(envLightData, TYPE, type) if (IsMatchingLightLayer(envLightData.lightLayers, builtinData.renderingLayers)) { EVALUATE_BSDF_ENV_SKY(envLightData, TYPE, type) }

    // First loop iteration
    if (featureFlags & (LIGHTFEATUREFLAGS_ENV | LIGHTFEATUREFLAGS_SKY | LIGHTFEATUREFLAGS_SSREFRACTION | LIGHTFEATUREFLAGS_SSREFLECTION))
    {
        float reflectionHierarchyWeight = 0.0; // Max: 1.0
        float refractionHierarchyWeight = _EnableSSRefraction ? 0.0 : 1.0; // Max: 1.0

        // Reflection / Refraction hierarchy is
        //  1. Screen Space Refraction / Reflection
        //  2. Environment Reflection / Refraction
        //  3. Sky Reflection / Refraction

        // Apply SSR.
    #if (defined(_SURFACE_TYPE_TRANSPARENT) && !defined(_DISABLE_SSR_TRANSPARENT)) || (!defined(_SURFACE_TYPE_TRANSPARENT) && !defined(_DISABLE_SSR))
        {
            IndirectLighting indirect = EvaluateBSDF_ScreenSpaceReflection(posInput, preLightData, bsdfData,
                                                                           reflectionHierarchyWeight);
            AccumulateIndirectLighting(indirect, aggregateLighting);
        }
    #endif

#if HAS_REFRACTION
        uint perPixelEnvStart = envLightStart;
        uint perPixelEnvCount = envLightCount;
        // For refraction to be stable, we should reuse the same refraction probe for the whole object.
        // Otherwise as if the object span different tiles it could produce a different refraction probe picking and thus have visual artifacts.
        // For this we need to find the tile that is at the center of the object that is being rendered.
        // And then select the first refraction probe of this list.
        if ((featureFlags & LIGHTFEATUREFLAGS_SSREFRACTION) && (_EnableSSRefraction > 0))
        {
            // grab current object position and retrieve in which tile it belongs too
            float4x4 modelMat = GetObjectToWorldMatrix();
            float3 objPos = modelMat._m03_m13_m23;
            float4 posClip = TransformWorldToHClip(objPos);
            posClip.xyz = posClip.xyz / posClip.w;

            uint2 tileObj = (saturate(posClip.xy * 0.5f + 0.5f) * _ScreenSize.xy) / GetTileSize();

            uint envLightStart, envLightCount;

            // Fetch first env light to provide the scene proxy for screen space refraction computation
            PositionInputs localInput;
            ZERO_INITIALIZE(PositionInputs, localInput);
            localInput.tileCoord = tileObj.xy;
            localInput.linearDepth = posClip.w;

            GetCountAndStart(localInput, LIGHTCATEGORY_ENV, envLightStart, envLightCount);

            EnvLightData envLightData;
            if (envLightCount > 0)
            {
                envLightData = FetchEnvLight(FetchIndex(envLightStart, 0));
            }
            else if (perPixelEnvCount > 0) // If no refraction probe at the object center, we either fallback on the per pixel result.
            {
                envLightData = FetchEnvLight(FetchIndex(perPixelEnvStart, 0));
            }
            else // .. or the sky
            {
                envLightData = InitDefaultRefractionEnvLightData(0);
            }

            IndirectLighting lighting = EvaluateBSDF_ScreenspaceRefraction(context, V, posInput, preLightData, bsdfData, envLightData, refractionHierarchyWeight);
            AccumulateIndirectLighting(lighting, aggregateLighting);
        }
#endif

        //---------------------------------------
        // Sum up of operations on indirect diffuse lighting
        // Let's define SSGI as SSGI/RTGI/Mixed or APV without lightmaps
        // Let's define GI as Lightmaps/Lightprobe/APV with lightmaps

        // By default we do those operations in deferred
        // GBuffer pass : GI * AO + Emissive -> lightingbuffer
        // Lightloop : indirectDiffuse = lightingbuffer; indirectDiffuse * SSAO
        // Note that SSAO is apply on emissive in this case and we have double occlusion between AO and SSAO on indirectDiffuse

        // By default we do those operation in forward
        // Lightloop : indirectDiffuse = GI; indirectDiffuse * min(AO, SSAO) + Emissive

        // With any SSGI effect we are performing those operations in deferred
        // GBuffer pass : Emissive == 0 ? AmbientOcclusion -> EncodeIn(lightingbuffer) : Emissive -> lightingbuffer
        // Lightloop : indirectDiffuse = SSGI; Emissive = lightingbuffer; AmbientOcclusion = Extract(lightingbuffer) or 1.0;
        // indirectDiffuse * min(AO, SSAO) + Emissive
        // Note that mean that we have the same behavior than forward path if Emissive is 0

        // With any SSGI effect we are performing those operations in Forward
        // Lightloop : indirectDiffuse = SSGI; indirectDiffuse * min(AO, SSAO) + Emissive
        //---------------------------------------

        // Explanation about APV and SSGI/RTGI/Mixed effects steps in the rendering pipeline.
        // All effects will output only Emissive inside the lighting buffer (gbuffer3) in case of deferred (For APV this is done only if we are not a lightmap).
        // The Lightmaps/Lightprobes contribution is 0 for those cases. Code enforce it in SampleBakedGI(). The remaining code of Material pass (and the debug code)
        // is exactly the same with or without effects on, including the EncodeToGbuffer.
        // builtinData.isLightmap is used by APV to know if we have lightmap or not and is harcoded based on preprocessor in InitBuiltinData()
        // AO is also store with a hack in Gbuffer3 if possible. Otherwise it is set to 1.
        // In case of regular deferred path (when effects aren't enable) AO is already apply on lightmap and emissive is added on to of it. All is inside bakeDiffuseLighting and emissiveColor is 0. AO must be 1
        // When effects are enabled and for APV we don't have lightmaps, bakeDiffuseLighting is 0 and emissiveColor contain emissive (Either in deferred or forward) and AO should be the real AO value.
        // Then in the lightloop in below code we will evalaute APV or read the indirectDiffuseTexture to fill bakeDiffuseLighting.
        // We will then just do all the regular step we do with bakeDiffuseLighting in PostInitBuiltinData()
        // No code change is required to handle AO, it is the same for all path.
        // Note: Decals Emissive and Transparent Emissve aren't taken into account by RTGI/Mixed.
        // Forward opaque emissive work in all cases. The current code flow with Emissive store in GBuffer3 is only to manage the case of Opaque Lit Material with Emissive in case of deferred
        // Only APV can handle backFace lighting, all other effects are front face only.

        // If we use SSGI/RTGI/Mixed effect, we are fully replacing the value of builtinData.bakeDiffuseLighting which is 0 at this step.
        // If we are APV we only replace the non lightmaps part.
        bool replaceBakeDiffuseLighting = false;

#if !defined(_SURFACE_TYPE_TRANSPARENT) // No SSGI/RTGI/Mixed effect on transparent
        if (_IndirectDiffuseMode != INDIRECTDIFFUSEMODE_OFF)
            replaceBakeDiffuseLighting = true;
#endif

        if (replaceBakeDiffuseLighting)
        {
            BuiltinData tempBuiltinData;
            ZERO_INITIALIZE(BuiltinData, tempBuiltinData);

#if !defined(_SURFACE_TYPE_TRANSPARENT) && !defined(SCREEN_SPACE_INDIRECT_DIFFUSE_DISABLED)
            if (_IndirectDiffuseMode != INDIRECTDIFFUSEMODE_OFF)
            {
                Texture2D<float4> _IndirectDiffuseTexture = ResourceDescriptorHeap[frameUniform.ssgiUniform._IndirectDiffuseTextureIndex];
                float4 screenSize = frameUniform.baseUniform._ScreenSize;
                uint2 texcoord = uint2(posInput.positionSS.x, screenSize.y - 1 - posInput.positionSS.y);
                tempBuiltinData.bakeDiffuseLighting = LOAD_TEXTURE2D(_IndirectDiffuseTexture, texcoord).xyz * GetInverseCurrentExposureMultiplier(frameUniform);
            }
#endif


#ifdef MODIFY_BAKED_DIFFUSE_LIGHTING
            ModifyBakedDiffuseLighting(V, posInput, preLightData, bsdfData, tempBuiltinData);
#endif
            // This is applied only on bakeDiffuseLighting as ModifyBakedDiffuseLighting combine both bakeDiffuseLighting and backBakeDiffuseLighting
            tempBuiltinData.bakeDiffuseLighting *= GetIndirectDiffuseMultiplier(frameUniform, builtinData.renderingLayers);

            // Replace original data
            builtinData.bakeDiffuseLighting = tempBuiltinData.bakeDiffuseLighting;

        } // if (replaceBakeDiffuseLighting)

        // Reflection probes are sorted by volume (in the increasing order).
        if (featureFlags & LIGHTFEATUREFLAGS_ENV)
        {
            context.sampleReflection = SINGLE_PASS_CONTEXT_SAMPLE_REFLECTION_PROBES;

            EnvLightData s_envLightData = frameUniform.lightDataUniform.envData;
            
            if (reflectionHierarchyWeight < 1.0)
            {
                if (IsMatchingLightLayer(s_envLightData.lightLayers, builtinData.renderingLayers))
                {
                    IndirectLighting lighting = EvaluateBSDF_Env(context, samplerStruct, V, posInput, preLightData, s_envLightData, bsdfData,
                        s_envLightData.influenceShapeType, GPUIMAGEBASEDLIGHTINGTYPE_REFLECTION, reflectionHierarchyWeight);
                    AccumulateIndirectLighting(lighting, aggregateLighting);
                }
            }
            // // Refraction probe and reflection probe will process exactly the same weight. It will be good for performance to be able to share this computation
            // // However it is hard to deal with the fact that reflectionHierarchyWeight and refractionHierarchyWeight have not the same values, they are independent
            // // The refraction probe is rarely used and happen only with sphere shape and high IOR. So we accept the slow path that use more simple code and
            // // doesn't affect the performance of the reflection which is more important.
            // // We reuse LIGHTFEATUREFLAGS_SSREFRACTION flag as refraction is mainly base on the screen. Would be a waste to not use screen and only cubemap.
            // if ((featureFlags & LIGHTFEATUREFLAGS_SSREFRACTION) && (refractionHierarchyWeight < 1.0))
            // {
            //     EVALUATE_BSDF_ENV(s_envLightData, REFRACTION, refraction);
            // }
        }

        // Only apply the sky IBL if the sky texture is available
        if ((featureFlags & LIGHTFEATUREFLAGS_SKY) && _EnvLightSkyEnabled)
        {
            // The sky is a single cubemap texture separate from the reflection probe texture array (different resolution and compression)
            context.sampleReflection = SINGLE_PASS_CONTEXT_SAMPLE_SKY;

            // The sky data are generated on the fly so the compiler can optimize the code
            EnvLightData envLightSky = InitSkyEnvLightData(0);

            // Only apply the sky if we haven't yet accumulated enough IBL lighting.
            if (reflectionHierarchyWeight < 1.0)
            {
                EVALUATE_BSDF_ENV_SKY(envLightSky, REFLECTION, reflection);
            }

            if ((featureFlags & LIGHTFEATUREFLAGS_SSREFRACTION) && (refractionHierarchyWeight < 1.0))
            {
                EVALUATE_BSDF_ENV_SKY(envLightSky, REFRACTION, refraction);
            }
        }
    }
#undef EVALUATE_BSDF_ENV
#undef EVALUATE_BSDF_ENV_SKY
    
    if (featureFlags & LIGHTFEATUREFLAGS_DIRECTIONAL)
    {
        DirectionalLightData light = frameUniform.lightDataUniform.directionalLightData;
        {
            if (IsMatchingLightLayer(light.lightLayers, builtinData.renderingLayers))
            {
                DirectLighting lighting = EvaluateBSDF_Directional(samplerStruct, context, V, posInput, preLightData, light, bsdfData, builtinData);
                AccumulateDirectLighting(lighting, aggregateLighting);
            }
        }
    }

    // Note: We can't apply the IndirectDiffuseMultiplier here as with GBuffer, Emissive is part of the bakeDiffuseLighting.
    // so IndirectDiffuseMultiplier is apply in PostInitBuiltinData or related location (like for probe volume)
    aggregateLighting.indirect.specularReflected *= GetIndirectSpecularMultiplier(frameUniform, builtinData.renderingLayers);

    // Also Apply indiret diffuse (GI)
    // PostEvaluateBSDF will perform any operation wanted by the material and sum everything into diffuseLighting and specularLighting
    PostEvaluateBSDF(frameUniform, context, V, posInput, preLightData, bsdfData, builtinData, aggregateLighting, lightLoopOutput);
    
}

#endif
