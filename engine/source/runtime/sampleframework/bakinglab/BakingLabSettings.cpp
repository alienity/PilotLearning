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


}
