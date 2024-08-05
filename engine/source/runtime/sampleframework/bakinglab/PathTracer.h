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

// Forward declarations
struct __RTCScene;
typedef __RTCScene* RTCScene;

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
    std::vector<TextureData<glm::uint32>> MaterialDiffuseMaps;
    std::vector<TextureData<glm::uint32>> MaterialNormalMaps;
    std::vector<TextureData<glm::uint32>> MaterialRoughnessMaps;
    std::vector<TextureData<glm::uint32>> MaterialMetallicMaps;

    ~BVHData()
    {
        if(Scene != nullptr)
        {
            rtcDeleteScene(Scene);
            Scene = nullptr;
        }
    }
};

// Wrapper for an embree ray
struct EmbreeRay : public RTCRay
{
    EmbreeRay(const glm::float3& origin, const glm::float3& direction, float nearDist = 0.0f, float farDist = FLT_MAX)
    {
        org[0] = origin.x;
        org[1] = origin.y;
        org[2] = origin.z;
        dir[0] = direction.x;
        dir[1] = direction.y;
        dir[2] = direction.z;
        tnear = nearDist;
        tfar = farDist;
        geomID = RTC_INVALID_GEOMETRY_ID;
        primID = RTC_INVALID_GEOMETRY_ID;
        instID = RTC_INVALID_GEOMETRY_ID;
        mask = 0xFFFFFFFF;
        time = 0.0f;
    }

    bool Hit() const
    {
        return geomID != RTC_INVALID_GEOMETRY_ID;
    }

    glm::float3 Origin() const
    {
        return glm::float3(org[0], org[1], org[2]);
    }

    glm::float3 Direction() const
    {
        return glm::float3(dir[0], dir[1], dir[2]);
    }
};

StaticAssert_(sizeof(EmbreeRay) == sizeof(RTCRay));

enum class IntegrationTypes
{
    Pixel = 0,
    Lens,
    BRDF,
    Sun,
    AreaLight,

    NumValues,
};

static const uint64 NumIntegrationTypes = uint64(IntegrationTypes::NumValues);

// A list of pseudo-random sample points used for Monte Carlo integration, with enough
// sample points for a group of adjacent pixels/texels
struct IntegrationSamples
{
    std::vector<glm::float2> Samples;
    uint64 NumPixels = 0;
    uint64 NumTypes = 0;
    uint64 NumSamples = 0;

    void Init(uint64 numPixels, uint64 numTypes, uint64 numSamples)
    {
        NumPixels = numPixels;
        NumSamples = numSamples;
        NumTypes = numTypes;
        Samples.resize(numPixels * numTypes * numSamples);
    }

    uint64 ArrayIndex(uint64 pixelIdx, uint64 typeIdx, uint64 sampleIdx) const
    {
        Assert_(pixelIdx < NumPixels);
        Assert_(typeIdx < NumTypes);
        Assert_(sampleIdx < NumSamples);
        return pixelIdx * (NumSamples * NumTypes) + typeIdx * NumSamples + sampleIdx;
    }

    glm::float2 GetSample(uint64 pixelIdx, uint64 typeIdx, uint64 sampleIdx) const
    {
        const uint64 idx = ArrayIndex(pixelIdx, typeIdx, sampleIdx);
        return Samples[idx];
    }

    glm::float2* GetSamplesForType(uint64 pixelIdx, uint64 typeIdx)
    {
        const uint64 startIdx = ArrayIndex(pixelIdx, typeIdx, 0);
        return &Samples[startIdx];
    }

    const glm::float2* GetSamplesForType(uint64 pixelIdx, uint64 typeIdx) const
    {
        const uint64 startIdx = ArrayIndex(pixelIdx, typeIdx, 0);
        return &Samples[startIdx];
    }

    void GetSampleSet(uint64 pixelIdx, uint64 sampleIdx, glm::float2* sampleSet) const
    {
        Assert_(pixelIdx < NumPixels);
        Assert_(sampleIdx < NumSamples);
        Assert_(sampleSet != nullptr);
        const uint64 typeStride = NumSamples;
        uint64 idx = pixelIdx * (NumSamples * NumTypes) + sampleIdx;
        for(uint64 typeIdx = 0; typeIdx < NumTypes; ++typeIdx)
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

    void Init(const IntegrationSamples& samples, uint64 pixelIdx, uint64 sampleIdx)
    {
        Assert_(samples.NumTypes == NumIntegrationTypes);
        samples.GetSampleSet(pixelIdx, sampleIdx, Samples);
    }

    glm::float2 Pixel() const { return Samples[uint64(IntegrationTypes::Pixel)]; }
    glm::float2 Lens() const { return Samples[uint64(IntegrationTypes::Lens)]; }
    glm::float2 BRDF() const { return Samples[uint64(IntegrationTypes::BRDF)]; }
    glm::float2 Sun() const { return Samples[uint64(IntegrationTypes::Sun)]; }
    glm::float2 AreaLight() const { return Samples[uint64(IntegrationTypes::AreaLight)]; }
};

// Generates a full list of sample points for all integration types
void GenerateIntegrationSamples(IntegrationSamples& samples, uint64 sqrtNumSamples, uint64 tileSizeX, uint64 tileSizeY,
                                SampleModes sampleMode, uint64 numIntegrationTypes, Random& rng);

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
    uint32 EnableDirectAreaLight = false;
    uint8 EnableDirectSun = false;
    uint8 EnableDiffuse = false;
    uint8 EnableSpecular = false;
    uint8 EnableBounceSpecular = false;
    uint8 ViewIndirectSpecular = false;
    uint8 ViewIndirectDiffuse = false;
    int32 MaxPathLength = -1;
    int32 RussianRouletteDepth = -1;
    float RussianRouletteProbability = 0.5f;
    glm::float3 RayStart;
    float RayLen = 0.0f;
    const BVHData* SceneBVH = nullptr;
    const IntegrationSampleSet* SampleSet = nullptr;
    const SkyCache* SkyCache = nullptr;
    const TextureData<Half4>* EnvMaps = nullptr;
};

// Returns the incoming radiance along the ray specified by "RayDir", computed using unidirectional
// path tracing
glm::float3 PathTrace(const PathTracerParams& params, Random& randomGenerator, float& illuminance, bool& hitSky);