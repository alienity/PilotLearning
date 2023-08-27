#pragma once
#include "InputTypes.hlsli"
#include "BSDF.hlsli"

struct VaringStruct
{
    float4 vertex_position : SV_POSITION;
    float3 vertex_worldNormal : NORMAL;
    float4 vertex_worldTangent : TANGENT;
    float4 vertex_worldPosition : TEXCOORD0;
    float2 vertex_uv01 : TEXCOORD1;
};

struct MaterialInputs
{
    float4 baseColor;
    float roughness;
    float metallic;
    float reflectance;
    float ambientOcclusion;
    float4 emissive;
    float3 normal;
};

struct CommonShadingStruct
{
    float3x3 shading_tangentToWorld; // TBN matrix
    float3 shading_position; // position of the fragment in world space
    float3 shading_view; // normalized vector from the fragment to the eye
    float3 shading_normal; // normalized transformed normal, in world space
    float3 shading_geometricNormal; // normalized geometric normal, in world space
    float3 shading_reflected; // reflection of view about normal
    float shading_NoV; // dot(normal, view), always strictly >= MIN_N_DOT_V
    float2 shading_normalizedViewportCoord;
};

struct PixelParams
{
    float3 diffuseColor;
    float perceptualRoughness;
    float perceptualRoughnessUnclamped;
    float3 f0;
    float roughness;
    float3 dfg;
    float3 energyCompensation;
};

struct Light
{
    float4 colorIntensity; // rgb, pre-exposed intensity
    float3 l;
    float attenuation;
    float3 worldPosition;
    float NoL;
    float3 direction;
    float zLight;
    bool castsShadows;
    bool contactShadows;
    uint type;
    uint shadowIndex;
    uint channels;
};

void inflateMaterial(
    const VaringStruct varingStruct,
    const PerMaterialViewIndexBuffer materialViewIndexBuffer, 
    SamplerState materialSampler, inout MaterialInputs material)
{
    const PerMaterialParametersBuffer materialParameterBuffer = ResourceDescriptorHeap[materialViewIndexBuffer.parametersBufferIndex];
    
    const float2 uv = varingStruct.vertex_uv01;
    
    float4 baseColorFactor = materialParameterBuffer.baseColorFactor;
    float4 metallicFactor = materialParameterBuffer.metallicFactor;
    float roughnessFactor = materialParameterBuffer.roughnessFactor;
    float reflectanceFactor = materialParameterBuffer.reflectanceFactor;
    float clearCoatFactor = materialParameterBuffer.clearCoatFactor;
    float clearCoatRoughnessFactor = materialParameterBuffer.clearCoatRoughnessFactor;
    float anisotropyFactor = materialParameterBuffer.anisotropyFactor;
    float normalScale = materialParameterBuffer.normalScale;
    float occlusionStrength = materialParameterBuffer.occlusionStrength;
    
    float3 emissiveFactor = materialParameterBuffer.emissiveFactor;

    float2 base_color_tilling = materialParameterBuffer.base_color_tilling;
    float2 metallic_roughness_tilling = materialParameterBuffer.metallic_roughness_tilling;
    float2 normal_tilling = materialParameterBuffer.normal_tilling;
    float2 occlusion_tilling = materialParameterBuffer.occlusion_tilling;
    float2 emissive_tilling = materialParameterBuffer.emissive_tilling;
    
    Texture2D<float4> baseColorTexture = ResourceDescriptorHeap[materialViewIndexBuffer.baseColorIndex];
    Texture2D<float4> metallicRoughnessTexture = ResourceDescriptorHeap[materialViewIndexBuffer.metallicRoughnessIndex];
    Texture2D<float3> normalTexture = ResourceDescriptorHeap[materialViewIndexBuffer.normalIndex];
    Texture2D<float3> occlusionTexture = ResourceDescriptorHeap[materialViewIndexBuffer.occlusionIndex];
    Texture2D<float4> emissionTexture = ResourceDescriptorHeap[materialViewIndexBuffer.emissionIndex];
    
    material.baseColor = baseColorTexture.Sample(materialSampler, uv * base_color_tilling) * baseColorFactor;
    float2 metallicRoughness = metallicRoughnessTexture.Sample(materialSampler, uv * metallic_roughness_tilling).rg;
    material.roughness = metallicRoughness.r * roughnessFactor;
    material.metallic = metallicRoughness.g * metallicFactor;
    material.reflectance = reflectanceFactor;
    material.ambientOcclusion = occlusionTexture.Sample(materialSampler, uv * occlusion_tilling).r * occlusionStrength;
    material.emissive = emissionTexture.Sample(materialSampler, uv * base_color_tilling);
    material.emissive.w *= emissiveFactor;
    material.normal = (normalTexture.Sample(materialSampler, uv * normal_tilling) * 2.0f - 1.0f) * normalScale;

}

//------------------------------------------------------------------------------
// Material evaluation
//------------------------------------------------------------------------------

/**
 * Computes global shading parameters used to apply lighting, such as the view
 * vector in world space, the tangent frame at the shading point, etc.
 */
void computeShadingParams(const FrameUniforms frameUniforms, const VaringStruct varing, inout CommonShadingStruct commonShadingStruct)
{
    float3 n = varing.vertex_worldNormal;
    float3 t = varing.vertex_worldTangent.xyz;
    float3 b = cross(n, t) * sign(varing.vertex_worldTangent.w);
    
    commonShadingStruct.shading_geometricNormal = normalize(n);

    // We use unnormalized post-interpolation values, assuming mikktspace tangents
    commonShadingStruct.shading_tangentToWorld = float3x3(t, b, n);

    commonShadingStruct.shading_position = varing.vertex_worldPosition.xyz;

    // With perspective camera, the view vector is cast from the fragment pos to the eye position,
    // With ortho camera, however, the view vector is the same for all fragments:
    float4x4 projectionMatrix = frameUniforms.cameraUniform.clipFromViewMatrix;
    float4x4 worldFromViewMatrix = frameUniforms.cameraUniform.worldFromViewMatrix;
    
    float3 sv = isPerspectiveProjection(projectionMatrix) ? (worldFromViewMatrix[3].xyz - commonShadingStruct.shading_position) : worldFromViewMatrix[2].xyz; // ortho camera backward dir
    commonShadingStruct.shading_view = normalize(sv);

    // we do this so we avoid doing (matrix multiply), but we burn 4 varyings:
    //    p = clipFromWorldMatrix * shading_position;
    //    shading_normalizedViewportCoord = p.xy * 0.5 / p.w + 0.5
    commonShadingStruct.shading_normalizedViewportCoord = varing.vertex_position.xy * (0.5 / varing.vertex_position.w) + 0.5;
}

/**
 * Computes global shading parameters that the material might need to access
 * before lighting: N dot V, the reflected vector and the shading normal (before
 * applying the normal map). These parameters can be useful to material authors
 * to compute other material properties.
 *
 * This function must be invoked by the user's material code (guaranteed by
 * the material compiler) after setting a value for MaterialInputs.normal.
 */
void prepareMaterial(const MaterialInputs material, inout CommonShadingStruct commonShadingStruct)
{
    commonShadingStruct.shading_normal = normalize(mul(commonShadingStruct.shading_tangentToWorld, material.normal));
    commonShadingStruct.shading_NoV = clampNoV(dot(commonShadingStruct.shading_normal, commonShadingStruct.shading_view));
    commonShadingStruct.shading_reflected = reflect(-commonShadingStruct.shading_view, commonShadingStruct.shading_normal);
}

void getCommonPixelParams(const MaterialInputs material, inout PixelParams pixel)
{
    float4 baseColor = material.baseColor;

    //// Since we work in premultiplied alpha mode, we need to un-premultiply
    //// in fade mode so we can apply alpha to both the specular and diffuse
    //// components at the end
    //unpremultiply(baseColor);

    pixel.diffuseColor = computeDiffuseColor(baseColor, material.metallic);
    // Assumes an interface from air to an IOR of 1.5 for dielectrics
    float reflectance = computeDielectricF0(material.reflectance);
    pixel.f0 = computeF0(baseColor, material.metallic, reflectance);
}

void getRoughnessPixelParams(const CommonShadingStruct params, const MaterialInputs material, inout PixelParams pixel)
{
    float perceptualRoughness = material.roughness;

    // This is used by the refraction code and must be saved before we apply specular AA
    pixel.perceptualRoughnessUnclamped = perceptualRoughness;
    
    // Clamp the roughness to a minimum value to avoid divisions by 0 during lighting
    pixel.perceptualRoughness = clamp(perceptualRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
    // Remaps the roughness to a perceptually linear roughness (roughness^2)
    pixel.roughness = perceptualRoughnessToRoughness(pixel.perceptualRoughness);
}

/**
 * Computes all the parameters required to shade the current pixel/fragment.
 * These parameters are derived from the MaterialInputs structure computed
 * by the user's material code.
 *
 * This function is also responsible for discarding the fragment when alpha
 * testing fails.
 */
void getPixelParams(const CommonShadingStruct commonShadingStruct, const MaterialInputs materialInputs, out PixelParams pixel)
{
    getCommonPixelParams(materialInputs, pixel);
    getRoughnessPixelParams(commonShadingStruct, materialInputs, pixel);
}

float3 isotropicLobe(const PixelParams pixel, const float3 h, float NoV, float NoL, float NoH, float LoH)
{

    float D = distribution(pixel.roughness, NoH, h);
    float V = visibility(pixel.roughness, NoV, NoL);
    float3 F = fresnel(pixel.f0, LoH);

    return (D * V) * F;
}

float3 specularLobe(const PixelParams pixel, const float3 h, float NoV, float NoL, float NoH, float LoH)
{
    return isotropicLobe(pixel, h, NoV, NoL, NoH, LoH);
}

float3 diffuseLobe(const PixelParams pixel, float NoV, float NoL, float LoH)
{
    return pixel.diffuseColor * diffuse(pixel.roughness, NoV, NoL, LoH);
}

/**
 * Evaluates lit materials with the standard shading model. This model comprises
 * of 2 BRDFs: an optional clear coat BRDF, and a regular surface BRDF.
 *
 * Surface BRDF
 * The surface BRDF uses a diffuse lobe and a specular lobe to render both
 * dielectrics and conductors. The specular lobe is based on the Cook-Torrance
 * micro-facet model (see brdf.fs for more details). In addition, the specular
 * can be either isotropic or anisotropic.
 *
 * Clear coat BRDF
 * The clear coat BRDF simulates a transparent, absorbing dielectric layer on
 * top of the surface. Its IOR is set to 1.5 (polyutherane) to simplify
 * our computations. This BRDF only contains a specular lobe and while based
 * on the Cook-Torrance microfacet model, it uses cheaper terms than the surface
 * BRDF's specular lobe (see brdf.fs).
 */
float3 surfaceShading(const CommonShadingStruct params, const PixelParams pixel, const Light light, float occlusion)
{
    float3 h = normalize(params.shading_view + light.l);

    float NoV = params.shading_NoV;
    float NoL = saturate(light.NoL);
    float NoH = saturate(dot(params.shading_normal, h));
    float LoH = saturate(dot(light.l, h));

    float3 Fr = specularLobe(pixel, h, NoV, NoL, NoH, LoH);
    float3 Fd = diffuseLobe(pixel, NoV, NoL, LoH);

    // TODO: attenuate the diffuse lobe to avoid energy gain

    // The energy compensation term is used to counteract the darkening effect
    // at high roughness
    float3 color = Fd + Fr * pixel.energyCompensation;

    return (color * light.colorIntensity.rgb) * (light.colorIntensity.w * light.attenuation * NoL * occlusion);
}

Light getDirectionalLight(const CommonShadingStruct params, const FrameUniforms frameUniforms)
{
    Light light;
    // note: lightColorIntensity.w is always premultiplied by the exposure
    light.colorIntensity = frameUniforms.directionalLight.lightColorIntensity;
    light.l = frameUniforms.directionalLight.lightDirection;
    light.attenuation = 1.0;
    light.NoL = saturate(dot(params.shading_normal, light.l));
    return light;
}

void evaluateDirectionalLight(
    const FrameUniforms frameUniforms, 
    const PerRenderableMeshData perMeshData, 
    const CommonShadingStruct params,
    const MaterialInputs material, 
    const PixelParams pixel, inout float3 color)
{

    Light light = getDirectionalLight(params, frameUniforms);
    
    float visibility = 1.0;
#if defined(VARIANT_HAS_SHADOWING)
    if (light.NoL > 0.0)
    {
        float ssContactShadowOcclusion = 0.0;

        uint cascade                  = getShadowCascade(params, frameUniforms);
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
    }
#endif

    color.rgb += surfaceShading(params, pixel, light, visibility);
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
float4 evaluateLights(const FrameUniforms frameUniforms, const PerRenderableMeshData perMeshData, const CommonShadingStruct commonShadingStruct, const MaterialInputs materialInputs)
{
    PixelParams pixel;
    getPixelParams(commonShadingStruct, materialInputs, pixel);

    // Ideally we would keep the diffuse and specular components separate
    // until the very end but it costs more ALUs on mobile. The gains are
    // currently not worth the extra operations
    float3 color = float3(0.0);

    //// We always evaluate the IBL as not having one is going to be uncommon,
    //// it also saves 1 shader variant
    //evaluateIBL(materialInputs, pixel, color);

    evaluateDirectionalLight(frameUniforms, perMeshData, commonShadingStruct, materialInputs, pixel, color);

    //evaluatePunctualLights(material, pixel, color);

    // In fade mode we un-premultiply baseColor early on, so we need to
    // premultiply again at the end (affects diffuse and specular lighting)
    color *= materialInputs.baseColor.a;

    return color;
}

void addEmissive(const FrameUniforms frameUniforms, const MaterialInputs material, inout float4 color)
{
    float4 emissive = material.emissive;
    float attenuation = lerp(1.0, frameUniforms.cameraUniform.exposure, emissive.w);
    color.rgb += emissive.rgb * (attenuation * color.a);
}

/**
 * Evaluate lit materials. The actual shading model used to do so is defined
 * by the function surfaceShading() found in shading_model_*.fs.
 *
 * Returns a pre-exposed HDR RGBA color in linear space.
 */
float4 evaluateMaterial(const FrameUniforms frameUniforms, const PerRenderableMeshData perMeshData, const CommonShadingStruct commonShadingStruct, const MaterialInputs materialInputs)
{
    float4 color = evaluateLights(frameUniforms, perMeshData, commonShadingStruct, materialInputs);
    addEmissive(frameUniforms, materialInputs, color);
    return color;
}














