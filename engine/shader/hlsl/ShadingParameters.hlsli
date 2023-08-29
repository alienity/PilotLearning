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
    StructuredBuffer<PerMaterialParametersBuffer> _materialParameterBuffers = ResourceDescriptorHeap[materialViewIndexBuffer.parametersBufferIndex];

    const PerMaterialParametersBuffer materialParameterBuffer = _materialParameterBuffers[0];

    const float2 uv = varingStruct.vertex_uv01;
    
    float4 baseColorFactor = materialParameterBuffer.baseColorFactor;
    float metallicFactor = materialParameterBuffer.metallicFactor;
    float roughnessFactor = materialParameterBuffer.roughnessFactor;
    float reflectanceFactor = materialParameterBuffer.reflectanceFactor;
    float clearCoatFactor = materialParameterBuffer.clearCoatFactor;
    float clearCoatRoughnessFactor = materialParameterBuffer.clearCoatRoughnessFactor;
    float anisotropyFactor = materialParameterBuffer.anisotropyFactor;
    float normalScale = materialParameterBuffer.normalScale;
    float occlusionStrength = materialParameterBuffer.occlusionStrength;
    
    float emissiveFactor = materialParameterBuffer.emissiveFactor;

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
    material.metallic = metallicRoughness.r * metallicFactor;
    material.roughness = metallicRoughness.g * roughnessFactor;
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
    // http://www.mikktspace.com/
    float3 n = varing.vertex_worldNormal;
    float3 t = varing.vertex_worldTangent.xyz;
    float3 b = cross(n, t) * varing.vertex_worldTangent.w;
    
    commonShadingStruct.shading_geometricNormal = normalize(n);

    // We use unnormalized post-interpolation values, assuming mikktspace tangents
    commonShadingStruct.shading_tangentToWorld = transpose(float3x3(t, b, n));

    commonShadingStruct.shading_position = varing.vertex_worldPosition.xyz;

    // With perspective camera, the view vector is cast from the fragment pos to the eye position,
    // With ortho camera, however, the view vector is the same for all fragments:
    float4x4 projectionMatrix = frameUniforms.cameraUniform.clipFromViewMatrix;
    float4x4 worldFromViewMatrix = frameUniforms.cameraUniform.worldFromViewMatrix;
    
    float4x4 _worldFromViewMatrixTranspose = transpose(worldFromViewMatrix);

    float3 sv = isPerspectiveProjection(projectionMatrix) ? 
        (_worldFromViewMatrixTranspose[3].xyz - commonShadingStruct.shading_position) : _worldFromViewMatrixTranspose[2].xyz; // ortho camera backward dir
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

//------------------------------------------------------------------------------
// Shadow Sampling
//------------------------------------------------------------------------------

/**
 * Returns the cascade index for this fragment (between 0 and CONFIG_MAX_SHADOW_CASCADES - 1).
 */
void getShadowCascade(
    float4x4 viewFromWorldMatrix, float3 worldPosition, uint4 shadow_bounds, uint cascadeCount, 
    out uint cascadeLevel, out float2 shadow_bound_offset) 
{
    float2 posInView = mulMat4x4Float3(viewFromWorldMatrix, worldPosition).xy;
    float x = max(abs(posInView.x), abs(posInView.y));
    uint4 greaterZ = uint4(step(shadow_bounds * 0.9f * 0.5f, float4(x, x, x, x)));
    cascadeLevel = greaterZ.x + greaterZ.y + greaterZ.z + greaterZ.w;
    shadow_bound_offset = step(1, cascadeLevel & uint2(0x1, 0x2)) * 0.5;
}

float sampleDepth(
    const Texture2D<float> shadowmap, SamplerComparisonState shadowmapSampler, 
    const float2 offseted_uv, const float depth)
{
    float tempfShadow = shadowmap.SampleCmpLevelZero(shadowmapSampler, offseted_uv, depth);
    return tempfShadow;
}

float shadowSample_PCF_Low(
    const Texture2D<float> shadowmap, SamplerComparisonState shadowmapSampler, 
    const float4x4 light_proj_view, uint shadowmap_size, float2 shadow_bound_offset, const float3 positionWS, const float depthBias)
{
    float4 fragPosLightSpace = mul(light_proj_view, float4(positionWS, 1.0f));

    // perform perspective divide
    float3 shadowPosition = fragPosLightSpace.xyz / fragPosLightSpace.w;

    shadowPosition.xy = shadowPosition.xy * 0.5 + 0.5;
    shadowPosition.y = 1 - shadowPosition.y;

    float2 texelSize = float2(1.0f, 1.0f) / shadowmap_size;

    //  Castaño, 2013, "Shadow Mapping Summary Part 1"
    float depth = shadowPosition.z + depthBias;

    float2 offset = float2(0.5, 0.5);
    float2 uv = (shadowPosition.xy * shadowmap_size) + offset;
    float2 base = (floor(uv) - offset) * texelSize;
    float2 st = frac(uv);

    float2 uw = float2(3.0 - 2.0 * st.x, 1.0 + 2.0 * st.x);
    float2 vw = float2(3.0 - 2.0 * st.y, 1.0 + 2.0 * st.y);

    float2 u = float2((2.0 - st.x) / uw.x - 1.0, st.x / uw.y + 1.0);
    float2 v = float2((2.0 - st.y) / vw.x - 1.0, st.y / vw.y + 1.0);

    u *= texelSize.x;
    v *= texelSize.y;

    float sum = 0.0;
    sum += uw.x * vw.x * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.x, v.x)) * 0.5 + shadow_bound_offset, depth);
    sum += uw.y * vw.x * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.y, v.x)) * 0.5 + shadow_bound_offset, depth);
    sum += uw.x * vw.y * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.x, v.y)) * 0.5 + shadow_bound_offset, depth);
    sum += uw.y * vw.y * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.y, v.y)) * 0.5 + shadow_bound_offset, depth);
    return sum * (1.0 / 16.0);
}

// use hardware assisted PCF + 3x3 gaussian filter
float shadowSample_PCF(
    const Texture2D<float> shadowmap, SamplerComparisonState shadowmapSampler, 
    const float4x4 light_view_matrix, const float4x4 light_proj_view[4], 
    const float4 shadow_bounds, uint shadowmap_size, uint cascadeCount, const float3 positionWS, const float ndotl)
{
    uint cascadeLevel;
    float2 shadow_bound_offset;
    getShadowCascade(light_view_matrix, positionWS, shadow_bounds, cascadeCount, cascadeLevel, shadow_bound_offset);

    if(cascadeLevel == 4)
        return 1.0f;
    
    float shadow_bias = max(0.0015f * exp2(cascadeLevel) * (1.0 - ndotl), 0.0003f);  

    return shadowSample_PCF_Low(shadowmap, shadowmapSampler, 
        light_proj_view[cascadeLevel], shadowmap_size, shadow_bound_offset, positionWS, shadow_bias);
}

//------------------------------------------------------------------------------
// brdf calculation
//------------------------------------------------------------------------------

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

    pixel.energyCompensation = 1.0f;
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
    light.l = normalize(-frameUniforms.directionalLight.lightDirection);
    light.attenuation = 1.0;
    light.NoL = saturate(dot(params.shading_normal, light.l));
    return light;
}

void evaluateDirectionalLight(
    const FrameUniforms frameUniforms, 
    const PerRenderableMeshData perMeshData, 
    const CommonShadingStruct params,
    const MaterialInputs material, 
    const PixelParams pixel,
    const SamplerStruct samplerStruct, 
    inout float3 color)
{

    Light light = getDirectionalLight(params, frameUniforms);
    
    DirectionalLightShadowmap _dirShadowmap = frameUniforms.directionalLight.directionalLightShadowmap;

    float visibility = 1.0f;

    if(frameUniforms.directionalLight.useShadowmap)
    {
        Texture2D<float> shadowMap = ResourceDescriptorHeap[_dirShadowmap.shadowmap_srv_index];
        visibility = shadowSample_PCF(
            shadowMap, 
            samplerStruct.sdSampler, 
            _dirShadowmap.light_view_matrix, 
            _dirShadowmap.light_proj_view, 
            _dirShadowmap.shadow_bounds, 
            _dirShadowmap.shadowmap_width, 
            _dirShadowmap.cascadeCount, 
            params.shading_position, 
            light.NoL);
    }

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
float4 evaluateLights(const FrameUniforms frameUniforms, const PerRenderableMeshData perMeshData, const CommonShadingStruct commonShadingStruct, const MaterialInputs materialInputs, const SamplerStruct samplerStruct)
{
    PixelParams pixel;
    getPixelParams(commonShadingStruct, materialInputs, pixel);

    // Ideally we would keep the diffuse and specular components separate
    // until the very end but it costs more ALUs on mobile. The gains are
    // currently not worth the extra operations
    float3 color = float3(0.0, 0.0, 0.0);

    //// We always evaluate the IBL as not having one is going to be uncommon,
    //// it also saves 1 shader variant
    //evaluateIBL(materialInputs, pixel, color);

    evaluateDirectionalLight(frameUniforms, perMeshData, commonShadingStruct, materialInputs, pixel, samplerStruct, color);

    //evaluatePunctualLights(material, pixel, color);

    // // In fade mode we un-premultiply baseColor early on, so we need to
    // // premultiply again at the end (affects diffuse and specular lighting)
    // color *= materialInputs.baseColor.a;

    // return color;

    return float4(color.rgb, materialInputs.baseColor.a);
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
float4 evaluateMaterial(const FrameUniforms frameUniforms, const PerRenderableMeshData perMeshData, 
    const CommonShadingStruct commonShadingStruct, const MaterialInputs materialInputs, const SamplerStruct samplerStruct)
{
    float4 color = evaluateLights(frameUniforms, perMeshData, commonShadingStruct, materialInputs, samplerStruct);
    addEmissive(frameUniforms, materialInputs, color);
    return color;
}














