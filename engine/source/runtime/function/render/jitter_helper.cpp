#include "runtime/function/render/jitter_helper.h"
#include "runtime/function/render/render_camera.h"

namespace MoYu
{
    float FrustumJitter::points_Halton_2_3_x16[16 * 2] = {0};

    FrustumJitter::FrustumJitter()
    {
        // points_Halton_2_3_xN
        InitializeHalton_2_3(points_Halton_2_3_x16, 16 * 2);

        focalMotionPos = glm::float3(0, 0, 0);
        focalMotionDir = glm::float3(1, 0, 0);

        activeSample = glm::float4(0, 0, 0, 0);
        activeIndex  = -2;

        feedbackMin = 0.88f;
        feedbackMin = 0.97f;
        motionScale = 1.0f;
    }

    // http://en.wikipedia.org/wiki/Halton_sequence
    float FrustumJitter::HaltonSeq(int prime, int index)
    {
        float r = 0.0f;
        float f = 1.0f;
        int   i = index;
        while (i > 0)
        {
            f /= prime;
            r += f * (i % prime);
            i = (int)glm::floor(i / (float)prime);
        }
        return r;
    }

	void FrustumJitter::InitializeHalton_2_3(float seq[], int length)
    {
        for (int i = 0, n = length / 2; i != n; i++)
        {
            float u        = HaltonSeq(2, i + 1) - 0.5f;
            float v        = HaltonSeq(3, i + 1) - 0.5f;
            seq[2 * i + 0] = u;
            seq[2 * i + 1] = v;
        }
    }

    glm::float2 FrustumJitter::Sample(int index)
    {
        int n = 16;
        int i = index % n;

        float x = points_Halton_2_3_x16[2 * i + 0];
        float y = points_Halton_2_3_x16[2 * i + 1];

        return glm::float2(x, y);
    }

    glm::float4 FrustumJitter::UpdateSample(RenderCamera* camera)
    {
        // update motion dir
        {
            glm::float3 oldWorld = focalMotionPos;
            glm::float3 newWorld = camera->forward() * camera->m_nearClipPlane;

            glm::float4x4 worldToCameraMatrix = camera->getWorldToCameraMatrix();

            glm::float3 oldPoint = (glm::float3x3)worldToCameraMatrix * oldWorld;
            glm::float3 newPoint = (glm::float3x3)worldToCameraMatrix * newWorld;
            glm::float3 mewDelta = (newPoint - oldPoint);
            mewDelta.z = 0.0f;

            float mag = glm::length(mewDelta);
            if (mag != 0.0f)
            {
                glm::float3 dir = mewDelta / mag;
                if (glm::length2(dir) != 0.0f)
                {
                    focalMotionPos = newWorld;
                    focalMotionDir = glm::lerp(focalMotionDir, dir, 0.2f);
                }
            }
        }

        // update jitter
        {
            if (activeIndex == -2)
            {
                activeSample = glm::float4(0, 0, 0, 0);
                activeIndex += 1;
            }
            else
            {
                activeIndex += 1;
                activeIndex %= 16;

                glm::float2 sample = Sample(activeIndex);
                activeSample.z = activeSample.x;
                activeSample.w = activeSample.y;
                activeSample.x = sample.x;
                activeSample.y = sample.y;
            }
        }

        return activeSample;
    }

} // namespace MoYu
