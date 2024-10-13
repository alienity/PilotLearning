#pragma once
#include <cstdint>

#include "runtime/core/math/moyu_math2.h"

namespace EngineConfig
{
    enum class StencilUsage
    {
        Clear = 0,

        // Note: first bit is free and can still be used by both phases.

        // --- Following bits are used before transparent rendering ---
        IsUnlit = (1 << 0), // Unlit materials (shader and shader graph) except for the shadow matte
        RequiresDeferredLighting = (1 << 1),
        SubsurfaceScattering = (1 << 2),     //  SSS, Split Lighting
        TraceReflectionRay = (1 << 3),     //  SSR or RTR - slot is reuse in transparent
        Decals = (1 << 4),     //  Use to tag when an Opaque Decal is render into DBuffer
        ObjectMotionVector = (1 << 5),     //  Animated object (for motion blur, SSR, SSAO, TAA)

        // --- Stencil  is cleared after opaque rendering has finished ---

        // --- Following bits are used exclusively for what happens after opaque ---
        ExcludeFromTUAndAA = (1 << 1),    // Disable Temporal Upscaling and Antialiasing for certain objects
        DistortionVectors = (1 << 2),    // Distortion pass - reset after distortion pass, shared with SMAA
        SMAA = (1 << 2),    // Subpixel Morphological Antialiasing
        // Reserved TraceReflectionRay = (1 << 3) for transparent SSR or RTR
        WaterSurface = (1 << 4), // Reserved for water surface usage
        AfterOpaqueReservedBits = 0x38,        // Reserved for future usage

        // --- Following are user bits, we don't touch them inside HDRP and is up to the user to handle them ---
        UserBit0 = (1 << 6),
        UserBit1 = (1 << 7),

        HDRPReservedBits = 255 & ~(UserBit0 | UserBit1),
    };

    enum class AntialiasingMode
    {
        NONEMode,
        MSAAMode,
        FXAAMode,
        SMAAMode,
        TAAMode
    };

    extern AntialiasingMode g_AntialiasingMode;

    struct MSAAConfig
    {
        std::uint32_t m_MSAASampleCount = 4;
    };

    extern MSAAConfig g_MASSConfig;

    struct FXAAConfig
    {
        // [0.0312f, 0.0833f]
        float m_ContrastThreshold = 0.0312f;
        // [0.063f, 0.333f]
        float m_RelativeThreshold = 0.063f;
        // [0f, 1f]
        float m_SubpixelBlending = 0.0312f;

        enum FXAAQualityEnum
        {
            LOW_QUALITY,
            HIGH_QUALITY
        };

        FXAAQualityEnum m_FXAAQuality = HIGH_QUALITY;

        enum FXAALuminanceMode
        {
            Alpha,
            Green,
            Calculate
        };

        FXAALuminanceMode m_FXAALuminanceSource = Alpha;
    };

    extern FXAAConfig g_FXAAConfig;

    struct SMAAConfig
    {

    };
    extern SMAAConfig g_SMAAConfig;

    struct TAAConfig
    {
        float taaSharpenStrength = 0.5f; // Strength of the sharpening component of temporal anti-aliasing.
        float taaHistorySharpening = 0.35f; // Strength of the sharpening of the history sampled for TAA.
        float taaAntiFlicker = 0.5f; // Drive the anti-flicker mechanism. With high values flickering might be reduced, but it can lead to more ghosting or disocclusion artifacts.
        float taaMotionVectorRejection = 0.0f;// Higher this value, more likely history will be rejected when current and reprojected history motion vector differ by a substantial amount. High values can decrease ghosting but will also reintroduce aliasing on the aforementioned cases.
        float taaBaseBlendFactor = 0.45f; // Determines how much the history buffer is blended together with current frame result. Higher values means more history contribution.

        float taaJitterScale =  1.0f; // Scale to apply to the jittering applied when TAA is enabled.
    };

    extern TAAConfig g_TAAConfig;

    enum IndirectDiffuseMode
    {
        Off,
        ScreenSpace,
        Voxel,
        Mixed
    };

    struct GIConfig
    {
        IndirectDiffuseMode indirectDiffuseMode = ScreenSpace;
    };

    extern GIConfig g_GIConfig;

    struct HDRConfig
    {
        bool  m_EnableHDR              = true;
        bool  m_EnableAdaptiveExposure = true;
        float m_MinExposure            = -6.0f; // -8.0f, 0.0f, 0.25f ---- min max step
        float m_MaxExposure            = 6.0f;  // 0.0f, 8.0f, 0.25f
        float m_TargetLuminance        = 0.08f; // 0.01f, 0.99f, 0.01f
        float m_AdaptationRate         = 0.05f; // 0.01f, 1.0f, 0.01f
        float m_Exposure               = 2.0f;  // -8.0f, 8.0f, 0.25f
    };
    extern HDRConfig g_HDRConfig;

    extern const float kInitialMinLog;
    extern const float kInitialMaxLog;

    struct BloomConfig
    {
        bool m_BloomEnable = true;
        // The threshold luminance above which a pixel will start to bloom
        float m_BloomThreshold = 4.0f; // 0.0f, 8.0f, 0.1f
        // A modulator controlling how much bloom is added back into the image
        float m_BloomStrength  = 0.1f; // 0.0f, 2.0f, 0.05f
        // Controls the "focus" of the blur.  High values spread out more causing a haze.
        float m_BloomUpsampleFactor = 0.65f; // 0.0f, 1.0f, 0.05f
        // High quality blurs 5 octaves of bloom; low quality only blurs 3.
        bool m_HighQualityBloom = true;
    };
    extern BloomConfig g_BloomConfig;

    struct ToneMappingConfig
    {
        // Graphics/Display/Paper White (nits)
        float m_HDRPaperWhite = 200.0f; // 100.0f, 500.0f, 50.0f
        // Graphics/Display/Peak Brightness (nits)
        float m_MaxDisplayLuminance = 1000.0f; // 500.0f, 10000.0f, 100.0f
    };
    extern ToneMappingConfig g_ToneMappingConfig;

    enum ScreenSpaceReflectionAlgorithm
    {
        /// <summary>Legacy SSR approximation.</summary>
        Approximation,
        /// <summary>Screen Space Reflection, Physically Based with Accumulation through multiple frame.</summary>
        PBRAccumulation
    };

    struct SSRConfig
    {
        bool reflectSky = false; // When enabled, SSR handles sky reflection for opaque objects (not supported for SSR on transparent).
        ScreenSpaceReflectionAlgorithm usedAlgorithm = PBRAccumulation; // Screen Space Reflections Algorithm used.
        float screenFadeDistance = 0.1f; // Controls the typical thickness of objects the reflection rays may pass behind.
        int rayMaxIterations = 64; // Sets the maximum number of steps HDRP uses for raytracing. Affects both correctness and performance.
        float depthBufferThickness = 0.1f; // Controls the distance at which HDRP fades out SSR near the edge of the screen.
        float accumulationFactor = 0.95f; // Controls the amount of accumulation (0 no accumulation, 1 just accumulate)
        float biasFactor = 0.5f; // For PBR: Controls the bias of accumulation (0 no bias, 1 bias ssr)
        bool enableWorldSpeedRejection = true; // When enabled, world space speed from Motion vector is used to reject samples.
        float speedRejectionParam = 0.95f; // Controls the likelihood history will be rejected based on the previous frame motion vectors of both the surface and the hit object in world space.
        float speedRejectionScalerFactor = 0.2f; // Controls the upper range of speed. The faster the objects or camera are moving, the higher this number should be.
        bool speedSmoothReject = true; // When enabled, history can be partially rejected for moving objects which gives a smoother transition. When disabled, history is either kept or totally rejected.
        bool speedSurfaceOnly = true; // When enabled, speed rejection used world space motion of the reflecting surface.
        bool speedTargetOnly = true; // When enabled, speed rejection used world space motion of the hit surface by the SSR.

        float smoothnessFadeStart = 0.9f;// Controls the smoothness value at which the smoothness-controlled fade out starts.
        float minSmoothness = 0.01f; // Controls the smoothness value at which HDRP activates SSR and the smoothness-controlled fade out stops.

    };

    extern SSRConfig g_SSRConfig;

    struct SHConfig
    {
        glm::float4 _GSH[7];
    };

    extern SHConfig g_SHConfig;

    struct SSGIConfig
    {
        int SSGIRaySteps = 64; // Screen space global illumination step count for the ray marching.
        bool SSGIDenoise = true; // Flag that enables the first denoising pass.
        float SSGIDenoiserRadius = 0.75f; // Flag that defines the radius of the first denoiser.
        bool SSGISecondDenoise = true; // Flag that enables the second denoising pass.
    };

    extern SSGIConfig g_SSGIConfig;

    struct VoxelGIConfig
    {

    };

    extern VoxelGIConfig g_VoxelGIConfig;

    struct AOConfig
    {
        enum AOMode
        {
            None,
            SSAO,
            HBAO
        };

        AOMode _aoMode = AOMode::HBAO;
    };

    extern AOConfig g_AOConfig;

    struct VolumeLightConfig
    {
        int   mDownScaleMip = 3;
        int   mSampleCount = 128;
        float mScatteringCoef = 0.127f;
        float mExtinctionCoef = 0.01f;
        float mSkyboxExtinctionCoef = 0.9f;
        float mMieG = 0.319f;
        bool  mHeightFog = true;
        float mHeightScale = 0.114f;
        float mGroundLevel = 0;
        bool  mNoise = true;
        float mNoiseSccale = 0.009f;
        float mNoiseIntensity = 2.01f;
        float mNoiseIntensityOffset = 0.3f;
        glm::float2 mNoiseVelocity = glm::float2(11, 3);
        float mMaxRayLength = 128;
        float mMinStepSize = 0.2f;
    };

    extern VolumeLightConfig g_VolumeLightConfig;

    void InitEngineConfig();

    struct ColorParameter
    {
        glm::float4 color;
        bool hdr = false; // Is this color HDR?
        bool showAlpha = true; // Should the alpha channel be editable in the editor?
        bool showEyeDropper = true; // Should the eye dropper be visible in the editor?
    };

    enum FogColorMode
    {
        /// <summary>Fog is a constant color.</summary>
        ConstantColor,
        /// <summary>Fog uses the current sky to determine its color.</summary>
        SkyColor,
    };

    struct FogConfig
    {
        // Enable fog.
        bool enabled = false;

        // Fog color mode.
        FogColorMode colorMode = FogColorMode::SkyColor;
        // Specifies the constant color of the fog.
        ColorParameter color = ColorParameter{ glm::float4(0.5f, 0.5f, 0.5f, 1.0f), true, false, true };
        // Specifies the tint of the fog when using Sky Color.
        ColorParameter tint = ColorParameter{ glm::float4(1.0f, 1.0f, 1.0f, 1.0f), true, false, true };
        // Sets the maximum fog distance HDRP uses when it shades the skybox or the Far Clipping Plane of the Camera.
        float maxFogDistance = 5000.0f;
        // Controls the maximum mip map HDRP uses for mip fog (0 is the lowest mip and 1 is the highest mip).
        float mipFogMaxMip = 0.5f;
        // Sets the distance at which HDRP uses the minimum mip image of the blurred sky texture as the fog color.
        float mipFogNear = 0.0f;
        // Sets the distance at which HDRP uses the maximum mip image of the blurred sky texture as the fog color.
        float mipFogFar = 1000.0f;

        // Height Fog
        // Height fog base height.
        float baseHeight = 0.0f;
        // Height fog maximum height.
        float maximumHeight = 50.0f;
        // Fog Attenuation Distance
        float meanFreePath = 400.0f;

        // Optional Volumetric Fog
        // Enable volumetric fog.
        bool enableVolumetricFog = false;
        // Common Fog Parameters (Exponential/Volumetric)
        // Stores the fog albedo. This defines the color of the fog.
        ColorParameter albedo = ColorParameter{ glm::float4(1.0f, 1.0f, 1.0f, 1.0f), false, true, true };



    };
}

// clang-format off

// clang-format on