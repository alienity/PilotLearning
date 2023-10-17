/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "CubemapIBL.h"

#include "Cubemap.h"
#include "CubemapUtils.h"
#include "utilities.h"

#include "CubemapUtilsImpl.h"

#include "core/math/moyu_math2.h"

#include <random>
#include <vector>

namespace MoYu {
namespace ibl {

static float pow5(float x) {
    const float x2 = x * x;
    return x2 * x2 * x;
}

static float pow6(float x) {
    const float x2 = x * x;
    return x2 * x2 * x2;
}

static MoYu::MFloat3 hemisphereImportanceSampleDggx(MoYu::MFloat2 u, float a) { // pdf = D(a) * cosTheta
    const float phi = 2.0f * (float) F_PI * u.x;
    // NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
    const float cosTheta2 = (1 - u.y) / (1 + (a + 1) * ((a - 1) * u.y));
    const float cosTheta = std::sqrt(cosTheta2);
    const float sinTheta = std::sqrt(1 - cosTheta2);
    return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
}

static MoYu::MFloat3 hemisphereCosSample(MoYu::MFloat2 u) {  // pdf = cosTheta / F_PI;
    const float phi = 2.0f * (float) F_PI * u.x;
    const float cosTheta2 = 1 - u.y;
    const float cosTheta = std::sqrt(cosTheta2);
    const float sinTheta = std::sqrt(1 - cosTheta2);
    return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
}

static MoYu::MFloat3 hemisphereUniformSample(MoYu::MFloat2 u) { // pdf = 1.0 / (2.0 * F_PI);
    const float phi = 2.0f * (float) F_PI * u.x;
    const float cosTheta = 1 - u.y;
    const float sinTheta = std::sqrt(1 - cosTheta * cosTheta);
    return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
}

/*
 *
 * Importance sampling Charlie
 * ---------------------------
 *
 * In order to pick the most significative samples and increase the convergence rate, we chose to
 * rely on Charlie's distribution function for the pdf as we do in hemisphereImportanceSampleDggx.
 *
 * To determine the direction we then need to resolve the cdf associated to the chosen pdf for random inputs.
 *
 * Knowing pdf() = DCharlie(h) <n•h>
 *
 * We need to find the cdf:
 *
 * / 2pi     / pi/2
 * |         |  (2 + (1 / a)) * sin(theta) ^ (1 / a) * cos(theta) * sin(theta)
 * / phi=0   / theta=0
 *
 * We sample theta and phi independently.
 *
 * 1. as in all the other isotropic cases phi = 2 * pi * epsilon
 *    (https://www.tobias-franke.eu/log/2014/03/30/notes_on_importance_sampling.html)
 *
 * 2. we need to solve the integral on theta:
 *
 *             / sTheta
 * P(sTheta) = |  (2 + (1 / a)) * sin(theta) ^ (1 / a + 1) * cos(theta) * dtheta
 *             / theta=0
 *
 * By subsitution of u = sin(theta) and du = cos(theta) * dtheta
 *
 * /
 * |  (2 + (1 / a)) * u ^ (1 / a + 1) * du
 * /
 *
 * = (2 + (1 / a)) * u ^ (1 / a + 2) / (1 / a + 2)
 *
 * = u ^ (1 / a + 2)
 *
 * = sin(theta) ^ (1 / a + 2)
 *
 *             +-                          -+ sTheta
 * P(sTheta) = |  sin(theta) ^ (1 / a + 2)  |
 *             +-                          -+ 0
 *
 * P(sTheta) = sin(sTheta) ^ (1 / a + 2)
 *
 * We now need to resolve the cdf for an epsilon value:
 *
 * epsilon = sin(theta) ^ (a / ( 2 * a + 1))
 *
 *  +--------------------------------------------+
 *  |                                            |
 *  |  sin(theta) = epsilon ^ (a / ( 2 * a + 1)) |
 *  |                                            |
 *  +--------------------------------------------+
 */
static MoYu::MFloat3 hemisphereImportanceSampleDCharlie(MoYu::MFloat2 u, float a) { // pdf = DistributionCharlie() * cosTheta
    const float phi = 2.0f * (float) F_PI * u.x;

    const float sinTheta = std::pow(u.y, a / (2 * a + 1));
    const float cosTheta = std::sqrt(1 - sinTheta * sinTheta);

    return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
}

static float DistributionGGX(float NoH, float linearRoughness) {
    // NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
    float a = linearRoughness;
    float f = (a - 1) * ((a + 1) * (NoH * NoH)) + 1;
    return (a * a) / ((float) F_PI * f * f);
}

static float DistributionAshikhmin(float NoH, float linearRoughness) {
    float a = linearRoughness;
    float a2 = a * a;
    float cos2h = NoH * NoH;
    float sin2h = 1 - cos2h;
    float sin4h = sin2h * sin2h;
    return 1.0f / ((float) F_PI * (1 + 4 * a2)) * (sin4h + 4 * std::exp(-cos2h / (a2 * sin2h)));
}

static float DistributionCharlie(float NoH, float linearRoughness) {
    // Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
    float a = linearRoughness;
    float invAlpha = 1 / a;
    float cos2h = NoH * NoH;
    float sin2h = 1 - cos2h;
    return (2.0f + invAlpha) * std::pow(sin2h, invAlpha * 0.5f) / (2.0f * (float) F_PI);
}

static float Fresnel(float f0, float f90, float LoH) {
    const float Fc = pow5(1 - LoH);
    return f0 * (1 - Fc) + f90 * Fc;
}

static float Visibility(float NoV, float NoL, float a) {
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    // Height-correlated GGX
    const float a2 = a * a;
    const float GGXL = NoV * std::sqrt((NoL - NoL * a2) * NoL + a2);
    const float GGXV = NoL * std::sqrt((NoV - NoV * a2) * NoV + a2);
    return 0.5f / (GGXV + GGXL);
}

static float VisibilityAshikhmin(float NoV, float NoL, float /*a*/) {
    // Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
    return 1 / (4 * (NoL + NoV - NoL * NoV));
}

/*
 *
 * Importance sampling GGX - Trowbridge-Reitz
 * ------------------------------------------
 *
 * Important samples are chosen to integrate Dggx() * cos(theta) over the hemisphere.
 *
 * All calculations are made in tangent space, with n = [0 0 1]
 *
 *             l        h (important sample)
 *             .\      /.
 *             . \    / .
 *             .  \  /  .
 *             .   \/   .
 *         ----+---o----+-------> n [0 0 1]
 *     cos(2*theta)     cos(theta)
 *        = n•l            = n•h
 *
 *  v = n
 *  f0 = f90 = 1
 *  V = 1
 *
 *  h is micro facet's normal
 *
 *  l is the reflection of v (i.e.: n) around h  ==>  n•h = l•h = v•h
 *
 *  h = important_sample_ggx()
 *
 *  n•h = [0 0 1]•h = h.z
 *
 *  l = reflect(-n, h)
 *    = 2 * (n•h) * h - n;
 *
 *  n•l = cos(2 * theta)
 *      = cos(theta)^2 - sin(theta)^2
 *      = (n•h)^2 - (1 - (n•h)^2)
 *      = 2(n•h)^2 - 1
 *
 *
 *  pdf() = D(h) <n•h> |J(h)|
 *
 *               1
 *  |J(h)| = ----------
 *            4 <v•h>
 *
 *
 * Pre-filtered importance sampling
 * --------------------------------
 *
 *  see: "Real-time Shading with Filtered Importance Sampling", Jaroslav Krivanek
 *  see: "GPU-Based Importance Sampling, GPU Gems 3", Mark Colbert
 *
 *
 *                   Ωs
 *     lod = log4(K ----)
 *                   Ωp
 *
 *     log4(K) = 1, works well for box filters
 *     K = 4
 *
 *             1
 *     Ωs = ---------, solid-angle of an important sample
 *           N * pdf
 *
 *              4 PI
 *     Ωp ~ --------------, solid-angle of a sample in the base cubemap
 *           texel_count
 *
 *
 * Evaluating the integral
 * -----------------------
 *
 *                    K     fr(h)
 *            Er() = --- ∑ ------- L(h) <n•l>
 *                    N  h   pdf
 *
 * with:
 *
 *            fr() = D(h)
 *
 *                       N
 *            K = -----------------
 *                    fr(h)
 *                 ∑ ------- <n•l>
 *                 h   pdf
 *
 *
 *  It results that:
 *
 *            K           4 <v•h>
 *    Er() = --- ∑ D(h) ------------ L(h) <n•l>
 *            N  h        D(h) <n•h>
 *
 *
 *              K
 *    Er() = 4 --- ∑ L(h) <n•l>
 *              N  h
 *
 *                  N       4
 *    Er() = ------------- --- ∑ L(v) <n•l>
 *             4 ∑ <n•l>    N
 *
 *
 *  +------------------------------+
 *  |          ∑ <n•l> L(h)        |
 *  |  Er() = --------------       |
 *  |            ∑ <n•l>           |
 *  +------------------------------+
 *
 */

void CubemapIBL::roughnessFilter(Cubemap& dst, const std::vector<Cubemap>& levels,
        float linearRoughness, size_t maxNumSamples, MoYu::MFloat3 mirror, bool prefilter)
{
    const float numSamples = maxNumSamples;
    const float inumSamples = 1.0f / numSamples;
    const size_t maxLevel = levels.size()-1;
    const float maxLevelf = maxLevel;
    const Cubemap& base(levels[0]);
    const size_t dim0 = base.getDimensions();
    const float omegaP = (4.0f * (float) F_PI) / float(6 * dim0 * dim0);
    std::atomic_uint progress = {0};

    if (linearRoughness == 0) {
        auto scanline = [&]
                (CubemapUtils::EmptyState&, size_t y, Cubemap::Face f, Cubemap::Texel* data, size_t dim) {
                    const Cubemap& cm = levels[0];
                    for (size_t x = 0; x < dim; ++x, ++data) {
                        const MoYu::MFloat2 p(Cubemap::center(x, y));
                        const MoYu::MFloat3 N(dst.getDirectionFor(f, p.x, p.y) * mirror);
                        // FIXME: we should pick the proper LOD here and do trilinear filtering
                        Cubemap::writeAt(data, cm.sampleAt(N));
                    }
        };
        CubemapUtils::process<CubemapUtils::EmptyState>(dst, std::ref(scanline));
        return;
    }

    // be careful w/ the size of this structure, the smaller the better
    struct CacheEntry {
        MoYu::MFloat3 L;
        float brdf_NoL;
        float lerp;
        uint8_t l0;
        uint8_t l1;
    };

    std::vector<CacheEntry> cache;
    cache.reserve(maxNumSamples);

    // precompute everything that only depends on the sample #
    float weight = 0;
    // index of the sample to use
    // our goal is to use maxNumSamples for which NoL is > 0
    // to achieve this, we might have to try more samples than
    // maxNumSamples
    for (size_t sampleIndex = 0 ; sampleIndex < maxNumSamples; sampleIndex++) {

        // get Hammersley distribution for the half-sphere
        const MoYu::MFloat2 u = hammersley(uint32_t(sampleIndex), inumSamples);

        // Importance sampling GGX - Trowbridge-Reitz
        const MoYu::MFloat3 H = hemisphereImportanceSampleDggx(u, linearRoughness);

#if 0
        // This produces the same result that the code below using the the non-simplified
        // equation. This let's us see that N == V and that L = -reflect(V, H)
        // Keep this for reference.
        const MoYu::MFloat3 N = {0, 0, 1};
        const MoYu::MFloat3 V = N;
        const MoYu::MFloat3 L = 2 * dot(H, V) * H - V;
        const float NoL = dot(N, L);
        const float NoH = dot(N, H);
        const float NoH2 = NoH * NoH;
        const float NoV = dot(N, V);
#else
        const float NoH = H.z;
        const float NoH2 = H.z * H.z;
        const float NoL = 2 * NoH2 - 1;
        const MoYu::MFloat3 L(2 * NoH * H.x, 2 * NoH * H.y, NoL);
#endif

        if (NoL > 0) {
            const float pdf = DistributionGGX(NoH, linearRoughness) / 4;

            // K is a LOD bias that allows a bit of overlapping between samples
            constexpr float K = 4;
            const float omegaS = 1 / (numSamples * pdf);
            const float l = float(log4(omegaS) - log4(omegaP) + log4(K));
            const float mipLevel = prefilter ? glm::clamp(float(l), 0.0f, maxLevelf) : 0.0f;

            const float brdf_NoL = float(NoL);

            weight += brdf_NoL;

            uint8_t l0 = uint8_t(mipLevel);
            uint8_t l1 = uint8_t(std::min(maxLevel, size_t(l0 + 1)));
            float lerp = mipLevel - (float) l0;

            cache.push_back({ L, brdf_NoL, lerp, l0, l1 });
        }
    }

    for (auto& entry : cache) {
        entry.brdf_NoL *= 1.0f / weight;
    }

    // we can sample the cubemap in any order, sort by the weight, it could improve fp precision
    std::sort(cache.begin(), cache.end(), [](CacheEntry const& lhs, CacheEntry const& rhs) {
        return lhs.brdf_NoL < rhs.brdf_NoL;
    });


    struct State {
        // maybe blue-noise instead would look even better
        std::default_random_engine gen;
        std::uniform_real_distribution<float> distribution{ -F_PI, F_PI };
    };

    auto scanline = [&](State& state, size_t y,
            Cubemap::Face f, Cubemap::Texel* data, size_t dim) {
        MoYu::MMatrix3x3 R;
        const size_t numSamples = cache.size();
        for (size_t x = 0; x < dim; ++x, ++data) {
            const MoYu::MFloat2 p(Cubemap::center(x, y));
            const MoYu::MFloat3 N(dst.getDirectionFor(f, p.x, p.y) * mirror);

            // center the cone around the normal (handle case of normal close to up)
            const MoYu::MFloat3 up = std::abs(N.z) < 0.999 ? MoYu::MFloat3(0, 0, 1) : MoYu::MFloat3(1, 0, 0);
            
            MFloat3 _right = glm::normalize(glm::cross(up, N));
            MFloat3 _forward = glm::cross(N, _right); 

            R = MoYu::MMatrix3x3(_right, _forward, N);
            R = glm::transpose(R);

            MMatrix3x3 _rot = MYMatrix3x3::Identity;
            _rot.fromAngleAxis(MoYu::MFloat3 {0, 0, 1}, state.distribution(state.gen));

            R = R * _rot;

            //R *= mat3f::rotation(state.distribution(state.gen), MoYu::MFloat3{0,0,1});

            MoYu::MFloat3 Li = 0;
            for (size_t sample = 0; sample < numSamples; sample++) {
                const CacheEntry& e = cache[sample];
                const MoYu::MFloat3 L(R * e.L);
                const Cubemap& cmBase = levels[e.l0];
                const Cubemap& next = levels[e.l1];
                const MoYu::MFloat3 c0 = Cubemap::trilinearFilterAt(cmBase, next, e.lerp, L);
                Li += c0 * e.brdf_NoL;
            }
            Cubemap::writeAt(data, Cubemap::Texel(Li));
        }
    };

    CubemapUtils::process<State>(dst, std::ref(scanline));
}

/*
 *
 * Importance sampling
 * -------------------
 *
 * Important samples are chosen to integrate cos(theta) over the hemisphere.
 *
 * All calculations are made in tangent space, with n = [0 0 1]
 *
 *                      l (important sample)
 *                     /.
 *                    / .
 *                   /  .
 *                  /   .
 *         --------o----+-------> n (direction)
 *                   cos(theta)
 *                    = n•l
 *
 *
 *  'direction' is given as an input parameter, and serves as tge z direction of the tangent space.
 *
 *  l = important_sample_cos()
 *
 *  n•l = [0 0 1] • l = l.z
 *
 *           n•l
 *  pdf() = -----
 *           PI
 *
 *
 * Pre-filtered importance sampling
 * --------------------------------
 *
 *  see: "Real-time Shading with Filtered Importance Sampling", Jaroslav Krivanek
 *  see: "GPU-Based Importance Sampling, GPU Gems 3", Mark Colbert
 *
 *
 *                   Ωs
 *     lod = log4(K ----)
 *                   Ωp
 *
 *     log4(K) = 1, works well for box filters
 *     K = 4
 *
 *             1
 *     Ωs = ---------, solid-angle of an important sample
 *           N * pdf
 *
 *              4 PI
 *     Ωp ~ --------------, solid-angle of a sample in the base cubemap
 *           texel_count
 *
 *
 * Evaluating the integral
 * -----------------------
 *
 * We are trying to evaluate the following integral:
 *
 *                     /
 *             Ed() =  | L(s) <n•l> ds
 *                     /
 *                     Ω
 *
 * For this, we're using importance sampling:
 *
 *                    1     L(l)
 *            Ed() = --- ∑ ------- <n•l>
 *                    N  l   pdf
 *
 *
 *  It results that:
 *
 *             1           PI
 *    Ed() = ---- ∑ L(l) ------  <n•l>
 *            N   l        n•l
 *
 *
 *  To avoid multiplying by 1/PI in the shader, we do it here, which simplifies to:
 *
 *  +----------------------+
 *  |          1           |
 *  |  Ed() = ---- ∑ L(l)  |
 *  |          N   l       |
 *  +----------------------+
 *
 */

void CubemapIBL::diffuseIrradiance(Cubemap& dst, const std::vector<Cubemap>& levels,
        size_t maxNumSamples)
{
    const float numSamples = maxNumSamples;
    const float inumSamples = 1.0f / numSamples;
    const size_t maxLevel = levels.size()-1;
    const float maxLevelf = maxLevel;
    const Cubemap& base(levels[0]);
    const size_t dim0 = base.getDimensions();
    const float omegaP = (4.0f * (float) F_PI) / float(6 * dim0 * dim0);

    std::atomic_uint progress = {0};

    struct CacheEntry {
        MoYu::MFloat3 L;
        float lerp;
        uint8_t l0;
        uint8_t l1;
    };

    std::vector<CacheEntry> cache;
    cache.reserve(maxNumSamples);

    // precompute everything that only depends on the sample #
    for (size_t sampleIndex = 0; sampleIndex < maxNumSamples; sampleIndex++) {
        // get Hammersley distribution for the half-sphere
        const MoYu::MFloat2 u = hammersley(uint32_t(sampleIndex), inumSamples);
        const MoYu::MFloat3 L = hemisphereCosSample(u);
        const MoYu::MFloat3 N = { 0, 0, 1 };
        const float NoL = MFloat3::dot(N, L);

        if (NoL > 0) {
            float pdf = NoL * (float) F_1_PI;

            constexpr float K = 4;
            const float omegaS = 1.0f / (numSamples * pdf);
            const float l = float(log4(omegaS) - log4(omegaP) + log4(K));
            const float mipLevel = clamp(float(l), 0.0f, maxLevelf);

            uint8_t l0 = uint8_t(mipLevel);
            uint8_t l1 = uint8_t(std::min(maxLevel, size_t(l0 + 1)));
            float lerp = mipLevel - (float) l0;

            cache.push_back({ L, lerp, l0, l1 });
        }
    }

    CubemapUtils::process<CubemapUtils::EmptyState>(dst,
            [&](CubemapUtils::EmptyState&, size_t y, Cubemap::Face f, Cubemap::Texel* data, size_t dim) {

        Matrix3x3 R;
        const size_t numSamples = cache.size();
        for (size_t x = 0; x < dim; ++x, ++data) {
            const MoYu::MFloat2 p(Cubemap::center(x, y));
            const MoYu::MFloat3 N(dst.getDirectionFor(f, p.x, p.y));

            // center the cone around the normal (handle case of normal close to up)
            const MoYu::MFloat3 up = std::abs(N.z) < 0.999 ? MoYu::MFloat3(0, 0, 1) : MoYu::MFloat3(1, 0, 0);

            MFloat3 _right   = MoYu::MFloat3::normalize(MFloat3::cross(up, N));
            MFloat3 _forward = MFloat3::cross(N, _right);

            R = MoYu::Matrix3x3(_right, _forward, N);
            R = R.transpose();

            MoYu::MFloat3 Li = 0;
            for (size_t sample = 0; sample < numSamples; sample++) {
                const CacheEntry& e = cache[sample];
                const MoYu::MFloat3 L(R * e.L);
                const Cubemap& cmBase = levels[e.l0];
                const Cubemap& next = levels[e.l1];
                const MoYu::MFloat3 c0 = Cubemap::trilinearFilterAt(cmBase, next, e.lerp, L);
                Li += c0;
            }
            Cubemap::writeAt(data, Cubemap::Texel(Li * inumSamples));
        }
    });
}

// Not importance-sampled
static MoYu::MFloat2 DFV_NoIS(float NoV, float roughness, size_t numSamples) {
    MoYu::MFloat2 r = 0;
    const float linearRoughness = roughness * roughness;
    const MoYu::MFloat3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
    for (size_t i = 0; i < numSamples; i++) {
        const MoYu::MFloat2 u = hammersley(uint32_t(i), 1.0f / numSamples);
        const MoYu::MFloat3 H = hemisphereCosSample(u);
        const MoYu::MFloat3 L = 2 * MFloat3::dot(V, H) * H - V;
        const float VoH = saturate(MFloat3::dot(V, H));
        const float NoL = saturate(L.z);
        const float NoH = saturate(H.z);
        if (NoL > 0) {
            // Note: remember VoH == LoH  (H is half vector)
            const float J = 1.0f / (4.0f * VoH);
            const float pdf = NoH / (float) F_PI;
            const float d = DistributionGGX(NoH, linearRoughness) * NoL / (pdf * J);
            const float Fc = pow5(1 - VoH);
            const float v = Visibility(NoV, NoL, linearRoughness);
            r.x += d * v * (1.0f - Fc);
            r.y += d * v * Fc;
        }
    }
    return r / numSamples;
}

/*
 *
 * Importance sampling GGX - Trowbridge-Reitz
 * ------------------------------------------
 *
 * Important samples are chosen to integrate Dggx() * cos(theta) over the hemisphere.
 *
 * All calculations are made in tangent space, with n = [0 0 1]
 *
 *                      h (important sample)
 *                     /.
 *                    / .
 *                   /  .
 *                  /   .
 *         --------o----+-------> n
 *                   cos(theta)
 *                    = n•h
 *
 *  h is micro facet's normal
 *  l is the reflection of v around h, l = reflect(-v, h)  ==>  v•h = l•h
 *
 *  n•v is given as an input parameter at runtime
 *
 *  Since n = [0 0 1], we also have v.z = n•v
 *
 *  Since we need to compute v•h, we chose v as below. This choice only affects the
 *  computation of v•h (and therefore the fresnel term too), but doesn't affect
 *  n•l, which only relies on l.z (which itself only relies on v.z, i.e.: n•v)
 *
 *      | sqrt(1 - (n•v)^2)     (sin)
 *  v = | 0
 *      | n•v                   (cos)
 *
 *
 *  h = important_sample_ggx()
 *
 *  l = reflect(-v, h) = 2 * v•h * h - v;
 *
 *  n•l = [0 0 1] • l = l.z
 *
 *  n•h = [0 0 1] • l = h.z
 *
 *
 *  pdf() = D(h) <n•h> |J(h)|
 *
 *               1
 *  |J(h)| = ----------
 *            4 <v•h>
 *
 *
 * Evaluating the integral
 * -----------------------
 *
 * We are trying to evaluate the following integral:
 *
 *                    /
 *             Er() = | fr(s) <n•l> ds
 *                    /
 *                    Ω
 *
 * For this, we're using importance sampling:
 *
 *                    1     fr(h)
 *            Er() = --- ∑ ------- <n•l>
 *                    N  h   pdf
 *
 * with:
 *
 *            fr() = D(h) F(h) V(v, l)
 *
 *
 *  It results that:
 *
 *            1                        4 <v•h>
 *    Er() = --- ∑ D(h) F(h) V(v, l) ------------ <n•l>
 *            N  h                     D(h) <n•h>
 *
 *
 *  +-------------------------------------------+
 *  |          4                  <v•h>         |
 *  |  Er() = --- ∑ F(h) V(v, l) ------- <n•l>  |
 *  |          N  h               <n•h>         |
 *  +-------------------------------------------+
 *
 */

static MoYu::MFloat2 DFV(float NoV, float linearRoughness, size_t numSamples) {
    MoYu::MFloat2 r = 0;
    const MoYu::MFloat3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
    for (size_t i = 0; i < numSamples; i++) {
        const MoYu::MFloat2 u = hammersley(uint32_t(i), 1.0f / numSamples);
        const MoYu::MFloat3 H = hemisphereImportanceSampleDggx(u, linearRoughness);
        const MoYu::MFloat3 L = 2 * MFloat3::dot(V, H) * H - V;
        const float VoH = saturate(MFloat3::dot(V, H));
        const float NoL = saturate(L.z);
        const float NoH = saturate(H.z);
        if (NoL > 0) {
            /*
             * Fc = (1 - V•H)^5
             * F(h) = f0*(1 - Fc) + f90*Fc
             *
             * f0 and f90 are known at runtime, but thankfully can be factored out, allowing us
             * to split the integral in two terms and store both terms separately in a LUT.
             *
             * At runtime, we can reconstruct Er() exactly as below:
             *
             *            4                      <v•h>
             *   DFV.x = --- ∑ (1 - Fc) V(v, l) ------- <n•l>
             *            N  h                   <n•h>
             *
             *
             *            4                      <v•h>
             *   DFV.y = --- ∑ (    Fc) V(v, l) ------- <n•l>
             *            N  h                   <n•h>
             *
             *
             *   Er() = f0 * DFV.x + f90 * DFV.y
             *
             */
            const float v = Visibility(NoV, NoL, linearRoughness) * NoL * (VoH / NoH);
            const float Fc = pow5(1 - VoH);
            r.x += v * (1.0f - Fc);
            r.y += v * Fc;
        }
    }
    return r * (4.0f / numSamples);
}

static MoYu::MFloat2 DFV_Multiscatter(float NoV, float linearRoughness, size_t numSamples) {
    MoYu::MFloat2 r = 0;
    const MoYu::MFloat3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
    for (size_t i = 0; i < numSamples; i++) {
        const MoYu::MFloat2 u = hammersley(uint32_t(i), 1.0f / numSamples);
        const MoYu::MFloat3 H = hemisphereImportanceSampleDggx(u, linearRoughness);
        const MoYu::MFloat3 L = 2 * MFloat3::dot(V, H) * H - V;
        const float VoH = saturate(MFloat3::dot(V, H));
        const float NoL = saturate(L.z);
        const float NoH = saturate(H.z);
        if (NoL > 0) {
            const float v = Visibility(NoV, NoL, linearRoughness) * NoL * (VoH / NoH);
            const float Fc = pow5(1 - VoH);
            /*
             * Assuming f90 = 1
             *   Fc = (1 - V•H)^5
             *   F(h) = f0*(1 - Fc) + Fc
             *
             * f0 and f90 are known at runtime, but thankfully can be factored out, allowing us
             * to split the integral in two terms and store both terms separately in a LUT.
             *
             * At runtime, we can reconstruct Er() exactly as below:
             *
             *            4                <v•h>
             *   DFV.x = --- ∑ Fc V(v, l) ------- <n•l>
             *            N  h             <n•h>
             *
             *
             *            4                <v•h>
             *   DFV.y = --- ∑    V(v, l) ------- <n•l>
             *            N  h             <n•h>
             *
             *
             *   Er() = (1 - f0) * DFV.x + f0 * DFV.y
             *
             *        = mix(DFV.xxx, DFV.yyy, f0)
             *
             */
            r.x += v * Fc;
            r.y += v;
        }
    }
    return r * (4.0f / numSamples);
}

static float DFV_LazanyiTerm(float NoV, float linearRoughness, size_t numSamples) {
    float r = 0;
    const float cosThetaMax = (float) std::cos(81.7 * F_PI / 180.0);
    const float q = 1.0f / (cosThetaMax * pow6(1.0f - cosThetaMax));
    const MoYu::MFloat3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
    for (size_t i = 0; i < numSamples; i++) {
        const MoYu::MFloat2 u = hammersley(uint32_t(i), 1.0f / numSamples);
        const MoYu::MFloat3 H = hemisphereImportanceSampleDggx(u, linearRoughness);
        const MoYu::MFloat3 L = 2 * MFloat3::dot(V, H) * H - V;
        const float VoH = saturate(MFloat3::dot(V, H));
        const float NoL = saturate(L.z);
        const float NoH = saturate(H.z);
        if (NoL > 0) {
            const float v = Visibility(NoV, NoL, linearRoughness) * NoL * (VoH / NoH);
            const float Fc = pow6(1 - VoH);
            r += v * Fc * VoH * q;
        }
    }
    return r * (4.0f / numSamples);
}

static float DFV_Charlie_Uniform(float NoV, float linearRoughness, size_t numSamples) {
    float r = 0.0;
    const MoYu::MFloat3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
    for (size_t i = 0; i < numSamples; i++) {
        const MoYu::MFloat2 u = hammersley(uint32_t(i), 1.0f / numSamples);
        const MoYu::MFloat3 H = hemisphereUniformSample(u);
        const MoYu::MFloat3 L = 2 * MFloat3::dot(V, H) * H - V;
        const float VoH = saturate(MFloat3::dot(V, H));
        const float NoL = saturate(L.z);
        const float NoH = saturate(H.z);
        if (NoL > 0) {
            const float v = VisibilityAshikhmin(NoV, NoL, linearRoughness);
            const float d = DistributionCharlie(NoH, linearRoughness);
            r += v * d * NoL * VoH; // VoH comes from the Jacobian, 1/(4*VoH)
        }
    }
    // uniform sampling, the PDF is 1/2pi, 4 comes from the Jacobian
    return r * (4.0f * 2.0f * (float) F_PI / numSamples);
}

/*
 *
 * Importance sampling Charlie
 * ---------------------------
 *
 * Important samples are chosen to integrate DCharlie() * cos(theta) over the hemisphere.
 *
 * All calculations are made in tangent space, with n = [0 0 1]
 *
 *                      h (important sample)
 *                     /.
 *                    / .
 *                   /  .
 *                  /   .
 *         --------o----+-------> n
 *                   cos(theta)
 *                    = n•h
 *
 *  h is micro facet's normal
 *  l is the reflection of v around h, l = reflect(-v, h)  ==>  v•h = l•h
 *
 *  n•v is given as an input parameter at runtime
 *
 *  Since n = [0 0 1], we also have v.z = n•v
 *
 *  Since we need to compute v•h, we chose v as below. This choice only affects the
 *  computation of v•h (and therefore the fresnel term too), but doesn't affect
 *  n•l, which only relies on l.z (which itself only relies on v.z, i.e.: n•v)
 *
 *      | sqrt(1 - (n•v)^2)     (sin)
 *  v = | 0
 *      | n•v                   (cos)
 *
 *
 *  h = hemisphereImportanceSampleDCharlie()
 *
 *  l = reflect(-v, h) = 2 * v•h * h - v;
 *
 *  n•l = [0 0 1] • l = l.z
 *
 *  n•h = [0 0 1] • l = h.z
 *
 *
 *  pdf() = DCharlie(h) <n•h> |J(h)|
 *
 *               1
 *  |J(h)| = ----------
 *            4 <v•h>
 *
 *
 * Evaluating the integral
 * -----------------------
 *
 * We are trying to evaluate the following integral:
 *
 *                    /
 *             Er() = | fr(s) <n•l> ds
 *                    /
 *                    Ω
 *
 * For this, we're using importance sampling:
 *
 *                    1     fr(h)
 *            Er() = --- ∑ ------- <n•l>
 *                    N  h   pdf
 *
 * with:
 *
 *            fr() = DCharlie(h) V(v, l)
 *
 *
 *  It results that:
 *
 *            1                          4 <v•h>
 *    Er() = --- ∑ DCharlie(h) V(v, l) ------------ <n•l>
 *            N  h                     DCharlie(h) <n•h>
 *
 *
 *  +---------------------------------------+
 *  |          4             <v•h>          |
 *  |  Er() = --- ∑ V(v, l) ------- <n•l>   |
 *  |          N  h          <n•h>          |
 *  +---------------------------------------+
 *
 */
static float DFV_Charlie_IS(float NoV, float linearRoughness, size_t numSamples) {
    float r = 0.0;
    const MoYu::MFloat3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
    for (size_t i = 0; i < numSamples; i++) {
        const MoYu::MFloat2 u = hammersley(uint32_t(i), 1.0f / numSamples);
        const MoYu::MFloat3 H = hemisphereImportanceSampleDCharlie(u, linearRoughness);
        const MoYu::MFloat3 L = 2 * MFloat3::dot(V, H) * H - V;
        const float VoH = saturate(MFloat3::dot(V, H));
        const float NoL = saturate(L.z);
        const float NoH = saturate(H.z);
        if (NoL > 0) {
            const float J = 1.0f / (4.0f * VoH);
            const float pdf = NoH; // D has been removed as it cancels out in the previous equation
            const float v = VisibilityAshikhmin(NoV, NoL, linearRoughness);

            r += v * NoL / (pdf * J);
        }
    }
    return r / numSamples;
}

void CubemapIBL::DFG(Image& dst, bool multiscatter, bool cloth) {
    auto dfvFunction = multiscatter ? DFV_Multiscatter : DFV;

    const size_t width = dst.getWidth();
    const size_t height = dst.getHeight();
    size_t y0 = size_t(0);
    for (size_t y = y0; y < y0 + dst.getHeight(); y++) {
        Cubemap::Texel* data =
                static_cast<Cubemap::Texel*>(dst.getPixelRef(0, y));

        const float h = (float) height;
        const float coord = saturate((h - y + 0.5f) / h);
        // map the coordinate in the texture to a linear_roughness,
        // here we're using ^2, but other mappings are possible.
        // ==> coord = sqrt(linear_roughness)
        const float linear_roughness = coord * coord;
        for (size_t x = 0; x < height; x++, data++) {
            // const float NoV = float(x) / (width-1);
            const float NoV = saturate((x + 0.5f) / width);
            MoYu::MFloat3 r = { dfvFunction(NoV, linear_roughness, 1024), 0 };
            if (cloth) {
                r.z = float(DFV_Charlie_Uniform(NoV, linear_roughness, 4096));
            }
            *data = r;
        }
    }
}

} // namespace ibl
} // namespace MoYu
