#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    namespace VolumeCloudSpace
    {

        struct CloudsConstants
        {
            glm::float4 AmbientColor;
            glm::float4 WindDir;
            float  WindSpeed;
            float  Time;
            float  Crispiness;
            float  Curliness;
            float  Coverage;
            float  Absorption;
            float  BottomHeight;
            float  TopHeight;
            float  DensityFactor;
            float  DistanceToFadeFrom; // when to start fading
            float  DistanceOfFade;     // for how long to fade
            float  PlanetRadius;
            glm::float2 UpsampleRatio;
            glm::float2 padding0;
        };

        struct CloudsConsCB
        {
            CloudsConstants cloudsCons;
        };

    }
}

