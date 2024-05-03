#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    namespace VolumeCloudSpace
    {
        /*
        float mCrispiness = 43.0f;
		float mCurliness = 1.1f;
		float mCoverage = 0.305f;
		float mAmbientColor[3] = { 102.0f / 255.0f, 104.0f / 255.0f, 105.0f / 255.0f };
		float mWindSpeedMultiplier = 175.0f;
		float mLightAbsorption = 0.0015f;
		float mCloudsBottomHeight = 2340.0f;
		float mCloudsTopHeight = 16400.0f;
		float mDensityFactor = 0.006f;
		float mDownscaleFactor = 0.5f;
		float mDistanceToFadeFrom = 58000.0f;
		float mDistanceOfFade = 10000.0f;
        */

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
            float  CloudsBottomHeight;
            float  CloudsTopHeight;
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

        CloudsConsCB InitDefaultCloudsCons(float time, glm::float3 wind);

    }
}

