#pragma once
#include <cstdint>

#include "runtime/core/math/moyu_math2.h"

namespace EngineConfig
{
    enum AntialiasingMode
    {
        NONE,
        MSAA,
        FXAA,
        SMAA,
        TAA
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
        float taaBaseBlendFactor = 0.875f; // Determines how much the history buffer is blended together with current frame result. Higher values means more history contribution.
    };

    extern TAAConfig g_TAAConfig;

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

    struct SHConfig
    {
        glm::float4 _GSH[7];
    };

    extern SHConfig g_SHConfig;

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
}

// clang-format off

// clang-format on