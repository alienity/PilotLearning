//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include "runtime/core/math/moyu_math2.h"

// SphericalGaussian(dir) := Amplitude * exp(Sharpness * (dot(Axis, Direction) - 1.0f))
struct SG
{
    glm::float3 Amplitude;
	float Sharpness = 1.0f;
    glm::float3 Axis;

	// exp(2 * Sharpness * (dot(Axis, Direction) - 1.0f)) integrated over the sampling domain.
	float BasisSqIntegralOverDomain;
};

// Evaluates an SG given a direction on a unit sphere
inline glm::float3 EvaluateSG(const SG& sg, glm::float3 dir)
{
    return sg.Amplitude * std::exp(sg.Sharpness * (glm::dot(dir, sg.Axis) - 1.0f));
}

// Computes the inner product of two SG's, which is equal to Integrate(SGx(v) * SGy(v) * dv).
inline glm::float3 SGInnerProduct(const SG& x, const SG& y)
{
    float umLength = glm::length(x.Sharpness * x.Axis + y.Sharpness * y.Axis);
    glm::float3 expo = std::exp(umLength - x.Sharpness - y.Sharpness) * x.Amplitude * y.Amplitude;
    float other = 1.0f - std::exp(-2.0f * umLength);
    return (2.0f * MoYu::f::PI * expo * other) / umLength;
}

// Returns an approximation of the clamped cosine lobe represented as an SG
inline SG CosineLobeSG(glm::float3 direction)
{
    SG cosineLobe;
    cosineLobe.Axis = direction;
    cosineLobe.Sharpness = 2.133f;
    cosineLobe.Amplitude = glm::float3(1.17f);

    return cosineLobe;
}

// Computes the approximate integral of an SG over the entire sphere. The error vs. the
// non-approximate version decreases as sharpeness increases.
inline glm::float3 ApproximateSGIntegral(const SG& sg)
{
    return 2 * MoYu::f::PI * (sg.Amplitude / sg.Sharpness);
}

// Computes the approximate incident irradiance from a single SG lobe containing incoming radiance.
// The irradiance is computed using a fitted approximation polynomial. This approximation
// and its implementation were provided by Stephen Hill.
inline glm::float3 SGIrradianceFitted(const SG& lightingLobe, const glm::float3& normal)
{
    float muDotN = glm::dot(lightingLobe.Axis, normal);
    float lambda = lightingLobe.Sharpness;

    float c0 = 0.36f;
    float c1 = 1.0f / (4.0f * c0);

    float eml  = std::exp(-lambda);
    float em2l = eml * eml;
    float rl   = 1.0f / lambda;

    float scale = 1.0f + 2.0f * em2l - rl;
    float bias  = (eml - em2l) * rl - em2l;

    float x  = std::sqrt(1.0f - scale);
    float x0 = c0 * muDotN;
    float x1 = c1 * x;

    float n = x0 + x1;

    float y = (std::abs(x0) <= x1) ? n * n / x : MoYu::Saturate(muDotN);

    float normalizedIrradiance = scale * y + bias;

    return normalizedIrradiance * ApproximateSGIntegral(lightingLobe);
}

// Input parameters for the solve
struct SGSolveParam
{
    // StrikePlate plate;                           // radiance over the sphere
    glm::float3* XSamples = nullptr;
    glm::float3* YSamples = nullptr;
    glm::uint64 NumSamples = 0;

    glm::uint64 NumSGs = 0;                         // number of SG's we want to solve for

    SG* OutSGs;                                     // output of final SG's we solve for
};

enum class SGDistribution : glm::uint32
{
    Spherical,
    Hemispherical,
};

void InitializeSGSolver(glm::uint64 numSGs, SGDistribution distribution);
const SG* InitialGuess();

// Solve for k-number of SG's based on a hemisphere of radiance
void SolveSGs(SGSolveParam& params);

void ProjectOntoSGs(const glm::float3& dir, const glm::float3& color, SG* outSGs, glm::uint64 numSGs);

void SGRunningAverage(const glm::float3& dir, const glm::float3& color, SG* outSGs, glm::uint64 numSGs, float sampleIdx, float* lobeWeights, bool nonNegative);
