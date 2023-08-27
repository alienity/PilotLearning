#pragma once

void addEmissive(const FrameUniforms frameUniforms, const MaterialInputs material, inout float4 color)
{
#if defined(MATERIAL_HAS_EMISSIVE)
    float4 emissive    = material.emissive;
    float  attenuation = lerp(1.0, frameUniforms.exposure, emissive.w);
    color.rgb += emissive.rgb * (attenuation * color.a);
#endif
}

/**
 * Evaluates unlit materials. In this lighting model, only the base color and
 * emissive properties are taken into account:
 *
 * finalColor = baseColor + emissive
 *
 * The emissive factor is only applied if the fragment passes the optional
 * alpha test.
 *
 * When the shadowMultiplier property is enabled on the material, the final
 * color is multiplied by the inverse light visibility to apply a shadow.
 * This is mostly useful in AR to cast shadows on unlit transparent shadow
 * receiving planes.
 */
float4 evaluateMaterial(const FramUniforms frameUniforms, const MaterialInputs material, const FrameUniforms frameUniforms)
{
    float4 color = material.baseColor;

#if defined(VARIANT_HAS_DIRECTIONAL_LIGHTING)
/*
#if defined(VARIANT_HAS_SHADOWING)
    float visibility               = 1.0;
    uint  cascade                  = getShadowCascade();
    bool  cascadeHasVisibleShadows = bool(frameUniforms.cascades & ((1u << cascade) << 8u));
    bool  hasDirectionalShadows    = bool(frameUniforms.directionalShadows & 1u);
    if (hasDirectionalShadows && cascadeHasVisibleShadows)
    {
        float4 shadowPosition = getShadowPosition(cascade);
        visibility            = shadow(true, light_shadowMap, cascade, shadowPosition, 0.0f);
    }
    if ((frameUniforms.directionalShadows & 0x2u) != 0u && visibility > 0.0)
    {
        if ((object_uniforms.flagsChannels & FILAMENT_OBJECT_CONTACT_SHADOWS_BIT) != 0u)
        {
            visibility *= (1.0 - screenSpaceContactShadow(frameUniforms.lightDirection));
        }
    }
    color *= 1.0 - visibility;
#else
    color = float4(0.0);
#endif
*/
    color = float4(0.0);
#elif defined(MATERIAL_HAS_SHADOW_MULTIPLIER)
    color = float4(0.0);
#endif

    addEmissive(frameUniforms, material, color);

    return color;
}
