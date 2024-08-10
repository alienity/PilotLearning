//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "MeshBaker.h"

#include "../Graphics/SH.h"
#include "../Graphics/Skybox.h"
#include "../Graphics/Textures.h"
#include "../Graphics/Sampling.h"
#include "BRDF.h" 

#include "BakingLabSettings.h"
#include "SG.h"
#include "PathTracer.h"

/*
// Suppress vs2013: "new behavior: elements of array 'array' will be default initialized"
#pragma warning(disable : 4351)

static const glm::uint64 TileSize = 16;
static const glm::uint64 BakeGroupSizeX = 8;
static const glm::uint64 BakeGroupSizeY = 8;
static const glm::uint64 BakeGroupSize = BakeGroupSizeX * BakeGroupSizeY;

// Info about a gutter texel
struct GutterTexel
{
    glm::uvec2 TexelPos;
    glm::uvec2 NeighborPos;
};

// Returns the final monte-carlo weighting factor using the PDF of a cosine-weighted hemisphere
static float CosineWeightedMonteCarloFactor(glm::uint64 numSamples)
{
    // Integrating cosine factor about the hemisphere gives you MoYu::f::PI, and the PDF
    // of a cosine-weighted hemisphere function is 1 / MoYu::f::PI.
    // So the final monte-carlo weighting factor is 1 / NumSamples
    return (1.0f / numSamples);
}

static float HemisphereMonteCarloFactor(glm::uint64 numSamples)
{
    // The area of a unit hemisphere is 2 * MoYu::f::PI, so the PDF is 1 / (2 * MoYu::f::PI)
    return ((2.0f * MoYu::f::PI) / numSamples);
}

static float SphereMonteCarloFactor(glm::uint64 numSamples)
{
    // The area of a unit hemisphere is 2 * MoYu::f::PI, so the PDF is 1 / (2 * MoYu::f::PI)
    return ((4.0f * MoYu::f::PI) / numSamples);
}

// == Baking ======================================================================================

// Bakes irradiance / MoYu::f::PI as 3 floats per texel
struct DiffuseBaker
{
    static const glm::uint64 BasisCount = 1;

    glm::uint64 NumSamples = 0;
    glm::float3 ResultSum;

    void Init(glm::uint64 numSamples, glm::float4 prevResult[BasisCount])
    {
        NumSamples = numSamples;
        ResultSum = glm::float3(0.0f);
    }

    glm::float3 SampleDirection(glm::float2 samplePoint)
    {
        return SampleCosineHemisphere(samplePoint.x, samplePoint.y);
    }

    void AddSample(glm::float3 sampleDirTS, glm::uint64 sampleIdx, glm::float3 sample, glm::float3 sampleDirWS, glm::float3 normal)
    {
        ResultSum += sample;
    }

    void FinalResult(glm::float4 bakeOutput[BasisCount])
    {
        glm::float3 finalResult = ResultSum * CosineWeightedMonteCarloFactor(NumSamples);
        bakeOutput[0] = glm::float4(MoYu::Clamp(finalResult, 0.0f, FLT_MAX), 1.0f);
    }

    void ProgressiveResult(glm::float4 bakeOutput[BasisCount], glm::uint64 passIdx)
    {
        const float lerpFactor = passIdx / (passIdx + 1.0f);
        glm::float3 newSample = ResultSum * CosineWeightedMonteCarloFactor(1);
        glm::float3 currValue = bakeOutput[0];
        currValue = MoYu::Lerp<glm::float3>(newSample, currValue, lerpFactor);
        bakeOutput[0] = glm::float4(MoYu::Clamp(currValue, 0.0f, FLT_MAX), 1.0f);
    }
};

// Bakes irradiance based on Enlighten's directional approach, with 3 floats for color,
// 3 for lighting main direction information, and 1 float to ensure that the directional
// term evaluates to 1 when the surface normal aligns with normal used when baking.
//
// NOTE: A directional map can be encoded per RGB channel which puts the memory cost at
// the cost of L1 SH but at a worse quality. This implementation with a single directional
// map is provided as a cheap alternative at the expense of quality.
//
// Reference: https://static.docs.arm.com/100837/0308/enlighten_3-08_sdk_documentation__100837_0308_00_en.pdf
struct DirectionalBaker
{
    static const glm::uint64 BasisCount = 2;

    glm::uint64 NumSamples = 0;
    glm::float3 ResultSum;
    glm::float3 DirectionSum;
    float DirectionWeightSum;
    glm::float3 NormalSum;

    void Init(glm::uint64 numSamples, glm::float4 prevResult[BasisCount])
    {
        NumSamples = numSamples;

        ResultSum = glm::float3(0.0f);
        DirectionSum = glm::float3(0.0f);
        DirectionWeightSum = 0.0;
        NormalSum = glm::float3(0.0f);
    }

    glm::float3 SampleDirection(glm::float2 samplePoint)
    {
        return SampleDirectionHemisphere(samplePoint.x, samplePoint.y);
    }

    void AddSample(glm::float3 sampleDirTS, glm::uint64 sampleIdx, glm::float3 sample, glm::float3 sampleDirWS, glm::float3 normal)
    {
        normal = glm::normalize(normal);

        const glm::float3 sampleDir = glm::normalize(sampleDirWS);

        ResultSum += sample;
        DirectionSum += sampleDir * MoYu::ComputeLuminance(sample);
        DirectionWeightSum += MoYu::ComputeLuminance(sample);
        NormalSum += normal;
    }

    void FinalResult(glm::float4 bakeOutput[BasisCount])
    {
        glm::float3 finalColorResult = ResultSum * CosineWeightedMonteCarloFactor(NumSamples);

        glm::float3 finalDirection = glm::normalize(DirectionSum / std::max(DirectionWeightSum, 0.0001f));

        glm::float3 averageNormal = glm::normalize(NormalSum * CosineWeightedMonteCarloFactor(NumSamples));
        glm::float4 tau = glm::float4(averageNormal, 1.0f) * 0.5f;

        bakeOutput[0] = glm::float4(MoYu::Clamp(finalColorResult, 0.0f, FLT_MAX), 1.0f);
        bakeOutput[1] = glm::float4(finalDirection * 0.5f + 0.5f, std::max(glm::dot(tau, glm::float4(finalDirection, 1.0f)), 0.0001f));
    }

    void ProgressiveResult(glm::float4 bakeOutput[BasisCount], glm::uint64 passIdx)
    {
    }
};

// Bakes irradiance based on Enlighten's RGB directional approach, with 3 floats for
// color, and 4 floats per RGB channel were 3 of the floats are the direction for the
// specific channel and the 4th float ensures that the directional term evaluates to 1
// when the surface normal aligns with normal used when baking.
//
// Reference: https://static.docs.arm.com/100837/0308/enlighten_3-08_sdk_documentation__100837_0308_00_en.pdf
struct DirectionalRGBBaker
{
    static const glm::uint64 BasisCount = 4;

    glm::uint64 NumSamples = 0;
    glm::float3 ResultSum;
    glm::float3 DirectionSum[3];
    glm::float3 DirectionWeightSum;
    glm::float3 NormalSum;

    void Init(glm::uint64 numSamples, glm::float4 prevResult[BasisCount])
    {
        NumSamples = numSamples;

        ResultSum = glm::float3(0.0f);
        DirectionSum[0] = glm::float3(0.0f);
        DirectionSum[1] = glm::float3(0.0f);
        DirectionSum[2] = glm::float3(0.0f);
        DirectionWeightSum = glm::float3(0.0f);
        NormalSum = glm::float3(0.0f);
    }

    glm::float3 SampleDirection(glm::float2 samplePoint)
    {
        return SampleDirectionHemisphere(samplePoint.x, samplePoint.y);
    }

    void AddSample(glm::float3 sampleDirTS, glm::uint64 sampleIdx, glm::float3 sample, glm::float3 sampleDirWS, glm::float3 normal)
    {
        normal = glm::normalize(normal);

        const glm::float3 sampleDir = glm::normalize(sampleDirWS);

        ResultSum += sample;
        DirectionSum[0] += sampleDir * sample.x;
        DirectionSum[1] += sampleDir * sample.y;
        DirectionSum[2] += sampleDir * sample.z;
        DirectionWeightSum += sample;
        NormalSum += normal;
    }

    void FinalResult(glm::float4 bakeOutput[BasisCount])
    {
        glm::float3 finalColorResult = ResultSum * CosineWeightedMonteCarloFactor(NumSamples);

        glm::float3 finalDirectionR = glm::normalize(DirectionSum[0] / std::max(DirectionWeightSum.x, 0.0001f));
        glm::float3 finalDirectionG = glm::normalize(DirectionSum[1] / std::max(DirectionWeightSum.y, 0.0001f));
        glm::float3 finalDirectionB = glm::normalize(DirectionSum[2] / std::max(DirectionWeightSum.z, 0.0001f));

        glm::float3 averageNormal = glm::normalize(NormalSum * CosineWeightedMonteCarloFactor(NumSamples));
        glm::float4 tau = glm::float4(averageNormal, 1.0f) * 0.5f;

        bakeOutput[0] = glm::float4(MoYu::Clamp(finalColorResult, 0.0f, FLT_MAX), 1.0f);
        bakeOutput[1] = glm::float4(finalDirectionR * 0.5f + 0.5f, std::max(glm::dot(tau, glm::float4(finalDirectionR, 1.0f)), 0.0001f));
        bakeOutput[2] = glm::float4(finalDirectionG * 0.5f + 0.5f, std::max(glm::dot(tau, glm::float4(finalDirectionG, 1.0f)), 0.0001f));
        bakeOutput[3] = glm::float4(finalDirectionB * 0.5f + 0.5f, std::max(glm::dot(tau, glm::float4(finalDirectionB, 1.0f)), 0.0001f));
    }

    void ProgressiveResult(glm::float4 bakeOutput[BasisCount], glm::uint64 passIdx)
    {
    }
};

// Bakes irradiance projected onto the Half-Life 2 basis, with 9 floats per texel
struct HL2Baker
{
    static const glm::uint64 BasisCount = 3;

    glm::uint64 NumSamples = 0;
    glm::float3 ResultSum[3];

    void Init(glm::uint64 numSamples, glm::float4 prevResult[BasisCount])
    {
        NumSamples = numSamples;
        ResultSum[0] = ResultSum[1] = ResultSum[2] = glm::float3(0.0f);
    }

    glm::float3 SampleDirection(glm::float2 samplePoint)
    {
        return SampleDirectionHemisphere(samplePoint.x, samplePoint.y);
    }

    void AddSample(glm::float3 sampleDirTS, glm::uint64 sampleIdx, glm::float3 sample, glm::float3 sampleDirWS, glm::float3 normal)
    {
        static const glm::float3 BasisDirs[BasisCount] =
        {
            glm::float3(-1.0f / std::sqrt(6.0f), -1.0f / std::sqrt(2.0f), 1.0f / std::sqrt(3.0f)),
            glm::float3(-1.0f / std::sqrt(6.0f), 1.0f / std::sqrt(2.0f), 1.0f / std::sqrt(3.0f)),
            glm::float3(std::sqrt(2.0f / 3.0f), 0.0f, 1.0f / std::sqrt(3.0f)),
        };
        for(glm::uint64 i = 0; i < BasisCount; ++i)
            ResultSum[i] += sample * glm::dot(sampleDirTS, BasisDirs[i]);
    }

    void FinalResult(glm::float4 bakeOutput[BasisCount])
    {
        for(glm::uint64 i = 0; i < BasisCount; ++i)
        {
            glm::float3 result = ResultSum[i] * HemisphereMonteCarloFactor(NumSamples);
            bakeOutput[i] = glm::float4(MoYu::Clamp(result, -FLT_MAX, FLT_MAX), 1.0f);
        }
    }

    void ProgressiveResult(glm::float4 bakeOutput[BasisCount], glm::uint64 passIdx)
    {
        const float lerpFactor = passIdx / (passIdx + 1.0f);
        for(glm::uint64 i = 0; i < BasisCount; ++i)
        {
            glm::float3 newSample = ResultSum[i] * HemisphereMonteCarloFactor(1);
            glm::float3 currValue = bakeOutput[i];
            currValue = MoYu::Lerp<glm::float3>(newSample, currValue, lerpFactor);
            bakeOutput[i] = glm::float4(MoYu::Clamp(currValue, -FLT_MAX, FLT_MAX), 1.0f);
        }
    }
};

// Bakes radiance projected onto L1 SH, with 12 floats per texel
struct SH4Baker
{
    static const glm::uint64 BasisCount = 4;

    glm::uint64 NumSamples = 0;
    SH4Color ResultSum;

    void Init(glm::uint64 numSamples, glm::float4 prevResult[BasisCount])
    {
        NumSamples = numSamples;
        ResultSum = SH4Color();
    }

    glm::float3 SampleDirection(glm::float2 samplePoint)
    {
        return SampleDirectionHemisphere(samplePoint.x, samplePoint.y);
    }

    void AddSample(glm::float3 sampleDirTS, glm::uint64 sampleIdx, glm::float3 sample, glm::float3 sampleDirWS, glm::float3 normal)
    {
        const glm::float3 sampleDir = SampleFramework11::WorldSpaceBake ? sampleDirWS : sampleDirTS;
        ResultSum += ProjectOntoSH4Color(sampleDir, sample);
    }

    void FinalResult(glm::float4 bakeOutput[BasisCount])
    {
        SH4Color result = ResultSum * HemisphereMonteCarloFactor(NumSamples);
        for(glm::uint64 i = 0; i < BasisCount; ++i)
            bakeOutput[i] = glm::float4(MoYu::Clamp(result.Coefficients[i], -FLT_MAX, FLT_MAX), 1.0f);
    }

    void ProgressiveResult(glm::float4 bakeOutput[BasisCount], glm::uint64 passIdx)
    {
        const float lerpFactor = passIdx / (passIdx + 1.0f);
        for(glm::uint64 i = 0; i < BasisCount; ++i)
        {
            glm::float3 newSample = ResultSum.Coefficients[i] * HemisphereMonteCarloFactor(1);
            glm::float3 currValue = bakeOutput[i];
            currValue = MoYu::Lerp<glm::float3>(newSample, currValue, lerpFactor);
            bakeOutput[i] = glm::float4(MoYu::Clamp(currValue, -FLT_MAX, FLT_MAX), 1.0f);
        }
    }
};

// Bakes radiance projected onto L2 SH, with 27 floats per texel
struct SH9Baker
{
    static const glm::uint64 BasisCount = 9;

    glm::uint64 NumSamples = 0;
    SH9Color ResultSum;

    void Init(glm::uint64 numSamples, glm::float4 prevResult[BasisCount])
    {
        NumSamples = numSamples;
        ResultSum = SH9Color();
    }

    glm::float3 SampleDirection(glm::float2 samplePoint)
    {
        return SampleDirectionHemisphere(samplePoint.x, samplePoint.y);
    }

    void AddSample(glm::float3 sampleDirTS, glm::uint64 sampleIdx, glm::float3 sample, glm::float3 sampleDirWS, glm::float3 normal)
    {
        const glm::float3 sampleDir = SampleFramework11::WorldSpaceBake ? sampleDirWS : sampleDirTS;
        ResultSum += ProjectOntoSH9Color(sampleDir, sample);
    }

    void FinalResult(glm::float4 bakeOutput[BasisCount])
    {
        SH9Color result = ResultSum * HemisphereMonteCarloFactor(NumSamples);
        for(glm::uint64 i = 0; i < BasisCount; ++i)
            bakeOutput[i] = glm::float4(MoYu::Clamp(result.Coefficients[i], -FLT_MAX, FLT_MAX), 1.0f);
    }

    void ProgressiveResult(glm::float4 bakeOutput[BasisCount], glm::uint64 passIdx)
    {
        const float lerpFactor = passIdx / (passIdx + 1.0f);
        for(glm::uint64 i = 0; i < BasisCount; ++i)
        {
            glm::float3 newSample = ResultSum.Coefficients[i] * HemisphereMonteCarloFactor(1);
            glm::float3 currValue = bakeOutput[i];
            currValue = MoYu::Lerp<glm::float3>(newSample, currValue, lerpFactor);
            bakeOutput[i] = glm::float4(MoYu::Clamp(currValue, -FLT_MAX, FLT_MAX), 1.0f);
        }
    }
};

// Bakes irradiance projected onto L1 H-basis, with 12 floats per texel
struct H4Baker
{
    static const glm::uint64 BasisCount = 4;

    glm::uint64 NumSamples = 0;
    SH9Color ResultSum;

    void Init(glm::uint64 numSamples, glm::float4 prevResult[BasisCount])
    {
        NumSamples = numSamples;
        ResultSum = SH9Color();
    }

    glm::float3 SampleDirection(glm::float2 samplePoint)
    {
        return SampleDirectionHemisphere(samplePoint.x, samplePoint.y);
    }

    void AddSample(glm::float3 sampleDirTS, glm::uint64 sampleIdx, glm::float3 sample, glm::float3 sampleDirWS, glm::float3 normal)
    {
        ResultSum += ProjectOntoSH9Color(sampleDirTS, sample);
    }

    void FinalResult(glm::float4 bakeOutput[BasisCount])
    {
        SH9Color shResult = ResultSum;
        shResult.ConvolveWithCosineKernel();
        H4Color result = ConvertToH4(shResult) * HemisphereMonteCarloFactor(NumSamples);
        for(glm::uint64 i = 0; i < BasisCount; ++i)
            bakeOutput[i] = glm::float4(MoYu::Clamp(result.Coefficients[i], -FLT_MAX, FLT_MAX), 1.0f);
    }

    void ProgressiveResult(glm::float4 bakeOutput[BasisCount], glm::uint64 passIdx)
    {
        SH9Color shResult = ResultSum;
        shResult.ConvolveWithCosineKernel();
        H4Color result = ConvertToH4(shResult) * HemisphereMonteCarloFactor(1);

        const float lerpFactor = passIdx / (passIdx + 1.0f);
        for(glm::uint64 i = 0; i < BasisCount; ++i)
        {
            glm::float3 newSample = result.Coefficients[i];
            glm::float3 currValue = bakeOutput[i];
            currValue = MoYu::Lerp<glm::float3>(newSample, currValue, lerpFactor);
            bakeOutput[i] = glm::float4(MoYu::Clamp(currValue, -FLT_MAX, FLT_MAX), 1.0f);
        }
    }
};

// Bakes irradiance projected onto L2 H-basis, with 18 floats per texel
struct H6Baker
{
    static const glm::uint64 BasisCount = 6;

    glm::uint64 NumSamples = 0;
    SH9Color ResultSum;

    void Init(glm::uint64 numSamples, glm::float4 prevResult[BasisCount])
    {
        NumSamples = numSamples;
        ResultSum = SH9Color();
    }

    glm::float3 SampleDirection(glm::float2 samplePoint)
    {
        return SampleDirectionHemisphere(samplePoint.x, samplePoint.y);
    }

    void AddSample(glm::float3 sampleDirTS, glm::uint64 sampleIdx, glm::float3 sample, glm::float3 sampleDirWS, glm::float3 normal)
    {
        ResultSum += ProjectOntoSH9Color(sampleDirTS, sample);
    }

    void FinalResult(glm::float4 bakeOutput[BasisCount])
    {
        SH9Color shResult = ResultSum;
        shResult.ConvolveWithCosineKernel();
        H6Color result = ConvertToH6(shResult) * HemisphereMonteCarloFactor(NumSamples);
        for(glm::uint64 i = 0; i < BasisCount; ++i)
            bakeOutput[i] = glm::float4(MoYu::Clamp(result.Coefficients[i], -FLT_MAX, FLT_MAX), 1.0f);
    }

    void ProgressiveResult(glm::float4 bakeOutput[BasisCount], glm::uint64 passIdx)
    {
        SH9Color shResult = ResultSum;
        shResult.ConvolveWithCosineKernel();
        H6Color result = ConvertToH6(shResult) * HemisphereMonteCarloFactor(1);

        const float lerpFactor = passIdx / (passIdx + 1.0f);
        for(glm::uint64 i = 0; i < BasisCount; ++i)
        {
            glm::float3 newSample = result.Coefficients[i];
            glm::float3 currValue = bakeOutput[i];
            currValue = MoYu::Lerp<glm::float3>(newSample, currValue, lerpFactor);
            bakeOutput[i] = glm::float4(MoYu::Clamp(currValue, -FLT_MAX, FLT_MAX), 1.0f);
        }
    }
};

// Bakes radiance into a set of SG lobes, which is computed using a solve or by
// using an ad-hoc projection. Bakes into SGCount * 3 floats.
template<glm::uint64 SGCount> struct SGBaker
{
    static const glm::uint64 BasisCount = SGCount;

    glm::uint64 NumSamples = 0;
    glm::uint64 CurrSampleIdx = 0;
    std::vector<glm::float3> SampleDirs;
    std::vector<glm::float3> Samples;
    SG ProjectedResult[SGCount];
    float RunningAverageWeights[SGCount] = { };

    void Init(glm::uint64 numSamples, glm::float4 prevResult[BasisCount])
    {
        CurrSampleIdx = 0;
        NumSamples = numSamples;

        if(NumSamples != SampleDirs.size())
            SampleDirs.resize(NumSamples);
        if(NumSamples != Samples.size())
            Samples.resize(NumSamples);

        const SG* initialGuess = InitialGuess();
        for(glm::uint64 i = 0; i < SGCount; ++i)
            ProjectedResult[i] = initialGuess[i];

        if(SampleFramework11::SolveMode == SolveModes::RunningAverage || SampleFramework11::SolveMode == SolveModes::RunningAverageNN)
        {
            for(glm::uint64 i = 0; i < SGCount; ++i)
            {
                ProjectedResult[i].Amplitude = prevResult[i];
                RunningAverageWeights[i] = prevResult[i].w;
            }
        }
    }

    glm::float3 SampleDirection(glm::float2 samplePoint)
    {
        return SampleDirectionHemisphere(samplePoint.x, samplePoint.y);
    }

    void AddSample(glm::float3 sampleDirTS, glm::uint64 sampleIdx, glm::float3 sample, glm::float3 sampleDirWS, glm::float3 normal)
    {
        const glm::float3 sampleDir = SampleFramework11::WorldSpaceBake ? sampleDirWS : sampleDirTS;
        SampleDirs[CurrSampleIdx] = sampleDir;
        Samples[CurrSampleIdx] = sample;
        ++CurrSampleIdx;

        if(SampleFramework11::SolveMode == SolveModes::RunningAverage)
            SGRunningAverage(sampleDir, sample, ProjectedResult, SGCount, (float)sampleIdx, RunningAverageWeights, false);
        else if(SampleFramework11::SolveMode == SolveModes::RunningAverageNN)
            SGRunningAverage(sampleDir, sample, ProjectedResult, SGCount, (float)sampleIdx, RunningAverageWeights, true);
        else
            ProjectOntoSGs(sampleDir, sample, ProjectedResult, SGCount);
    }

    void FinalResult(glm::float4 bakeOutput[BasisCount])
    {
        SG sgLobes[SGCount];

        SGSolveParam params;
        params.NumSGs = SGCount;
        params.OutSGs = sgLobes;
        params.XSamples = SampleDirs.data();
        params.YSamples = Samples.data();
        params.NumSamples = NumSamples;
        SolveSGs(params);

        for(glm::uint64 i = 0; i < SGCount; ++i)
            bakeOutput[i] = glm::float4(MoYu::Clamp(sgLobes[i].Amplitude, 0.0f, FLT_MAX), 1.0f);
    }

    void ProgressiveResult(glm::float4 bakeOutput[BasisCount], glm::uint64 passIdx)
    {
        if(SampleFramework11::SolveMode == SolveModes::RunningAverage || SampleFramework11::SolveMode == SolveModes::RunningAverageNN)
        {
            for(glm::uint64 i = 0; i < SGCount; ++i)
                bakeOutput[i] = glm::float4(MoYu::Clamp(ProjectedResult[i].Amplitude, -FLT_MAX, FLT_MAX), RunningAverageWeights[i]);
        }
        else
        {
            const float lerpFactor = passIdx / (passIdx + 1.0f);
            for(glm::uint64 i = 0; i < SGCount; ++i)
            {
                glm::float3 newSample = ProjectedResult[i].Amplitude * HemisphereMonteCarloFactor(1);
                glm::float3 currValue = bakeOutput[i];
                currValue = MoYu::Lerp<glm::float3>(newSample, currValue, lerpFactor);
                bakeOutput[i] = glm::float4(MoYu::Clamp(currValue, -FLT_MAX, FLT_MAX), 1.0f);
            }
        }
    }
};

typedef SGBaker<5> SG5Baker;
typedef SGBaker<6> SG6Baker;
typedef SGBaker<9> SG9Baker;
typedef SGBaker<12> SG12Baker;

// Data used by the baking threads
struct BakeThreadContext
{
    glm::uint64 BakeTag = glm::uint64(-1);
    SkyCache SkyCache;
    const BVHData* SceneBVH = nullptr;
    const TextureData<Half4>* EnvMaps = nullptr;
    const std::vector<BakePoint>* BakePoints = nullptr;
    glm::uint64 CurrNumBatches = 0;
    glm::uint64 CurrLightMapSize = 0;
    BakeModes CurrBakeMode = BakeModes::Diffuse;
    SolveModes CurrSolveMode = SolveModes::NNLS;
    Random RandomGenerator;
    SampleModes CurrSampleMode = SampleModes::Random;
    glm::uint64 CurrNumSamples = 0;
    const std::vector<IntegrationSamples>* Samples;
    FixedArray<glm::float4>* BakeOutput = nullptr;
    volatile int64* CurrBatch = nullptr;

    void Init(FixedArray<glm::float4>* bakeOutput, const std::vector<IntegrationSamples>* samples,
              volatile int64* currBatch, const MeshBaker* meshBaker, glm::uint64 newTag)
    {
        if(BakeTag == glm::uint64(-1))
            RandomGenerator.SeedWithRandomValue();

        BakeTag = newTag;
        SkyCache.Init(AppSettings::SunDirection, AppSettings::GroundAlbedo, AppSettings::Turbidity);
        SceneBVH = &meshBaker->sceneBVH;
        EnvMaps = meshBaker->input.EnvMapData;
        BakePoints = &meshBaker->bakePoints;
        CurrNumBatches = meshBaker->currNumBakeBatches;
        CurrLightMapSize = meshBaker->currLightMapSize;
        CurrBakeMode = meshBaker->currBakeMode;
        CurrSolveMode = meshBaker->currSolveMode;
        BakeOutput = bakeOutput;
        CurrBatch = currBatch;
        CurrSampleMode = AppSettings::BakeSampleMode;
        CurrNumSamples = AppSettings::NumBakeSamples;
        Samples = samples;
    }
};

// Runs a single iteration of the bake thread. If the bake mode supports progressive baking,
// then this function will add 1 path tracer sample to all texels within the bake group.
// Otherwise, it will completely bake a single texel within a bake group and flood fill
// its unbaked neighbors within the thread group.
template<typename TBaker> static bool BakeDriver(BakeThreadContext& context, TBaker& baker)
{
    if(context.CurrNumBatches == 0)
        return false;

    const glm::uint64 batchIdx = InterlockedIncrement64(context.CurrBatch) - 1;
    if(batchIdx >= context.CurrNumBatches)
        return false;

    // Are we baking one sample per texel and progessively integrating, or are we going to
    // fully compute the final baked texel value and flood fill the neighbors?
    const bool progressiveintegration = AppSettings::SupportsProgressiveIntegration(context.CurrBakeMode, context.CurrSolveMode);

    // Figure out which 8x8 group we're working on
    const glm::uint64 numGroupsX = (context.CurrLightMapSize + (BakeGroupSizeX - 1)) / BakeGroupSizeX;
    const glm::uint64 numGroupsY = (context.CurrLightMapSize + (BakeGroupSizeY - 1)) / BakeGroupSizeY;
    const glm::uint64 numBakeGroups = numGroupsX * numGroupsY;

    const glm::uint64 grouMoYu::f::PIdx = batchIdx % numBakeGroups;
    const glm::uint64 grouMoYu::f::PIdxX = grouMoYu::f::PIdx % numGroupsX;
    const glm::uint64 grouMoYu::f::PIdxY = grouMoYu::f::PIdx / numGroupsX;

    const glm::uint64 sqrtNumSamples = context.CurrNumSamples;
    const glm::uint64 numSamplesPerTexel = sqrtNumSamples * sqrtNumSamples;

    Random& random = context.RandomGenerator;

    const bool addAreaLight = AppSettings::EnableAreaLight && AppSettings::BakeDirectAreaLight;

    // Get the set of integration samples to use, which is tiled across threads
    const glm::uint64 numThreads = context.Samples->size();
    const IntegrationSamples& integrationSamples = (*context.Samples)[grouMoYu::f::PIdx % numThreads];

    PathTracerParams params;
    params.EnableDirectAreaLight = false;
    params.EnableDirectSun = false;
    params.EnableDiffuse = true;
    params.EnableSpecular = false;
    params.EnableBounceSpecular = false;
    params.MaxPathLength = AppSettings::MaxBakePathLength;
    params.RussianRouletteDepth = AppSettings::BakeRussianRouletteDepth;
    params.RussianRouletteProbability = AppSettings::BakeRussianRouletteProbability;
    params.RayLen = FLT_MAX;
    params.SceneBVH = context.SceneBVH;
    params.SkyCache = &context.SkyCache;
    params.EnvMaps = context.EnvMaps;

    if(progressiveintegration)
    {
        const glm::uint64 sampleIdx = batchIdx / numBakeGroups;

        // Loop over all texels in the 8x8 group, and compute 1 sample for each
        for(glm::uint64 groupTexelIdxX = 0; groupTexelIdxX < BakeGroupSizeX; ++groupTexelIdxX)
        {
            for(glm::uint64 groupTexelIdxY = 0; groupTexelIdxY < BakeGroupSizeY; ++groupTexelIdxY)
            {
                const glm::uint64 groupTexelIdx = groupTexelIdxY * BakeGroupSizeX + groupTexelIdxX;

                // Compute the absolute indices of the texel we're going to work on
                const glm::uint64 texelIdxX = grouMoYu::f::PIdxX * BakeGroupSizeX + groupTexelIdxX;
                const glm::uint64 texelIdxY = grouMoYu::f::PIdxY * BakeGroupSizeY + groupTexelIdxY;
                const glm::uint64 texelIdx = texelIdxY * context.CurrLightMapSize + texelIdxX;
                if(texelIdxX >= context.CurrLightMapSize || texelIdxY >= context.CurrLightMapSize)
                    continue;

                // Skip if the texel is empty
                const std::vector<BakePoint>& bakePoints = *context.BakePoints;
                const BakePoint& bakePoint = bakePoints[texelIdx];
                if(bakePoint.Coverage == 0 || bakePoint.Coverage == 0xFFFFFFFF)
                    continue;

                glm::float4 texelResults[TBaker::BasisCount];
                if(sampleIdx > 0)
                {
                    for(glm::uint64 basisIdx = 0; basisIdx < TBaker::BasisCount; ++basisIdx)
                        texelResults[basisIdx] = context.BakeOutput[basisIdx][texelIdx];
                }

                // The baker only accumulates one sample per MoYu::f::PIxel in progressive rendering.
                baker.Init(1, texelResults);

                IntegrationSampleSet sampleSet;
                sampleSet.Init(integrationSamples, groupTexelIdx, sampleIdx);

                glm::float3x3 tangentFrame;
                tangentFrame.SetXBasis(bakePoint.Tangent);
                tangentFrame.SetYBasis(bakePoint.Bitangent);
                tangentFrame.SetZBasis(bakePoint.Normal);

                // Create a random ray direction in tangent space, then convert to world space
                glm::float3 rayStart = bakePoint.Position;
                glm::float3 rayDirTS = baker.SampleDirection(sampleSet.MoYu::f::PIxel());
                glm::float3 rayDirWS = glm::float3::Transform(rayDirTS, tangentFrame);
                rayDirWS = glm::normalize(rayDirWS);

                glm::float3 sampleResult = 0.0f;

                glm::float2 directAreaLightSample = sampleSet.Lens();
                if(addAreaLight && directAreaLightSample.x >= 0.5f)
                {
                    glm::float3 areaLightIrradiance;
                    sampleResult = SampleAreaLight(bakePoint.Position, bakePoint.Normal, context.SceneBVH->Scene,
                                                   1.0f, 0.0f, false, 0.0f, 1.0f, sampleSet.Lens().x,
                                                   sampleSet.Lens().y, areaLightIrradiance, rayDirWS);
                    rayDirTS = glm::float3::Transform(rayDirWS, glm::float3x3::Transpose(tangentFrame));
                }
                else
                {
                    params.RayDir = rayDirWS;
                    params.RayStart = rayStart + 0.1f * rayDirWS;
                    params.RayLen = FLT_MAX;
                    params.SampleSet = &sampleSet;

                    float illuminance = 0.0f;
                    bool hitSky = false;
                    sampleResult = PathTrace(params, random, illuminance, hitSky);

                    if(AppSettings::BakeDirectSunLight)
                    {
                        glm::float3 sunLightIrradiance;
                        sampleResult += SampleSunLight(bakePoint.Position, bakePoint.Normal, context.SceneBVH->Scene,
                            1.0f, 0.0f, false, 0.0f, 1.0f, sampleSet.Lens().x,
                            sampleSet.Lens().y, sunLightIrradiance);
                    }
                }

                // Account for equally distributing our samples among the area light and the rest of the environment
                if(addAreaLight)
                    sampleResult *= 2.0f;

                if (!isfinite(sampleResult.x) || !isfinite(sampleResult.y) || !isfinite(sampleResult.z))
                    sampleResult = 0.0;

                baker.AddSample(rayDirTS, sampleIdx, sampleResult, rayDirWS, bakePoint.Normal);

                baker.ProgressiveResult(texelResults, sampleIdx);

                for(glm::uint64 basisIdx = 0; basisIdx < TBaker::BasisCount; ++basisIdx)
                    context.BakeOutput[basisIdx][texelIdx] = texelResults[basisIdx];
            }
        }
    }
    else
    {
        glm::float4 texelResults[TBaker::BasisCount];
        baker.Init(numSamplesPerTexel, texelResults);

        // Figure out the texel within the group that we're working on (we do 64 passes per group, each one a different texel)
        const glm::uint64 groupTexelIdx =  batchIdx / numBakeGroups;
        const glm::uint64 groupTexelIdxX = groupTexelIdx % BakeGroupSizeX;
        const glm::uint64 groupTexelIdxY = groupTexelIdx / BakeGroupSizeX;

        const glm::uint64 texelIdxX = grouMoYu::f::PIdxX * BakeGroupSizeX + groupTexelIdxX;
        const glm::uint64 texelIdxY = grouMoYu::f::PIdxY * BakeGroupSizeY + groupTexelIdxY;
        const glm::uint64 texelIdx = texelIdxY * context.CurrLightMapSize + texelIdxX;
        if(texelIdxX >= context.CurrLightMapSize || texelIdxY >= context.CurrLightMapSize)
            return true;

        // Skip if the texel is empty
        const std::vector<BakePoint>& bakePoints = *context.BakePoints;
        const BakePoint& bakePoint = bakePoints[texelIdx];
        if(bakePoint.Coverage == 0 || bakePoint.Coverage == 0xFFFFFFFF)
            return true;

        glm::float3x3 tangentFrame;
        tangentFrame.SetXBasis(bakePoint.Tangent);
        tangentFrame.SetYBasis(bakePoint.Bitangent);
        tangentFrame.SetZBasis(bakePoint.Normal);

        for(glm::uint64 sampleIdx = 0; sampleIdx < numSamplesPerTexel; ++sampleIdx)
        {
            IntegrationSampleSet sampleSet;
            sampleSet.Init(integrationSamples, groupTexelIdx, sampleIdx);

            // Create a random ray direction in tangent space, then convert to world space
            glm::float3 rayStart = bakePoint.Position;
            glm::float3 rayDirTS = baker.SampleDirection(sampleSet.MoYu::f::PIxel());
            glm::float3 rayDirWS = glm::float3::Transform(rayDirTS, tangentFrame);
            rayDirWS = glm::normalize(rayDirWS);

            glm::float3 sampleResult;

            glm::float2 directAreaLightSample = sampleSet.Lens();
            if(addAreaLight && directAreaLightSample.x >= 0.5f)
            {
                glm::float3 areaLightIrradiance;
                sampleResult += SampleAreaLight(bakePoint.Position, bakePoint.Normal, context.SceneBVH->Scene,
                                                1.0f, 0.0f, false, 0.0f, 1.0f, sampleSet.Lens().x,
                                                sampleSet.Lens().y, areaLightIrradiance, rayDirWS);
                rayDirTS = glm::float3::Transform(rayDirWS, glm::float3x3::Transpose(tangentFrame));
            }
            else
            {
                params.RayDir = rayDirWS;
                params.RayStart = rayStart + 0.1f * rayDirWS;
                params.RayLen = FLT_MAX;
                params.SampleSet = &sampleSet;

                float illuminance = 0.0f;
                bool hitSky = false;
                sampleResult = PathTrace(params, random, illuminance, hitSky);

                if(AppSettings::BakeDirectSunLight)
                {
                    glm::float3 sunLightIrradiance;
                    sampleResult += SampleSunLight(bakePoint.Position, bakePoint.Normal, context.SceneBVH->Scene,
                        1.0f, 0.0f, false, 0.0f, 1.0f, sampleSet.Lens().x,
                        sampleSet.Lens().y, sunLightIrradiance);
                }
            }

            // Account for equally distributing our samples among the area light and the rest of the environment
            if(addAreaLight)
                sampleResult *= 2.0f;

            if (!isfinite(sampleResult.x) || !isfinite(sampleResult.y) || !isfinite(sampleResult.z))
                sampleResult = 0.0;

            baker.AddSample(rayDirTS, sampleIdx, sampleResult, rayDirWS, bakePoint.Normal);
        }

        baker.FinalResult(texelResults);
        for(glm::uint64 basisIdx = 0; basisIdx < TBaker::BasisCount; ++basisIdx)
            context.BakeOutput[basisIdx][texelIdx] = texelResults[basisIdx];

        // Temporarily fill in the rest of the texels in the group
        for(glm::uint64 i = groupTexelIdx; i < BakeGroupSize; ++i)
        {
            const glm::uint64 offsetX = i % BakeGroupSizeX;
            const glm::uint64 offsetY = i / BakeGroupSizeX;
            const glm::uint64 neighborX = grouMoYu::f::PIdxX * BakeGroupSizeX + offsetX;
            const glm::uint64 neighborY = grouMoYu::f::PIdxY * BakeGroupSizeY + offsetY;
            if(neighborX >= context.CurrLightMapSize || neighborY >= context.CurrLightMapSize)
                continue;

            glm::uint64 neighborTexelIdx = neighborY * context.CurrLightMapSize + neighborX;
            for(glm::uint64 basisIdx = 0; basisIdx < TBaker::BasisCount; ++basisIdx)
                context.BakeOutput[basisIdx][neighborTexelIdx] = texelResults[basisIdx];
        }
    }

    return true;
}

// Data passed to the bake thread entry point
struct BakeThreadData
{
    FixedArray<glm::float4>* BakeOutput = nullptr;
    const std::vector<IntegrationSamples>* Samples = nullptr;
    volatile int64* CurrBatch = nullptr;
    const MeshBaker* Baker = nullptr;
};

// Entry point for a bake thread
template<typename TBaker> uint32 __stdcall BakeThread(void* data)
{
    BakeThreadData* threadData = reinterpret_cast<BakeThreadData*>(data);
    const MeshBaker* meshBaker = threadData->Baker;

    BakeThreadContext context;
    TBaker baker;

    while(meshBaker->killBakeThreads == false)
    {
        const glm::uint64 currTag = meshBaker->bakeTag;
        if(context.BakeTag != currTag)
            context.Init(threadData->BakeOutput, threadData->Samples,
                         threadData->CurrBatch, threadData->Baker, currTag);

        if(BakeDriver<TBaker>(context, baker) == false)
            Sleep(5);
    }

    return 0;
}


// Builds a BVH tree for an entire model/scene
static void BuildBVH(const Model& model, BVHData& bvhData, ID3D11Device* d3dDevice, RTCDevice device)
{
    if(bvhData.Scene != nullptr)
    {
        rtcDeleteScene(bvhData.Scene);
        bvhData.Scene = nullptr;
    }
    bvhData.Scene = rtcDeviceNewScene(device, RTC_SCENE_DYNAMIC, RTC_INTERSECT1);
    bvhData.Device = device;

    // Count the total number of vertices and triangles
    uint32 totalNumVertices = 0;
    uint32 totalNumTriangles = 0;
    for(glm::uint64 i = 0; i < model.Meshes().size(); ++i)
    {
        const Mesh& mesh = model.Meshes()[i];
        Assert_(mesh.VertexStride() == sizeof(Vertex));
        totalNumVertices += mesh.NumVertices();
        totalNumTriangles += mesh.NumIndices() / 3;
    }

    bvhData.Triangles.resize(totalNumTriangles);
    bvhData.Vertices.resize(totalNumVertices);
    bvhData.MaterialIndices.resize(totalNumTriangles);
    std::vector<glm::float4> vertices(totalNumVertices);

    uint32 vtxOffset = 0;
    uint32 triOffset = 0;

    // Add the data for each mesh
    for(glm::uint64 meshIdx = 0; meshIdx < model.Meshes().size(); ++meshIdx)
    {
        const Mesh& mesh = model.Meshes()[meshIdx];
        const Vertex* vertexData = reinterpret_cast<const Vertex*>(mesh.Vertices());
        const uint8* indexData = mesh.Indices();
        const uint32 numVertices = mesh.NumVertices();
        const uint32 numIndices = mesh.NumIndices();
        const uint32 indexSize = mesh.IndexSize();
        const uint32 vertexStride = mesh.VertexStride();

        // Prepare the triangles
        const uint32 numTriangles = numIndices / 3;
        for(glm::uint64 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
        {
            const MeshPart& meshPart = mesh.MeshParts()[partIdx];
            const uint32 startTriangle = meshPart.IndexStart / 3;
            const uint32 endTriangle = (meshPart.IndexStart + meshPart.IndexCount) / 3;
            for(uint32 i = startTriangle; i < endTriangle; ++i)
            {
                const uint32 idx0 = GetIndex(indexData, i * 3 + 0, indexSize) + vtxOffset;
                const uint32 idx1 = GetIndex(indexData, i * 3 + 1, indexSize) + vtxOffset;
                const uint32 idx2 = GetIndex(indexData, i * 3 + 2, indexSize) + vtxOffset;

                bvhData.Triangles[i + triOffset] = Uint3(idx0, idx1, idx2);
                bvhData.MaterialIndices[i + triOffset] = meshPart.MaterialIdx;
            }
        }

        // Prepare the vertices
        for(uint32 i = 0; i < numVertices; ++i)
        {
            const glm::float3& position = vertexData[i].Position;
            vertices[i + vtxOffset] = glm::float4(position, 0.0f);
            bvhData.Vertices[i + vtxOffset] = vertexData[i];
        }

        triOffset += numTriangles;
        vtxOffset += numVertices;
    }

    uint32 geoID = rtcNewTriangleMesh(bvhData.Scene, RTC_GEOMETRY_STATIC, totalNumTriangles, totalNumVertices);

    glm::float4* meshVerts = reinterpret_cast<glm::float4*>(rtcMapBuffer(bvhData.Scene, geoID, RTC_VERTEX_BUFFER));
    memcpy(meshVerts, vertices.data(), totalNumVertices * sizeof(glm::float4));
    rtcUnmapBuffer(bvhData.Scene, geoID, RTC_VERTEX_BUFFER);

    Uint3* meshTriangles = reinterpret_cast<Uint3*>(rtcMapBuffer(bvhData.Scene, geoID, RTC_INDEX_BUFFER));
    memcpy(meshTriangles, bvhData.Triangles.data(), totalNumTriangles * sizeof(Uint3));
    rtcUnmapBuffer(bvhData.Scene, geoID, RTC_INDEX_BUFFER);

    rtcCommit(bvhData.Scene);

    RTCError embreeError = rtcDeviceGetError(device);
    Assert_(embreeError == RTC_NO_ERROR);
    if(embreeError != RTC_NO_ERROR)
        throw Exception(L"Failed to build embree scene!");

    // Load the material texture data
    const glm::uint64 numMaterials = model.Materials().size();
    bvhData.MaterialDiffuseMaps.resize(numMaterials);
    bvhData.MaterialNormalMaps.resize(numMaterials);
    bvhData.MaterialRoughnessMaps.resize(numMaterials);
    bvhData.MaterialMetallicMaps.resize(numMaterials);

    for(glm::uint64 i = 0; i < numMaterials; ++i)
    {
        const MeshMaterial& material = model.Materials()[i];
        GetTextureData(d3dDevice, material.DiffuseMap, bvhData.MaterialDiffuseMaps[i]);
        GetTextureData(d3dDevice, material.NormalMap, bvhData.MaterialNormalMaps[i]);
        GetTextureData(d3dDevice, material.RoughnessMap, bvhData.MaterialRoughnessMaps[i]);
        GetTextureData(d3dDevice, material.MetallicMap, bvhData.MaterialMetallicMaps[i]);
    }
}

// Computes lightmap sample points and gutter texels
static void ExtractBakePoints(const BakeInputData& bakeInput, std::vector<BakePoint>& bakePoints,
                              std::vector<GutterTexel>& gutterTexels)
{
    const uint32 LightMapSize = AppSettings::LightMapResolution;
    const glm::uint64 NumTexels = LightMapSize * LightMapSize;

    bakePoints.clear();
    bakePoints.resize(NumTexels);
    gutterTexels.clear();

    Timer timer;
    PrintString("Extracting light map sample points...");

    ID3D11Device* device = bakeInput.Device;
    ID3D11DeviceContextPtr context;
    device->GetImmediateContext(&context);

    // Rasterize the mesh to the lightmap in UV space
    const uint32 NumTargets = 5;
    const DXGI_FORMAT RTFormats[NumTargets] =
    {
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32_UINT,
    };

    RenderTarget2D targets[NumTargets];
    RenderTarget2D msaaTargets[NumTargets];
    for(glm::uint64 i = 0; i < NumTargets; ++i)
    {
        targets[i].Initialize(bakeInput.Device, LightMapSize, LightMapSize, RTFormats[i]);
        msaaTargets[i].Initialize(bakeInput.Device, LightMapSize, LightMapSize, RTFormats[i], 1, 8, 0);
    }

    ID3D11RenderTargetView* rtViews[NumTargets];
    for(glm::uint64 i = 0; i < NumTargets; ++i)
        rtViews[i] = msaaTargets[i].RTView;

    context->OMSetRenderTargets(NumTargets, rtViews, nullptr);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    for(glm::uint64 i = 0; i < NumTargets; ++i)
        context->ClearRenderTargetView(rtViews[i], clearColor);

    VertexShaderPtr vs = ComMoYu::f::PIleVSFromFile(device, L"LightMapRasterization.hlsl");
    MoYu::f::PIxelShaderPtr ps = ComMoYu::f::PIlePSFromFile(device, L"LightMapRasterization.hlsl");

    context->VSSetShader(vs, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);

    RasterizerStates rasterizerStates;
    rasterizerStates.Initialize(device);
    context->RSSetState(rasterizerStates.NoCull());

    BlendStates blendStates;
    blendStates.Initialize(device);
    float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);

    DepthStencilStates dsStates;
    dsStates.Initialize(device);
    context->OMSetDepthStencilState(dsStates.DepthDisabled(), 0);

    D3D11_VIEWPORT viewport;
    viewport.Width = float(LightMapSize);
    viewport.Height = float(LightMapSize);
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    ConstantBuffer<uint32> constantBuffer;
    constantBuffer.Initialize(device);
    constantBuffer.SetPS(context, 0);

    uint32 vertexOffset = 0;

    const Model& model = *bakeInput.SceneModel;
    const std::vector<Mesh>& meshes = model.Meshes();
    for(glm::uint64 meshIdx = 0; meshIdx < meshes.size(); ++meshIdx)
    {
        const Mesh& mesh = meshes[meshIdx];

        ID3D11InputLayoutPtr inputLayout;
        DXCall(device->CreateInputLayout(mesh.InputElements(), mesh.NumInputElements(),
                                         vs->ByteCode->GetBufferPointer(),
                                         vs->ByteCode->GetBufferSize(), &inputLayout));
        context->IASetInputLayout(inputLayout);

        ID3D11Buffer* vertexBuffers[1] = { mesh.VertexBuffer() };
        UINT vertexStrides[1] = { mesh.VertexStride() };
        UINT offsets[1] = { 0 };
        context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
        context->IASetIndexBuffer(mesh.IndexBuffer(), mesh.IndexBufferFormat(), 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        constantBuffer.Data = vertexOffset;
        constantBuffer.ApplyChanges(context);

        context->DrawIndexed(mesh.NumIndices(), 0, 0);

        vertexOffset += mesh.NumVertices();
    }

    // Resolve the targets
    VertexShaderPtr resolveVS = ComMoYu::f::PIleVSFromFile(device, L"LightMapRasterization.hlsl", "ResolveVS");
    MoYu::f::PIxelShaderPtr resolvePS = ComMoYu::f::PIlePSFromFile(device, L"LightMapRasterization.hlsl", "ResolvePS");
    context->VSSetShader(resolveVS, nullptr, 0);
    context->PSSetShader(resolvePS, nullptr, 0);

    for(glm::uint64 i = 0; i < NumTargets; ++i)
        rtViews[i] = targets[i].RTView;
    context->OMSetRenderTargets(NumTargets, rtViews, nullptr);

    context->IASetInputLayout(nullptr);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11Buffer* vertexBuffers[1] = { nullptr };
    UINT vertexStrides[1] = { 0 };
    UINT offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);

    ID3D11ShaderResourceView* srViews[NumTargets];
    for(glm::uint64 i = 0; i < NumTargets; ++i)
        srViews[i] = msaaTargets[i].SRView;
    context->PSSetShaderResources(0, NumTargets, srViews);

    context->Draw(3, 0);

    context->VSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);

    for(glm::uint64 i = 0; i < NumTargets; ++i)
    {
        rtViews[i] = nullptr;
        srViews[i] = nullptr;
    }

    context->OMSetRenderTargets(NumTargets, rtViews, nullptr);
    context->PSSetShaderResources(0, NumTargets, srViews);

    // Read back the results, and extract the sample points
    StagingTexture2D stagingTextures[NumTargets];
    uint32 MoYu::f::PItches[5] = { 0 };
    const uint8* textureData[5] = { nullptr };
    for(glm::uint64 i = 0; i < NumTargets; ++i)
    {
        stagingTextures[i].Initialize(device, LightMapSize, LightMapSize, RTFormats[i]);
        context->CopyResource(stagingTextures[i].Texture, targets[i].Texture);
        textureData[i] = reinterpret_cast<uint8*>(stagingTextures[i].Map(context, 0, MoYu::f::PItches[i]));
    }

    for(uint32 y = 0; y < LightMapSize; ++y)
    {
        const glm::float4* positions = reinterpret_cast<const glm::float4*>(textureData[0] + y * MoYu::f::PItches[0]);
        const glm::float4* normals = reinterpret_cast<const glm::float4*>(textureData[1] + y * MoYu::f::PItches[1]);
        const glm::float4* tangents = reinterpret_cast<const glm::float4*>(textureData[2] + y * MoYu::f::PItches[2]);
        const glm::float4* bitangents = reinterpret_cast<const glm::float4*>(textureData[3] + y * MoYu::f::PItches[3]);
        const uint32* coverage = reinterpret_cast<const uint32*>(textureData[4] + y * MoYu::f::PItches[4]);
        for(uint32 x = 0; x < LightMapSize; ++x)
        {
            const glm::uint64 pointIdx = y * LightMapSize + x;
            BakePoint& bakePoint = bakePoints[pointIdx];

            if(coverage[x] != 0)
            {
                // Active texel, extract the relevent data from the rasterization result
                bakePoint.Position = positions[x].To3D();
                bakePoint.Normal = normals[x].To3D();
                bakePoint.Tangent = tangents[x].To3D();
                bakePoint.Bitangent = bitangents[x].To3D();
                bakePoint.Size = glm::float2(positions[x].w, normals[x].w);
                bakePoint.Coverage = coverage[x];
                bakePoint.TexelPos = glm::uint2(x, y);
                bakePoints.push_back(bakePoint);
            }
            else
            {
                // Check if this is a gutter texel that needs to replicate its value from a neighbor
                GutterTexel gutterTexel;
                gutterTexel.TexelPos = glm::uint2(x, y);
                int32 currDist = 0;
                bool foundNeighbor = false;

                // Empty texel, look for nearby active texels to see if we're a gutter texel
                for(int32 ny = -1; ny <= 1; ++ny)
                {
                    int32 neighborY = y + ny;
                    if(neighborY < 0 || neighborY >= int32(LightMapSize))
                        continue;

                    for(int32 nx = -1; nx <= 1; ++nx)
                    {
                        if(nx == 0 && ny == 0)
                            continue;

                        int32 neighborX = x + nx;
                        if(neighborX < 0 || neighborX >= int32(LightMapSize))
                            continue;

                        int32 dist = std::abs(nx) + std::abs(ny);
                        if(foundNeighbor && dist >= currDist)
                            continue;

                        int32 offset = neighborY * MoYu::f::PItches[4] + neighborX * sizeof(uint32);
                        const uint32 neighborCoverage = *reinterpret_cast<const uint32*>(textureData[4] + offset);
                        if(neighborCoverage != 0)
                        {
                            gutterTexel.NeighborPos = glm::uint2(neighborX, neighborY);
                            foundNeighbor = true;
                            currDist = dist;
                        }
                    }
                }

                if(foundNeighbor)
                {
                    // Mark it as a gutter texel
                    bakePoint.Coverage = 0xFFFFFFFF;
                    bakePoint.TexelPos = gutterTexel.NeighborPos;
                    gutterTexels.push_back(gutterTexel);
                }
            }
        }
    }

    for(glm::uint64 i = 0; i < NumTargets; ++i)
        stagingTextures[i].Unmap(context, 0);


    timer.Update();
    PrintString("Finished! (%fs)", timer.DeltaSecondsF());
}

// == Ground Truth Rendering ======================================================================

// Data uses by the ground truth render thread
struct RenderThreadContext
{
    glm::uint64 RenderTag = glm::uint64(-1);
    SkyCache SkyCache;
    const BVHData* SceneBVH = nullptr;
    const TextureData<Half4>* EnvMaps = nullptr;
    uint32 OutputWidth;
    uint32 OutputHeight;
    glm::float3 CameraPos;
    Quaternion CameraOrientation;
    glm::float4x4 Proj;
    glm::float4x4 ViewProjInv;
    glm::uint64 CurrNumTiles;
    Random RandomGenerator;
    SampleModes CurrSampleMode = SampleModes::Random;
    glm::uint64 CurrNumSamples = 0;
    const std::vector<IntegrationSamples>* Samples;
    FixedArray<Half4>* RenderBuffer = nullptr;
    FixedArray<float>* RenderWeightBuffer = nullptr;
    volatile int64* CurrTile = nullptr;

    void Init(FixedArray<Half4>* renderBuffer, FixedArray<float>* renderWeightBuffer,
              const std::vector<IntegrationSamples>* samples,
              volatile int64* currTile, const MeshBaker* meshBaker, glm::uint64 newTag)
    {
        if(RenderTag == glm::uint64(-1))
            RandomGenerator.SeedWithRandomValue();

        RenderTag = newTag;
        SkyCache.Init(AppSettings::SunDirection, AppSettings::GroundAlbedo, AppSettings::Turbidity);
        SceneBVH = &meshBaker->sceneBVH;
        EnvMaps = meshBaker->input.EnvMapData;
        OutputWidth = meshBaker->currWidth;
        OutputHeight = meshBaker->currHeight;
        CameraPos = meshBaker->currCameraPos;
        CameraOrientation = meshBaker->currCameraOrientation;
        Proj = meshBaker->currProj;
        ViewProjInv = meshBaker->currViewProjInv;
        CurrNumTiles = meshBaker->currNumTiles;
        RenderBuffer = renderBuffer;
        RenderWeightBuffer = renderWeightBuffer;
        CurrTile = currTile;
        CurrSampleMode = AppSettings::RenderSampleMode;
        CurrNumSamples = AppSettings::NumRenderSamples;
        Samples = samples;
    }
};

// Runs a single iteration of the ground truth render thread. This function will compute
// a single radiance for every MoYu::f::PIxel within a tile, and blend with with the previous result.
static bool RenderDriver(RenderThreadContext& context)
{
    if(context.CurrNumTiles == 0)
        return false;

    const glm::uint64 tileIdx = InterlockedIncrement64(context.CurrTile) - 1;
    const glm::uint64 passIdx = tileIdx / context.CurrNumTiles;
    const glm::uint64 passTileIdx = tileIdx % context.CurrNumTiles;

    const glm::uint64 sqrtNumSamples = context.CurrNumSamples;
    const glm::uint64 numSamplesPerMoYu::f::PIxel = sqrtNumSamples * sqrtNumSamples;
    if(passIdx >= numSamplesPerMoYu::f::PIxel)
        return false;

    const glm::uint64 numMoYu::f::PIxelsPerTile = TileSize * TileSize;

    const glm::float4x4 viewProjInv = context.ViewProjInv;
    const glm::uint64 screenWidth = context.OutputWidth;
    const glm::uint64 screenHeight = context.OutputHeight;
    const glm::uint64 numTilesX = (screenWidth + (TileSize - 1)) / TileSize;
    const glm::uint64 numTilesY = (screenHeight + (TileSize - 1)) / TileSize;
    const glm::uint64 tileX = passTileIdx % numTilesX;
    const glm::uint64 tileY = passTileIdx / numTilesX;
    const glm::uint64 startX = tileX * TileSize;
    const glm::uint64 startY = tileY * TileSize;
    const glm::uint64 endX = std::min(startX + TileSize, screenWidth);
    const glm::uint64 endY = std::min(startY + TileSize, screenHeight);

    const glm::float3x3 cameraRot = context.CameraOrientation.Toglm::float3x3();
    const glm::float3 cameraX = cameraRot.Right();
    const glm::float3 cameraY = cameraRot.Up();

    const float focusDistance = AppSettings::FocusDistance;
    const float apertureSize = AppSettings::ApertureWidth;

    const float numSides = float(AppSettings::NumBlades);
    const float polyAmt = AppSettings::BokehPolygonAmount;

    const glm::uint64 passIdxX = passIdx % sqrtNumSamples;
    const glm::uint64 passIdxY = passIdx / sqrtNumSamples;

    const bool32 enableDOF = AppSettings::EnableDOF;

    const glm::uint64 numThreads = context.Samples->size();
    const IntegrationSamples& samples = (*context.Samples)[passTileIdx % numThreads];

    const int32 pathLength = AppSettings::EnableIndirectLighting ? AppSettings::MaxRenderPathLength : 2;

    glm::uint64 tileMoYu::f::PIxelIdx = 0;
    for(glm::uint64 y = startY; y < endY; ++y)
    {
        for(glm::uint64 x = startX; x < endX; ++x)
        {
            const glm::uint64 MoYu::f::PIxelIdx = (y * screenWidth + x);
            IntegrationSampleSet sampleSet;
            sampleSet.Init(samples, tileMoYu::f::PIxelIdx, passIdx);

            glm::float2 MoYu::f::PIxelSample = sampleSet.MoYu::f::PIxel();

            glm::float3 rayStart;
            glm::float3 rayDir;
            if(enableDOF)
            {
                // MoYu::f::PIck a random point on the lens to use for sampling. Use a mapMoYu::f::PIng from unit square
                // to disc/polygon. We negate the position to make the sampling match the gather
                // pattern used in the real-time shader.
                glm::float2 lensSample = sampleSet.Lens();
                const glm::float2 lensPos = SquareToConcentricDiskMapMoYu::f::PIng(lensSample.x, lensSample.y, numSides, polyAmt) * apertureSize * -1.0f;

                // Set up a ray going from the sensor through the lens.
                rayStart = context.CameraPos + lensPos.x * cameraX + lensPos.y * cameraY;
                float ncdX = (x + MoYu::f::PIxelSample.x) / (screenWidth / 2.0f) - 1.0f;
                float ncdY = ((y + MoYu::f::PIxelSample.y) / (screenHeight / 2.0f) - 1.0f) * -1.0f;
                float ncdZ = glm::float3::Transform(glm::float3(0.0f, 0.0f, focusDistance), context.Proj).z;
                glm::float3 focusPoint = glm::float3::Transform(glm::float3(ncdX, ncdY, ncdZ), viewProjInv);
                rayDir = glm::normalize(focusPoint - rayStart);
            }
            else
            {
                float ncdX = (x + MoYu::f::PIxelSample.x) / (screenWidth / 2.0f) - 1.0f;
                float ncdY = ((y + MoYu::f::PIxelSample.y) / (screenHeight / 2.0f) - 1.0f) * -1.0f;
                rayStart = glm::float3::Transform(glm::float3(ncdX, ncdY, 0.0f), viewProjInv);
                glm::float3 rayEnd = glm::float3::Transform(glm::float3(ncdX, ncdY, 1.0f), viewProjInv);
                rayDir = glm::normalize(rayEnd - rayStart);
            }

            float illuminance = 0.0f;
            bool hitSky;
            PathTracerParams params;
            params.RayDir = rayDir;
            params.RayStart = rayStart;
            params.RayLen = FLT_MAX;
            params.SceneBVH = context.SceneBVH;
            params.SampleSet = &sampleSet;
            params.SkyCache = &context.SkyCache;
            params.EnvMaps = context.EnvMaps;
            params.EnableDirectAreaLight = true;
            params.EnableDirectSun = true;
            params.EnableDiffuse = AppSettings::EnableDiffuse;
            params.EnableSpecular = AppSettings::EnableSpecular;
            params.EnableBounceSpecular = uint8(AppSettings::EnableRenderBounceSpecular);
            params.ViewIndirectSpecular = uint8(AppSettings::ViewIndirectSpecular);
            params.ViewIndirectDiffuse = uint8(AppSettings::ViewIndirectDiffuse);
            params.MaxPathLength = pathLength;
            params.RussianRouletteDepth = AppSettings::RenderRussianRouletteDepth;
            params.RussianRouletteProbability = AppSettings::RenderRussianRouletteProbability;
            glm::float3 radiance = PathTrace(params, context.RandomGenerator, illuminance, hitSky);

            FixedArray<Half4>& renderBuffer = *context.RenderBuffer;
            FixedArray<float>& renderWeightBuffer = *context.RenderWeightBuffer;
            glm::float4 oldValue = renderBuffer[MoYu::f::PIxelIdx].Toglm::float4();

            float oldWeight = passIdx > 0 ? renderWeightBuffer[MoYu::f::PIxelIdx] : 0.0f;
            glm::float4 newValue = (oldValue * oldWeight) + glm::float4(radiance, illuminance);
            float newWeight = oldWeight + 1.0f;
            renderBuffer[MoYu::f::PIxelIdx] = glm::float4::Clamp(newValue / newWeight, 0.0f, FLT_MAX);
            renderWeightBuffer[MoYu::f::PIxelIdx] = newWeight;

            ++tileMoYu::f::PIxelIdx;
        }
    }

    return true;
}

// Data passed to the ground truth render thread
struct RenderThreadData
{
    FixedArray<Half4>* RenderBuffer = nullptr;
    FixedArray<float>* RenderWeightBuffer = nullptr;
    const std::vector<IntegrationSamples>* Samples = nullptr;
    volatile int64* CurrTile = nullptr;
    const MeshBaker* Baker = nullptr;
};

// Entry point for the ground truth render thread
static uint32 __stdcall RenderThread(void* data)
{
    RenderThreadData* threadData = reinterpret_cast<RenderThreadData*>(data);
    const MeshBaker* meshBaker = threadData->Baker;

    RenderThreadContext context;

    while(meshBaker->killRenderThreads == false)
    {
        const glm::uint64 currTag = meshBaker->renderTag;
        if(context.RenderTag != currTag)
            context.Init(threadData->RenderBuffer, threadData->RenderWeightBuffer, threadData->Samples,
                         threadData->CurrTile, threadData->Baker, currTag);

        if(RenderDriver(context) == false)
            Sleep(5);
    }

    return 0;
}

// == MeshBaker ===================================================================================

static glm::uint64 GetNumThreads()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return std::max<glm::uint64>(1, sysInfo.dwNumberOfProcessors - 1);
}

MeshBaker::MeshBaker()
{
}

MeshBaker::~MeshBaker()
{
    Shutdown();
}

void MeshBaker::Initialize(const BakeInputData& inputData)
{
    input = inputData;
    for(glm::uint64 i = 0; i < AppSettings::NumCubeMaps; ++i)
        GetTextureData(input.Device, input.EnvMaps[i], input.EnvMapData[i]);

     // Init embree
    rtcDevice = rtcNewDevice();
    RTCError embreeError = rtcDeviceGetError(rtcDevice);
    if(embreeError == RTC_UNSUPPORTED_CPU)
        throw Exception(L"Your CPU does not meet the minimum requirements for embree");
    else if(embreeError != RTC_NO_ERROR)
        throw Exception(L"Failed to initialize embree!");

    // Build the BVHs
    BuildBVH(*input.SceneModel, sceneBVH, input.Device, rtcDevice);

    renderSampleMode = AppSettings::RenderSampleMode;
    numRenderSamples = AppSettings::NumRenderSamples;

    bakeSampleMode = AppSettings::BakeSampleMode;
    numBakeSamples = AppSettings::NumBakeSamples;

    numThreads = GetNumThreads();
    renderSamples.resize(numThreads);
    bakeSamples.resize(numThreads);

    for(glm::uint64 i = 0; i < numThreads; ++i)
    {
        GenerateIntegrationSamples(renderSamples[i], numRenderSamples, TileSize, TileSize,
                                   renderSampleMode, NumIntegrationTypes, rng);

        GenerateIntegrationSamples(bakeSamples[i], numBakeSamples, BakeGroupSize, 1,
                                   bakeSampleMode, NumIntegrationTypes, rng);
    }

    initialized = true;
}

void MeshBaker::Shutdown()
{
    if(initialized == false)
        return;

    KillBakeThreads();
    KillRenderThreads();

    // Shutdown embree
    sceneBVH = BVHData();
    rtcDeleteDevice(rtcDevice);
    rtcDevice = nullptr;
}

MeshBakerStatus MeshBaker::Update(const Camera& camera, uint32 screenWidth, uint32 screenHeight,
                                  ID3D11DeviceContext* deviceContext, const Model* currentModel)
{
    Assert_(initialized);

    const bool32 showGroundTruth = AppSettings::ShowGroundTruth;

    if(currentModel != input.SceneModel)
    {
        KillBakeThreads();
        KillRenderThreads();

        sceneBVH = BVHData();
        input.SceneModel = currentModel;
        BuildBVH(*input.SceneModel, sceneBVH, input.Device, rtcDevice);

        InterlockedIncrement64(&renderTag);
        InterlockedIncrement64(&bakeTag);
        currTile = 0;
        currBakeBatch = 0;

        // Make sure that we re-extract the lightmap data
        currLightMapSize = 0;
    }

    if(showGroundTruth == false)
    {
        // Handle light map size change, which requires re-extraction of sample points
        const uint32 lightMapSize = AppSettings::LightMapResolution;
        const BakeModes bakeMode = AppSettings::BakeMode;
        const SolveModes solveMode = AppSettings::SolveMode;
        if(lightMapSize != currLightMapSize || bakeMode != currBakeMode || solveMode != currSolveMode || AppSettings::WorldSpaceBake.Changed())
        {
            KillBakeThreads();
            KillRenderThreads();

            ExtractBakePoints(input, bakePoints, gutterTexels);
            bakePointBuffer.Initialize(input.Device, sizeof(BakePoint), uint32(bakePoints.size()),
                                       false, false, false, bakePoints.data());

            const glm::uint64 basisCount = AppSettings::BasisCount(bakeMode);
            const glm::uint64 numTexels = lightMapSize * lightMapSize;
            for(glm::uint64 i = 0; i < AppSettings::MaxBasisCount; ++i)
                bakeResults[i].Shutdown();

            for(glm::uint64 i = 0; i < basisCount; ++i)
                bakeResults[i].Init(numTexels);

            const glm::uint64 numGroupsX = (lightMapSize + (BakeGroupSizeX - 1)) / BakeGroupSizeX;
            const glm::uint64 numGroupsY = (lightMapSize + (BakeGroupSizeY - 1)) / BakeGroupSizeY;
            if(AppSettings::SupportsProgressiveIntegration(bakeMode, solveMode))
                currNumBakeBatches = numGroupsX * numGroupsY * AppSettings::NumBakeSamples * AppSettings::NumBakeSamples;
            else
                currNumBakeBatches = numGroupsX * numGroupsY * BakeGroupSize;

            currLightMapSize = lightMapSize;
            currBakeMode = bakeMode;
            currSolveMode = solveMode;
            InterlockedIncrement64(&bakeTag);
            currBakeBatch = 0;

            const glm::uint64 sgCount = AppSettings::SGCount(currBakeMode);
            SGDistribution distribution = AppSettings::WorldSpaceBake ? SGDistribution::Spherical : SGDistribution::Hemispherical;
            if(sgCount > 0)
                InitializeSGSolver(sgCount, distribution);

            const SG* initalGuess = InitialGuess();
            sgSharpness = initalGuess[0].Sharpness;
            for(glm::uint64  i = 0; i < sgCount; ++i)
                sgDirections[i] = initalGuess[i].Axis;

            D3D11_TEXTURE2D_DESC texDesc;
            texDesc.Width = lightMapSize;
            texDesc.Height = lightMapSize;
            texDesc.ArraySize = uint32(basisCount);
            texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            texDesc.CPUAccessFlags = 0;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.SampleDesc.Count = 1;
            texDesc.SampleDesc.Quality = 0;
            texDesc.MipLevels = 1;
            texDesc.MiscFlags = 0;

            bakeTexture = nullptr;
            bakeTextureSRV = nullptr;
            DXCall(input.Device->CreateTexture2D(&texDesc, nullptr, &bakeTexture));

            // Force the SRV to be a texture array view
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = texDesc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MostDetailedMip = 0;
            srvDesc.Texture2DArray.MipLevels = 1;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.ArraySize = texDesc.ArraySize;
            DXCall(input.Device->CreateShaderResourceView(bakeTexture, &srvDesc, &bakeTextureSRV));

            // Create staging textures to use for updating the bake texture
            D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
            stagingDesc.Usage = D3D11_USAGE_STAGING;
            stagingDesc.BindFlags = 0;
            stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            stagingDesc.ArraySize = 1;
            for(glm::uint64 i = 0; i < NumStagingTextures; ++i)
                DXCall(input.Device->CreateTexture2D(&stagingDesc, nullptr, &bakeStagingTextures[i]));
        }

        if(AppSettings::BakeSampleMode != bakeSampleMode || AppSettings::NumBakeSamples != numBakeSamples)
        {
            bakeSampleMode = AppSettings::BakeSampleMode;
            numBakeSamples = AppSettings::NumBakeSamples;

            KillBakeThreads();
            KillRenderThreads();

            for(glm::uint64 i = 0; i < numThreads; ++i)
                GenerateIntegrationSamples(bakeSamples[i], numBakeSamples, BakeGroupSize, 1,
                                           bakeSampleMode, NumIntegrationTypes, rng);

            const glm::uint64 numGroupsX = (lightMapSize + (BakeGroupSizeX - 1)) / BakeGroupSizeX;
            const glm::uint64 numGroupsY = (lightMapSize + (BakeGroupSizeY - 1)) / BakeGroupSizeY;
            if(AppSettings::SupportsProgressiveIntegration(bakeMode, solveMode))
                currNumBakeBatches = numGroupsX * numGroupsY * AppSettings::NumBakeSamples * AppSettings::NumBakeSamples;
            else
                currNumBakeBatches = numGroupsX * numGroupsY * BakeGroupSize;

            InterlockedIncrement64(&bakeTag);
            currBakeBatch = 0;
        }
    }
    else
    {
        // Handle screen resize, which requires resizing render buffers
        if(screenWidth != currWidth || screenHeight != currHeight)
        {
            KillBakeThreads();
            KillRenderThreads();

            currWidth = screenWidth;
            currHeight = screenHeight;

            D3D11_TEXTURE2D_DESC texDesc;
            texDesc.Width = currWidth;
            texDesc.Height = currHeight;
            texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            texDesc.ArraySize = 1;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.MipLevels = 1;
            texDesc.SampleDesc.Quality = 0;
            texDesc.SampleDesc.Count = 1;
            texDesc.CPUAccessFlags = 0;
            texDesc.MiscFlags = 0;

            DXCall(input.Device->CreateTexture2D(&texDesc, NULL, &renderTexture));
            DXCall(input.Device->CreateShaderResourceView(renderTexture, NULL, &renderTextureSRV));

              // Create staging textures to use for updating the render texture
            D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
            stagingDesc.Usage = D3D11_USAGE_STAGING;
            stagingDesc.BindFlags = 0;
            stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            for(glm::uint64 i = 0; i < NumStagingTextures; ++i)
                DXCall(input.Device->CreateTexture2D(&stagingDesc, nullptr, &renderStagingTextures[i]));

            glm::uint64 numTilesX = (screenWidth + (TileSize - 1)) / TileSize;
            glm::uint64 numTilesY = (screenHeight + (TileSize - 1)) / TileSize;
            glm::uint64 numTiles = numTilesX * numTilesY;
            currNumTiles = numTiles;

            currViewProjInv = glm::float4x4::Invert(camera.ViewProjectionMatrix());

            InterlockedIncrement64(&renderTag);
            currTile = 0;

            const glm::uint64 numMoYu::f::PIxels = numTiles * TileSize * TileSize;
            renderBuffer.Init(numMoYu::f::PIxels);
            renderWeightBuffer.Init(numMoYu::f::PIxels);
            renderWeightBuffer.Fill(0.0f);

        }

        if(AppSettings::RenderSampleMode != renderSampleMode || AppSettings::NumRenderSamples != numRenderSamples)
        {
            renderSampleMode = AppSettings::RenderSampleMode;
            numRenderSamples = AppSettings::NumRenderSamples;

            KillBakeThreads();
            KillRenderThreads();

            for(glm::uint64 i = 0; i < numThreads; ++i)
                GenerateIntegrationSamples(renderSamples[i], numRenderSamples, TileSize, TileSize,
                                           renderSampleMode, NumIntegrationTypes, rng);

            InterlockedIncrement64(&renderTag);
            currTile = 0;
        }
    }

    if(showGroundTruth)
    {
        KillBakeThreads();
        StartRenderThreads();
    }
    else
    {
        KillRenderThreads();
        StartBakeThreads();
    }

    // Change checks common to bake and ground truth
    if(AppSettings::AreaLightColor.Changed() || AppSettings::AreaLightSize.Changed()
        || AppSettings::AreaLightX.Changed() || AppSettings::AreaLightY.Changed()
        || AppSettings::AreaLightZ.Changed() || AppSettings::EnableAreaLight.Changed()
        || AppSettings::SkyMode.Changed() || AppSettings::EnableSun.Changed()
        || AppSettings::SkyColor.Changed() || AppSettings::GroundAlbedo.Changed()
        || AppSettings::Turbidity.Changed() || AppSettings::HasSunDirChanged()
        || AppSettings::SunTintColor.Changed() || AppSettings::SunIntensityScale.Changed()
        || AppSettings::SunSize.Changed() || AppSettings::NormalizeSunIntensity.Changed()
        || AppSettings::DiffuseAlbedoScale.Changed() || AppSettings::EnableAlbedoMaps.Changed()
        || AppSettings::EnableAreaLightShadows.Changed() || AppSettings::MetallicOffset.Changed()
        || AppSettings::BakeDirectSunLight.Changed() || AppSettings::BakeDirectAreaLight.Changed())
    {
        InterlockedIncrement64(&renderTag);
        InterlockedIncrement64(&bakeTag);
        currTile = 0;
        currBakeBatch = 0;
    }

    // Change checks for baking only
    if(AppSettings::BakeDirectSunLight.Changed() || AppSettings::BakeDirectAreaLight.Changed()
        || AppSettings::BakeRussianRouletteDepth.Changed() || AppSettings::BakeRussianRouletteProbability.Changed()
        || AppSettings::MaxBakePathLength.Changed() || AppSettings::SolveMode.Changed())
    {
        InterlockedIncrement64(&bakeTag);
        currBakeBatch = 0;
    }

    // Change checks for ground truth render only
    if(currCameraPos != camera.Position() || currCameraOrientation != camera.Orientation() || currProj != camera.ProjectionMatrix())
    {
        currCameraPos = camera.Position();
        currCameraOrientation = camera.Orientation();
        currProj = camera.ProjectionMatrix();
        currViewProjInv = glm::float4x4::Invert(camera.ViewProjectionMatrix());

        InterlockedIncrement64(&renderTag);
        currTile = 0;
    }

    if(AppSettings::EnableNormalMaps.Changed() || AppSettings::EnableDirectLighting.Changed()
        || AppSettings::EnableIndirectLighting.Changed() || AppSettings::RoughnessScale.Changed()
        || AppSettings::EnableIndirectDiffuse.Changed() || AppSettings::EnableIndirectSpecular.Changed()
        || AppSettings::NormalMaMoYu::f::PIntensity.Changed() || AppSettings::NumRenderSamples.Changed()
        || AppSettings::RenderSampleMode.Changed() || AppSettings::FocalLength.Changed()
        || AppSettings::FilmSize.Changed() || AppSettings::EnableDOF.Changed()
        || AppSettings::FocalLength.Changed() || AppSettings::NumBlades.Changed()
        || AppSettings::ApertureSize.Changed() || AppSettings::FocusDistance.Changed()
        || AppSettings::RenderRussianRouletteDepth.Changed() || AppSettings::RenderRussianRouletteProbability.Changed()
        || AppSettings::EnableRenderBounceSpecular.Changed() || AppSettings::MaxRenderPathLength.Changed()
        || AppSettings::EnableDiffuse.Changed() || AppSettings::EnableSpecular.Changed()
        || AppSettings::ViewIndirectSpecular.Changed() || AppSettings::ViewIndirectDiffuse.Changed()
        || AppSettings::RoughnessOverride.Changed())
    {
        InterlockedIncrement64(&renderTag);
        currTile = 0;
    }

    MeshBakerStatus status;
    status.GroundTruth = renderTextureSRV;
    status.LightMap = bakeTextureSRV;
    status.BakePoints = bakePointBuffer.SRView;
    status.NumBakePoints = bakePointBuffer.NumElements;

    const glm::uint64 sgCount = AppSettings::SGCount(currBakeMode);
    for(glm::uint64 i = 0; i < sgCount; ++i)
        status.SGDirections[i] = sgDirections[i];
    status.SGSharpness = sgCount > 0 ? sgSharpness : 0.0f;

    if(showGroundTruth)
    {
        const glm::uint64 numPasses = AppSettings::NumRenderSamples * AppSettings::NumRenderSamples;
        status.GroundTruthProgress = Saturate(currTile / float(numPasses * currNumTiles));
        status.BakeProgress = 1.0f;
        if(lastTileNum < currTile)
            status.GroundTruthSampleCount = (currTile - lastTileNum) * (TileSize * TileSize);
        lastTileNum = currTile;

        renderStagingTextureIdx = (renderStagingTextureIdx + 1) % NumStagingTextures;
        ID3D11Texture2D* stagingTexture = renderStagingTextures[renderStagingTextureIdx];

        D3D11_MAPPED_SUBRESOURCE mapped;
        ZeroMemory(&mapped, sizeof(mapped));
        if(SUCCEEDED(deviceContext->Map(stagingTexture, 0, D3D11_MAP_WRITE, 0, &mapped)))
        {
            uint8* dst = reinterpret_cast<uint8*>(mapped.pData);
            const Half4* src = renderBuffer.Data();
            for(glm::uint64 y = 0; y < screenHeight; ++y)
            {
                memcpy(dst, src, sizeof(Half4) * screenWidth);
                dst += mapped.RowMoYu::f::PItch;
                src += screenWidth;
            }

            deviceContext->Unmap(stagingTexture, 0);
        }

        deviceContext->CopyResource(renderTexture, stagingTexture);
    }
    else
    {
        const uint32 LightMapSize = AppSettings::LightMapResolution;
        const glm::uint64 numPasses = AppSettings::NumBakeSamples * AppSettings::NumBakeSamples;
        status.BakeProgress = Saturate(currBakeBatch / (currNumBakeBatches - 1.0f));
        status.GroundTruthProgress = 1.0f;
        lastTileNum = INT64_MAX;

        const glm::uint64 basisCount = AppSettings::BasisCount(currBakeMode);
        bakeStagingTextureIdx = (bakeStagingTextureIdx + 1) % NumStagingTextures;
        bakeTextureUpdateIdx = (bakeTextureUpdateIdx + 1) % basisCount;
        ID3D11Texture2D* stagingTexture = bakeStagingTextures[bakeStagingTextureIdx];

        D3D11_MAPPED_SUBRESOURCE mapped;
        ZeroMemory(&mapped, sizeof(mapped));
        if(SUCCEEDED(deviceContext->Map(stagingTexture, 0, D3D11_MAP_WRITE, 0, &mapped)))
        {
            Half4* dst = reinterpret_cast<Half4*>(mapped.pData);
            const glm::float4* src = bakeResults[bakeTextureUpdateIdx].Data();
            for(glm::uint64 y = 0; y < LightMapSize; ++y)
            {
                for(glm::uint64 x = 0; x < LightMapSize; ++x)
                    dst[x] = Half4(src[x]);

                dst += mapped.RowMoYu::f::PItch / sizeof(Half4);
                src += LightMapSize;
            }

            const glm::uint64 numGutterTexels = gutterTexels.size();
            for(glm::uint64 i = 0; i < numGutterTexels; ++i)
            {
                const GutterTexel& gutterTexel = gutterTexels[i];
                const glm::uint64 srcIdx = gutterTexel.NeighborPos.y * LightMapSize + gutterTexel.NeighborPos.x;
                uint8* gutterDst = reinterpret_cast<uint8*>(mapped.pData) + gutterTexel.TexelPos.y * mapped.RowMoYu::f::PItch;
                gutterDst += gutterTexel.TexelPos.x * sizeof(Half4);
                const Half4& gutterSrc = bakeResults[bakeTextureUpdateIdx][srcIdx];
                *reinterpret_cast<Half4*>(gutterDst) = gutterSrc;
            }

            deviceContext->Unmap(stagingTexture, 0);
        }

        deviceContext->CopySubresourceRegion(bakeTexture, uint32(bakeTextureUpdateIdx), 0, 0, 0, stagingTexture, 0, nullptr);
    }

    Sleep(0);

    return status;
}

void MeshBaker::KillBakeThreads()
{
    if(bakeThreadsSuspended)
        return;

    Assert_(killBakeThreads == false);
    killBakeThreads = true;
    for(glm::uint64 i = 0; i < bakeThreads.size(); ++i)
    {
        WaitForSingleObject(bakeThreads[i], INFINITE);
        CloseHandle(bakeThreads[i]);
    }

    bakeThreads.clear();
    bakeThreadData.clear();
    killBakeThreads = false;
    bakeThreadsSuspended = true;
}

void MeshBaker::StartBakeThreads()
{
    if(bakeThreadsSuspended == false)
        return;

    Assert_(killBakeThreads == false);
    if(bakeThreads.size() > 0)
        return;

    uint32 (__stdcall* threadFunction)(void*) = BakeThread<DiffuseBaker>;
    if(currBakeMode == BakeModes::HL2)
        threadFunction = BakeThread<HL2Baker>;
    else if (currBakeMode == BakeModes::Directional)
        threadFunction = BakeThread<DirectionalBaker>;
    else if(currBakeMode == BakeModes::DirectionalRGB)
        threadFunction = BakeThread<DirectionalRGBBaker>;
    else if(currBakeMode == BakeModes::SH4)
        threadFunction = BakeThread<SH4Baker>;
    else if(currBakeMode == BakeModes::SH9)
        threadFunction = BakeThread<SH9Baker>;
    else if(currBakeMode == BakeModes::H4)
        threadFunction = BakeThread<H4Baker>;
    else if(currBakeMode == BakeModes::H6)
        threadFunction = BakeThread<H6Baker>;
    else if(currBakeMode == BakeModes::SG5)
        threadFunction = BakeThread<SG5Baker>;
    else if(currBakeMode == BakeModes::SG6)
        threadFunction = BakeThread<SG6Baker>;
    else if(currBakeMode == BakeModes::SG9)
        threadFunction = BakeThread<SG9Baker>;
    else if(currBakeMode == BakeModes::SG12)
        threadFunction = BakeThread<SG12Baker>;

    bakeThreads.resize(numThreads);
    bakeThreadData.resize(numThreads);
    for(glm::uint64 i = 0; i < numThreads; ++i)
    {
        BakeThreadData* threadData = &bakeThreadData[i];
        threadData->BakeOutput = bakeResults;
        threadData->Samples = &bakeSamples;
        threadData->CurrBatch = &currBakeBatch;
        threadData->Baker = this;
        bakeThreads[i] = HANDLE(_beginthreadex(nullptr, 0, threadFunction, threadData, 0, nullptr));
        if(bakeThreads[i] == 0)
        {
            AssertFail_("Failed to create thread for light map baking");
            throw Exception(L"Failed to create thread for light map baking");
        }
    }

    bakeThreadsSuspended = false;
}

void MeshBaker::KillRenderThreads()
{
    if(renderThreadsSuspended)
        return;

    Assert_(killRenderThreads == false);
    killRenderThreads = true;
    for(glm::uint64 i = 0; i < renderThreads.size(); ++i)
    {
        WaitForSingleObject(renderThreads[i], INFINITE);
        CloseHandle(renderThreads[i]);
    }

    renderThreads.clear();
    renderThreadData.clear();
    killRenderThreads = false;
    renderThreadsSuspended = true;
}

void MeshBaker::StartRenderThreads()
{
    if(renderThreadsSuspended == false)
        return;

    Assert_(killRenderThreads == false);
    if(renderThreads.size() > 0)
        return;

    renderThreads.resize(numThreads);
    renderThreadData.resize(numThreads);
    for(glm::uint64 i = 0; i < numThreads; ++i)
    {
        RenderThreadData* threadData = &renderThreadData[i];
        threadData->RenderBuffer = &renderBuffer;
        threadData->RenderWeightBuffer = &renderWeightBuffer;
        threadData->Samples = &renderSamples;
        threadData->CurrTile = &currTile;
        threadData->Baker = this;
        renderThreads[i] = HANDLE(_beginthreadex(nullptr, 0, RenderThread, threadData, 0, nullptr));
        if(renderThreads[i] == 0)
        {
            AssertFail_("Failed to create thread for ground truth rendering");
            throw Exception(L"Failed to create thread for ground truth rendering");
        }
    }

    renderThreadsSuspended = false;
}
*/