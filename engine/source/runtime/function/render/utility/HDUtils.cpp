#pragma once

#include "runtime/core/math/moyu_math2.h"

namespace MoYu
{
    class RenderCamera;

    class FrustumJitter
    {
    public:
        FrustumJitter();

        static float HaltonSeq(int prime, int index = 1 /* NOT! zero-based */);
        static void InitializeHalton_2_3(float seq[], int length);
        static glm::float2 Sample(int index);

        glm::float4 UpdateSample(RenderCamera* camera);

    public:
        static float points_Halton_2_3_x16[16 * 2];
        static float patternScale;

        glm::float3 focalMotionPos;
        glm::float3 focalMotionDir;

        glm::float4 activeSample;
        int activeIndex;

        float feedbackMin;
        float feedbackMax;
        float motionScale;
    };


} // namespace MoYu
