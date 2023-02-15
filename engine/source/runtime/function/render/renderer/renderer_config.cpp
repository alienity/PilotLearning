#include "renderer_config.h"

namespace EngineConfig
{

    AntialiasingMode g_AntialiasingMode = AntialiasingMode::FXAA;

    MSAAConfig g_MASSConfig = {4};

    FXAAConfig g_FXAAConfig = FXAAConfig();

    HDRConfig g_HDRConfig = HDRConfig();

    BloomConfig g_BloomConfig = BloomConfig();

}