#pragma once

#include "diffusion_pfoile_settings.h"

namespace MoYu
{
    DiffusionProfile::DiffusionProfile()
    {
        ResetToDefault();
    }

    void DiffusionProfile::ResetToDefault()
    {
        scatteringDistance = glm::float4(0.5, 0.5, 0.5, 1);
        scatteringDistanceMultiplier = 1;
        transmissionTint = glm::float4(1, 1, 1, 1);
        texturingMode = TexturingMode::PreAndPostScatter;
        transmissionMode = TransmissionMode::ThinObject;
        thicknessRemap = glm::float2(0.0f, 5.0f);
        worldScale = 1.0f;
        ior = 1.4f; // Typical value for skin specular reflectance
    }

    void DiffusionProfile::Validate()
    {
        thicknessRemap.y = glm::max(thicknessRemap.y, 0.0f);
        thicknessRemap.x = glm::clamp(thicknessRemap.x, 0.0f, thicknessRemap.y);
        worldScale = glm::max(worldScale, 0.001f);
        ior = glm::clamp(ior, 1.0f, 2.0f);

        UpdateKernel();
    }

    void DiffusionProfile::UpdateKernel()
    {
        glm::float3 sd = scatteringDistanceMultiplier * (glm::float3)(scatteringDistance);

        // Rather inconvenient to support (S = Inf).
        shapeParam = glm::float3(
            glm::min(float(16777216), 1.0f / sd.x), 
            glm::min(float(16777216), 1.0f / sd.y), 
            glm::min(float(16777216), 1.0f / sd.z));

        // Filter radius is, strictly speaking, infinite.
        // The magnitude of the function decays exponentially, but it is never truly zero.
        // To estimate the radius, we can use adapt the "three-sigma rule" by defining
        // the radius of the kernel by the value of the CDF which corresponds to 99.7%
        // of the energy of the filter.
        float cdf = 0.997f;

        // Importance sample the normalized diffuse reflectance profile for the computed value of 's'.
        // ------------------------------------------------------------------------------------
        // R[r, phi, s]   = s * (Exp[-r * s] + Exp[-r * s / 3]) / (8 * Pi * r)
        // PDF[r, phi, s] = r * R[r, phi, s]
        // CDF[r, s]      = 1 - 1/4 * Exp[-r * s] - 3/4 * Exp[-r * s / 3]
        // ------------------------------------------------------------------------------------
        // We importance sample the color channel with the widest scattering distance.
        maxScatteringDistance = glm::max(glm::max(sd.x, sd.y), sd.z);

        filterRadius = SampleBurleyDiffusionProfile(cdf, maxScatteringDistance);
    }

    float DiffusionProfile::SampleBurleyDiffusionProfile(float u, float rcpS)
    {
        u = 1 - u; // Convert CDF to CCDF

        float g = 1 + (4 * u) * (2 * u + glm::sqrt(1 + (4 * u) * u));
        float n = glm::pow(g, -1.0f / 3.0f);                      // g^(-1/3)
        float p = (g * n) * n;                                   // g^(+1/3)
        float c = 1 + p + n;                                     // 1 + g^(+1/3) + g^(-1/3)
        float x = 3 * glm::log(c / (4 * u));

        return x * rcpS;
    }

    DiffusionProfileSettings::DiffusionProfileSettings()
    {
        UpdateCache();
    }

    void DiffusionProfileSettings::UpdateCache()
    {
        worldScaleAndFilterRadiusAndThicknessRemap = glm::float4(
            profile.worldScale,
            profile.filterRadius,
            profile.thicknessRemap.x,
            profile.thicknessRemap.y - profile.thicknessRemap.x);
        shapeParamAndMaxScatterDist = glm::float4(profile.shapeParam, profile.maxScatteringDistance);
        // Convert ior to fresnel0
        float fresnel0 = (profile.ior - 1.0f) / (profile.ior + 1.0f);
        fresnel0 *= fresnel0; // square
        transmissionTintAndFresnel0 = glm::float4(
            profile.transmissionTint.r * 0.25f, 
            profile.transmissionTint.g * 0.25f, 
            profile.transmissionTint.b * 0.25f, 
            fresnel0); // Premultiplied
        disabledTransmissionTintAndFresnel0 = glm::float4(0.0f, 0.0f, 0.0f, fresnel0);
    }
}