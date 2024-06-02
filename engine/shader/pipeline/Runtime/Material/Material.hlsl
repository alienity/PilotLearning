#ifndef MATERIAL_INCLUDED
#define MATERIAL_INCLUDED

#include "../ShaderLibrary/Color.hlsl"
#include "../ShaderLibrary/Packing.hlsl"
#include "../ShaderLibrary/BSDF.hlsl"
#include "../ShaderLibrary/GeometricTools.hlsl"
#include "../ShaderLibrary/CommonMaterial.hlsl"
#include "../ShaderLibrary/EntityLighting.hlsl"
#include "../ShaderLibrary/ImageBasedLighting.hlsl"
 //#include "../Lighting/AtmosphericScattering/AtmosphericScattering.hlsl"
#include "../Material/MaterialBlendModeEnum.hlsl"

// Guidelines for Material Keyword.
// There is a set of Material Keyword that a HD shaders must define (or not define). We call them system KeyWord.
// .Shader need to define:
// - _SURFACE_TYPE_TRANSPARENT if they use a transparent material
// - _ENABLE_FOG_ON_TRANSPARENT if fog is enable on transparent surface
// - _DISABLE_DECALS if the material don't support decals

#define HAVE_DECALS ( (defined(DECALS_3RT) || defined(DECALS_4RT)) && !defined(_DISABLE_DECALS) )

// If decals require surface gradients, we will use gradients too
#if HAVE_DECALS && defined(DECAL_SURFACE_GRADIENT)
#define DECAL_NORMAL_BLENDING
#endif

#define APPLY_FOG_ON_SKY_REFLECTIONS

//-----------------------------------------------------------------------------
// ApplyBlendMode function
//-----------------------------------------------------------------------------

float4 ApplyBlendMode(float3 diffuseLighting, float3 specularLighting, float opacity)
{

#ifndef _SURFACE_TYPE_TRANSPARENT
    #ifndef _ALPHATEST_ON
        // We hardcode opacity to 1 to avoid issues in forward when alpha might be coming from the texture source, but we don't want to keep it in case alpha is preserved.
        opacity = 1;
    #endif
    return float4(diffuseLighting + specularLighting, opacity);
#else

    // ref: http://advances.realtimerendering.com/other/2016/naughty_dog/NaughtyDog_TechArt_Final.pdf
    // Lit transparent object should have reflection and transmission.
    // Transmission when not using "rough refraction mode" (with fetch in preblured background) is handled with blend mode.
    // However reflection should not be affected by blend mode. For example a glass should still display reflection and not lose the highlight when blend
    // This is the purpose of following function, "Cancel" the blend mode effect on the specular lighting but not on the diffuse lighting

    // In the case of alpha blend mode the code should be float4(diffuseLighting + (specularLighting / max(opacity, 0.01)), opacity)
    // However this have precision issue when reaching 0, so we change the blend mode and apply src * src_a inside the shader instead
    if (_BlendMode == BLENDMODE_ALPHA || _BlendMode == BLENDMODE_ADDITIVE)
        return float4(diffuseLighting * opacity + specularLighting * (
#ifdef SUPPORT_BLENDMODE_PRESERVE_SPECULAR_LIGHTING
        _EnableBlendModePreserveSpecularLighting ? 1.0f :
#endif
            opacity), opacity);
    else
        return float4(diffuseLighting + specularLighting * (
#ifdef SUPPORT_BLENDMODE_PRESERVE_SPECULAR_LIGHTING
        _EnableBlendModePreserveSpecularLighting ? 1.0f :
#endif
            opacity), opacity);

#endif
}

float4 ApplyBlendMode(float3 color, float opacity)
{
    return ApplyBlendMode(color, float3(0.0, 0.0, 0.0), opacity);
}

//-----------------------------------------------------------------------------
// Fog sampling function for materials
//-----------------------------------------------------------------------------

// Used for transparent object. input color is color + alpha of the original transparent pixel.
// This must be call after ApplyBlendMode to work correctly
// Caution: Must stay in sync with VFXApplyFog in VFXCommon.hlsl
float4 EvaluateAtmosphericScattering(PositionInputs posInput, float3 V, float4 inputColor)
{
    float4 result = inputColor;

#ifdef _ENABLE_FOG_ON_TRANSPARENT
    float3 volColor, volOpacity;
    EvaluateAtmosphericScattering(posInput, V, volColor, volOpacity); // Premultiplied alpha

    if (_BlendMode == BLENDMODE_ALPHA)
    {
        // Regular alpha blend need to multiply fog color by opacity (as we do src * src_a inside the shader)
        // result.rgb = lerp(result.rgb, unpremul_volColor * result.a, volOpacity);
        // result.rgb = result.rgb + volOpacity * (unpremul_volColor * result.a - result.rgb);
        // result.rgb = result.rgb + volColor * result.a - result.rgb * volOpacity;
        result.rgb = result.rgb * (1 - volOpacity) + volColor * result.a;
    }
    else if (_BlendMode == BLENDMODE_ADDITIVE)
    {
        // For additive, we just need to fade to black with fog density (black + background == background color == fog color)
        result.rgb = result.rgb * (1.0 - volOpacity);
    }
    else if (_BlendMode == BLENDMODE_PREMULTIPLY)
    {
        // For Pre-Multiplied Alpha Blend, we need to multiply fog color by src alpha to match regular alpha blending formula.
        // result.rgb = lerp(result.rgb, unpremul_volColor * result.a, volOpacity);
        result.rgb = result.rgb * (1 - volOpacity) + volColor * result.a;
        // Note: this formula for color is correct, assuming we apply the Over operator afterwards
        // (see the appendix in the Deep Compositing paper). But do we?
        // Additionally, we do not modify the alpha here, which is most certainly WRONG.
    }

#else
    // Evaluation of fog for opaque objects is currently done in a full screen pass independent from any material parameters.
    // but this funtction is called in generic forward shader code so we need it to be neutral in this case.
#endif

    return result;
}
/*
// Used for sky reflection. The sky reflection cubemap doesn't contain fog.
// Apply height fog between pixel position and skybox, ignores volumetric fog for performance.
float4 EvaluateFogForSkyReflections(ShaderVariablesGlobal shaderVariablesGlobal, float3 positionWS, float3 R)
{
    float  startHeight = positionWS.y;
    float  cosZenith = R.y;

    float3 volAlbedo = shaderVariablesGlobal._HeightFogBaseScattering.xyz / shaderVariablesGlobal._HeightFogBaseExtinction;
    float odFallback = OpticalDepthHeightFog(shaderVariablesGlobal._HeightFogBaseExtinction, shaderVariablesGlobal._HeightFogBaseHeight,
            shaderVariablesGlobal._HeightFogExponents, cosZenith, startHeight, shaderVariablesGlobal._MaxFogDistance);
    float  trFallback = TransmittanceFromOpticalDepth(odFallback);

    float3 fog = GetFogColor(-R, shaderVariablesGlobal._MaxFogDistance) * volAlbedo * (1 - trFallback);
    return float4(fog, trFallback);
}
*/
//-----------------------------------------------------------------------------
// Alpha test replacement
//-----------------------------------------------------------------------------

// This function must be use instead of clip instruction. It allow to manage in which case the clip is perform for optimization purpose
void DoAlphaTest(float alpha, float alphaCutoff)
{
    // For Deferred:
    // If we have a prepass, we  may want to remove the clip from the GBuffer pass (otherwise HiZ does not work on PS4) - SHADERPASS_GBUFFER_BYPASS_ALPHA_TEST
    // For Forward Opaque:
    // If we have a prepass, we may want to remove the clip from the forward pass (otherwise HiZ does not work on PS4) - SHADERPASS_FORWARD_BYPASS_ALPHA_TEST
    // For Forward Transparent
    // Note: If SHADERPASS_GBUFFER_BYPASS_ALPHA_TEST or SHADERPASS_FORWARD_BYPASS_ALPHA_TEST are used, it mean that we must use ZTest depth equal for the pass (Need to use _ZTestDepthEqualForOpaque property).
#if !defined(SHADERPASS_FORWARD_BYPASS_ALPHA_TEST) && !defined(SHADERPASS_GBUFFER_BYPASS_ALPHA_TEST)
    clip(alpha - alphaCutoff);
#endif
}

// This function is the alternative version used for ray tracing
void DoAlphaTest(float alpha, float alphaCutoff, out bool alphaTestResult)
{
    alphaTestResult = alpha >= alphaCutoff;
}

//-----------------------------------------------------------------------------
// Reflection / Refraction hierarchy handling
//-----------------------------------------------------------------------------

// This function is use with reflection and refraction hierarchy of LightLoop
// It will add weight to hierarchyWeight but ensure that hierarchyWeight is not more than one
// by updating the weight value. Returned weight value must be apply on current lighting
// Example: Total hierarchyWeight is 0.8 and weight is 0.4. Function return hierarchyWeight of 1.0 and weight of 0.2
// hierarchyWeight and weight must be positive and between 0 and 1
void UpdateLightingHierarchyWeights(inout float hierarchyWeight, inout float weight)
{
    float accumulatedWeight = hierarchyWeight + weight;
    hierarchyWeight = saturate(accumulatedWeight);
    weight -= saturate(accumulatedWeight - hierarchyWeight);
}

#endif // MATERIAL_INCLUDED
