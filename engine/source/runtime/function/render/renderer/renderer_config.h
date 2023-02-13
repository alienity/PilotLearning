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
}

// clang-format off

// clang-format on