
#ifndef __SHADING_LIT_HLSLI__
#define __SHADING_LIT_HLSLI__

#include "MaterialInputFS.hlsli"
#include "ShadingParameters.hlsli"
#include "ShadingModelStandard.hlsli"
#include "LightDirectional.hlsli"

//------------------------------------------------------------------------------
// Lighting
//------------------------------------------------------------------------------

float computeDiffuseAlpha(float a, const FrameUniforms frameUniforms)
{
#if defined(BLEND_MODE_TRANSPARENT) || defined(BLEND_MODE_FADE)
    return a;
#else
    return 1.0;
#endif
}

void getCommonPixelParams(const MaterialInputs material, inout PixelParams pixel)
{
    float4 baseColor = material.baseColor;

#if defined(BLEND_MODE_FADE) && !defined(SHADING_MODEL_UNLIT)
    // Since we work in premultiplied alpha mode, we need to un-premultiply
    // in fade mode so we can apply alpha to both the specular and diffuse
    // components at the end
    unpremultiply(baseColor);
#endif

#if defined(SHADING_MODEL_SPECULAR_GLOSSINESS)
    // This is from KHR_materials_pbrSpecularGlossiness.
    float3  specularColor = material.specularColor;
    float metallic      = computeMetallicFromSpecularColor(specularColor);

    pixel.diffuseColor = computeDiffuseColor(baseColor, material.metallic);
    pixel.f0           = specularColor;
#elif !defined(SHADING_MODEL_CLOTH)
    pixel.diffuseColor = computeDiffuseColor(baseColor, material.metallic);
#if !defined(SHADING_MODEL_SUBSURFACE) && (!defined(MATERIAL_HAS_REFLECTANCE) && defined(MATERIAL_HAS_IOR))
    float reflectance  = iorToF0(max(1.0, material.ior), 1.0);
#else
    // Assumes an interface from air to an IOR of 1.5 for dielectrics
    float reflectance = computeDielectricF0(material.reflectance);
#endif
    pixel.f0           = computeF0(baseColor, material.metallic, reflectance);
#else
    pixel.diffuseColor    = baseColor.rgb;
    pixel.f0              = material.sheenColor;
#if defined(MATERIAL_HAS_SUBSURFACE_COLOR)
    pixel.subsurfaceColor = material.subsurfaceColor;
#endif
#endif

#if !defined(SHADING_MODEL_CLOTH) && !defined(SHADING_MODEL_SUBSURFACE)
#if defined(MATERIAL_HAS_REFRACTION)
    // Air's Index of refraction is 1.000277 at STP but everybody uses 1.0
    const float airIor = 1.0;
#if !defined(MATERIAL_HAS_IOR)
    // [common case] ior is not set in the material, deduce it from F0
    float materialor = f0ToIor(pixel.f0.g);
#else
    // if ior is set in the material, use it (can lead to unrealistic materials)
    float materialor   = max(1.0, material.ior);
#endif
    pixel.etaIR = airIor / materialor; // air -> material
    pixel.etaRI = materialor / airIor; // material -> air
#if defined(MATERIAL_HAS_TRANSMISSION)
    pixel.transmission = saturate(material.transmission);
#else
    pixel.transmission = 1.0;
#endif
#if defined(MATERIAL_HAS_ABSORPTION)
#if defined(MATERIAL_HAS_THICKNESS) || defined(MATERIAL_HAS_MICRO_THICKNESS)
    pixel.absorption = max(float3(0.0), material.absorption);
#else
    pixel.absorption = saturate(material.absorption);
#endif
#else
    pixel.absorption   = float3(0.0);
#endif
#if defined(MATERIAL_HAS_THICKNESS)
    pixel.thickness = max(0.0, material.thickness);
#endif
#if defined(MATERIAL_HAS_MICRO_THICKNESS) && (REFRACTION_TYPE == REFRACTION_TYPE_THIN)
    pixel.uThickness = max(0.0, material.microThickness);
#else
    pixel.uThickness   = 0.0;
#endif
#endif
#endif
}

void getSheenPixelParams(const CommonShadingStruct commonShadingStruct, const MaterialInputs material, inout PixelParams pixel) {
#if defined(MATERIAL_HAS_SHEEN_COLOR) && !defined(SHADING_MODEL_CLOTH) && !defined(SHADING_MODEL_SUBSURFACE)
    pixel.sheenColor = material.sheenColor;

    float sheenPerceptualRoughness = material.sheenRoughness;
    sheenPerceptualRoughness = clamp(sheenPerceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
    
    pixel.sheenPerceptualRoughness = sheenPerceptualRoughness;
    pixel.sheenRoughness = perceptualRoughnessToRoughness(sheenPerceptualRoughness);
#endif
}

void getClearCoatPixelParams(const CommonShadingStruct params, const MaterialInputs material, inout PixelParams pixel) {
#if defined(MATERIAL_HAS_CLEAR_COAT)
    pixel.clearCoat = material.clearCoat;

    // Clamp the clear coat roughness to avoid divisions by 0
    float clearCoatPerceptualRoughness = material.clearCoatRoughness;
    clearCoatPerceptualRoughness =
            clamp(clearCoatPerceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
    
    pixel.clearCoatPerceptualRoughness = clearCoatPerceptualRoughness;
    pixel.clearCoatRoughness = perceptualRoughnessToRoughness(clearCoatPerceptualRoughness);

#if defined(CLEAR_COAT_IOR_CHANGE)
    // The base layer's f0 is computed assuming an interface from air to an IOR
    // of 1.5, but the clear coat layer forms an interface from IOR 1.5 to IOR
    // 1.5. We recompute f0 by first computing its IOR, then reconverting to f0
    // by using the correct interface
    pixel.f0 = lerp(pixel.f0, f0ClearCoatToSurface(pixel.f0), pixel.clearCoat);
#endif
#endif
}

void getRoughnessPixelParams(const CommonShadingStruct params, const MaterialInputs material, inout PixelParams pixel) {
#if defined(SHADING_MODEL_SPECULAR_GLOSSINESS)
    float perceptualRoughness = computeRoughnessFromGlossiness(material.glossiness);
#else
    float perceptualRoughness = material.roughness;
#endif

    // This is used by the refraction code and must be saved before we apply specular AA
    pixel.perceptualRoughnessUnclamped = perceptualRoughness;
    
#if defined(MATERIAL_HAS_CLEAR_COAT) && defined(MATERIAL_HAS_CLEAR_COAT_ROUGHNESS)
    // This is a hack but it will do: the base layer must be at least as rough
    // as the clear coat layer to take into account possible diffusion by the
    // top layer
    float basePerceptualRoughness = max(perceptualRoughness, pixel.clearCoatPerceptualRoughness);
    perceptualRoughness = lerp(perceptualRoughness, basePerceptualRoughness, pixel.clearCoat);
#endif

    // Clamp the roughness to a minimum value to avoid divisions by 0 during lighting
    pixel.perceptualRoughness = clamp(perceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
    // Remaps the roughness to a perceptually linear roughness (roughness^2)
    pixel.roughness = perceptualRoughnessToRoughness(pixel.perceptualRoughness);
}

void getSubsurfacePixelParams(const MaterialInputs material, inout PixelParams pixel) {
#if defined(SHADING_MODEL_SUBSURFACE)
    pixel.subsurfacePower = material.subsurfacePower;
    pixel.subsurfaceColor = material.subsurfaceColor;
    pixel.thickness = saturate(material.thickness);
#endif
}

void getAnisotropyPixelParams(const CommonShadingStruct commonShadingStruct, const MaterialInputs material, inout PixelParams pixel)
{
#if defined(MATERIAL_HAS_ANISOTROPY)
    float3 direction   = material.anisotropyDirection;
    pixel.anisotropy   = material.anisotropy;
    pixel.anisotropicT = normalize(getWorldTangentFrame(commonShadingStruct) * direction);
    pixel.anisotropicB = normalize(cross(getWorldGeometricNormalVector(commonShadingStruct), pixel.anisotropicT));
#endif
}
/*
void getEnergyCompensationPixelParams(const CommonShadingStruct commonShadingStruct, inout PixelParams pixel) {
    // Pre-filtered DFG term used for image-based lighting
    pixel.dfg = prefilteredDFG(pixel.perceptualRoughness, commonShadingStruct.shading_NoV);

#if !defined(SHADING_MODEL_CLOTH)
    // Energy compensation for multiple scattering in a microfacet model
    // See "Multiple-Scattering Microfacet BSDFs with the Smith Model"
    pixel.energyCompensation = 1.0 + pixel.f0 * (1.0 / pixel.dfg.y - 1.0);
#else
    pixel.energyCompensation = float3(1.0);
#endif

#if !defined(SHADING_MODEL_CLOTH)
#if defined(MATERIAL_HAS_SHEEN_COLOR)
    pixel.sheenDFG = prefilteredDFG(pixel.sheenPerceptualRoughness, commonShadingStruct.shading_NoV).z;
    pixel.sheenScaling = 1.0 - max3(pixel.sheenColor) * pixel.sheenDFG;
#endif
#endif
}
*/
/**
 * Computes all the parameters required to shade the current pixel/fragment.
 * These parameters are derived from the MaterialInputs structure computed
 * by the user's material code.
 *
 * This function is also responsible for discarding the fragment when alpha
 * testing fails.
 */
void getPixelParams(const CommonShadingStruct commonShadingStruct, const MaterialInputs materialInputs, out PixelParams pixel) {
    getCommonPixelParams(materialInputs, pixel);
    getSheenPixelParams(commonShadingStruct, materialInputs, pixel);
    getClearCoatPixelParams(commonShadingStruct, materialInputs, pixel);
    getRoughnessPixelParams(commonShadingStruct, materialInputs, pixel);
    getSubsurfacePixelParams(materialInputs, pixel);
    getAnisotropyPixelParams(commonShadingStruct, materialInputs, pixel);
    //getEnergyCompensationPixelParams(commonShadingStruct, pixel);
}

/**
 * This function evaluates all lights one by one:
 * - Image based lights (IBL)
 * - Directional lights
 * - Punctual lights
 *
 * Area lights are currently not supported.
 *
 * Returns a pre-exposed HDR RGBA color in linear space.
 */
float4 evaluateLights(const FramUniforms frameUniforms, const CommonShadingStruct commonShadingStruct, const MaterialInputs materialInputs) {
    PixelParams pixel;
    getPixelParams(commonShadingStruct, materialInputs, pixel);

    // Ideally we would keep the diffuse and specular components separate
    // until the very end but it costs more ALUs on mobile. The gains are
    // currently not worth the extra operations
    float3 color = float3(0.0);

    //// We always evaluate the IBL as not having one is going to be uncommon,
    //// it also saves 1 shader variant
    //evaluateIBL(material, pixel, color);

#if defined(VARIANT_HAS_DIRECTIONAL_LIGHTING)
    evaluateDirectionalLight(material, pixel, color);
#endif

#if defined(VARIANT_HAS_DYNAMIC_LIGHTING)
    evaluatePunctualLights(material, pixel, color);
#endif

#if defined(BLEND_MODE_FADE) && !defined(SHADING_MODEL_UNLIT)
    // In fade mode we un-premultiply baseColor early on, so we need to
    // premultiply again at the end (affects diffuse and specular lighting)
    color *= material.baseColor.a;
#endif

    return float4(color, computeDiffuseAlpha(material.baseColor.a));
}

void addEmissive(const FramUniforms frameUniforms, const MaterialInputs material, inout float4 color) {
#if defined(MATERIAL_HAS_EMISSIVE)
    float4 emissive = material.emissive;
    float attenuation = lerp(1.0, getExposure(frameUniforms), emissive.w);
    color.rgb += emissive.rgb * (attenuation * color.a);
#endif
}

/**
 * Evaluate lit materials. The actual shading model used to do so is defined
 * by the function surfaceShading() found in shading_model_*.fs.
 *
 * Returns a pre-exposed HDR RGBA color in linear space.
 */
float4 evaluateMaterial(const FramUniforms frameUniforms, const CommonShadingStruct commonShadingStruct, const MaterialInputs materialInputs) {
    float4 color = evaluateLights(frameUniforms, commonShadingStruct, materialInputs);
    addEmissive(frameUniforms, material, color);
    return color;
}

#endif // __SHADING_LIT_HLSLI__