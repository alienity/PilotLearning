//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "runtime/core/math/moyu_math2.h"

namespace SampleFramework11
{

// Calculates the Fresnel factor using Schlick's approximation
inline glm::float3 Fresnel(glm::float3 specAlbedo, glm::float3 h, glm::float3 l)
{
    glm::float3 fresnel = specAlbedo + (glm::float3(1.0f) - specAlbedo) * std::pow((1.0f - MoYu::Saturate(glm::float3::Dot(l, h))), 5.0f);

    // Fade out spec entirely when lower than 0.1% albedo
    fresnel *= MoYu::Saturate(glm::dot(specAlbedo, 333.0f));

    return fresnel;
}

// Calculates the Fresnel factor using Schlick's approximation
inline glm::float3 Fresnel(glm::float3 specAlbedo, glm::float3 fresnelAlbedo, glm::float3 h, glm::float3 l)
{
    glm::float3 fresnel = specAlbedo + (fresnelAlbedo - specAlbedo) * std::pow((1.0f - MoYu::Saturate(glm::float3::Dot(l, h))), 5.0f);

    // Fade out spec entirely when lower than 0.1% albedo
    fresnel *= MoYu::Saturate(glm::float3::Dot(specAlbedo, 333.0f));

    return fresnel;
}

// Helper for computing the GGX visibility term
inline float GGX_V1(float m2, float nDotX)
{
    return 1.0f / (nDotX + sqrt(m2 + (1 - m2) * nDotX * nDotX));
}

// Computes the specular term using a GGX microfacet distribution, with a matching
// geometry factor and visibility term. Based on "Microfacet Models for Refraction Through
// Rough Surfaces" [Walter 07]. m is roughness, n is the surface normal, h is the half vector,
// l is the direction to the light source, and specAlbedo is the RGB specular albedo
inline float GGX_Specular(float m, const glm::float3& n, const glm::float3& h, const glm::float3& v, const glm::float3& l)
{
    float nDotH = MoYu::Saturate(glm::float3::Dot(n, h));
    float nDotL = MoYu::Saturate(glm::float3::Dot(n, l));
    float nDotV = MoYu::Saturate(glm::float3::Dot(n, v));

    float nDotH2 = nDotH * nDotH;
    float m2 = m * m;

    // Calculate the distribution term
    float d = m2 / (Pi * Square(nDotH * nDotH * (m2 - 1) + 1));

    // Calculate the matching visibility term
    float v1i = GGX_V1(m2, nDotL);
    float v1o = GGX_V1(m2, nDotV);
    float vis = v1i * v1o;

    return d * vis;
}
// Computes the radiance reflected off a surface towards the eye given
// the differential irradiance from a given direction
inline glm::float3 CalcLighting(const glm::float3& normal, const glm::float3& lightIrradiance,
                    const glm::float3& lightDir, const glm::float3& diffuseAlbedo, const glm::float3& position,
                    const glm::float3& cameraPos, float roughness, bool includeSpecular, glm::float3 specAlbedo)
{
    glm::float3 lighting = diffuseAlbedo * InvPi;

    if(includeSpecular && glm::float3::Dot(normal, lightDir) > 0.0f)
    {
        glm::float3 view = glm::float3::Normalize(cameraPos - position);
        glm::float3 h = glm::float3::Normalize(view + lightDir);

        glm::float3 fresnel = Fresnel(specAlbedo, h, lightDir);

        float specular = GGX_Specular(roughness, normal, h, view, lightDir);
        lighting += specular * fresnel;
    }

    return lighting * lightIrradiance;
}

}