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
#include "../graphics/Textures.h"
#include "../graphics/Skybox.h"
#include "BakingLabSettings.h"
#include "embree4/rtcore.h"

using namespace SampleFramework11;

// Single vertex in the mesh vertex buffer
struct Vertex
{
    glm::float3 Position;
    glm::float3 Normal;
    glm::float2 TexCoord;
    glm::float2 LightMapUV;
    glm::float3 Tangent;
    glm::float3 Bitangent;
};

// Data returned after building a BVH
struct BVHData
{
    RTCDevice Device = nullptr;
    RTCScene Scene = nullptr;
    std::vector<glm::int3> Triangles;
    std::vector<Vertex> Vertices;
    std::vector<glm::uint32> MaterialIndices;
    std::vector<MoYu::MoYuScratchImage> MaterialDiffuseMaps;
    std::vector<MoYu::MoYuScratchImage> MaterialNormalMaps;
    std::vector<MoYu::MoYuScratchImage> MaterialRoughnessMaps;
    std::vector<MoYu::MoYuScratchImage> MaterialMetallicMaps;

    ~BVHData() {}
};

// Wrapper for an embree ray
struct EmbreeRay : public RTCRay
{
    EmbreeRay(const glm::float3& origin, const glm::float3& direction, float nearDist = 0.0f, float farDist = FLT_MAX)
    {
        org_x = origin.x;
        org_y = origin.y;
        org_z = origin.z;
        dir_x = direction.x;
        dir_y = direction.y;
        dir_z = direction.z;
        tnear = nearDist;
        tfar = farDist;
        geomID = RTC_INVALID_GEOMETRY_ID;
        primID = RTC_INVALID_GEOMETRY_ID;
        instID[0] = RTC_INVALID_GEOMETRY_ID;
        mask = 0xFFFFFFFF;
        time = 0.0f;
    }

    bool Hit() const
    {
        return geomID != RTC_INVALID_GEOMETRY_ID;
    }

    glm::float3 Origin() const
    {
        return glm::float3(org_x, org_y, org_z);
    }

    glm::float3 Direction() const
    {
        return glm::float3(dir_x, dir_y, dir_z);
    }

    unsigned int primID;           //!< primitive ID
    unsigned int geomID;           //!< geometry ID
    unsigned int instID[RTC_MAX_INSTANCE_LEVEL_COUNT];           //!< instance ID
};

enum class IntegrationTypes
{
    Pixel = 0,
    Lens,
    BRDF,
    Sun,
    AreaLight,

    NumValues,
};

static const glm::uint64 NumIntegrationTypes = glm::uint64(IntegrationTypes::NumValues);

// A list of pseudo-random sample points used for Monte Carlo integration, with enough
// sample points for a group of adjacent pixels/texels
struct IntegrationSamples
{
    std::vector<glm::float2> Samples;
    glm::uint64 NumPixels = 0;
    glm::uint64 NumTypes = 0;
    glm::uint64 NumSamples = 0;

    void Init(glm::uint64 numPixels, glm::uint64 numTypes, glm::uint64 numSamples)
    {
        NumPixels = numPixels;
        NumSamples = numSamples;
        NumTypes = numTypes;
        Samples.resize(numPixels * numTypes * numSamples);
    }

    glm::uint64 ArrayIndex(glm::uint64 pixelIdx, glm::uint64 typeIdx, glm::uint64 sampleIdx) const
    {
        assert(pixelIdx < NumPixels);
        assert(typeIdx < NumTypes);
        assert(sampleIdx < NumSamples);
        return pixelIdx * (NumSamples * NumTypes) + typeIdx * NumSamples + sampleIdx;
    }

    glm::float2 GetSample(glm::uint64 pixelIdx, glm::uint64 typeIdx, glm::uint64 sampleIdx) const
    {
        const glm::uint64 idx = ArrayIndex(pixelIdx, typeIdx, sampleIdx);
        return Samples[idx];
    }

    glm::float2* GetSamplesForType(glm::uint64 pixelIdx, glm::uint64 typeIdx)
    {
        const glm::uint64 startIdx = ArrayIndex(pixelIdx, typeIdx, 0);
        return &Samples[startIdx];
    }

    const glm::float2* GetSamplesForType(glm::uint64 pixelIdx, glm::uint64 typeIdx) const
    {
        const glm::uint64 startIdx = ArrayIndex(pixelIdx, typeIdx, 0);
        return &Samples[startIdx];
    }

    void GetSampleSet(glm::uint64 pixelIdx, glm::uint64 sampleIdx, glm::float2* sampleSet) const
    {
        assert(pixelIdx < NumPixels);
        assert(sampleIdx < NumSamples);
        assert(sampleSet != nullptr);
        const glm::uint64 typeStride = NumSamples;
        glm::uint64 idx = pixelIdx * (NumSamples * NumTypes) + sampleIdx;
        for(glm::uint64 typeIdx = 0; typeIdx < NumTypes; ++typeIdx)
        {
            sampleSet[typeIdx] = Samples[idx];
            idx += typeStride;
        }
    }
};

// A single set of sample points for running a single step of the path tracer
struct IntegrationSampleSet
{
    glm::float2 Samples[NumIntegrationTypes];

    void Init(const IntegrationSamples& samples, glm::uint64 pixelIdx, glm::uint64 sampleIdx)
    {
        assert(samples.NumTypes == NumIntegrationTypes);
        samples.GetSampleSet(pixelIdx, sampleIdx, Samples);
    }

    glm::float2 Pixel() const { return Samples[glm::uint64(IntegrationTypes::Pixel)]; }
    glm::float2 Lens() const { return Samples[glm::uint64(IntegrationTypes::Lens)]; }
    glm::float2 BRDF() const { return Samples[glm::uint64(IntegrationTypes::BRDF)]; }
    glm::float2 Sun() const { return Samples[glm::uint64(IntegrationTypes::Sun)]; }
    glm::float2 AreaLight() const { return Samples[glm::uint64(IntegrationTypes::AreaLight)]; }
};

// Generates a full list of sample points for all integration types
void GenerateIntegrationSamples(IntegrationSamples& samples, glm::uint64 sqrtNumSamples, glm::uint64 tileSizeX, glm::uint64 tileSizeY,
                                SampleFramework11::SampleModes sampleMode, glm::uint64 numIntegrationTypes, MoYu::Random& rng);

// Samples the spherical area light using a set of 2D sample points
glm::float3 SampleAreaLight(const glm::float3& position, const glm::float3& normal, RTCScene scene,
                       const glm::float3& diffuseAlbedo, const glm::float3& cameraPos,
                       bool includeSpecular, glm::float3 specAlbedo, float roughness,
                       float u1, float u2, glm::float3& irradiance, glm::float3& sampleDir);

glm::float3 SampleSunLight(const glm::float3& position, const glm::float3& normal, RTCScene scene,
                      const glm::float3& diffuseAlbedo, const glm::float3& cameraPos,
                      bool includeSpecular, glm::float3 specAlbedo, float roughness,
                      float u1, float u2, glm::float3& irradiance);

// Options for path tracing
struct PathTracerParams
{
    glm::float3 RayDir;
    glm::uint32 EnableDirectAreaLight = false;
    glm::uint8 EnableDirectSun = false;
    glm::uint8 EnableDiffuse = false;
    glm::uint8 EnableSpecular = false;
    glm::uint8 EnableBounceSpecular = false;
    glm::uint8 ViewIndirectSpecular = false;
    glm::uint8 ViewIndirectDiffuse = false;
    glm::int32 MaxPathLength = -1;
    glm::int32 RussianRouletteDepth = -1;
    float RussianRouletteProbability = 0.5f;
    glm::float3 RayStart;
    float RayLen = 0.0f;
    const BVHData* SceneBVH = nullptr;
    const IntegrationSampleSet* SampleSet = nullptr;
    // const SkyCache* SkyCache = nullptr;
    const MoYu::MoYuScratchImage* EnvMaps = nullptr;
};

// Returns the incoming radiance along the ray specified by "RayDir", computed using unidirectional
// path tracing
glm::float3 PathTrace(const PathTracerParams& params, MoYu::Random& randomGenerator, float& illuminance, bool& hitSky);