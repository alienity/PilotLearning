//------------------------------------------------------------------------------
// Image based lighting configuration
//------------------------------------------------------------------------------

// Number of spherical harmonics bands (1, 2 or 3)
#define SPHERICAL_HARMONICS_BANDS           3

// IBL integration algorithm
#define IBL_INTEGRATION_PREFILTERED_CUBEMAP         0
#define IBL_INTEGRATION_IMPORTANCE_SAMPLING         1

#define IBL_INTEGRATION                             IBL_INTEGRATION_PREFILTERED_CUBEMAP

#define IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT   64

//------------------------------------------------------------------------------
// IBL utilities
//------------------------------------------------------------------------------

float3 decodeDataForIBL(const float4 data) {
    return data.rgb;
}

//------------------------------------------------------------------------------
// IBL prefiltered DFG term implementations
//------------------------------------------------------------------------------

float3 PrefilteredDFG_LUT(Texture2D<float3> light_iblDFG, SamplerState light_iblDFGSampler, float lod, float NoV)
{
    // coord = sqrt(linear_roughness), which is the mapping used by cmgen.
    //return textureLod(light_iblDFG, float2(NoV, lod), 0.0).rgb;
    return light_iblDFG.SampleLevel(light_iblDFGSampler, float2(NoV, lod), 0.0).rgb;
}

//------------------------------------------------------------------------------
// IBL environment BRDF dispatch
//------------------------------------------------------------------------------

float3 prefilteredDFG(Texture2D<float3> light_iblDFG, SamplerState light_iblDFGSampler, float perceptualRoughness, float NoV) {
    // PrefilteredDFG_LUT() takes a LOD, which is sqrt(roughness) = perceptualRoughness
    return PrefilteredDFG_LUT(light_iblDFG, light_iblDFGSampler, perceptualRoughness, NoV);
}

//------------------------------------------------------------------------------
// IBL irradiance implementations
//------------------------------------------------------------------------------

float3 Irradiance_SphericalHarmonics(const FrameUniforms frameUniforms, const float3 n) {
    return max(
          frameUniforms.iblSH[0]
#if SPHERICAL_HARMONICS_BANDS >= 2
        + frameUniforms.iblSH[1] * (n.y)
        + frameUniforms.iblSH[2] * (n.z)
        + frameUniforms.iblSH[3] * (n.x)
#endif
#if SPHERICAL_HARMONICS_BANDS >= 3
        + frameUniforms.iblSH[4] * (n.y * n.x)
        + frameUniforms.iblSH[5] * (n.y * n.z)
        + frameUniforms.iblSH[6] * (3.0 * n.z * n.z - 1.0)
        + frameUniforms.iblSH[7] * (n.z * n.x)
        + frameUniforms.iblSH[8] * (n.x * n.x - n.y * n.y)
#endif
        , 0.0);
}

float3 Irradiance_RoughnessOne(Texture2D<float4> light_iblSpecular, SamplerState light_iblSpecularSampler, const float3 n)
{
    // note: lod used is always integer, hopefully the hardware skips tri-linear filtering
    //return decodeDataForIBL(textureLod(light_iblSpecular, n, frameUniforms.iblRoughnessOneLevel));
    return decodeDataForIBL(light_iblSpecular.SampleLevel(light_iblSpecularSampler, n, frameUniforms.iblRoughnessOneLevel));
}

//------------------------------------------------------------------------------
// IBL irradiance dispatch
//------------------------------------------------------------------------------

float3 diffuseIrradiance(Texture2D<float4> light_iblSpecular, SamplerState light_iblSpecularSampler, const FrameUniforms frameUniforms, const float3 n) {
    if (frameUniforms.iblSH[0].x == 65504.0) {
#if FILAMENT_QUALITY < FILAMENT_QUALITY_HIGH
        return Irradiance_RoughnessOne(light_iblSpecular, light_iblSpecularSampler, n);
#else
        //float2 s = textureSize(light_iblSpecular, int(frameUniforms.iblRoughnessOneLevel));
        float3 s;
        light_iblSpecular.GetDimensions(int(frameUniforms.iblRoughnessOneLevel), s.x, s.y, s.z);
        float du = 1.0 / float(s.x);
        float dv = 1.0 / float(s.y);
        float3 m0 = normalize(cross(n, float3(0.0, 1.0, 0.0)));
        float3 m1 = cross(m0, n);
        float3 m0du = m0 * du;
        float3 m1dv = m1 * dv;
        float3 c;
        c  = Irradiance_RoughnessOne(light_iblSpecular, light_iblSpecularSampler, n - m0du - m1dv);
        c += Irradiance_RoughnessOne(light_iblSpecular, light_iblSpecularSampler, n + m0du - m1dv);
        c += Irradiance_RoughnessOne(light_iblSpecular, light_iblSpecularSampler, n + m0du + m1dv);
        c += Irradiance_RoughnessOne(light_iblSpecular, light_iblSpecularSampler, n - m0du + m1dv);
        return c * 0.25;
#endif
    } else {
        return Irradiance_SphericalHarmonics(frameUniforms, n);
    }
}

//------------------------------------------------------------------------------
// IBL specular
//------------------------------------------------------------------------------

float perceptualRoughnessToLod(float perceptualRoughness) {
    // The mapping below is a quadratic fit for log2(perceptualRoughness)+iblRoughnessOneLevel when
    // iblRoughnessOneLevel is 4. We found empirically that this mapping works very well for
    // a 256 cubemap with 5 levels used. But also scales well for other iblRoughnessOneLevel values.
    return frameUniforms.iblRoughnessOneLevel * perceptualRoughness * (2.0 - perceptualRoughness);
}

float3 prefilteredRadiance(const float3 r, float perceptualRoughness) {
    float lod = perceptualRoughnessToLod(perceptualRoughness);
    return decodeDataForIBL(textureLod(light_iblSpecular, r, lod));
}

float3 prefilteredRadiance(const float3 r, float roughness, float offset) {
    float lod = frameUniforms.iblRoughnessOneLevel * roughness;
    return decodeDataForIBL(textureLod(light_iblSpecular, r, lod + offset));
}

float3 getSpecularDominantDirection(const float3 n, const float3 r, float roughness) {
    return mix(r, n, roughness * roughness);
}

float3 specularDFG(const PixelParams pixel) {
#if defined(SHADING_MODEL_CLOTH)
    return pixel.f0 * pixel.dfg.z;
#else
    return mix(pixel.dfg.xxx, pixel.dfg.yyy, pixel.f0);
#endif
}

float3 getReflectedVector(const PixelParams pixel, const float3 n) {
#if defined(MATERIAL_HAS_ANISOTROPY)
    float3 r = getReflectedVector(pixel, shading_view, n);
#else
    float3 r = shading_reflected;
#endif
    return getSpecularDominantDirection(n, r, pixel.roughness);
}

//------------------------------------------------------------------------------
// Prefiltered importance sampling
//------------------------------------------------------------------------------

#if IBL_INTEGRATION == IBL_INTEGRATION_IMPORTANCE_SAMPLING
float2 hammersley(uint index) {
    const uint numSamples = uint(IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT);
    const float invNumSamples = 1.0 / float(numSamples);
    const float tof = 0.5 / float(0x80000000U);
    uint bits = index;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float2(float(index) * invNumSamples, float(bits) * tof);
}

float3 importanceSamplingNdfDggx(float2 u, float roughness) {
    // Importance sampling D_GGX
    float a2 = roughness * roughness;
    float phi = 2.0 * F_PI * u.x;
    float cosTheta2 = (1.0 - u.y) / (1.0 + (a2 - 1.0) * u.y);
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.0 - cosTheta2);
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float3 hemisphereCosSample(float2 u) {
    float phi = 2.0f * F_PI * u.x;
    float cosTheta2 = 1.0 - u.y;
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.0 - cosTheta2);
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float3 importanceSamplingVNdfDggx(float2 u, float roughness, float3 v) {
    // See: "A Simpler and Exact Sampling Routine for the GGX Distribution of Visible Normals", Eric Heitz
    float alpha = roughness;

    // stretch view
    v = normalize(float3(alpha * v.x, alpha * v.y, v.z));

    // orthonormal basis
    float3 up = select(abs(v.z) < 0.9999, float3(0.0, 0.0, 1.0), float3(1.0, 0.0, 0.0));
    float3 t = normalize(cross(up, v));
    float3 b = cross(t, v);

    // sample point with polar coordinates (r, phi)
    float a = 1.0 / (1.0 + v.z);
    float r = sqrt(u.x);
    float phi = select((u.y < a), u.y / a * F_PI, F_PI + (u.y - a) / (1.0 - a) * F_PI);
    float p1 = r * cos(phi);
    float p2 = r * sin(phi) * select((u.y < a), 1.0, v.z);

    // compute normal
    float3 h = p1 * t + p2 * b + sqrt(max(0.0, 1.0 - p1*p1 - p2*p2)) * v;

    // unstretch
    h = normalize(float3(alpha * h.x, alpha * h.y, max(0.0, h.z)));
    return h;
}

float prefilteredImportanceSampling(float ipdf, float omegaP) {
    // See: "Real-time Shading with Filtered Importance Sampling", Jaroslav Krivanek
    // Prefiltering doesn't work with anisotropy
    const float numSamples = float(IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT);
    const float invNumSamples = 1.0 / float(numSamples);
    const float K = 4.0;
    float omegaS = invNumSamples * ipdf;
    float mipLevel = log2(K * omegaS / omegaP) * 0.5;    // log4
    return mipLevel;
}

float3 isEvaluateSpecularIBL(const PixelParams pixel, const float3 n, const float3 v, const float NoV) {
    const uint numSamples = uint(IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT);
    const float invNumSamples = 1.0 / float(numSamples);
    const float3 up = float3(0.0, 0.0, 1.0);

    // TODO: for a true anisotropic BRDF, we need a real tangent space
    // tangent space
    mat3 T;
    T[0] = normalize(cross(up, n));
    T[1] = cross(n, T[0]);
    T[2] = n;

    // Random rotation around N per pixel
    const float3 m = float3(0.06711056, 0.00583715, 52.9829189);
    float a = 2.0 * F_PI * fract(m.z * fract(dot(gl_FragCoord.xy, m.xy)));
    float c = cos(a);
    float s = sin(a);
    mat3 R;
    R[0] = float3( c, s, 0);
    R[1] = float3(-s, c, 0);
    R[2] = float3( 0, 0, 1);
    T *= R;

    float roughness = pixel.roughness;
    float dim = float(textureSize(light_iblSpecular, 0).x);
    float omegaP = (4.0 * F_PI) / (6.0 * dim * dim);

    float3 indirectSpecular = float3(0.0);
    for (uint i = 0u; i < numSamples; i++) {
        float2 u = hammersley(i);
        float3 h = T * importanceSamplingNdfDggx(u, roughness);

        // Since anisotropy doesn't work with prefiltering, we use the same "faux" anisotropy
        // we do when we use the prefiltered cubemap
        float3 l = getReflectedVector(pixel, v, h);

        // Compute this sample's contribution to the brdf
        float NoL = saturate(dot(n, l));
        if (NoL > 0.0) {
            float NoH = dot(n, h);
            float LoH = saturate(dot(l, h));

            // PDF inverse (we must use D_GGX() here, which is used to generate samples)
            float ipdf = (4.0 * LoH) / (D_GGX(roughness, NoH, h) * NoH);
            float mipLevel = prefilteredImportanceSampling(ipdf, omegaP);
            float3 L = decodeDataForIBL(textureLod(light_iblSpecular, l, mipLevel));

            float D = distribution(roughness, NoH, h);
            float V = visibility(roughness, NoV, NoL);
            float3 F = fresnel(pixel.f0, LoH);
            float3 Fr = F * (D * V * NoL * ipdf * invNumSamples);

            indirectSpecular += (Fr * L);
        }
    }

    return indirectSpecular;
}

float3 isEvaluateDiffuseIBL(const PixelParams pixel, float3 n, float3 v) {
    const uint numSamples = uint(IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT);
    const float invNumSamples = 1.0 / float(numSamples);
    const float3 up = float3(0.0, 0.0, 1.0);

    // TODO: for a true anisotropic BRDF, we need a real tangent space
    // tangent space
    mat3 T;
    T[0] = normalize(cross(up, n));
    T[1] = cross(n, T[0]);
    T[2] = n;

    // Random rotation around N per pixel
    const float3 m = float3(0.06711056, 0.00583715, 52.9829189);
    float a = 2.0 * F_PI * fract(m.z * fract(dot(gl_FragCoord.xy, m.xy)));
    float c = cos(a);
    float s = sin(a);
    mat3 R;
    R[0] = float3( c, s, 0);
    R[1] = float3(-s, c, 0);
    R[2] = float3( 0, 0, 1);
    T *= R;

    float dim = float(textureSize(light_iblSpecular, 0).x);
    float omegaP = (4.0 * F_PI) / (6.0 * dim * dim);

    float3 indirectDiffuse = float3(0.0);
    for (uint i = 0u; i < numSamples; i++) {
        float2 u = hammersley(i);
        float3 h = T * hemisphereCosSample(u);

        // Since anisotropy doesn't work with prefiltering, we use the same "faux" anisotropy
        // we do when we use the prefiltered cubemap
        float3 l = getReflectedVector(pixel, v, h);

        // Compute this sample's contribution to the brdf
        float NoL = saturate(dot(n, l));
        if (NoL > 0.0) {
            // PDF inverse (we must use D_GGX() here, which is used to generate samples)
            float ipdf = F_PI / NoL;
            // we have to bias the mipLevel (+1) to help with very strong highlights
            float mipLevel = prefilteredImportanceSampling(ipdf, omegaP) + 1.0;
            float3 L = decodeDataForIBL(textureLod(light_iblSpecular, l, mipLevel));
            indirectDiffuse += L;
        }
    }

    return indirectDiffuse * invNumSamples; // we bake 1/F_PI here, which cancels out
}

void isEvaluateClearCoatIBL(const PixelParams pixel, float specularAO, inout float3 Fd, inout float3 Fr) {
#if defined(MATERIAL_HAS_CLEAR_COAT)
#if defined(MATERIAL_HAS_NORMAL) || defined(MATERIAL_HAS_CLEAR_COAT_NORMAL)
    // We want to use the geometric normal for the clear coat layer
    float clearCoatNoV = clampNoV(dot(shading_clearCoatNormal, shading_view));
    float3 clearCoatNormal = shading_clearCoatNormal;
#else
    float clearCoatNoV = shading_NoV;
    float3 clearCoatNormal = shading_normal;
#endif
    // The clear coat layer assumes an IOR of 1.5 (4% reflectance)
    float Fc = F_Schlick(0.04, 1.0, clearCoatNoV) * pixel.clearCoat;
    float attenuation = 1.0 - Fc;
    Fd *= attenuation;
    Fr *= attenuation;

    PixelParams p;
    p.perceptualRoughness = pixel.clearCoatPerceptualRoughness;
    p.f0 = float3(0.04);
    p.roughness = perceptualRoughnessToRoughness(p.perceptualRoughness);
#if defined(MATERIAL_HAS_ANISOTROPY)
    p.anisotropy = 0.0;
#endif

    float3 clearCoatLobe = isEvaluateSpecularIBL(p, clearCoatNormal, shading_view, clearCoatNoV);
    Fr += clearCoatLobe * (specularAO * pixel.clearCoat);
#endif
}
#endif

//------------------------------------------------------------------------------
// IBL evaluation
//------------------------------------------------------------------------------

void evaluateClothIndirectDiffuseBRDF(const PixelParams pixel, inout float diffuse) {
#if defined(SHADING_MODEL_CLOTH)
#if defined(MATERIAL_HAS_SUBSURFACE_COLOR)
    // Simulate subsurface scattering with a wrap diffuse term
    diffuse *= Fd_Wrap(shading_NoV, 0.5);
#endif
#endif
}

void evaluateSheenIBL(const PixelParams pixel, float diffuseAO,
        const in SSAOInterpolationCache cache, inout float3 Fd, inout float3 Fr) {
#if !defined(SHADING_MODEL_CLOTH) && !defined(SHADING_MODEL_SUBSURFACE)
#if defined(MATERIAL_HAS_SHEEN_COLOR)
    // Albedo scaling of the base layer before we layer sheen on top
    Fd *= pixel.sheenScaling;
    Fr *= pixel.sheenScaling;

    float3 reflectance = pixel.sheenDFG * pixel.sheenColor;
    reflectance *= specularAO(shading_NoV, diffuseAO, pixel.sheenRoughness, cache);

    Fr += reflectance * prefilteredRadiance(shading_reflected, pixel.sheenPerceptualRoughness);
#endif
#endif
}

void evaluateClearCoatIBL(const PixelParams pixel, float diffuseAO,
        const in SSAOInterpolationCache cache, inout float3 Fd, inout float3 Fr) {
#if IBL_INTEGRATION == IBL_INTEGRATION_IMPORTANCE_SAMPLING
    float specularAO = specularAO(shading_NoV, diffuseAO, pixel.clearCoatRoughness, cache);
    isEvaluateClearCoatIBL(pixel, specularAO, Fd, Fr);
    return;
#endif

#if defined(MATERIAL_HAS_CLEAR_COAT)
    if (pixel.clearCoat > 0.0) {
#if defined(MATERIAL_HAS_NORMAL) || defined(MATERIAL_HAS_CLEAR_COAT_NORMAL)
        // We want to use the geometric normal for the clear coat layer
        float clearCoatNoV = clampNoV(dot(shading_clearCoatNormal, shading_view));
        float3 clearCoatR = reflect(-shading_view, shading_clearCoatNormal);
#else
        float clearCoatNoV = shading_NoV;
        float3 clearCoatR = shading_reflected;
#endif
        // The clear coat layer assumes an IOR of 1.5 (4% reflectance)
        float Fc = F_Schlick(0.04, 1.0, clearCoatNoV) * pixel.clearCoat;
        float attenuation = 1.0 - Fc;
        Fd *= attenuation;
        Fr *= attenuation;

        // TODO: Should we apply specularAO to the attenuation as well?
        float specularAO = specularAO(clearCoatNoV, diffuseAO, pixel.clearCoatRoughness, cache);
        Fr += prefilteredRadiance(clearCoatR, pixel.clearCoatPerceptualRoughness) * (specularAO * Fc);
    }
#endif
}

void evaluateSubsurfaceIBL(const PixelParams pixel, const float3 diffuseIrradiance, inout float3 Fd, inout float3 Fr) {
#if defined(SHADING_MODEL_SUBSURFACE)
    float3 viewIndependent = diffuseIrradiance;
    float3 viewDependent = prefilteredRadiance(-shading_view, pixel.roughness, 1.0 + pixel.thickness);
    float attenuation = (1.0 - pixel.thickness) / (2.0 * F_PI);
    Fd += pixel.subsurfaceColor * (viewIndependent + viewDependent) * attenuation;
#elif defined(SHADING_MODEL_CLOTH) && defined(MATERIAL_HAS_SUBSURFACE_COLOR)
    Fd *= saturate(pixel.subsurfaceColor + shading_NoV);
#endif
}

#if defined(MATERIAL_HAS_REFRACTION)

struct Refraction {
    float3 position;
    float3 direction;
    float d;
};

void refractionSolidSphere(const PixelParams pixel, const float3 n, float3 r, out Refraction ray) {
    r = refract(r, n, pixel.etaIR);
    float NoR = dot(n, r);
    float d = pixel.thickness * -NoR;
    ray.position = float3(shading_position + r * d);
    ray.d = d;
    float3 n1 = normalize(NoR * r - n * 0.5);
    ray.direction = refract(r, n1,  pixel.etaRI);
}

void refractionSolidBox(const PixelParams pixel, const float3 n, float3 r, out Refraction ray) {
    float3 rr = refract(r, n, pixel.etaIR);
    float NoR = dot(n, rr);
    float d = pixel.thickness / max(-NoR, 0.001);
    ray.position = float3(shading_position + rr * d);
    ray.direction = r;
    ray.d = d;
#if REFRACTION_MODE == REFRACTION_MODE_CUBEMAP
    // fudge direction vector, so we see the offset due to the thickness of the object
    float envDistance = 10.0; // this should come from a ubo
    ray.direction = normalize((ray.position - shading_position) + ray.direction * envDistance);
#endif
}

void refractionThinSphere(const PixelParams pixel, const float3 n, float3 r, out Refraction ray) {
    float d = 0.0;
#if defined(MATERIAL_HAS_MICRO_THICKNESS)
    // note: we need the refracted ray to calculate the distance traveled
    // we could use shading_NoV, but we would lose the dependency on ior.
    float3 rr = refract(r, n, pixel.etaIR);
    float NoR = dot(n, rr);
    d = pixel.uThickness / max(-NoR, 0.001);
    ray.position = float3(shading_position + rr * d);
#else
    ray.position = float3(shading_position);
#endif
    ray.direction = r;
    ray.d = d;
}

float3 evaluateRefraction(const PixelParams pixel, const float3 n0, float3 E) {

    Refraction ray;

#if REFRACTION_TYPE == REFRACTION_TYPE_SOLID
    refractionSolidSphere(pixel, n0, -shading_view, ray);
#elif REFRACTION_TYPE == REFRACTION_TYPE_THIN
    refractionThinSphere(pixel, n0, -shading_view, ray);
#else
#error invalid REFRACTION_TYPE
#endif

    // compute transmission T
#if defined(MATERIAL_HAS_ABSORPTION)
#if defined(MATERIAL_HAS_THICKNESS) || defined(MATERIAL_HAS_MICRO_THICKNESS)
    float3 T = min(float3(1.0), exp(-pixel.absorption * ray.d));
#else
    float3 T = 1.0 - pixel.absorption;
#endif
#endif

    // Roughness remapping so that an IOR of 1.0 means no microfacet refraction and an IOR
    // of 1.5 has full microfacet refraction
    float perceptualRoughness = mix(pixel.perceptualRoughnessUnclamped, 0.0,
            saturate(pixel.etaIR * 3.0 - 2.0));
#if REFRACTION_TYPE == REFRACTION_TYPE_THIN
    // For thin surfaces, the light will bounce off at the second interface in the direction of
    // the reflection, effectively adding to the specular, but this process will repeat itself.
    // Each time the ray exits the surface on the front side after the first bounce,
    // it's multiplied by E^2, and we get: E + E(1-E)^2 + E^3(1-E)^2 + ...
    // This infinite series converges and is easy to simplify.
    // Note: we calculate these bounces only on a single component,
    // since it's a fairly subtle effect.
    E *= 1.0 + pixel.transmission * (1.0 - E.g) / (1.0 + E.g);
#endif

    /* sample the cubemap or screen-space */
#if REFRACTION_MODE == REFRACTION_MODE_CUBEMAP
    // when reading from the cubemap, we are not pre-exposed so we apply iblLuminance
    // which is not the case when we'll read from the screen-space buffer
    float3 Ft = prefilteredRadiance(ray.direction, perceptualRoughness) * frameUniforms.iblLuminance;
#else
    float3 Ft;

    // compute the point where the ray exits the medium, if needed
    float4 p = float4(getClipFromWorldMatrix() * float4(ray.position, 1.0));
    p.xy = uvToRenderTargetUV(p.xy * (0.5 / p.w) + 0.5);

    // distance to camera plane
    const float invLog2sqrt5 = 0.8614;
    float lod = max(0.0, (2.0f * log2(perceptualRoughness) + frameUniforms.refractionLodOffset) * invLog2sqrt5);
    Ft = textureLod(light_ssr, float3(p.xy, 0.0), lod).rgb;
#endif

    // base color changes the amount of light passing through the boundary
    Ft *= pixel.diffuseColor;

    // fresnel from the first interface
    Ft *= 1.0 - E;

    // apply absorption
#if defined(MATERIAL_HAS_ABSORPTION)
    Ft *= T;
#endif

    return Ft;
}
#endif

void evaluateIBL(const MaterialInputs material, const PixelParams pixel, inout float3 color) {
    // specular layer
    float3 Fr = float3(0.0f);

    SSAOInterpolationCache interpolationCache;
#if defined(BLEND_MODE_OPAQUE) || defined(BLEND_MODE_MASKED) || defined(MATERIAL_HAS_REFLECTIONS)
    interpolationCache.uv = uvToRenderTargetUV(getNormalizedPhysicalViewportCoord().xy);
#endif

    // screen-space reflections
#if defined(MATERIAL_HAS_REFLECTIONS)
    float4 ssrFr = float4(0.0f);
#if defined(BLEND_MODE_OPAQUE) || defined(BLEND_MODE_MASKED)
    // do the uniform based test first
    if (frameUniforms.ssrDistance > 0.0f) {
        // There is no point doing SSR for very high roughness because we're limited by the fov
        // of the screen, in addition it doesn't really add much to the final image.
        // TODO: maybe make this a parameter
        const float maxPerceptualRoughness = sqrt(0.5);
        if (pixel.perceptualRoughness < maxPerceptualRoughness) {
            // distance to camera plane
            const float invLog2sqrt5 = 0.8614;
            float d = -mulMat4x4Float3(getViewFromWorldMatrix(), getWorldPosition()).z;
            float lod = max(0.0, (log2(pixel.roughness / d) + frameUniforms.refractionLodOffset) * invLog2sqrt5);
            ssrFr = textureLod(light_ssr, float3(interpolationCache.uv, 1.0), lod);
        }
    }
#else // BLEND_MODE_OPAQUE
    // TODO: for blended transparency, we have to ray-march here (limited to mirror reflections)
#endif
#else // MATERIAL_HAS_REFLECTIONS
    const float4 ssrFr = float4(0.0f);
#endif

    // If screen-space reflections are turned on and have full contribution (ssr.a == 1.0f), then we
    // skip sampling the IBL down below.

#if IBL_INTEGRATION == IBL_INTEGRATION_PREFILTERED_CUBEMAP
    float3 E = specularDFG(pixel);
    if (ssrFr.a < 1.0f) { // prevent reading the IBL if possible
        float3 r = getReflectedVector(pixel, shading_normal);
        Fr = E * prefilteredRadiance(r, pixel.perceptualRoughness);
    }
#elif IBL_INTEGRATION == IBL_INTEGRATION_IMPORTANCE_SAMPLING
    float3 E = float3(0.0); // TODO: fix for importance sampling
    if (ssrFr.a < 1.0f) { // prevent evaluating the IBL if possible
        Fr = isEvaluateSpecularIBL(pixel, shading_normal, shading_view, shading_NoV);
    }
#endif

    // Ambient occlusion
    float ssao = evaluateSSAO(interpolationCache);
    float diffuseAO = min(material.ambientOcclusion, ssao);
    float specularAO = specularAO(shading_NoV, diffuseAO, pixel.roughness, interpolationCache);

    float3 specularSingleBounceAO = singleBounceAO(specularAO) * pixel.energyCompensation;
    Fr *= specularSingleBounceAO;
#if defined(MATERIAL_HAS_REFLECTIONS)
    ssrFr.rgb *= specularSingleBounceAO;
#endif

    // diffuse layer
    float diffuseBRDF = singleBounceAO(diffuseAO); // Fd_Lambert() is baked in the SH below
    evaluateClothIndirectDiffuseBRDF(pixel, diffuseBRDF);

#if defined(MATERIAL_HAS_BENT_NORMAL)
    float3 diffuseNormal = shading_bentNormal;
#else
    float3 diffuseNormal = shading_normal;
#endif

#if IBL_INTEGRATION == IBL_INTEGRATION_PREFILTERED_CUBEMAP
    float3 diffuseIrradiance = diffuseIrradiance(diffuseNormal);
#elif IBL_INTEGRATION == IBL_INTEGRATION_IMPORTANCE_SAMPLING
    float3 diffuseIrradiance = isEvaluateDiffuseIBL(pixel, diffuseNormal, shading_view);
#endif
    float3 Fd = pixel.diffuseColor * diffuseIrradiance * (1.0 - E) * diffuseBRDF;

    // subsurface layer
    evaluateSubsurfaceIBL(pixel, diffuseIrradiance, Fd, Fr);

    // extra ambient occlusion term for the base and subsurface layers
    multiBounceAO(diffuseAO, pixel.diffuseColor, Fd);
    multiBounceSpecularAO(specularAO, pixel.f0, Fr);

    // sheen layer
    evaluateSheenIBL(pixel, diffuseAO, interpolationCache, Fd, Fr);

    // clear coat layer
    evaluateClearCoatIBL(pixel, diffuseAO, interpolationCache, Fd, Fr);

    Fr *= frameUniforms.iblLuminance;
    Fd *= frameUniforms.iblLuminance;

#if defined(MATERIAL_HAS_REFRACTION)
    float3 Ft = evaluateRefraction(pixel, shading_normal, E);
    Ft *= pixel.transmission;
    Fd *= (1.0 - pixel.transmission);
#endif

#if defined(MATERIAL_HAS_REFLECTIONS)
    Fr = Fr * (1.0 - ssrFr.a) + (E * ssrFr.rgb);
#endif

    // Combine all terms
    // Note: iblLuminance is already premultiplied by the exposure

    color.rgb += Fr + Fd;
#if defined(MATERIAL_HAS_REFRACTION)
    color.rgb += Ft;
#endif
}
