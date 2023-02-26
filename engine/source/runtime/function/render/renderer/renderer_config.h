#pragma once
#include <cstdint>

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
        bool m_HighQualityBloom = false;
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

}

// clang-format off

// clang-format on