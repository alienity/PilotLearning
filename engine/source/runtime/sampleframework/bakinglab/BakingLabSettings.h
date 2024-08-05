//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "runtime/core/math/moyu_math2.h"

#pragma once

namespace SampleFramework11
{
    enum class SunDirectionTypes
    {
        UnitVector = 0,
        HorizontalCoordSystem = 1,

        NumValues
    };

    enum class SkyModes
    {
        None = 0,
        Procedural = 1,
        Simple = 2,
        CubeMapEnnis = 3,
        CubeMapGraceCathedral = 4,
        CubeMapUffizi = 5,

        NumValues
    };

    enum class LightUnits
    {
        Luminance = 0,
        Illuminance = 1,
        LuminousPower = 2,
        EV100 = 3,

        NumValues
    };

    enum class ExposureModes
    {
        ManualSimple = 0,
        Manual_SBS = 1,
        Manual_SOS = 2,
        Automatic = 3,

        NumValues
    };

    enum class FStops
    {
        FStop1Point8 = 0,
        FStop2Point0 = 1,
        FStop2Point2 = 2,
        FStop2Point5 = 3,
        FStop2Point8 = 4,
        FStop3Point2 = 5,
        FStop3Point5 = 6,
        FStop4Point0 = 7,
        FStop4Point5 = 8,
        FStop5Point0 = 9,
        FStop5Point6 = 10,
        FStop6Point3 = 11,
        FStop7Point1 = 12,
        FStop8Point0 = 13,
        FStop9Point0 = 14,
        FStop10Point0 = 15,
        FStop11Point0 = 16,
        FStop13Point0 = 17,
        FStop14Point0 = 18,
        FStop16Point0 = 19,
        FStop18Point0 = 20,
        FStop20Point0 = 21,
        FStop22Point0 = 22,

        NumValues
    };

    enum class ISORatings
    {
        ISO100 = 0,
        ISO200 = 1,
        ISO400 = 2,
        ISO800 = 3,

        NumValues
    };

    enum class ShutterSpeeds
    {
        ShutterSpeed1Over1 = 0,
        ShutterSpeed1Over2 = 1,
        ShutterSpeed1Over4 = 2,
        ShutterSpeed1Over8 = 3,
        ShutterSpeed1Over15 = 4,
        ShutterSpeed1Over30 = 5,
        ShutterSpeed1Over60 = 6,
        ShutterSpeed1Over125 = 7,
        ShutterSpeed1Over250 = 8,
        ShutterSpeed1Over500 = 9,
        ShutterSpeed1Over1000 = 10,
        ShutterSpeed1Over2000 = 11,
        ShutterSpeed1Over4000 = 12,

        NumValues
    };

    enum class ToneMappingModes
    {
        FilmStock = 0,
        Linear = 1,
        ACES = 2,
        Hejl2015 = 3,
        Hable = 4,

        NumValues
    };

    enum class MSAAModes
    {
        MSAANone = 0,
        MSAA2x = 1,
        MSAA4x = 2,
        MSAA8x = 3,

        NumValues
    };

    enum class FilterTypes
    {
        Box = 0,
        Triangle = 1,
        Gaussian = 2,
        BlackmanHarris = 3,
        Smoothstep = 4,
        BSpline = 5,

        NumValues
    };

    enum class JitterModes
    {
        None = 0,
        Uniform2x = 1,
        Hammersley4x = 2,
        Hammersley8x = 3,
        Hammersley16x = 4,

        NumValues
    };

    enum class SGDiffuseModes
    {
        InnerProduct = 0,
        Punctual = 1,
        Fitted = 2,

        NumValues
    };

    enum class SGSpecularModes
    {
        Punctual = 0,
        SGWarp = 1,
        ASGWarp = 2,

        NumValues
    };

    enum class SH4DiffuseModes
    {
        Convolution = 0,
        Geomerics = 1,
        ZH3 = 2,

        NumValues
    };

    enum class SHSpecularModes
    {
        Convolution = 0,
        DominantDirection = 1,
        Punctual = 2,
        Prefiltered = 3,

        NumValues
    };

    enum class SampleModes
    {
        Random = 0,
        Stratified = 1,
        Hammersley = 2,
        UniformGrid = 3,
        CMJ = 4,

        NumValues
    };

    enum class BakeModes
    {
        Diffuse = 0,
        Directional = 1,
        DirectionalRGB = 2,
        HL2 = 3,
        SH4 = 4,
        SH9 = 5,
        H4 = 6,
        H6 = 7,
        SG5 = 8,
        SG6 = 9,
        SG9 = 10,
        SG12 = 11,

        NumValues
    };

    enum class SolveModes
    {
        Projection = 0,
        SVD = 1,
        NNLS = 2,
        RunningAverage = 3,
        RunningAverageNN = 4,

        NumValues
    };

    enum class BRDF
    {
        GGX = 0,
        Beckmann = 1,
        Velvet = 2,

        NumValues
    };

    extern SunDirectionTypes SunDirectionType;
    extern SkyModes SkyMode;
    extern LightUnits LightUnit;
    extern ExposureModes ExposureMode;
    extern FStops FStop;
    extern ISORatings ISORating;
    extern ShutterSpeeds ShutterSpeed;
    extern ToneMappingModes ToneMappingMode;
    extern MSAAModes MSAAMode;
    extern FilterTypes FilterType;
    extern JitterModes JitterMode;
    extern SGDiffuseModes SGDiffuseMode;
    extern SGSpecularModes SGSpecularMode;
    extern SH4DiffuseModes SH4DiffuseMode;
    extern SHSpecularModes SHSpecularMode;
    extern SampleModes SampleMode;
    extern BakeModes BakeMode;
    extern SolveModes SolveMode;
    extern BRDF BRDfMode;

    static const float BaseSunSize = 0.2700f;
    static const glm::int64 MaxSGCount = 12;
    static const glm::int64 MaxBasisCount = 12;

}