#pragma once

//------------------------------------------------------------------------------
// Shadowing
//------------------------------------------------------------------------------

#if defined(VARIANT_HAS_SHADOWING)
/**
 * Computes the light space position of the specified world space point.
 * The returned point may contain a bias to attempt to eliminate common
 * shadowing artifacts such as "acne". To achieve this, the world space
 * normal at the point must also be passed to this function.
 * Normal bias is not used for VSM.
 */

float4 computeLightSpacePosition(float3         p,
                                 const float3   n,
                                 const float3   dir,
                                 const float    b,
                                 const float4x4 lightFromWorldMatrix)
{

#if !defined(VARIANT_HAS_VSM)
    float cosTheta = saturate(dot(n, dir));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    p += n * (sinTheta * b);
#endif

    return mulMat4x4Float3(lightFromWorldMatrix, p);
}

#endif // VARIANT_HAS_SHADOWING
