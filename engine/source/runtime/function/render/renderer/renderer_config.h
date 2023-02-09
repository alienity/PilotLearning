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

    extern std::uint32_t    g_MSAASampleCount;
    extern AntialiasingMode g_AntialiasingMode;
    

}

// clang-format off

// clang-format on