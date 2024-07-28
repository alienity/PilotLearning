#include "../../ShaderLibrary/Macros.hlsl"
#include "../../Material/BuiltinUtilities.hlsl"

#ifndef LIGHTLOOP_HLSL
#define LIGHTLOOP_HLSL

#define LIGHTLOOP_DISABLE_TILE_AND_CLUSTER

#ifndef SCALARIZE_LIGHT_LOOP
// We perform scalarization only for forward rendering as for deferred loads will already be scalar since tiles will match waves and therefore all threads will read from the same tile.
// More info on scalarization: https://flashypixels.wordpress.com/2018/11/10/intro-to-gpu-scalarization-part-2-scalarize-all-the-lights/ .
// Note that it is currently disabled on gamecore platforms for issues with wave intrinsics and the new compiler, it will be soon investigated, but we disable it in the meantime.
#define SCALARIZE_LIGHT_LOOP (defined(PLATFORM_SUPPORTS_WAVE_INTRINSICS) && !defined(LIGHTLOOP_DISABLE_TILE_AND_CLUSTER) && SHADERPASS == SHADERPASS_FORWARD)
#endif

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
