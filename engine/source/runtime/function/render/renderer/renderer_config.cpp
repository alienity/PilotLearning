#include "renderer_config.h"

namespace EngineConfig
{

    AntialiasingMode g_AntialiasingMode = AntialiasingMode::FXAA;

    MSAAConfig g_MASSConfig = {4};

    FXAAConfig g_FXAAConfig = FXAAConfig();

    HDRConfig g_HDRConfig = HDRConfig();

    const float kInitialMinLog = -12.0f;
    const float kInitialMaxLog = 4.0f;

    BloomConfig g_BloomConfig = BloomConfig();

    ToneMappingConfig g_ToneMappingConfig = ToneMappingConfig();

    SHConfig g_SHConfig = SHConfig();

    AOConfig g_AAConfig = AOConfig();
}