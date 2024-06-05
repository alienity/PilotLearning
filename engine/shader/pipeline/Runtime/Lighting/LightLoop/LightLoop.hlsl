#include "../../ShaderLibrary/Macros.hlsl"
#include "../../Material/BuiltinUtilities.hlsl"

#ifndef LIGHTLOOP_CS_HLSL
#define LIGHTLOOP_CS_HLSL

//
// UnityEngine.Rendering.HighDefinition.LightDefinitions:  static fields
//
#define VIEWPORT_SCALE_Z (1)
#define USE_LEFT_HAND_CAMERA_SPACE (1)
#define TILE_SIZE_FPTL (16)
#define TILE_SIZE_CLUSTERED (32)
#define TILE_SIZE_BIG_TILE (64)
#define TILE_INDEX_MASK (32767)
#define TILE_INDEX_SHIFT_X (0)
#define TILE_INDEX_SHIFT_Y (15)
#define TILE_INDEX_SHIFT_EYE (30)
#define NUM_FEATURE_VARIANTS (29)
#define LIGHT_FEATURE_MASK_FLAGS (16773120)
#define LIGHT_FEATURE_MASK_FLAGS_OPAQUE (16642048)
#define LIGHT_FEATURE_MASK_FLAGS_TRANSPARENT (16510976)
#define MATERIAL_FEATURE_MASK_FLAGS (4095)
#define RAY_TRACED_SCREEN_SPACE_SHADOW_FLAG (4096)
#define SCREEN_SPACE_COLOR_SHADOW_FLAG (256)
#define INVALID_SCREEN_SPACE_SHADOW (255)
#define SCREEN_SPACE_SHADOW_INDEX_MASK (255)
#define CONTACT_SHADOW_FADE_BITS (8)
#define CONTACT_SHADOW_MASK_BITS (24)
#define CONTACT_SHADOW_FADE_MASK (255)
#define CONTACT_SHADOW_MASK_MASK (16777215)

//
// UnityEngine.Rendering.HighDefinition.LightVolumeType:  static fields
//
#define LIGHTVOLUMETYPE_CONE (0)
#define LIGHTVOLUMETYPE_SPHERE (1)
#define LIGHTVOLUMETYPE_BOX (2)
#define LIGHTVOLUMETYPE_COUNT (3)

//
// UnityEngine.Rendering.HighDefinition.LightFeatureFlags:  static fields
//
#define LIGHTFEATUREFLAGS_PUNCTUAL (4096)
#define LIGHTFEATUREFLAGS_AREA (8192)
#define LIGHTFEATUREFLAGS_DIRECTIONAL (16384)
#define LIGHTFEATUREFLAGS_ENV (32768)
#define LIGHTFEATUREFLAGS_SKY (65536)
#define LIGHTFEATUREFLAGS_SSREFRACTION (131072)
#define LIGHTFEATUREFLAGS_SSREFLECTION (262144)

//
// UnityEngine.Rendering.HighDefinition.ClusterDebugMode:  static fields
//
#define CLUSTERDEBUGMODE_VISUALIZE_OPAQUE (0)
#define CLUSTERDEBUGMODE_VISUALIZE_SLICE (1)

//
// UnityEngine.Rendering.HighDefinition.LightCategory:  static fields
//
#define LIGHTCATEGORY_PUNCTUAL (0)
#define LIGHTCATEGORY_AREA (1)
#define LIGHTCATEGORY_ENV (2)
#define LIGHTCATEGORY_DECAL (3)
#define LIGHTCATEGORY_COUNT (4)

// Generated from UnityEngine.Rendering.HighDefinition.SFiniteLightBound
// PackingRules = Exact
struct SFiniteLightBound
{
    float3 boxAxisX;
    float3 boxAxisY;
    float3 boxAxisZ;
    float3 center;
    float scaleXY;
    float radius;
};

// Generated from UnityEngine.Rendering.HighDefinition.LightVolumeData
// PackingRules = Exact
struct LightVolumeData
{
    float3 lightPos;
    uint lightVolume;
    float3 lightAxisX;
    uint lightCategory;
    float3 lightAxisY;
    float radiusSq;
    float3 lightAxisZ;
    float cotan;
    float3 boxInnerDist;
    uint featureFlags;
    float3 boxInvRange;
    float unused2;
};

// Generated from UnityEngine.Rendering.HighDefinition.ShaderVariablesLightList
// PackingRules = Exact
struct ShaderVariablesLightList
{
    float4x4 g_mInvScrProjectionArr[2];
    float4x4 g_mScrProjectionArr[2];
    float4x4 g_mInvProjectionArr[2];
    float4x4 g_mProjectionArr[2];
    float4 g_screenSize;
    int2 g_viDimensions;
    int g_iNrVisibLights;
    uint g_isOrthographic;
    uint g_BaseFeatureFlags;
    int g_iNumSamplesMSAA;
    uint _EnvLightIndexShift;
    uint _DecalIndexShift;  
};

//
// Accessors for UnityEngine.Rendering.HighDefinition.SFiniteLightBound
//
float3 GetBoxAxisX(SFiniteLightBound value)
{
    return value.boxAxisX;
}
float3 GetBoxAxisY(SFiniteLightBound value)
{
    return value.boxAxisY;
}
float3 GetBoxAxisZ(SFiniteLightBound value)
{
    return value.boxAxisZ;
}
float3 GetCenter(SFiniteLightBound value)
{
    return value.center;
}
float GetScaleXY(SFiniteLightBound value)
{
    return value.scaleXY;
}
float GetRadius(SFiniteLightBound value)
{
    return value.radius;
}
//
// Accessors for UnityEngine.Rendering.HighDefinition.LightVolumeData
//
float3 GetLightPos(LightVolumeData value)
{
    return value.lightPos;
}
uint GetLightVolume(LightVolumeData value)
{
    return value.lightVolume;
}
float3 GetLightAxisX(LightVolumeData value)
{
    return value.lightAxisX;
}
uint GetLightCategory(LightVolumeData value)
{
    return value.lightCategory;
}
float3 GetLightAxisY(LightVolumeData value)
{
    return value.lightAxisY;
}
float GetRadiusSq(LightVolumeData value)
{
    return value.radiusSq;
}
float3 GetLightAxisZ(LightVolumeData value)
{
    return value.lightAxisZ;
}
float GetCotan(LightVolumeData value)
{
    return value.cotan;
}
float3 GetBoxInnerDist(LightVolumeData value)
{
    return value.boxInnerDist;
}
uint GetFeatureFlags(LightVolumeData value)
{
    return value.featureFlags;
}
float3 GetBoxInvRange(LightVolumeData value)
{
    return value.boxInvRange;
}
float GetUnused2(LightVolumeData value)
{
    return value.unused2;
}

#endif

#ifndef LIGHTLOOP_HLSL
#define LIGHTLOOP_HLSL

#ifndef SCALARIZE_LIGHT_LOOP
// We perform scalarization only for forward rendering as for deferred loads will already be scalar since tiles will match waves and therefore all threads will read from the same tile.
// More info on scalarization: https://flashypixels.wordpress.com/2018/11/10/intro-to-gpu-scalarization-part-2-scalarize-all-the-lights/ .
// Note that it is currently disabled on gamecore platforms for issues with wave intrinsics and the new compiler, it will be soon investigated, but we disable it in the meantime.
#define SCALARIZE_LIGHT_LOOP (defined(PLATFORM_SUPPORTS_WAVE_INTRINSICS) && !defined(LIGHTLOOP_DISABLE_TILE_AND_CLUSTER) && SHADERPASS == SHADERPASS_FORWARD)
#endif


//-----------------------------------------------------------------------------
// LightLoop
// ----------------------------------------------------------------------------

bool UseScreenSpaceShadow(DirectionalLightData light, float3 normalWS)
{
    // Two different options are possible here
    // - We have a ray trace shadow in which case we have no valid signal for a transmission and we need to fallback on the rasterized shadow
    // - We have a screen space shadow and it already contains the transmission shadow and we can use it straight away
    bool visibleLight = dot(normalWS, -light.forward) > 0.0;
    bool validScreenSpaceShadow = (light.screenSpaceShadowIndex & SCREEN_SPACE_SHADOW_INDEX_MASK) != INVALID_SCREEN_SPACE_SHADOW;
    bool rayTracedShadow = (light.screenSpaceShadowIndex & RAY_TRACED_SCREEN_SPACE_SHADOW_FLAG) != 0;
    return (validScreenSpaceShadow && ((rayTracedShadow && visibleLight) || !rayTracedShadow));
}

void LightLoop( float3 V, PositionInputs posInput, PreLightData preLightData, BSDFData bsdfData, BuiltinData builtinData, uint featureFlags,
                out LightLoopOutput lightLoopOutput)
{
    // Init LightLoop output structure
    ZERO_INITIALIZE(LightLoopOutput, lightLoopOutput);

    LightLoopContext context;

    context.shadowContext    = InitShadowContext();
    context.shadowValue      = 1;
    context.sampleReflection = 0;
    context.splineVisibility = -1;
#ifdef APPLY_FOG_ON_SKY_REFLECTIONS
    context.positionWS       = posInput.positionWS;
#endif

    // With XR single-pass and camera-relative: offset position to do lighting computations from the combined center view (original camera matrix).
    // This is required because there is only one list of lights generated on the CPU. Shadows are also generated once and shared between the instanced views.
    ApplyCameraRelativeXR(posInput.positionWS);

    // Initialize the contactShadow and contactShadowFade fields
    InitContactShadow(posInput, context);

    // First of all we compute the shadow value of the directional light to reduce the VGPR pressure
    if (featureFlags & LIGHTFEATUREFLAGS_DIRECTIONAL)
    {
        // Evaluate sun shadows.
        if (_DirectionalShadowIndex >= 0)
        {
            DirectionalLightData light = _DirectionalLightDatas[_DirectionalShadowIndex];

#if defined(SCREEN_SPACE_SHADOWS_ON) && !defined(_SURFACE_TYPE_TRANSPARENT)
            if (UseScreenSpaceShadow(light, bsdfData.normalWS))
            {
                context.shadowValue = GetScreenSpaceColorShadow(posInput, light.screenSpaceShadowIndex).SHADOW_TYPE_SWIZZLE;
            }
            else
#endif
            {
                // TODO: this will cause us to load from the normal buffer first. Does this cause a performance problem?
                float3 L = -light.forward;

                // Is it worth sampling the shadow map?
                if ((light.lightDimmer > 0) && (light.shadowDimmer > 0) && // Note: Volumetric can have different dimmer, thus why we test it here
                    IsNonZeroBSDF(V, L, preLightData, bsdfData) &&
                    !ShouldEvaluateThickObjectTransmission(V, L, preLightData, bsdfData, light.shadowIndex))
                {
                    float3 positionWS = posInput.positionWS;

#ifdef LIGHT_EVALUATION_SPLINE_SHADOW_BIAS
                    positionWS += L * GetSplineOffsetForShadowBias(bsdfData);
#endif
                    context.shadowValue = GetDirectionalShadowAttenuation(context.shadowContext,
                                                                          posInput.positionSS, positionWS, GetNormalForShadowBias(bsdfData),
                                                                          light.shadowIndex, L);

#ifdef LIGHT_EVALUATION_SPLINE_SHADOW_VISIBILITY_SAMPLE
                    // Tap the shadow a second time for strand visibility term.
                    context.splineVisibility = GetDirectionalShadowAttenuation(context.shadowContext,
                                                                               posInput.positionSS, posInput.positionWS, GetNormalForShadowBias(bsdfData),
                                                                               light.shadowIndex, L);
#endif
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

#ifndef LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
        GetCountAndStart(posInput, LIGHTCATEGORY_PUNCTUAL, lightStart, lightCount);
#else   // LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
        lightCount = _PunctualLightCount;
        lightStart = 0;
#endif

        bool fastPath = false;
    #if SCALARIZE_LIGHT_LOOP
        uint lightStartLane0;
        fastPath = IsFastPath(lightStart, lightStartLane0);

        if (fastPath)
        {
            lightStart = lightStartLane0;
        }
    #endif

        // Scalarized loop. All lights that are in a tile/cluster touched by any pixel in the wave are loaded (scalar load), only the one relevant to current thread/pixel are processed.
        // For clarity, the following code will follow the convention: variables starting with s_ are meant to be wave uniform (meant for scalar register),
        // v_ are variables that might have different value for each thread in the wave (meant for vector registers).
        // This will perform more loads than it is supposed to, however, the benefits should offset the downside, especially given that light data accessed should be largely coherent.
        // Note that the above is valid only if wave intriniscs are supported.
        uint v_lightListOffset = 0;
        uint v_lightIdx = lightStart;

#if NEED_TO_CHECK_HELPER_LANE
        // On some platform helper lanes don't behave as we'd expect, therefore we prevent them from entering the loop altogether.
        // IMPORTANT! This has implications if ddx/ddy is used on results derived from lighting, however given Lightloop is called in compute we should be
        // sure it will not happen. 
        bool isHelperLane = WaveIsHelperLane();
        while (!isHelperLane && v_lightListOffset < lightCount)
#else
        while (v_lightListOffset < lightCount)
#endif
        {
            v_lightIdx = FetchIndex(lightStart, v_lightListOffset);
#if SCALARIZE_LIGHT_LOOP
            uint s_lightIdx = ScalarizeElementIndex(v_lightIdx, fastPath);
#else
            uint s_lightIdx = v_lightIdx;
#endif
            if (s_lightIdx == -1)
                break;

            LightData s_lightData = FetchLight(s_lightIdx);

            // If current scalar and vector light index match, we process the light. The v_lightListOffset for current thread is increased.
            // Note that the following should really be ==, however, since helper lanes are not considered by WaveActiveMin, such helper lanes could
            // end up with a unique v_lightIdx value that is smaller than s_lightIdx hence being stuck in a loop. All the active lanes will not have this problem.
            if (s_lightIdx >= v_lightIdx)
            {
                v_lightListOffset++;
                if (IsMatchingLightLayer(s_lightData.lightLayers, builtinData.renderingLayers))
                {
                    DirectLighting lighting = EvaluateBSDF_Punctual(context, V, posInput, preLightData, s_lightData, bsdfData, builtinData);
                    AccumulateDirectLighting(lighting, aggregateLighting);
                }
            }
        }
    }


    // Define macro for a better understanding of the loop
    // TODO: this code is now much harder to understand...
#define EVALUATE_BSDF_ENV_SKY(envLightData, TYPE, type) \
        IndirectLighting lighting = EvaluateBSDF_Env(context, V, posInput, preLightData, envLightData, bsdfData, envLightData.influenceShapeType, MERGE_NAME(GPUIMAGEBASEDLIGHTINGTYPE_, TYPE), MERGE_NAME(type, HierarchyWeight)); \
        AccumulateIndirectLighting(lighting, aggregateLighting);

// Environment cubemap test lightlayers, sky don't test it
#define EVALUATE_BSDF_ENV(envLightData, TYPE, type) if (IsMatchingLightLayer(envLightData.lightLayers, builtinData.renderingLayers)) { EVALUATE_BSDF_ENV_SKY(envLightData, TYPE, type) }

    // First loop iteration
    if (featureFlags & (LIGHTFEATUREFLAGS_ENV | LIGHTFEATUREFLAGS_SKY | LIGHTFEATUREFLAGS_SSREFRACTION | LIGHTFEATUREFLAGS_SSREFLECTION))
    {
        float reflectionHierarchyWeight = 0.0; // Max: 1.0
        float refractionHierarchyWeight = _EnableSSRefraction ? 0.0 : 1.0; // Max: 1.0

        uint envLightStart, envLightCount;

        // Fetch first env light to provide the scene proxy for screen space computation
#ifndef LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
        GetCountAndStart(posInput, LIGHTCATEGORY_ENV, envLightStart, envLightCount);
#else   // LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
        envLightCount = _EnvLightCount;
        envLightStart = 0;
#endif

        bool fastPath = false;
    #if SCALARIZE_LIGHT_LOOP
        uint envStartFirstLane;
        fastPath = IsFastPath(envLightStart, envStartFirstLane);
    #endif

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
#if defined(PROBE_VOLUMES_L1) || defined(PROBE_VOLUMES_L2)
        float3 lightInReflDir = float3(-1, -1, -1); // This variable is used with APV for reflection probe normalization - see code for LIGHTFEATUREFLAGS_ENV
#endif

#if !defined(_SURFACE_TYPE_TRANSPARENT) // No SSGI/RTGI/Mixed effect on transparent
        if (_IndirectDiffuseMode != INDIRECTDIFFUSEMODE_OFF)
            replaceBakeDiffuseLighting = true;
#endif
#if defined(PROBE_VOLUMES_L1) || defined(PROBE_VOLUMES_L2)
        if (!builtinData.isLightmap)
            replaceBakeDiffuseLighting = true;
#endif

        if (replaceBakeDiffuseLighting)
        {
            BuiltinData tempBuiltinData;
            ZERO_INITIALIZE(BuiltinData, tempBuiltinData);

#if !defined(_SURFACE_TYPE_TRANSPARENT) && !defined(SCREEN_SPACE_INDIRECT_DIFFUSE_DISABLED)
            if (_IndirectDiffuseMode != INDIRECTDIFFUSEMODE_OFF)
            {
                tempBuiltinData.bakeDiffuseLighting = LOAD_TEXTURE2D_X(_IndirectDiffuseTexture, posInput.positionSS).xyz * GetInverseCurrentExposureMultiplier();
            }
            else
#endif
            {
#if defined(PROBE_VOLUMES_L1) || defined(PROBE_VOLUMES_L2)
                if (_EnableProbeVolumes)
                {
                    // Reflect normal to get lighting for reflection probe tinting
                    float3 R = reflect(-V, bsdfData.normalWS);

                    EvaluateAdaptiveProbeVolume(GetAbsolutePositionWS(posInput.positionWS),
                        bsdfData.normalWS,
                        -bsdfData.normalWS,
                        R,
                        V,
                        posInput.positionSS,
                        tempBuiltinData.bakeDiffuseLighting,
                        tempBuiltinData.backBakeDiffuseLighting,
                        lightInReflDir);
                }
                else // If probe volume is disabled we fallback on the ambient probes
                {
                    tempBuiltinData.bakeDiffuseLighting = EvaluateAmbientProbe(bsdfData.normalWS);
                    tempBuiltinData.backBakeDiffuseLighting = EvaluateAmbientProbe(-bsdfData.normalWS);
                }
#endif
            }

#ifdef MODIFY_BAKED_DIFFUSE_LIGHTING
#ifdef DEBUG_DISPLAY
            // When the lux meter is enabled, we don't want the albedo of the material to modify the diffuse baked lighting
            if (_DebugLightingMode != DEBUGLIGHTINGMODE_LUX_METER)
#endif
                ModifyBakedDiffuseLighting(V, posInput, preLightData, bsdfData, tempBuiltinData);
#endif
            // This is applied only on bakeDiffuseLighting as ModifyBakedDiffuseLighting combine both bakeDiffuseLighting and backBakeDiffuseLighting
            tempBuiltinData.bakeDiffuseLighting *= GetIndirectDiffuseMultiplier(builtinData.renderingLayers);

            ApplyDebugToBuiltinData(tempBuiltinData); // This will not affect emissive as we don't use it

#if defined(DEBUG_DISPLAY) && (SHADERPASS == SHADERPASS_DEFERRED_LIGHTING)
            // We need to handle the specific case of deferred for debug lighting mode here
            if (_DebugLightingMode == DEBUGLIGHTINGMODE_EMISSIVE_LIGHTING)
            {
                tempBuiltinData.bakeDiffuseLighting = real3(0.0, 0.0, 0.0);
            }
#endif

            // Replace original data
            builtinData.bakeDiffuseLighting = tempBuiltinData.bakeDiffuseLighting;

        } // if (replaceBakeDiffuseLighting)

        // Reflection probes are sorted by volume (in the increasing order).
        if (featureFlags & LIGHTFEATUREFLAGS_ENV)
        {
            context.sampleReflection = SINGLE_PASS_CONTEXT_SAMPLE_REFLECTION_PROBES;

        #if SCALARIZE_LIGHT_LOOP
            if (fastPath)
            {
                envLightStart = envStartFirstLane;
            }
        #endif

            // Scalarized loop, same rationale of the punctual light version
            uint v_envLightListOffset = 0;
            uint v_envLightIdx = envLightStart;
#if NEED_TO_CHECK_HELPER_LANE
            // On some platform helper lanes don't behave as we'd expect, therefore we prevent them from entering the loop altogether.
            // IMPORTANT! This has implications if ddx/ddy is used on results derived from lighting, however given Lightloop is called in compute we should be
            // sure it will not happen. 
            bool isHelperLane = WaveIsHelperLane();
            while (!isHelperLane && v_envLightListOffset < envLightCount)
#else
            while (v_envLightListOffset < envLightCount)
#endif
            {
                v_envLightIdx = FetchIndex(envLightStart, v_envLightListOffset);
#if SCALARIZE_LIGHT_LOOP
                uint s_envLightIdx = ScalarizeElementIndex(v_envLightIdx, fastPath);
#else
                uint s_envLightIdx = v_envLightIdx;
#endif
                if (s_envLightIdx == -1)
                    break;

                EnvLightData s_envLightData = FetchEnvLight(s_envLightIdx);    // Scalar load.

                // If current scalar and vector light index match, we process the light. The v_envLightListOffset for current thread is increased.
                // Note that the following should really be ==, however, since helper lanes are not considered by WaveActiveMin, such helper lanes could
                // end up with a unique v_envLightIdx value that is smaller than s_envLightIdx hence being stuck in a loop. All the active lanes will not have this problem.
                if (s_envLightIdx >= v_envLightIdx)
                {
                    v_envLightListOffset++;
                    if (reflectionHierarchyWeight < 1.0)
                    {
                        if (IsMatchingLightLayer(s_envLightData.lightLayers, builtinData.renderingLayers))
                        {
                            IndirectLighting lighting = EvaluateBSDF_Env(context, V, posInput, preLightData, s_envLightData, bsdfData, s_envLightData.influenceShapeType, GPUIMAGEBASEDLIGHTINGTYPE_REFLECTION, reflectionHierarchyWeight);
#if defined(PROBE_VOLUMES_L1) || defined(PROBE_VOLUMES_L2)

                            if (s_envLightData.normalizeWithAPV > 0 && all(lightInReflDir >= 0))
                            {
                                float factor = GetReflectionProbeNormalizationFactor(lightInReflDir, bsdfData.normalWS, s_envLightData.L0L1, s_envLightData.L2_1, s_envLightData.L2_2);
                                lighting.specularReflected *= factor;
                            }
#endif

                            AccumulateIndirectLighting(lighting, aggregateLighting);
                        }
                    }
                    // Refraction probe and reflection probe will process exactly the same weight. It will be good for performance to be able to share this computation
                    // However it is hard to deal with the fact that reflectionHierarchyWeight and refractionHierarchyWeight have not the same values, they are independent
                    // The refraction probe is rarely used and happen only with sphere shape and high IOR. So we accept the slow path that use more simple code and
                    // doesn't affect the performance of the reflection which is more important.
                    // We reuse LIGHTFEATUREFLAGS_SSREFRACTION flag as refraction is mainly base on the screen. Would be a waste to not use screen and only cubemap.
                    if ((featureFlags & LIGHTFEATUREFLAGS_SSREFRACTION) && (refractionHierarchyWeight < 1.0))
                    {
                        EVALUATE_BSDF_ENV(s_envLightData, REFRACTION, refraction);
                    }
                }

            }
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

    uint i = 0; // Declare once to avoid the D3D11 compiler warning.
    if (featureFlags & LIGHTFEATUREFLAGS_DIRECTIONAL)
    {
        for (i = 0; i < _DirectionalLightCount; ++i)
        {
            if (IsMatchingLightLayer(_DirectionalLightDatas[i].lightLayers, builtinData.renderingLayers))
            {
                DirectLighting lighting = EvaluateBSDF_Directional(context, V, posInput, preLightData, _DirectionalLightDatas[i], bsdfData, builtinData);
                AccumulateDirectLighting(lighting, aggregateLighting);
            }
        }
    }

#if SHADEROPTIONS_AREA_LIGHTS
    if (featureFlags & LIGHTFEATUREFLAGS_AREA)
    {
        uint lightCount, lightStart;

    #ifndef LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
        GetCountAndStart(posInput, LIGHTCATEGORY_AREA, lightStart, lightCount);
    #else
        lightCount = _AreaLightCount;
        lightStart = _PunctualLightCount;
    #endif

        // COMPILER BEHAVIOR WARNING!
        // If rectangle lights are before line lights, the compiler will duplicate light matrices in VGPR because they are used differently between the two types of lights.
        // By keeping line lights first we avoid this behavior and save substantial register pressure.
        // TODO: This is based on the current Lit.shader and can be different for any other way of implementing area lights, how to be generic and ensure performance ?

        if (lightCount > 0)
        {
            i = 0;

            uint      last      = lightCount - 1;
            LightData lightData = FetchLight(lightStart, i);

            while (i <= last && lightData.lightType == GPULIGHTTYPE_TUBE)
            {
                lightData.lightType = GPULIGHTTYPE_TUBE; // Enforce constant propagation
                lightData.cookieMode = COOKIEMODE_NONE;  // Enforce constant propagation

                if (IsMatchingLightLayer(lightData.lightLayers, builtinData.renderingLayers))
                {
                    DirectLighting lighting = EvaluateBSDF_Area(context, V, posInput, preLightData, lightData, bsdfData, builtinData);
                    AccumulateDirectLighting(lighting, aggregateLighting);
                }

                lightData = FetchLight(lightStart, min(++i, last));
            }

            while (i <= last) // GPULIGHTTYPE_RECTANGLE
            {
                lightData.lightType = GPULIGHTTYPE_RECTANGLE; // Enforce constant propagation

                if (IsMatchingLightLayer(lightData.lightLayers, builtinData.renderingLayers))
                {
                    DirectLighting lighting = EvaluateBSDF_Area(context, V, posInput, preLightData, lightData, bsdfData, builtinData);
                    AccumulateDirectLighting(lighting, aggregateLighting);
                }

                lightData = FetchLight(lightStart, min(++i, last));
            }
        }
    }
#endif
    
    // Note: We can't apply the IndirectDiffuseMultiplier here as with GBuffer, Emissive is part of the bakeDiffuseLighting.
    // so IndirectDiffuseMultiplier is apply in PostInitBuiltinData or related location (like for probe volume)
    aggregateLighting.indirect.specularReflected *= GetIndirectSpecularMultiplier(builtinData.renderingLayers);

    // Also Apply indiret diffuse (GI)
    // PostEvaluateBSDF will perform any operation wanted by the material and sum everything into diffuseLighting and specularLighting
    PostEvaluateBSDF(context, V, posInput, preLightData, bsdfData, builtinData, aggregateLighting, lightLoopOutput);
}

#endif
