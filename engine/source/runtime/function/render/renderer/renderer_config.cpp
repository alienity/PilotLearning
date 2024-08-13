#include "renderer_config.h"

namespace EngineConfig
{

    AntialiasingMode g_AntialiasingMode = AntialiasingMode::TAA;

    MSAAConfig g_MASSConfig = {4};

    FXAAConfig g_FXAAConfig = FXAAConfig();

    TAAConfig g_TAAConfig = TAAConfig();

    HDRConfig g_HDRConfig = HDRConfig();

    const float kInitialMinLog = -12.0f;
    const float kInitialMaxLog = 4.0f;

    BloomConfig g_BloomConfig = BloomConfig();

    ToneMappingConfig g_ToneMappingConfig = ToneMappingConfig();

    SHConfig g_SHConfig = SHConfig();

    AOConfig g_AOConfig = AOConfig();

    VolumeLightConfig g_VolumeLightConfig = VolumeLightConfig();
}