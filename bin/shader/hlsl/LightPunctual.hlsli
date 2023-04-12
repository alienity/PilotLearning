#pragma once

//------------------------------------------------------------------------------
// Directional light evaluation
//------------------------------------------------------------------------------

#if FILAMENT_QUALITY >= FILAMENT_QUALITY_HIGH
#define SUN_AS_AREA_LIGHT
#endif

float3 sampleSunAreaLight(const FrameUniforms frameUniforms, const float3 lightDirection)
{
#if defined(SUN_AS_AREA_LIGHT)
    if (frameUniforms.sun.w >= 0.0)
    {
        // simulate sun as disc area light
        float  LoR = dot(lightDirection, shading_reflected);
        float  d   = frameUniforms.sun.x;
        float3 s   = shading_reflected - LoR * lightDirection;
        return LoR < d ? normalize(lightDirection * d + normalize(s) * frameUniforms.sun.y) : shading_reflected;
    }
#endif
    return lightDirection;
}

Light getDirectionalLight(const FrameUniforms frameUniforms)
{
    Light light;
    // note: lightColorIntensity.w is always premultiplied by the exposure
    light.colorIntensity = frameUniforms.lightColorIntensity;
    light.l              = sampleSunAreaLight(frameUniforms.lightDirection);
    light.attenuation    = 1.0;
    light.NoL            = saturate(dot(shading_normal, light.l));
    light.channels       = frameUniforms.lightChannels & 0xFFu;
    return light;
}

void evaluateDirectionalLight(const FrameUniforms  frameUniforms,
                              const MaterialInputs material,
                              const PixelParams    pixel,
                              inout float3         color)
{

    Light light = getDirectionalLight(frameUniforms);

    uint channels = object_uniforms.flagsChannels & 0xFFu;
    if ((light.channels & channels) == 0u)
    {
        return;
    }

#if defined(MATERIAL_CAN_SKIP_LIGHTING)
    if (light.NoL <= 0.0)
    {
        return;
    }
#endif

    float visibility = 1.0;
#if defined(VARIANT_HAS_SHADOWING)
    if (light.NoL > 0.0)
    {
        float ssContactShadowOcclusion = 0.0;

        uint cascade                  = getShadowCascade();
        bool cascadeHasVisibleShadows = bool(frameUniforms.cascades & ((1u << cascade) << 8u));
        bool hasDirectionalShadows    = bool(frameUniforms.directionalShadows & 1u);
        if (hasDirectionalShadows && cascadeHasVisibleShadows)
        {
            float4 shadowPosition = getShadowPosition(cascade);
            visibility            = shadow(true, light_shadowMap, cascade, shadowPosition, 0.0f);
        }
        if ((frameUniforms.directionalShadows & 0x2u) != 0u && visibility > 0.0)
        {
            if ((object_uniforms.flagsChannels & FILAMENT_OBJECT_CONTACT_SHADOWS_BIT) != 0u)
            {
                ssContactShadowOcclusion = screenSpaceContactShadow(light.l);
            }
        }

        visibility *= 1.0 - ssContactShadowOcclusion;

#if defined(MATERIAL_HAS_AMBIENT_OCCLUSION)
        visibility *= computeMicroShadowing(light.NoL, material.ambientOcclusion);
#endif
#if defined(MATERIAL_CAN_SKIP_LIGHTING)
        if (visibility <= 0.0)
        {
            return;
        }
#endif
    }
#endif

#if defined(MATERIAL_HAS_CUSTOM_SURFACE_SHADING)
    color.rgb += customSurfaceShading(material, pixel, light, visibility);
#else
    color.rgb += surfaceShading(pixel, light, visibility);
#endif
}
