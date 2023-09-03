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
    const float4x4 light_proj_view, uint2 shadowmap_size, float2 uv_offset, const float uv_scale,
    const float3 positionWS, const float depthBias)
{
    float4 fragPosLightSpace = mul(light_proj_view, float4(positionWS, 1.0f));

    // perform perspective divide
    float3 shadowPosition = fragPosLightSpace.xyz / fragPosLightSpace.w;

    if(max(abs(shadowPosition.x), abs(shadowPosition.y)) > 1)
        return 0.0f;

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
    sum += uw.x * vw.x * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.x, v.x)) * uv_scale + uv_offset, depth);
    sum += uw.y * vw.x * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.y, v.x)) * uv_scale + uv_offset, depth);
    sum += uw.x * vw.y * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.x, v.y)) * uv_scale + uv_offset, depth);
    sum += uw.y * vw.y * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.y, v.y)) * uv_scale + uv_offset, depth);
    return sum * (1.0 / 16.0);
}

// use hardware assisted PCF + 3x3 gaussian filter
float shadowSample_PCF(
    const Texture2D<float> shadowmap, SamplerComparisonState shadowmapSampler, 
    const float4x4 light_view_matrix, const float4x4 light_proj_view[4], 
    const float4 shadow_bounds, uint2 shadowmap_size, uint cascadeCount, const float3 positionWS, const float ndotl)
{
    uint cascadeLevel;
    float2 shadow_bound_offset;
    getShadowCascade(light_view_matrix, positionWS, shadow_bounds, cascadeCount, cascadeLevel, shadow_bound_offset);

    if(cascadeLevel == 4)
        return 1.0f;
    
    float shadow_bias = max(0.0015f * exp2(cascadeLevel) * (1.0 - ndotl), 0.0003f);  
    float uv_scale = 1.0f / log2(cascadeCount);

    return shadowSample_PCF_Low(shadowmap, shadowmapSampler, 
        light_proj_view[cascadeLevel], shadowmap_size, shadow_bound_offset, uv_scale, positionWS, shadow_bias);
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
//------------------------------------------------------------------------------
// Directional lights evaluation
//------------------------------------------------------------------------------

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

    if(frameUniforms.directionalLight.shadowType != 0)
    {
        Texture2D<float> shadowMap = ResourceDescriptorHeap[_dirShadowmap.shadowmap_srv_index];
        visibility = shadowSample_PCF(
            shadowMap, 
            samplerStruct.sdSampler, 
            _dirShadowmap.light_view_matrix, 
            _dirShadowmap.light_proj_view, 
            _dirShadowmap.shadow_bounds, 
            _dirShadowmap.shadowmap_size, 
            _dirShadowmap.cascadeCount, 
            params.shading_position, 
            light.NoL);
    }

    color.rgb += surfaceShading(params, pixel, light, visibility);
}

//------------------------------------------------------------------------------
// Punctual lights evaluation
//------------------------------------------------------------------------------

float getSquareFalloffAttenuation(float distanceSquare, float falloff)
{
    float factor = distanceSquare * falloff;
    float smoothFactor = saturate(1.0 - factor * factor);
    // We would normally divide by the square distance here
    // but we do it at the call site
    return smoothFactor * smoothFactor;
}

float getDistanceAttenuation(const CommonShadingStruct params, const FrameUniforms frameUniforms, const float3 posToLight, float falloff)
{
    float distanceSquare = dot(posToLight, posToLight);
    float attenuation = getSquareFalloffAttenuation(distanceSquare, falloff);
    // light far attenuation
    // float3 v = getWorldPosition(params) - getWorldCameraPosition(frameUniforms);
    float3 v = params.shading_position - frameUniforms.cameraUniform.cameraPosition;
    float d = dot(v, v);
    attenuation *= saturate(frameUniforms.directionalLight.lightFarAttenuationParams.x - d * frameUniforms.directionalLight.lightFarAttenuationParams.y);
    // Assume a punctual light occupies a volume of 1cm to avoid a division by 0
    return attenuation / max(distanceSquare, 1e-4);
}

float getAngleAttenuation(const float3 lightDir, const float3 l, const float2 inoutRadians) {
    float cd = dot(lightDir, l);
    float acd = acos(cd);
    float inRadians = inoutRadians.x * 0.5f;
    float outRadians = inoutRadians.y * 0.5f;
    float attenuation = saturate((acd - outRadians)/(inRadians - outRadians));
    return attenuation * attenuation;
}

/**
 * Returns a Light structure (see common_lighting.fs) describing a point or spot light.
 * The colorIntensity field will store the *pre-exposed* intensity of the light
 * in the w component.
 *
 * The light parameters used to compute the Light structure are fetched from the
 * lightsUniforms uniform buffer.
 */

Light getPointLight(const CommonShadingStruct params, const FrameUniforms frameUniforms, const PointLightStruct pointLightStruct)
{
    float3 colorIES = pointLightStruct.lightIntensity.rgb;
    float intensity = pointLightStruct.lightIntensity.a;
    float3 positionFalloff = pointLightStruct.lightPosition;
    float falloff = pointLightStruct.falloff;

    // poition-to-light vector
    float3 worldPosition = params.shading_position;
    float3 posToLight = positionFalloff.xyz - worldPosition;

    // and populate the Light structure
    Light light;
    light.colorIntensity.rgb = colorIES.rgb;
    light.colorIntensity.w = intensity;
    light.l = normalize(posToLight);
    light.attenuation = getDistanceAttenuation(params, frameUniforms, posToLight, falloff);
    light.NoL = saturate(dot(params.shading_normal, light.l));
    light.worldPosition = positionFalloff.xyz;
    light.contactShadows = 0;
    light.type = (pointLightStruct.shadowType & 0x1u);
    light.shadowIndex = 0xFFFFFFFF;
    light.castsShadows = 0;

    return light;
}

Light getSpotLight(const CommonShadingStruct params, const FrameUniforms frameUniforms, const SpotLightStruct spotLightStruct)
{
    float3 colorIES = spotLightStruct.lightIntensity.rgb;
    float intensity = spotLightStruct.lightIntensity.a;
    float3 positionFalloff = spotLightStruct.lightPosition;
    float3 direction = spotLightStruct.lightDirection;
    float2 inoutRadians = max(float2(spotLightStruct.inner_radians, spotLightStruct.outer_radians), float2(0.001f, 0.001f));
    float falloff = spotLightStruct.falloff;

    // poition-to-light vector
    float3 worldPosition = params.shading_position;
    float3 posToLight = positionFalloff.xyz - worldPosition;

    // and populate the Light structure
    Light light;
    light.colorIntensity.rgb = colorIES.rgb;
    light.colorIntensity.w = intensity;
    light.l = normalize(posToLight);
    light.attenuation = getDistanceAttenuation(params, frameUniforms, posToLight, falloff);
    light.NoL = saturate(dot(params.shading_normal, light.l));
    light.worldPosition = positionFalloff.xyz;
    light.contactShadows = 0;
    light.type = (spotLightStruct.shadowType & 0x1u);
    light.shadowIndex = spotLightStruct.spotLightShadowmap.shadowmap_srv_index;
    light.castsShadows = 0;

    light.direction = direction;
    light.attenuation *= getAngleAttenuation(direction, -light.l, inoutRadians);

    return light;
}

/**
 * Evaluates all punctual lights that my affect the current fragment.
 * The result of the lighting computations is accumulated in the color
 * parameter, as linear HDR RGB.
 */
void evaluatePointLights(
    const FrameUniforms frameUniforms, 
    const PerRenderableMeshData perMeshData, 
    const CommonShadingStruct params,
    const MaterialInputs material, 
    const PixelParams pixel,
    const SamplerStruct samplerStruct, 
    inout float3 color)
{
    PointLightUniform pointLightUniform = frameUniforms.pointLightUniform;

    uint index = 0;
    uint end = pointLightUniform.pointLightCounts;

    for ( ; index < end; index++)
    {
        Light light = getPointLight(params, frameUniforms, pointLightUniform.pointLightStructs[index]);

        float visibility = 1.0f;
        /*
        if(frameUniforms.directionalLight.shadowType != 0)
        {
            DirectionalLightShadowmap _dirShadowmap = frameUniforms.directionalLight.directionalLightShadowmap;
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
        */
        color.rgb += surfaceShading(params, pixel, light, visibility);
    }
}

void evaluateSpotLights(
    const FrameUniforms frameUniforms, 
    const PerRenderableMeshData perMeshData, 
    const CommonShadingStruct params,
    const MaterialInputs material, 
    const PixelParams pixel,
    const SamplerStruct samplerStruct, 
    inout float3 color)
{
    SpotLightUniform spotLightUniform = frameUniforms.spotLightUniform;

    uint index = 0;
    uint end = spotLightUniform.spotLightCounts;

    for ( ; index < end; index++)
    {
        SpotLightStruct _spotLightStr = spotLightUniform.spotLightStructs[index];
        Light light = getSpotLight(params, frameUniforms, _spotLightStr);
        float visibility = 1.0f;

        if(frameUniforms.directionalLight.shadowType != 0)
        {
            SpotLightShadowmap _spotLightShadowmap = _spotLightStr.spotLightShadowmap;
            Texture2D<float> shadowMap = ResourceDescriptorHeap[_spotLightShadowmap.shadowmap_srv_index];

            float shadow_bias = max(0.005f * (1.0 - light.NoL), 0.005f);  

            visibility = shadowSample_PCF_Low(
                shadowMap, 
                samplerStruct.sdSampler, 
                _spotLightShadowmap.light_proj_view, 
                _spotLightShadowmap.shadowmap_size, 
                float2(0, 0), 
                1.0f, 
                params.shading_position, 
                shadow_bias);
        }

        color.rgb += surfaceShading(params, pixel, light, visibility);
    }
}

void evaluatePunctualLights(
    const FrameUniforms frameUniforms, 
    const PerRenderableMeshData perMeshData, 
    const CommonShadingStruct params,
    const MaterialInputs material, 
    const PixelParams pixel,
    const SamplerStruct samplerStruct, 
    inout float3 color)
{
    evaluatePointLights(frameUniforms, perMeshData, params, material, pixel, samplerStruct, color);
    evaluateSpotLights(frameUniforms, perMeshData, params, material, pixel, samplerStruct, color);
}


//------------------------------------------------------------------------------
// Image Based Lighting
//------------------------------------------------------------------------------

float3 decodeDataForIBL(const float4 data)
{
    return data.rgb;
}

float3 Irradiance_SphericalHarmonics(const FrameUniforms frameUniforms, const float3 n)
{
    return max(
          frameUniforms.iblUniform.iblSH[0]
        + frameUniforms.iblUniform.iblSH[1] * (n.y)
        + frameUniforms.iblUniform.iblSH[2] * (n.z)
        + frameUniforms.iblUniform.iblSH[3] * (n.x)
        + frameUniforms.iblUniform.iblSH[4] * (n.y * n.x)
        + frameUniforms.iblUniform.iblSH[5] * (n.y * n.z)
        + frameUniforms.iblUniform.iblSH[6] * (3.0 * n.z * n.z - 1.0)
        + frameUniforms.iblUniform.iblSH[7] * (n.z * n.x)
        + frameUniforms.iblUniform.iblSH[8] * (n.x * n.x - n.y * n.y)
        , 0.0);
}

float3 Irradiance_RoughnessOne(const FrameUniforms frameUniforms, const float3 n) {
    // note: lod used is always integer, hopefully the hardware skips tri-linear filtering
    return decodeDataForIBL(textureLod(light_iblSpecular, n, frameUniforms.iblUniform.iblRoughnessOneLevel));
}


//------------------------------------------------------------------------------
// IBL irradiance dispatch
//------------------------------------------------------------------------------


float3 diffuseIrradiance(const FrameUniforms frameUniforms, const float3 n) {
    if (frameUniforms.iblUniform.iblSH[0].x == 65504.0) {
        uint2 s = textureSize(light_iblSpecular, int(frameUniforms.iblUniform.iblRoughnessOneLevel));
        float du = 1.0 / float(s.x);
        float dv = 1.0 / float(s.y);
        float3 m0 = normalize(cross(n, float3(0.0, 1.0, 0.0)));
        float3 m1 = cross(m0, n);
        float3 m0du = m0 * du;
        float3 m1dv = m1 * dv;
        float3 c;
        c  = Irradiance_RoughnessOne(frameUniforms, n - m0du - m1dv);
        c += Irradiance_RoughnessOne(frameUniforms, n + m0du - m1dv);
        c += Irradiance_RoughnessOne(frameUniforms, n + m0du + m1dv);
        c += Irradiance_RoughnessOne(frameUniforms, n - m0du + m1dv);
        return c * 0.25;
    } else {
        return Irradiance_SphericalHarmonics(frameUniforms, n);
    }
}


//------------------------------------------------------------------------------
// IBL specular
//------------------------------------------------------------------------------

float perceptualRoughnessToLod(const FrameUniforms frameUniforms, float perceptualRoughness)
{
    // The mapping below is a quadratic fit for log2(perceptualRoughness)+iblRoughnessOneLevel when
    // iblRoughnessOneLevel is 4. We found empirically that this mapping works very well for
    // a 256 cubemap with 5 levels used. But also scales well for other iblRoughnessOneLevel values.
    return frameUniforms.iblUniform.iblRoughnessOneLevel * perceptualRoughness * (2.0 - perceptualRoughness);
}

float3 prefilteredRadiance(const FrameUniforms frameUniforms, const float3 r, float perceptualRoughness) {
    float lod = perceptualRoughnessToLod(frameUniforms, perceptualRoughness);
    return decodeDataForIBL(textureLod(light_iblSpecular, r, lod));
}

float3 prefilteredRadiance(const FrameUniforms frameUniforms, const float3 r, float roughness, float offset) {
    float lod = frameUniforms.iblUniform.iblRoughnessOneLevel * roughness;
    return decodeDataForIBL(textureLod(light_iblSpecular, r, lod + offset));
}

float3 getSpecularDominantDirection(const float3 n, const float3 r, float roughness)
{
    return lerp(r, n, roughness * roughness);
}

float3 specularDFG(const PixelParams pixel)
{
    return lerp(pixel.dfg.xxx, pixel.dfg.yyy, pixel.f0);
}

float3 getReflectedVector(const CommonShadingStruct params, const PixelParams pixel)
{
    return getSpecularDominantDirection(params.shading_normal, params.shading_reflected, pixel.roughness);
}


void evaluateIBL(
    const FrameUniforms frameUniforms, 
    const PerRenderableMeshData perMeshData, 
    const CommonShadingStruct params,
    const MaterialInputs material, 
    const PixelParams pixel,
    const SamplerStruct samplerStruct, 
    inout float3 color)
{
    // specular layer
    float3 Fr = float3(0.0f, 0.0f, 0.0f);

    float3 E = specularDFG(pixel);
    float3 r = getReflectedVector(params, pixel);
    Fr = E * prefilteredRadiance(frameUniforms, r, pixel.perceptualRoughness);

    // diffuse layer

    float3 diffuseNormal = params.shading_normal;
    float3 diffuseIrradiance = diffuseIrradiance(frameUniforms, diffuseNormal);

    float diffuseBRDF = 1.0f;
    float3 Fd = pixel.diffuseColor * diffuseIrradiance * (1.0 - E) * diffuseBRDF;

    // Combine all terms
    // Note: iblLuminance is already premultiplied by the exposure

    color.rgb += Fr + Fd;
    
}

/*
vec3 irradianceSH(vec3 n) {
    // uniform vec3 sphericalHarmonics[9]
    // We can use only the first 2 bands for better performance
    return
          sphericalHarmonics[0]
        + sphericalHarmonics[1] * (n.y)
        + sphericalHarmonics[2] * (n.z)
        + sphericalHarmonics[3] * (n.x)
        + sphericalHarmonics[4] * (n.y * n.x)
        + sphericalHarmonics[5] * (n.y * n.z)
        + sphericalHarmonics[6] * (3.0 * n.z * n.z - 1.0)
        + sphericalHarmonics[7] * (n.z * n.x)
        + sphericalHarmonics[8] * (n.x * n.x - n.y * n.y);
}

// NOTE: this is the DFG LUT implementation of the function above
vec2 prefilteredDFG_LUT(float coord, float NoV) {
    // coord = sqrt(roughness), which is the mapping used by the
    // IBL prefiltering code when computing the mipmaps
    return textureLod(dfgLut, vec2(NoV, coord), 0.0).rg;
}

vec3 evaluateSpecularIBL(vec3 r, float perceptualRoughness) {
    // This assumes a 256x256 cubemap, with 9 mip levels
    float lod = 8.0 * perceptualRoughness;
    // decodeEnvironmentMap() either decodes RGBM or is a no-op if the
    // cubemap is stored in a float texture
    return decodeEnvironmentMap(textureCubeLodEXT(environmentMap, r, lod));
}

vec3 evaluateIBL(vec3 n, vec3 v, vec3 diffuseColor, vec3 f0, vec3 f90, float perceptualRoughness) {
    float NoV = max(dot(n, v), 0.0);
    vec3 r = reflect(-v, n);

    // Specular indirect
    vec3 indirectSpecular = evaluateSpecularIBL(r, perceptualRoughness);
    vec2 env = prefilteredDFG_LUT(perceptualRoughness, NoV);
    vec3 specularColor = f0 * env.x + f90 * env.y;

    // Diffuse indirect
    // We multiply by the Lambertian BRDF to compute radiance from irradiance
    // With the Disney BRDF we would have to remove the Fresnel term that
    // depends on NoL (it would be rolled into the SH). The Lambertian BRDF
    // can be baked directly in the SH to save a multiplication here
    vec3 indirectDiffuse = max(irradianceSH(n), 0.0) * Fd_Lambert();

    // Indirect contribution
    return diffuseColor * indirectDiffuse + indirectSpecular * specularColor;
}
*/

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

    // We always evaluate the IBL as not having one is going to be uncommon,
    // it also saves 1 shader variant
    evaluateIBL(frameUniforms, perMeshData, commonShadingStruct, materialInputs, pixel, samplerStruct, color);

    evaluateDirectionalLight(frameUniforms, perMeshData, commonShadingStruct, materialInputs, pixel, samplerStruct, color);

    evaluatePunctualLights(frameUniforms, perMeshData, commonShadingStruct, materialInputs, pixel, samplerStruct, color);

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














