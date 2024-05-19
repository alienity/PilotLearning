#pragma once
#include "core/math/moyu_math2.h"

namespace MoYu
{

    // https://therealmjp.github.io/posts/sss-intro/#screen-space-subsurface-scattering-ssss
    // https://advances.realtimerendering.com/s2018/Efficient%20screen%20space%20subsurface%20scattering%20Siggraph%202018.pdf
    // https://www.slideshare.net/colinbb/colin-barrebrisebois-gdc-2011-approximating-translucency-for-a-fast-cheap-and-convincing-subsurfacescattering-look-7170855
    // http://www.iryoku.com/downloads/Next-Generation-Character-Rendering-v6.pptx

    class DiffusionProfileConstants
    {
    public:
        const int DIFFUSION_PROFILE_COUNT = 16; // Max. number of profiles, including the slot taken by the neutral profile
        const int DIFFUSION_PROFILE_NEUTRAL_ID = 0;  // Does not result in blurring
        const int SSS_PIXELS_PER_SAMPLE = 4;
    };

    class DiffusionProfile
    {
    public:
        enum TexturingMode
        {
            PreAndPostScatter = 0,
            PostScatter = 1
        };

        enum TransmissionMode
        {
            Regular = 0,
            ThinObject = 1
        };

        glm::float4 scatteringDistance; // Per color channel (no meaningful units)
        float scatteringDistanceMultiplier = 1.0f;
        glm::float4 transmissionTint;           // HDR color
        TexturingMode texturingMode;
        TransmissionMode transmissionMode;
        glm::float2 thicknessRemap;       // X = min, Y = max (in millimeters)
        float worldScale;                 // Size of the world unit in meters
        float ior;                        // 1.4 for skin (mean ~0.028)
    
        glm::float3 shapeParam;           // RGB = shape parameter: S = 1 / D
        float filterRadius;               // In millimeters
        float maxScatteringDistance;      // No meaningful units

        // Unique hash used in shaders to identify the index in the diffusion profile array
        int hash = 0;

    public:
        DiffusionProfile();

        void ResetToDefault();
        
        void Validate();
        
        // Ref: Approximate Reflectance Profiles for Efficient Subsurface Scattering by Pixar.
        void UpdateKernel();

        // https://zero-radiance.github.io/post/sampling-diffusion/
        // Performs sampling of a Normalized Burley diffusion profile in polar coordinates.
        // 'u' is the random number (the value of the CDF): [0, 1).
        // rcp(s) = 1 / ShapeParam = ScatteringDistance.
        // Returns the sampled radial distance, s.t. (u = 0 -> r = 0) and (u = 1 -> r = Inf).
        static float SampleBurleyDiffusionProfile(float u, float rcpS);

    };

    class DiffusionProfileSettings
    {
    public:
        DiffusionProfile profile;

        glm::float4 worldScaleAndFilterRadiusAndThicknessRemap; // X = meters per world unit, Y = filter radius (in mm), Z = remap start, W = end - start
        glm::float4 shapeParamAndMaxScatterDist;                // RGB = S = 1 / D, A = d = RgbMax(D)
        glm::float4 transmissionTintAndFresnel0;                // RGB = color, A = fresnel0
        glm::float4 disabledTransmissionTintAndFresnel0;        // RGB = black, A = fresnel0 - For debug to remove the transmission

        DiffusionProfileSettings();

        void UpdateCache();
    };


}