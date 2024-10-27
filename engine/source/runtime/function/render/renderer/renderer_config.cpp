#include "renderer_config.h"

namespace EngineConfig
{
    const float kInitialMinLog = -12.0f;
    const float kInitialMaxLog = 4.0f;

    AntialiasingMode g_AntialiasingMode = AntialiasingMode::TAAMode;

    MSAAConfig g_MASSConfig = {4};

    FXAAConfig g_FXAAConfig = FXAAConfig();

    TAAConfig g_TAAConfig = TAAConfig();

    GIConfig g_GIConfig = GIConfig();

    SSGIConfig g_SSGIConfig = SSGIConfig();

    HDRConfig g_HDRConfig = HDRConfig();

    BloomConfig g_BloomConfig = BloomConfig();

    ToneMappingConfig g_ToneMappingConfig = ToneMappingConfig();

    SSRConfig g_SSRConfig = SSRConfig();

    SHConfig g_SHConfig = SHConfig();

    AOConfig g_AOConfig = AOConfig();

    VolumeLightConfig g_VolumeLightConfig = VolumeLightConfig();

    void InitEngineConfig()
    {
        g_AntialiasingMode = AntialiasingMode::TAAMode;
        g_MASSConfig = { 4 };
        g_FXAAConfig = FXAAConfig();
        g_TAAConfig = TAAConfig();
        g_GIConfig = GIConfig();
        g_HDRConfig = HDRConfig();
        g_BloomConfig = BloomConfig();
        g_ToneMappingConfig = ToneMappingConfig();
        g_SSRConfig = SSRConfig();
        g_SHConfig = SHConfig();
        g_AOConfig = AOConfig();
        g_VolumeLightConfig = VolumeLightConfig();
        g_SSGIConfig = SSGIConfig();
        g_VoxelGIConfig = VoxelGIConfig();
    }
}