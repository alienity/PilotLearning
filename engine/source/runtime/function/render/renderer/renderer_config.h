#pragma once

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

}

// clang-format off

// clang-format on