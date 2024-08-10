//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "BakingLabSettings.h"

#include "sampleframework/graphics/Sampling.h"
#include "sampleframework/graphics/Spectrum.h"

namespace SampleFramework11
{

    bool EnableSun = true;
    bool SunAreaLightApproximation = true;
    bool BakeDirectSunLight = false;
    glm::vec3 SunTintColor = glm::vec3(1.0000f, 1.0000f, 1.0000f);
    float SunIntensityScale = 1.0000f;
    float SunSize = 0.2700f;
    bool NormalizeSunIntensity = false;
    SunDirectionTypes SunDirType = SunDirectionTypes::HorizontalCoordSystem;
    glm::vec3 SunDirection = glm::vec3(-0.7500f, 0.9770f, -0.4000f);
    float SunAzimuth;
    float SunElevation;
    SkyModes SkyMode;
    glm::vec3 SkyColor;
    float Turbidity;
    glm::vec3 GroundAlbedo;
    bool EnableAreaLight;
    glm::vec3 AreaLightColor;
    float AreaLightIlluminance;
    float AreaLightLuminousPower;
    float AreaLightEV100;
    LightUnits AreaLightUnits;
    float AreaLightIlluminanceDistance;
    float AreaLightSize;
    float AreaLightX;
    float AreaLightY;
    float AreaLightZ;
    float AreaLightShadowBias;
    bool BakeDirectAreaLight;
    bool EnableAreaLightShadows;
    ExposureModes ExposureMode;
    float ManualExposure;
    FStops ApertureSize;
    ISORatings ISORating;
    ShutterSpeeds ShutterSpeed;
    float FilmSize;
    float FocalLength;
    float FocusDistance;
    int NumBlades;
    bool EnableDOF;
    float KeyValue;
    float AdaptationRate;
    float ApertureFNumber;
    float ApertureWidth;
    float ShutterSpeedValue;
    float ISO;
    float BokehPolygonAmount;
    ToneMappingModes ToneMappingMode;
    float WhitePoint_Hejl;
    float ShoulderStrength;
    float LinearStrength;
    float LinearAngle;
    float ToeStrength;
    float WhitePoint_Hable;
    MSAAModes MSAAMode;
    FilterTypes FilterType;
    float FilterSize;
    float GaussianSigma;
    bool EnableTemporalAA;
    JitterModes JitterMode;
    float JitterScale;
    SGDiffuseModes SGDiffuseMode;
    SGSpecularModes SGSpecularMode;
    SH4DiffuseModes SH4DiffuseMode;
    SHSpecularModes SHSpecularMode;
    int LightMapResolution;
    int NumBakeSamples;
    SampleModes BakeSampleMode;
    int MaxBakePathLength;
    int BakeRussianRouletteDepth;
    float BakeRussianRouletteProbability;
    BakeModes BakeMode;
    SolveModes SolveMode;
    bool WorldSpaceBake;
    bool EnableDiffuse;
    bool EnableSpecular;
    bool EnableDirectLighting;
    bool EnableIndirectLighting;
    bool EnableIndirectDiffuse;
    bool EnableIndirectSpecular;
    bool EnableAlbedoMaps;
    bool EnableNormalMaps;
    float NormalMapIntensity;
    float DiffuseAlbedoScale;
    float RoughnessScale;
    float MetallicOffset;
    bool ShowGroundTruth;
    float NumRenderSamples;
    SampleModes RenderSampleMode;
    int MaxRenderPathLength;
    int RenderRussianRouletteDepth;
    float RenderRussianRouletteProbability;
    bool EnableRenderBounceSpecular;
    float BloomExposure;
    float BloomMagnitude;
    float BloomBlurSigma;
    bool EnableLuminancePicker;
    bool ShowBakeDataVisualizer;
    bool ViewIndirectDiffuse;
    bool ViewIndirectSpecular;
    float RoughnessOverride;

    // Returns the result of performing a irradiance/illuminance integral over the portion
    // of the hemisphere covered by a region with angular radius = theta
    static float IlluminanceIntegral(float theta)
    {
        float cosTheta = std::cos(theta);
        return MoYu::f::PI * (1.0f - (cosTheta * cosTheta));
    }

    glm::float3 SunLuminance(bool& cached)
    {
        glm::float3 sunDirection = SampleFramework11::SunDirection;
        sunDirection.y = MoYu::Saturate(sunDirection.y);
        sunDirection = glm::normalize(sunDirection);
        const float turbidity = MoYu::Clamp(SampleFramework11::Turbidity, 1.0f, 32.0f);
        const float intensityScale = SampleFramework11::SunIntensityScale;
        const glm::float3 tintColor = SampleFramework11::SunTintColor;
        const bool normalizeIntensity = SampleFramework11::NormalizeSunIntensity;
        const float sunSize = SampleFramework11::SunSize;

        static float turbidityCache = 2.0f;
        static glm::float3 sunDirectionCache = glm::float3(-0.579149902f, 0.754439294f, -0.308879942f);
        static glm::float3 luminanceCache = glm::float3(1.61212531e+009f, 1.36822630e+009f, 1.07235315e+009f);
        static glm::float3 sunTintCache = glm::float3(1.0f, 1.0f, 1.0f);
        static float sunIntensityCache = 1.0f;
        static bool normalizeCache = false;
        static float sunSizeCache = SampleFramework11::BaseSunSize;

        if(turbidityCache == turbidity && sunDirection == sunDirectionCache
            && intensityScale == sunIntensityCache && tintColor == sunTintCache
            && normalizeCache == normalizeIntensity && sunSize == sunSizeCache)
        {
            cached = true;
            return luminanceCache;
        }

        cached = false;

        float thetaS = std::acos(1.0f - sunDirection.y);
        float elevation = MoYu::f::PI_2 - thetaS;

        // Get the sun's luminance, then apply tint and scale factors
        glm::float3 sunLuminance;

        // For now, we'll compute an average luminance value from Hosek solar radiance model, even though
        // we could compute illuminance directly while we're sampling the disk
        SampledSpectrum groundAlbedoSpectrum = SampledSpectrum::FromRGB(GroundAlbedo);
        SampledSpectrum solarRadiance;

        const glm::uint64 NumDiscSamples = 8;
        for(glm::uint64 x = 0; x < NumDiscSamples; ++x)
        {
            for(glm::uint64 y = 0; y < NumDiscSamples; ++y)
            {
                float u = (x + 0.5f) / NumDiscSamples;
                float v = (y + 0.5f) / NumDiscSamples;
                glm::float2 discSamplePos = SquareToConcentricDiskMapping(u, v);

                float theta = elevation + discSamplePos.y * MoYu::f::DEG_TO_RAD * SampleFramework11::BaseSunSize;
                float gamma = discSamplePos.x * MoYu::f::DEG_TO_RAD * SampleFramework11::BaseSunSize;

                for(glm::int32 i = 0; i < NumSpectralSamples; ++i)
                {
                    // ArHosekSkyModelState* skyState = arhosekskymodelstate_alloc_init(elevation, turbidity, groundAlbedoSpectrum[i]);
                    // float wavelength = Lerp(float(SampledLambdaStart), float(SampledLambdaEnd), i / float(NumSpectralSamples));
                    //
                    // solarRadiance[i] = float(arhosekskymodel_solar_radiance(skyState, theta, gamma, wavelength));
                    //
                    // arhosekskymodelstate_free(skyState);
                    // skyState = nullptr;
                }

                glm::float3 sampleRadiance = solarRadiance.ToRGB();
                sunLuminance += sampleRadiance;
            }
        }

        // Account for luminous efficiency, coordinate system scaling, and sample averaging
        sunLuminance *= 683.0f * 100.0f * (1.0f / NumDiscSamples) * (1.0f / NumDiscSamples);

        sunLuminance = sunLuminance * tintColor;
        sunLuminance = sunLuminance * intensityScale;

        if(normalizeIntensity)
        {
            // Normalize so that the intensity stays the same even when the sun is bigger or smaller
            const float baseIntegral = IlluminanceIntegral(MoYu::f::DEG_TO_RAD * SampleFramework11::BaseSunSize);
            const float currIntegral = IlluminanceIntegral(MoYu::f::DEG_TO_RAD * SampleFramework11::SunSize);
            sunLuminance *= (baseIntegral / currIntegral);
        }

        turbidityCache = turbidity;
        sunDirectionCache = sunDirection;
        luminanceCache = sunLuminance;
        sunIntensityCache = intensityScale;
        sunTintCache = tintColor;
        normalizeCache = normalizeIntensity;
        sunSizeCache = sunSize;

        return sunLuminance;
    }
    
    glm::float3 SunLuminance()
    {
        bool cached = false;
        return SunLuminance(cached);
    }

    glm::float3 SunIlluminance()
    {
        glm::float3 sunLuminance = SunLuminance();

        // Compute partial integral over the hemisphere in order to compute illuminance
        float theta = MoYu::f::DEG_TO_RAD * SampleFramework11::SunSize;
        float integralFactor = IlluminanceIntegral(theta);

        return sunLuminance * integralFactor;
    }
    
}
