#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "BSDF.hlsli"
#include "IBLHelper.hlsli"
#include "SphericalHarmonicsHelper.hlsli"

#define LIGHTFEATUREFLAGS_PUNCTUAL (4096)
#define LIGHTFEATUREFLAGS_AREA (8192)
#define LIGHTFEATUREFLAGS_DIRECTIONAL (16384)
#define LIGHTFEATUREFLAGS_ENV (32768)
#define LIGHTFEATUREFLAGS_SKY (65536)
#define LIGHTFEATUREFLAGS_SSREFRACTION (131072)
#define LIGHTFEATUREFLAGS_SSREFLECTION (262144)

//================================================================================================

// Ligthing convention
// Light direction is oriented backward (-Z). i.e in shader code, light direction is -lightData.forward

//-----------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------

// Performs the mapping of the vector 'v' centered within the axis-aligned cube
// of dimensions [-1, 1]^3 to a vector centered within the unit sphere.
// The function expects 'v' to be within the cube (possibly unexpected results otherwise).
// Ref: http://mathproofs.blogspot.com/2005/07/mapping-cube-to-sphere.html
float3 MapCubeToSphere(float3 v)
{
    float3 v2 = v * v;
    float2 vr3 = v2.xy * rcp(3.0);
    return v * sqrt((float3) 1.0 - 0.5 * v2.yzx - 0.5 * v2.zxy + vr3.yxx * v2.zzy);
}

// Computes the squared magnitude of the vector computed by MapCubeToSphere().
float ComputeCubeToSphereMapSqMagnitude(float3 v)
{
    float3 v2 = v * v;
    // Note: dot(v, v) is often computed before this function is called,
    // so the compiler should optimize and use the precomputed result here.
    return dot(v, v) - v2.x * v2.y - v2.y * v2.z - v2.z * v2.x + v2.x * v2.y * v2.z;
}

// texelArea = 4.0 / (resolution * resolution).
// Ref: http://bpeers.com/blog/?itemid=1017
// This version is less accurate, but much faster than this one:
// http://www.rorydriscoll.com/2012/01/15/cubemap-texel-solid-angle/
float ComputeCubemapTexelSolidAngle(float3 L, float texelArea)
{
    // Stretch 'L' by (1/d) so that it points at a side of a [-1, 1]^2 cube.
    float d = Max3(abs(L.x), abs(L.y), abs(L.z));
    // Since 'L' is a unit vector, we can directly compute its
    // new (inverse) length without dividing 'L' by 'd' first.
    float invDist = d;

    // dw = dA * cosTheta / (dist * dist), cosTheta = 1.0 / dist,
    // where 'dA' is the area of the cube map texel.
    return texelArea * invDist * invDist * invDist;
}

// Only makes sense for Monte-Carlo integration.
// Normalize by dividing by the total weight (or the number of samples) in the end.
// Integrate[6*(u^2+v^2+1)^(-3/2), {u,-1,1},{v,-1,1}] = 4 * Pi
// Ref: "Stupid Spherical Harmonics Tricks", p. 9.
float ComputeCubemapTexelSolidAngle(float2 uv)
{
    float u = uv.x, v = uv.y;
    return pow(1 + u * u + v * v, -1.5);
}

float ConvertEvToLuminance(float ev)
{
    return exp2(ev - 3.0);
}

float ConvertLuminanceToEv(float luminance)
{
    float k = 12.5f;

    return log2((luminance * 100.0) / k);
}

//-----------------------------------------------------------------------------
// Attenuation functions
//-----------------------------------------------------------------------------

// Ref: Moving Frostbite to PBR.

// Non physically based hack to limit light influence to attenuationRadius.
// Square the result to smoothen the function.
float DistanceWindowing(float distSquare, float rangeAttenuationScale, float rangeAttenuationBias)
{
    // If (range attenuation is enabled)
    //   rangeAttenuationScale = 1 / r^2
    //   rangeAttenuationBias  = 1
    // Else
    //   rangeAttenuationScale = 2^12 / r^2
    //   rangeAttenuationBias  = 2^24
    return saturate(rangeAttenuationBias - Sq(distSquare * rangeAttenuationScale));
}

float SmoothDistanceWindowing(float distSquare, float rangeAttenuationScale, float rangeAttenuationBias)
{
    float factor = DistanceWindowing(distSquare, rangeAttenuationScale, rangeAttenuationBias);
    return Sq(factor);
}

#define PUNCTUAL_LIGHT_THRESHOLD 0.01 // 1cm (in Unity 1 is 1m)

// Return physically based quadratic attenuation + influence limit to reach 0 at attenuationRadius
float SmoothWindowedDistanceAttenuation(float distSquare, float distRcp, float rangeAttenuationScale, float rangeAttenuationBias)
{
    float attenuation = min(distRcp, 1.0 / PUNCTUAL_LIGHT_THRESHOLD);
    attenuation *= DistanceWindowing(distSquare, rangeAttenuationScale, rangeAttenuationBias);

    // Effectively results in (distRcp)^2 * SmoothDistanceWindowing(...).
    return Sq(attenuation);
}

// Square the result to smoothen the function.
float AngleAttenuation(float cosFwd, float lightAngleScale, float lightAngleOffset)
{
    return saturate(cosFwd * lightAngleScale + lightAngleOffset);
}

float SmoothAngleAttenuation(float cosFwd, float lightAngleScale, float lightAngleOffset)
{
    float attenuation = AngleAttenuation(cosFwd, lightAngleScale, lightAngleOffset);
    return Sq(attenuation);
}

// Combines SmoothWindowedDistanceAttenuation() and SmoothAngleAttenuation() in an efficient manner.
// distances = {d, d^2, 1/d, d_proj}, where d_proj = dot(lightToSample, lightData.forward).
float PunctualLightAttenuation(float4 distances, float rangeAttenuationScale, float rangeAttenuationBias,
                              float lightAngleScale, float lightAngleOffset)
{
    float distSq = distances.y;
    float distRcp = distances.z;
    float distProj = distances.w;
    float cosFwd = distProj * distRcp;

    float attenuation = min(distRcp, 1.0 / PUNCTUAL_LIGHT_THRESHOLD);
    attenuation *= DistanceWindowing(distSq, rangeAttenuationScale, rangeAttenuationBias);
    attenuation *= AngleAttenuation(cosFwd, lightAngleScale, lightAngleOffset);

    // Effectively results in SmoothWindowedDistanceAttenuation(...) * SmoothAngleAttenuation(...).
    return Sq(attenuation);
}

// Applies SmoothDistanceWindowing() after transforming the attenuation ellipsoid into a sphere.
// If r = rsqrt(invSqRadius), then the ellipsoid is defined s.t. r1 = r / invAspectRatio, r2 = r3 = r.
// The transformation is performed along the major axis of the ellipsoid (corresponding to 'r1').
// Both the ellipsoid (e.i. 'axis') and 'unL' should be in the same coordinate system.
// 'unL' should be computed from the center of the ellipsoid.
float EllipsoidalDistanceAttenuation(float3 unL, float3 axis, float invAspectRatio,
                                    float rangeAttenuationScale, float rangeAttenuationBias)
{
    // Project the unnormalized light vector onto the axis.
    float projL = dot(unL, axis);

    // Transform the light vector so that we can work with
    // with the ellipsoid as if it was a sphere with the radius of light's range.
    float diff = projL - projL * invAspectRatio;
    unL -= diff * axis;

    float sqDist = dot(unL, unL);
    return SmoothDistanceWindowing(sqDist, rangeAttenuationScale, rangeAttenuationBias);
}

// Applies SmoothDistanceWindowing() using the axis-aligned ellipsoid of the given dimensions.
// Both the ellipsoid and 'unL' should be in the same coordinate system.
// 'unL' should be computed from the center of the ellipsoid.
float EllipsoidalDistanceAttenuation(float3 unL, float3 invHalfDim,
                                    float rangeAttenuationScale, float rangeAttenuationBias)
{
    // Transform the light vector so that we can work with
    // with the ellipsoid as if it was a unit sphere.
    unL *= invHalfDim;

    float sqDist = dot(unL, unL);
    return SmoothDistanceWindowing(sqDist, rangeAttenuationScale, rangeAttenuationBias);
}

// Applies SmoothDistanceWindowing() after mapping the axis-aligned box to a sphere.
// If the diagonal of the box is 'd', invHalfDim = rcp(0.5 * d).
// Both the box and 'unL' should be in the same coordinate system.
// 'unL' should be computed from the center of the box.
float BoxDistanceAttenuation(float3 unL, float3 invHalfDim,
                            float rangeAttenuationScale, float rangeAttenuationBias)
{
    float attenuation = 0.0;

    // Transform the light vector so that we can work with
    // with the box as if it was a [-1, 1]^2 cube.
    unL *= invHalfDim;

    // Our algorithm expects the input vector to be within the cube.
    if (!(Max3(abs(unL.x), abs(unL.y), abs(unL.z)) > 1.0))
    {
        float sqDist = ComputeCubeToSphereMapSqMagnitude(unL);
        attenuation = SmoothDistanceWindowing(sqDist, rangeAttenuationScale, rangeAttenuationBias);
    }
    return attenuation;
}

//-----------------------------------------------------------------------------
// IES Helper
//-----------------------------------------------------------------------------

float2 GetIESTextureCoordinate(float3x3 lightToWord, float3 L)
{
    // IES need to be sample in light space
    float3 dir = mul(lightToWord, -L); // Using matrix on left side do a transpose

    // convert to spherical coordinate
    float2 sphericalCoord; // .x is theta, .y is phi
    // Texture is encoded with cos(phi), scale from -1..1 to 0..1
    sphericalCoord.y = (dir.z * 0.5) + 0.5;
    float theta = atan2(dir.y, dir.x);
    sphericalCoord.x = theta * INV_TWO_PI;

    return sphericalCoord;
}

//-----------------------------------------------------------------------------
// Lighting functions
//-----------------------------------------------------------------------------

// Ref: Horizon Occlusion for Normal Mapped Reflections: http://marmosetco.tumblr.com/post/81245981087
float GetHorizonOcclusion(float3 V, float3 normalWS, float3 vertexNormal, float horizonFade)
{
    float3 R = reflect(-V, normalWS);
    float specularOcclusion = saturate(1.0 + horizonFade * dot(R, vertexNormal));
    // smooth it
    return specularOcclusion * specularOcclusion;
}

// Ref: Moving Frostbite to PBR - Gotanda siggraph 2011
// Return specular occlusion based on ambient occlusion (usually get from SSAO) and view/roughness info
float GetSpecularOcclusionFromAmbientOcclusion(float NdotV, float ambientOcclusion, float roughness)
{
    return saturate(PositivePow(NdotV + ambientOcclusion, exp2(-16.0 * roughness - 1.0)) - 1.0 + ambientOcclusion);
}

// ref: Practical floattime Strategies for Accurate Indirect Occlusion
// Update ambient occlusion to colored ambient occlusion based on statitics of how light is bouncing in an object and with the albedo of the object
float3 GTAOMultiBounce(float visibility, float3 albedo)
{
    float3 a = 2.0404 * albedo - 0.3324;
    float3 b = -4.7951 * albedo + 0.6417;
    float3 c = 2.7552 * albedo + 0.6903;

    float x = visibility;
    return max(x, ((x * a + b) * x + c) * x);
}

// Based on Oat and Sander's 2007 technique
// Area/solidAngle of intersection of two cone
float SphericalCapIntersectionSolidArea(float cosC1, float cosC2, float cosB)
{
    float r1 = FastACos(cosC1);
    float r2 = FastACos(cosC2);
    float rd = FastACos(cosB);
    float area = 0.0;

    if (rd <= max(r1, r2) - min(r1, r2))
    {
        // One cap is completely inside the other
        area = TWO_PI - TWO_PI * max(cosC1, cosC2);
    }
    else if (rd >= r1 + r2)
    {
        // No intersection exists
        area = 0.0;
    }
    else
    {
        float diff = abs(r1 - r2);
        float den = r1 + r2 - diff;
        float x = 1.0 - saturate((rd - diff) / max(den, 0.0001));
        area = smoothstep(0.0, 1.0, x);
        area *= TWO_PI - TWO_PI * max(cosC1, cosC2);
    }

    return area;
}

// ref: Practical floattime Strategies for Accurate Indirect Occlusion
// http://blog.selfshadow.com/publications/s2016-shading-course/#course_content
// Original Cone-Cone method with cosine weighted assumption (p129 s2016_pbs_activision_occlusion)
float GetSpecularOcclusionFromBentAO_ConeCone(float3 V, float3 bentNormalWS, float3 normalWS, float ambientOcclusion, float roughness)
{
    // Retrieve cone angle
    // Ambient occlusion is cosine weighted, thus use following equation. See slide 129
    float cosAv = sqrt(1.0 - ambientOcclusion);
    roughness = max(roughness, 0.01); // Clamp to 0.01 to avoid edge cases
    float cosAs = exp2((-log(10.0) / log(2.0)) * Sq(roughness));
    float cosB = dot(bentNormalWS, reflect(-V, normalWS));
    return SphericalCapIntersectionSolidArea(cosAv, cosAs, cosB) / (TWO_PI * (1.0 - cosAs));
}

float GetSpecularOcclusionFromBentAO(float3 V, float3 bentNormalWS, float3 normalWS, float ambientOcclusion, float roughness)
{
    // Pseudo code:
    //SphericalGaussian NDF = WarpedGGXDistribution(normalWS, roughness, V);
    //SphericalGaussian Visibility = VisibilityConeSG(bentNormalWS, ambientOcclusion);
    //SphericalGaussian UpperHemisphere = UpperHemisphereSG(normalWS);
    //return saturate( InnerProduct(NDF, Visibility) / InnerProduct(NDF, UpperHemisphere) );

    // 1. Approximate visibility cone with a spherical gaussian of amplitude A=1
    // For a cone angle X, we can determine sharpness so that all point inside the cone have a value > Y
    // sharpness = (log(Y) - log(A)) / (cos(X) - 1)
    // For AO cone, cos(X) = sqrt(1 - ambientOcclusion)
    // -> for Y = 0.1, sharpness = -1.0 / (sqrt(1-ao) - 1)
    float vs = -1.0f / min(sqrt(1.0f - ambientOcclusion) - 1.0f, -0.001f);

    // 2. Approximate upper hemisphere with sharpness = 0.8 and amplitude = 1
    float us = 0.8f;

    // 3. Compute warped SG Axis of GGX distribution
    // Ref: All-Frequency Rendering of Dynamic, Spatially-Varying Reflectance
    // https://www.microsoft.com/en-us/research/wp-content/uploads/2009/12/sg.pdf
    float NoV = dot(V, normalWS);
    float3 NDFAxis = (2 * NoV * normalWS - V) * (0.5f / max(roughness * roughness * NoV, 0.001f));

    float umLength1 = length(NDFAxis + vs * bentNormalWS);
    float umLength2 = length(NDFAxis + us * normalWS);
    float d1 = 1 - exp(-2 * umLength1);
    float d2 = 1 - exp(-2 * umLength2);

    float expFactor1 = exp(umLength1 - umLength2 + us - vs);

    return saturate(expFactor1 * (d1 * umLength2) / (d2 * umLength1));
}

// Ref: Steve McAuley - Energy-Conserving Wrapped Diffuse
float ComputeWrappedDiffuseLighting(float NdotL, float w)
{
    return saturate((NdotL + w) / ((1.0 + w) * (1.0 + w)));
}

// Ref: Stephen McAuley - Advances in Rendering: Graphics Research and Video Game Production
float3 ComputeWrappedNormal(float3 N, float3 L, float w)
{
    float NdotL = dot(N, L);
    float wrappedNdotL = saturate((NdotL + w) / (1 + w));
    float sinPhi = lerp(w, 0.f, wrappedNdotL);
    float cosPhi = sqrt(1.0f - sinPhi * sinPhi);
    return normalize(cosPhi * N + sinPhi * cross(cross(N, L), N));
}

// Jimenez variant for eye
float ComputeWrappedPowerDiffuseLighting(float NdotL, float w, float p)
{
    return pow(saturate((NdotL + w) / (1.0 + w)), p) * (p + 1) / (w * 2.0 + 2.0);
}

// Ref: The Technical Art of Uncharted 4 - Brinck and Maximov 2016
float ComputeMicroShadowing(float AO, float NdotL, float opacity)
{
    float aperture = 2.0 * AO * AO;
    float microshadow = saturate(NdotL + aperture - 1.0);
    return lerp(1.0, microshadow, opacity);
}

float3 ComputeShadowColor(float shadow, float3 shadowTint, float penumbraFlag)
{
    // The origin expression is
    // lerp(float3(1.0, 1.0, 1.0) - ((1.0 - shadow) * (float3(1.0, 1.0, 1.0) - shadowTint))
    //            , shadow * lerp(shadowTint, lerp(shadowTint, float3(1.0, 1.0, 1.0), shadow), shadow)
    //            , penumbraFlag);
    // it has been simplified to this
    float3 invTint = float3(1.0, 1.0, 1.0) - shadowTint;
    float shadow3 = shadow * shadow * shadow;
    return lerp(float3(1.0, 1.0, 1.0) - ((1.0 - shadow) * invTint)
                , shadow3 * invTint + shadow * shadowTint,
                penumbraFlag);

}

// This is the same method as the one above. Simply the shadow is a float3 to support colored shadows.
float3 ComputeShadowColor(float3 shadow, float3 shadowTint, float penumbraFlag)
{
    // The origin expression is
    // lerp(float3(1.0, 1.0, 1.0) - ((1.0 - shadow) * (float3(1.0, 1.0, 1.0) - shadowTint))
    //            , shadow * lerp(shadowTint, lerp(shadowTint, float3(1.0, 1.0, 1.0), shadow), shadow)
    //            , penumbraFlag);
    // it has been simplified to this
    float3 invTint = float3(1.0, 1.0, 1.0) - shadowTint;
    float3 shadow3 = shadow * shadow * shadow;
    return lerp(float3(1.0, 1.0, 1.0) - ((1.0 - shadow) * invTint)
                , shadow3 * invTint + shadow * shadowTint,
                penumbraFlag);

}

//-----------------------------------------------------------------------------
// Helper functions
//--------------------------------------------------------------------------- --

// Ref: "Crafting a Next-Gen Material Pipeline for The Order: 1886".
float ClampNdotV(float NdotV)
{
    return max(NdotV, 0.0001); // Approximately 0.0057 degree bias
}

// Helper function to return a set of common angle used when evaluating BSDF
// NdotL and NdotV are unclamped
void GetBSDFAngle(float3 V, float3 L, float NdotL, float NdotV,
                  out float LdotV, out float NdotH, out float LdotH, out float invLenLV)
{
    // Optimized math. Ref: PBR Diffuse Lighting for GGX + Smith Microsurfaces (slide 114), assuming |L|=1 and |V|=1
    LdotV = dot(L, V);
    invLenLV = rsqrt(max(2.0 * LdotV + 2.0, FLT_EPS)); // invLenLV = rcp(length(L + V)), clamp to avoid rsqrt(0) = inf, inf * 0 = NaN
    NdotH = saturate((NdotL + NdotV) * invLenLV);
    LdotH = saturate(invLenLV * LdotV + invLenLV);
}

// Inputs:    normalized normal and view vectors.
// Outputs:   front-facing normal, and the new non-negative value of the cosine of the view angle.
// Important: call Orthonormalize() on the tangent and recompute the bitangent afterwards.
float3 GetViewReflectedNormal(float3 N, float3 V, out float NdotV)
{
    // Fragments of front-facing geometry can have back-facing normals due to interpolation,
    // normal mapping and decals. This can cause visible artifacts from both direct (negative or
    // extremely high values) and indirect (incorrect lookup direction) lighting.
    // There are several ways to avoid this problem. To list a few:
    //
    // 1. Setting { NdotV = max(<N,V>, SMALL_VALUE) }. This effectively removes normal mapping
    // from the affected fragments, making the surface appear flat.
    //
    // 2. Setting { NdotV = abs(<N,V>) }. This effectively reverses the convexity of the surface.
    // It also reduces light leaking from non-shadow-casting lights. Note that 'NdotV' can still
    // be 0 in this case.
    //
    // It's important to understand that simply changing the value of the cosine is insufficient.
    // For one, it does not solve the incorrect lookup direction problem, since the normal itself
    // is not modified. There is a more insidious issue, however. 'NdotV' is a constituent element
    // of the mathematical system describing the relationships between different vectors - and
    // not just normal and view vectors, but also light vectors, half vectors, tangent vectors, etc.
    // Changing only one angle (or its cosine) leaves the system in an inconsistent state, where
    // certain relationships can take on different values depending on whether 'NdotV' is used
    // in the calculation or not. Therefore, it is important to change the normal (or another
    // vector) in order to leave the system in a consistent state.
    //
    // We choose to follow the conceptual approach (2) by reflecting the normal around the
    // (<N,V> = 0) boundary if necessary, as it allows us to preserve some normal mapping details.

    NdotV = dot(N, V);

    // N = (NdotV >= 0.0) ? N : (N - 2.0 * NdotV * V);
    N += (2.0 * saturate(-NdotV)) * V;
    NdotV = abs(NdotV);

    return N;
}

// Generates an orthonormal (row-major) basis from a unit vector. TODO: make it column-major.
// The resulting rotation matrix has the determinant of +1.
// Ref: 'ortho_basis_pixar_r2' from http://marc-b-reynolds.github.io/quaternions/2016/07/06/Orthonormal.html
float3x3 GetLocalFrame(float3 localZ)
{
    float x = localZ.x;
    float y = localZ.y;
    float z = localZ.z;
    float sz = FastSign(z);
    float a = 1 / (sz + z);
    float ya = y * a;
    float b = x * ya;
    float c = x * sz;

    float3 localX = float3(c * x * a - 1, sz * b, c);
    float3 localY = float3(b, y * ya - sz, y);

    // Note: due to the quaternion formulation, the generated frame is rotated by 180 degrees,
    // s.t. if localZ = {0, 0, 1}, then localX = {-1, 0, 0} and localY = {0, -1, 0}.
    return float3x3(localX, localY, localZ);
}

// Generates an orthonormal (row-major) basis from a unit vector. TODO: make it column-major.
// The resulting rotation matrix has the determinant of +1.
float3x3 GetLocalFrame(float3 localZ, float3 localX)
{
    float3 localY = cross(localZ, localX);

    return float3x3(localX, localY, localZ);
}

// Construct a right-handed view-dependent orthogonal basis around the normal:
// b0-b2 is the view-normal aka reflection plane.
float3x3 GetOrthoBasisViewNormal(float3 V, float3 N, float unclampedNdotV, bool testSingularity = false)
{
    float3x3 orthoBasisViewNormal;
    if (testSingularity && (abs(1.0 - unclampedNdotV) <= FLT_EPS))
    {
        // In this case N == V, and azimuth orientation around N shouldn't matter for the caller,
        // we can use any quaternion-based method, like Frisvad or Reynold's (Pixar):
        orthoBasisViewNormal = GetLocalFrame(N);
    }
    else
    {
        orthoBasisViewNormal[0] = normalize(V - N * unclampedNdotV);
        orthoBasisViewNormal[2] = N;
        orthoBasisViewNormal[1] = cross(orthoBasisViewNormal[2], orthoBasisViewNormal[0]);
    }
    return orthoBasisViewNormal;
}

// Move this here since it's used by both LightLoop.hlsl and RaytracingLightLoop.hlsl
bool IsMatchingLightLayer(uint lightLayers, uint renderingLayers)
{
    return (lightLayers & renderingLayers) != 0;
}

//================================================================================================

// Note: All NDF and diffuse term have a version with and without divide by PI.
// Version with divide by PI are use for direct lighting.
// Version without divide by PI are use for image based lighting where often the PI cancel during importance sampling

//-----------------------------------------------------------------------------
// Help for BSDF evaluation
//-----------------------------------------------------------------------------

// Cosine-weighted BSDF (a BSDF taking the projected solid angle into account).
// If some of the values are monochromatic, the compiler will optimize accordingly.
struct CBSDF
{
    float3 diffR; // Diffuse  reflection   (T -> MS -> T, same sides)
    float3 specR; // Specular reflection   (R, RR, TRT, etc)
    float3 diffT; // Diffuse  transmission (rough T or TT, opposite sides)
    float3 specT; // Specular transmission (T, TT, TRRT, etc)
};

//-----------------------------------------------------------------------------
// Fresnel term
//-----------------------------------------------------------------------------

float F_Schlick(float f0, float f90, float u)
{
    float x = 1.0 - u;
    float x2 = x * x;
    float x5 = x * x2 * x2;
    return (f90 - f0) * x5 + f0; // sub mul mul mul sub mad
}

float F_Schlick(float f0, float u)
{
    return F_Schlick(f0, 1.0, u); // sub mul mul mul sub mad
}

float3 F_Schlick(float3 f0, float f90, float u)
{
    float x = 1.0 - u;
    float x2 = x * x;
    float x5 = x * x2 * x2;
    return f0 * (1.0 - x5) + (f90 * x5); // sub mul mul mul sub mul mad*3
}

float3 F_Schlick(float3 f0, float u)
{
    return F_Schlick(f0, 1.0, u); // sub mul mul mul sub mad*3
}

// Does not handle TIR.
float F_Transm_Schlick(float f0, float f90, float u)
{
    float x = 1.0 - u;
    float x2 = x * x;
    float x5 = x * x2 * x2;
    return (1.0 - f90 * x5) - f0 * (1.0 - x5); // sub mul mul mul mad sub mad
}

// Does not handle TIR.
float F_Transm_Schlick(float f0, float u)
{
    return F_Transm_Schlick(f0, 1.0, u); // sub mul mul mad mad
}

// Does not handle TIR.
float3 F_Transm_Schlick(float3 f0, float f90, float u)
{
    float x = 1.0 - u;
    float x2 = x * x;
    float x5 = x * x2 * x2;
    return (1.0 - f90 * x5) - f0 * (1.0 - x5); // sub mul mul mul mad sub mad*3
}

// Does not handle TIR.
float3 F_Transm_Schlick(float3 f0, float u)
{
    return F_Transm_Schlick(f0, 1.0, u); // sub mul mul mad mad*3
}

// Compute the cos of critical angle: cos(asin(eta)) == sqrt(1.0 - eta*eta)
// eta == IORMedium/IORSource
// If eta >= 1 the it's an AirMedium interation, otherwise it's MediumAir interation
float CosCriticalAngle(float eta)
{
    return sqrt(max(1.0 - Sq(eta), 0.0));
    // For 1 <= IOR <= 4: Max error: 0.0268594
    //return eta >= 1.0 ? 0.0 : (((3.0 + eta) * sqrt(max(0.0, 1.0 - eta))) / (2.0 * sqrt(2.0)));
    // For 1 <= IOR <= 4: Max error: 0.00533065
    //return eta >= 1.0 ? 0.0 : (-((-23.0 - 10.0 * eta + Sq(eta)) * sqrt(max(0.0, 1.0 - eta))) / (16.0 * sqrt(2.0)));
    // For 1 <= IOR <= 4: Max error: 0.00129402
    //return eta >= 1.0 ? 0.0 : (((91.0 + 43.0 * eta - 7.0 * Sq(eta) + pow(eta, 3)) * sqrt(max(0.0, 1.0 - eta))) / (64. * sqrt(2.0)));
}

// Ref: https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
// Fresnel dielectric / dielectric
float F_FresnelDielectric(float ior, float u)
{
    float g = sqrt(Sq(ior) + Sq(u) - 1.0);

    // The "1.0 - saturate(1.0 - result)" formulation allows to recover form cases where g is undefined, for IORs < 1
    return 1.0 - saturate(1.0 - 0.5 * Sq((g - u) / (g + u)) * (1.0 + Sq(((g + u) * u - 1.0) / ((g - u) * u + 1.0))));
}

// Fresnel dieletric / conductor
// Note: etak2 = etak * etak (optimization for Artist Friendly Metallic Fresnel below)
// eta = eta_t / eta_i and etak = k_t / n_i
float3 F_FresnelConductor(float3 eta, float3 etak2, float cosTheta)
{
    float cosTheta2 = cosTheta * cosTheta;
    float sinTheta2 = 1.0 - cosTheta2;
    float3 eta2 = eta * eta;

    float3 t0 = eta2 - etak2 - sinTheta2;
    float3 a2plusb2 = sqrt(t0 * t0 + 4.0 * eta2 * etak2);
    float3 t1 = a2plusb2 + cosTheta2;
    float3 a = sqrt(0.5 * (a2plusb2 + t0));
    float3 t2 = 2.0 * a * cosTheta;
    float3 Rs = (t1 - t2) / (t1 + t2);

    float3 t3 = cosTheta2 * a2plusb2 + sinTheta2 * sinTheta2;
    float3 t4 = t2 * sinTheta2;
    float3 Rp = Rs * (t3 - t4) / (t3 + t4);

    return 0.5 * (Rp + Rs);
}

// Conversion FO/IOR

TEMPLATE_2_float(IorToFresnel0, transmittedIor, incidentIor, return Sq((transmittedIor - incidentIor) / (transmittedIor + incidentIor)) )
// ior is a value between 1.0 and 3.0. 1.0 is air interface
float IorToFresnel0(float transmittedIor)
{
    return IorToFresnel0(transmittedIor, 1.0);
}

// Assume air interface for top
// Note: We don't handle the case fresnel0 == 1
//float Fresnel0ToIor(float fresnel0)
//{
//    float sqrtF0 = sqrt(fresnel0);
//    return (1.0 + sqrtF0) / (1.0 - sqrtF0);
//}
TEMPLATE_1_float(Fresnel0ToIor, fresnel0, return ((1.0 + sqrt(fresnel0)) / (1.0 - sqrt(fresnel0))) )

// This function is a coarse approximation of computing fresnel0 for a different top than air (here clear coat of IOR 1.5) when we only have fresnel0 with air interface
// This function is equivalent to IorToFresnel0(Fresnel0ToIor(fresnel0), 1.5)
// mean
// float sqrtF0 = sqrt(fresnel0);
// return Sq(1.0 - 5.0 * sqrtF0) / Sq(5.0 - sqrtF0);
// Optimization: Fit of the function (3 mad) for range [0.04 (should return 0), 1 (should return 1)]
TEMPLATE_1_float(ConvertF0ForAirInterfaceToF0ForClearCoat15, fresnel0, return saturate(-0.0256868 + fresnel0 * (0.326846 + (0.978946 - 0.283835 * fresnel0) * fresnel0)))

// Even coarser approximation of ConvertF0ForAirInterfaceToF0ForClearCoat15 (above) for mobile (2 mad)
TEMPLATE_1_float(ConvertF0ForAirInterfaceToF0ForClearCoat15Fast, fresnel0, return saturate(fresnel0 * (fresnel0 * 0.526868 + 0.529324) - 0.0482256))

// Artist Friendly Metallic Fresnel Ref: http://jcgt.org/published/0003/04/03/paper.pdf

float3 GetIorN(float3 f0, float3 edgeTint)
{
    float3 sqrtF0 = sqrt(f0);
    return lerp((1.0 - f0) / (1.0 + f0), (1.0 + sqrtF0) / (1.0 - sqrt(f0)), edgeTint);
}

float3 getIorK2(float3 f0, float3 n)
{
    float3 nf0 = Sq(n + 1.0) * f0 - Sq(f0 - 1.0);
    return nf0 / (1.0 - f0);
}

// same as regular refract except there is not the test for total internal reflection + the vector is flipped for processing
float3 CoatRefract(float3 X, float3 N, float ieta)
{
    float XdotN = saturate(dot(N, X));
    return ieta * X + (sqrt(1 + ieta * ieta * (XdotN * XdotN - 1)) - ieta * XdotN) * N;
}

//-----------------------------------------------------------------------------
// Specular BRDF
//-----------------------------------------------------------------------------

float Lambda_GGX(float roughness, float3 V)
{
    return 0.5 * (sqrt(1.0 + (Sq(roughness * V.x) + Sq(roughness * V.y)) / Sq(V.z)) - 1.0);
}

float D_GGXNoPI(float NdotH, float roughness)
{
    float a2 = Sq(roughness);
    float s = (NdotH * a2 - NdotH) * NdotH + 1.0;

    // If roughness is 0, returns (NdotH == 1 ? 1 : 0).
    // That is, it returns 1 for perfect mirror reflection, and 0 otherwise.
    return SafeDiv(a2, s * s);
}

float D_GGX(float NdotH, float roughness)
{
    return INV_PI * D_GGXNoPI(NdotH, roughness);
}

// Ref: Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs, p. 19, 29.
// p. 84 (37/60)
float G_MaskingSmithGGX(float NdotV, float roughness)
{
    // G1(V, H)    = HeavisideStep(VdotH) / (1 + Lambda(V)).
    // Lambda(V)        = -0.5 + 0.5 * sqrt(1 + 1 / a^2).
    // a           = 1 / (roughness * tan(theta)).
    // 1 + Lambda(V)    = 0.5 + 0.5 * sqrt(1 + roughness^2 * tan^2(theta)).
    // tan^2(theta) = (1 - cos^2(theta)) / cos^2(theta) = 1 / cos^2(theta) - 1.
    // Assume that (VdotH > 0), e.i. (acos(LdotV) < Pi).

    return 1.0 / (0.5 + 0.5 * sqrt(1.0 + Sq(roughness) * (1.0 / Sq(NdotV) - 1.0)));
}

// Precompute part of lambdaV
float GetSmithJointGGXPartLambdaV(float NdotV, float roughness)
{
    float a2 = Sq(roughness);
    return sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
}

// Note: V = G / (4 * NdotL * NdotV)
// Ref: http://jcgt.org/published/0003/02/03/paper.pdf
float V_SmithJointGGX(float NdotL, float NdotV, float roughness, float partLambdaV)
{
    float a2 = Sq(roughness);

    // Original formulation:
    // lambda_v = (-1 + sqrt(a2 * (1 - NdotL2) / NdotL2 + 1)) * 0.5
    // lambda_l = (-1 + sqrt(a2 * (1 - NdotV2) / NdotV2 + 1)) * 0.5
    // G        = 1 / (1 + lambda_v + lambda_l);

    // Reorder code to be more optimal:
    float lambdaV = NdotL * partLambdaV;
    float lambdaL = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);

    // Simplify visibility term: (2.0 * NdotL * NdotV) /  ((4.0 * NdotL * NdotV) * (lambda_v + lambda_l))
    return 0.5 / max(lambdaV + lambdaL, float_MIN);
}

float V_SmithJointGGX(float NdotL, float NdotV, float roughness)
{
    float partLambdaV = GetSmithJointGGXPartLambdaV(NdotV, roughness);
    return V_SmithJointGGX(NdotL, NdotV, roughness, partLambdaV);
}

// Inline D_GGX() * V_SmithJointGGX() together for better code generation.
float DV_SmithJointGGX(float NdotH, float NdotL, float NdotV, float roughness, float partLambdaV)
{
    float a2 = Sq(roughness);
    float s = (NdotH * a2 - NdotH) * NdotH + 1.0;

    float lambdaV = NdotL * partLambdaV;
    float lambdaL = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);

    float2 D = float2(a2, s * s); // Fraction without the multiplier (1/Pi)
    float2 G = float2(1, lambdaV + lambdaL); // Fraction without the multiplier (1/2)

    // This function is only used for direct lighting.
    // If roughness is 0, the probability of hitting a punctual or directional light is also 0.
    // Therefore, we return 0. The most efficient way to do it is with a max().
    return INV_PI * 0.5 * (D.x * G.x) / max(D.y * G.y, float_MIN);
}

float DV_SmithJointGGX(float NdotH, float NdotL, float NdotV, float roughness)
{
    float partLambdaV = GetSmithJointGGXPartLambdaV(NdotV, roughness);
    return DV_SmithJointGGX(NdotH, NdotL, NdotV, roughness, partLambdaV);
}

// Precompute a part of LambdaV.
// Note on this linear approximation.
// Exact for roughness values of 0 and 1. Also, exact when the cosine is 0 or 1.
// Otherwise, the worst case relative error is around 10%.
// https://www.desmos.com/calculator/wtp8lnjutx
float GetSmithJointGGXPartLambdaVApprox(float NdotV, float roughness)
{
    float a = roughness;
    return NdotV * (1 - a) + a;
}

float V_SmithJointGGXApprox(float NdotL, float NdotV, float roughness, float partLambdaV)
{
    float a = roughness;

    float lambdaV = NdotL * partLambdaV;
    float lambdaL = NdotV * (NdotL * (1 - a) + a);

    return 0.5 / (lambdaV + lambdaL);
}

float V_SmithJointGGXApprox(float NdotL, float NdotV, float roughness)
{
    float partLambdaV = GetSmithJointGGXPartLambdaVApprox(NdotV, roughness);
    return V_SmithJointGGXApprox(NdotL, NdotV, roughness, partLambdaV);
}

// roughnessT -> roughness in tangent direction
// roughnessB -> roughness in bitangent direction
float D_GGXAnisoNoPI(float TdotH, float BdotH, float NdotH, float roughnessT, float roughnessB)
{
    float a2 = roughnessT * roughnessB;
    float3 v = float3(roughnessB * TdotH, roughnessT * BdotH, a2 * NdotH);
    float s = dot(v, v);

    // If roughness is 0, returns (NdotH == 1 ? 1 : 0).
    // That is, it returns 1 for perfect mirror reflection, and 0 otherwise.
    return SafeDiv(a2 * a2 * a2, s * s);
}

float D_GGXAniso(float TdotH, float BdotH, float NdotH, float roughnessT, float roughnessB)
{
    return INV_PI * D_GGXAnisoNoPI(TdotH, BdotH, NdotH, roughnessT, roughnessB);
}

float GetSmithJointGGXAnisoPartLambdaV(float TdotV, float BdotV, float NdotV, float roughnessT, float roughnessB)
{
    return length(float3(roughnessT * TdotV, roughnessB * BdotV, NdotV));
}

// Note: V = G / (4 * NdotL * NdotV)
// Ref: https://cedec.cesa.or.jp/2015/session/ENG/14698.html The Rendering Materials of Far Cry 4
float V_SmithJointGGXAniso(float TdotV, float BdotV, float NdotV, float TdotL, float BdotL, float NdotL, float roughnessT, float roughnessB, float partLambdaV)
{
    float lambdaV = NdotL * partLambdaV;
    float lambdaL = NdotV * length(float3(roughnessT * TdotL, roughnessB * BdotL, NdotL));

    return 0.5 / (lambdaV + lambdaL);
}

float V_SmithJointGGXAniso(float TdotV, float BdotV, float NdotV, float TdotL, float BdotL, float NdotL, float roughnessT, float roughnessB)
{
    float partLambdaV = GetSmithJointGGXAnisoPartLambdaV(TdotV, BdotV, NdotV, roughnessT, roughnessB);
    return V_SmithJointGGXAniso(TdotV, BdotV, NdotV, TdotL, BdotL, NdotL, roughnessT, roughnessB, partLambdaV);
}

// Inline D_GGXAniso() * V_SmithJointGGXAniso() together for better code generation.
float DV_SmithJointGGXAniso(float TdotH, float BdotH, float NdotH, float NdotV,
                           float TdotL, float BdotL, float NdotL,
                           float roughnessT, float roughnessB, float partLambdaV)
{
    float a2 = roughnessT * roughnessB;
    float3 v = float3(roughnessB * TdotH, roughnessT * BdotH, a2 * NdotH);
    float s = dot(v, v);

    float lambdaV = NdotL * partLambdaV;
    float lambdaL = NdotV * length(float3(roughnessT * TdotL, roughnessB * BdotL, NdotL));

    float2 D = float2(a2 * a2 * a2, s * s); // Fraction without the multiplier (1/Pi)
    float2 G = float2(1, lambdaV + lambdaL); // Fraction without the multiplier (1/2)

    // This function is only used for direct lighting.
    // If roughness is 0, the probability of hitting a punctual or directional light is also 0.
    // Therefore, we return 0. The most efficient way to do it is with a max().
    return (INV_PI * 0.5) * (D.x * G.x) / max(D.y * G.y, float_MIN);
}

float DV_SmithJointGGXAniso(float TdotH, float BdotH, float NdotH,
                           float TdotV, float BdotV, float NdotV,
                           float TdotL, float BdotL, float NdotL,
                           float roughnessT, float roughnessB)
{
    float partLambdaV = GetSmithJointGGXAnisoPartLambdaV(TdotV, BdotV, NdotV, roughnessT, roughnessB);
    return DV_SmithJointGGXAniso(TdotH, BdotH, NdotH, NdotV,
                                 TdotL, BdotL, NdotL,
                                 roughnessT, roughnessB, partLambdaV);
}

// Get projected roughness for a certain normalized direction V in tangent space
// and an anisotropic roughness
// Ref: Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs, Heitz 2014, pp. 86, 88 - 39/60, 41/60
float GetProjectedRoughness(float TdotV, float BdotV, float NdotV, float roughnessT, float roughnessB)
{
    float2 roughness = float2(roughnessT, roughnessB);
    float sinTheta2 = max((1 - Sq(NdotV)), FLT_MIN);
    // if sinTheta^2 = 0, NdotV = 1, TdotV = BdotV = 0 and roughness is arbitrary, no float azimuth
    // as there's a breakdown of the spherical parameterization, so we clamp under by FLT_MIN in any case
    // for safe division
    // Note:
    //       sin(thetaV)^2 * cos(phiV)^2 = (TdotV)^2
    //       sin(thetaV)^2 * sin(phiV)^2 = (BdotV)^2
    float2 vProj2 = Sq(float2(TdotV, BdotV)) * rcp(sinTheta2);
    //       vProj2 = (cos^2(phi), sin^2(phi))
    float projRoughness = sqrt(dot(vProj2, roughness * roughness));
    return projRoughness;
}

//-----------------------------------------------------------------------------
// Diffuse BRDF - diffuseColor is expected to be multiply by the caller
//-----------------------------------------------------------------------------

float LambertNoPI()
{
    return 1.0;
}

float Lambert()
{
    return INV_PI;
}

float DisneyDiffuseNoPI(float NdotV, float NdotL, float LdotV, float perceptualRoughness)
{
    // (2 * LdotH * LdotH) = 1 + LdotV
    // float fd90 = 0.5 + (2 * LdotH * LdotH) * perceptualRoughness;
    float fd90 = 0.5 + (perceptualRoughness + perceptualRoughness * LdotV);
    // Two schlick fresnel term
    float lightScatter = F_Schlick(1.0, fd90, NdotL);
    float viewScatter = F_Schlick(1.0, fd90, NdotV);

    // Normalize the BRDF for polar view angles of up to (Pi/4).
    // We use the worst case of (roughness = albedo = 1), and, for each view angle,
    // integrate (brdf * cos(theta_light)) over all light directions.
    // The resulting value is for (theta_view = 0), which is actually a little bit larger
    // than the value of the integral for (theta_view = Pi/4).
    // Hopefully, the compiler folds the constant together with (1/Pi).
    return rcp(1.03571) * (lightScatter * viewScatter);
}

#ifndef BUILTIN_TARGET_API
float DisneyDiffuse(float NdotV, float NdotL, float LdotV, float perceptualRoughness)
{
    return INV_PI * DisneyDiffuseNoPI(NdotV, NdotL, LdotV, perceptualRoughness);
}
#endif

// Ref: Diffuse Lighting for GGX + Smith Microsurfaces, p. 113.
float3 DiffuseGGXNoPI(float3 albedo, float NdotV, float NdotL, float NdotH, float LdotV, float roughness)
{
    float facing = 0.5 + 0.5 * LdotV; // (LdotH)^2
    float rough = facing * (0.9 - 0.4 * facing) * (0.5 / NdotH + 1);
    float transmitL = F_Transm_Schlick(0, NdotL);
    float transmitV = F_Transm_Schlick(0, NdotV);
    float smooth = transmitL * transmitV * 1.05; // Normalize F_t over the hemisphere
    float single = lerp(smooth, rough, roughness); // Rescaled by PI
    float multiple = roughness * (0.1159 * PI); // Rescaled by PI

    return single + albedo * multiple;
}

float3 DiffuseGGX(float3 albedo, float NdotV, float NdotL, float NdotH, float LdotV, float roughness)
{
    // Note that we could save 2 cycles by inlining the multiplication by INV_PI.
    return INV_PI * DiffuseGGXNoPI(albedo, NdotV, NdotL, NdotH, LdotV, roughness);
}

//-----------------------------------------------------------------------------
// Iridescence
//-----------------------------------------------------------------------------

// Ref: https://belcour.github.io/blog/research/2017/05/01/brdf-thin-film.html
// Evaluation XYZ sensitivity curves in Fourier space
float3 EvalSensitivity(float opd, float shift)
{
    // Use Gaussian fits, given by 3 parameters: val, pos and var
    float phase = 2.0 * PI * opd * 1e-6;
    float3 val = float3(5.4856e-13, 4.4201e-13, 5.2481e-13);
    float3 pos = float3(1.6810e+06, 1.7953e+06, 2.2084e+06);
    float3 var = float3(4.3278e+09, 9.3046e+09, 6.6121e+09);
    float3 xyz = val * sqrt(2.0 * PI * var) * cos(pos * phase + shift) * exp(-var * phase * phase);
    xyz.x += 9.7470e-14 * sqrt(2.0 * PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift) * exp(-4.5282e+09 * phase * phase);
    xyz /= 1.0685e-7;

    // Convert to linear sRGb color space here.
    // EvalIridescence works in linear sRGB color space and does not switch...
    float3 srgb = mul(XYZ_2_REC709_MAT, xyz);
    return srgb;
}

// Evaluate the reflectance for a thin-film layer on top of a dielectric medum.
float3 EvalIridescence(float eta_1, float cosTheta1, float iridescenceThickness, float3 baseLayerFresnel0, float iorOverBaseLayer = 0.0)
{
    float3 I;

    // iridescenceThickness unit is micrometer for this equation here. Mean 0.5 is 500nm.
    float Dinc = 3.0 * iridescenceThickness;

    // Note: Unlike the code provide with the paper, here we use schlick approximation
    // Schlick is a very poor approximation when dealing with iridescence to the Fresnel
    // term and there is no "neutral" value in this unlike in the original paper.
    // We use Iridescence mask here to allow to have neutral value

    // Hack: In order to use only one parameter (DInc), we deduced the ior of iridescence from current Dinc iridescenceThickness
    // and we use mask instead to fade out the effect
    float eta_2 = lerp(2.0, 1.0, iridescenceThickness);
    // Following line from original code is not needed for us, it create a discontinuity
    // Force eta_2 -> eta_1 when Dinc -> 0.0
    // float eta_2 = lerp(eta_1, eta_2, smoothstep(0.0, 0.03, Dinc));
    // Evaluate the cosTheta on the base layer (Snell law)
    float sinTheta2Sq = Sq(eta_1 / eta_2) * (1.0 - Sq(cosTheta1));

    // Handle TIR:
    // (Also note that with just testing sinTheta2Sq > 1.0, (1.0 - sinTheta2Sq) can be negative, as emitted instructions
    // can eg be a mad giving a small negative for (1.0 - sinTheta2Sq), while sinTheta2Sq still testing equal to 1.0), so we actually
    // test the operand [cosTheta2Sq := (1.0 - sinTheta2Sq)] < 0 directly:)
    float cosTheta2Sq = (1.0 - sinTheta2Sq);
    // Or use this "artistic hack" to get more continuity even though wrong (no TIR, continue the effect by mirroring it):
    //   if( cosTheta2Sq < 0.0 ) => { sinTheta2Sq = 2 - sinTheta2Sq; => so cosTheta2Sq = sinTheta2Sq - 1 }
    // ie don't test and simply do
    //   float cosTheta2Sq = abs(1.0 - sinTheta2Sq);
    if (cosTheta2Sq < 0.0)
        I = float3(1.0, 1.0, 1.0);
    else
    {

        float cosTheta2 = sqrt(cosTheta2Sq);

        // First interface
        float R0 = IorToFresnel0(eta_2, eta_1);
        float R12 = F_Schlick(R0, cosTheta1);
        float R21 = R12;
        float T121 = 1.0 - R12;
        float phi12 = 0.0;
        float phi21 = PI - phi12;

        // Second interface
        // The f0 or the base should account for the new computed eta_2 on top.
        // This is optionally done if we are given the needed current ior over the base layer that is accounted for
        // in the baseLayerFresnel0 parameter:
        if (iorOverBaseLayer > 0.0)
        {
            // Fresnel0ToIor will give us a ratio of baseIor/topIor, hence we * iorOverBaseLayer to get the baseIor
            float3 baseIor = iorOverBaseLayer * Fresnel0ToIor(baseLayerFresnel0 + 0.0001); // guard against 1.0
            baseLayerFresnel0 = IorToFresnel0(baseIor, eta_2);
        }

        float3 R23 = F_Schlick(baseLayerFresnel0, cosTheta2);
        float phi23 = 0.0;

        // Phase shift
        float OPD = Dinc * cosTheta2;
        float phi = phi21 + phi23;

        // Compound terms
        float3 R123 = clamp(R12 * R23, 1e-5, 0.9999);
        float3 r123 = sqrt(R123);
        float3 Rs = Sq(T121) * R23 / (float3(1.0, 1.0, 1.0) - R123);

        // Reflectance term for m = 0 (DC term amplitude)
        float3 C0 = R12 + Rs;
        I = C0;

        // Reflectance term for m > 0 (pairs of diracs)
        float3 Cm = Rs - T121;
        for (int m = 1; m <= 2; ++m)
        {
            Cm *= r123;
            float3 Sm = 2.0 * EvalSensitivity(m * OPD, m * phi);
            //vec3 SmP = 2.0 * evalSensitivity(m*OPD, m*phi2.y);
            I += Cm * Sm;
        }

        // Since out of gamut colors might be produced, negative color values are clamped to 0.
        I = max(I, float3(0.0, 0.0, 0.0));
    }

    return I;
}

//-----------------------------------------------------------------------------
// Fabric
//-----------------------------------------------------------------------------

// Ref: https://knarkowicz.wordpress.com/2018/01/04/cloth-shading/
float D_CharlieNoPI(float NdotH, float roughness)
{
    float invR = rcp(roughness);
    float cos2h = NdotH * NdotH;
    float sin2h = 1.0 - cos2h;
    // Note: We have sin^2 so multiply by 0.5 to cancel it
    return (2.0 + invR) * PositivePow(sin2h, invR * 0.5) / 2.0;
}

float D_Charlie(float NdotH, float roughness)
{
    return INV_PI * D_CharlieNoPI(NdotH, roughness);
}

float CharlieL(float x, float r)
{
    r = saturate(r);
    r = 1.0 - (1.0 - r) * (1.0 - r);

    float a = lerp(25.3245, 21.5473, r);
    float b = lerp(3.32435, 3.82987, r);
    float c = lerp(0.16801, 0.19823, r);
    float d = lerp(-1.27393, -1.97760, r);
    float e = lerp(-4.85967, -4.32054, r);

    return a / (1. + b * PositivePow(x, c)) + d * x + e;
}

// Note: This version don't include the softening of the paper: Production Friendly Microfacet Sheen BRDF
float V_Charlie(float NdotL, float NdotV, float roughness)
{
    float lambdaV = NdotV < 0.5 ? exp(CharlieL(NdotV, roughness)) : exp(2.0 * CharlieL(0.5, roughness) - CharlieL(1.0 - NdotV, roughness));
    float lambdaL = NdotL < 0.5 ? exp(CharlieL(NdotL, roughness)) : exp(2.0 * CharlieL(0.5, roughness) - CharlieL(1.0 - NdotL, roughness));

    return 1.0 / ((1.0 + lambdaV + lambdaL) * (4.0 * NdotV * NdotL));
}

// We use V_Ashikhmin instead of V_Charlie in practice for game due to the cost of V_Charlie
float V_Ashikhmin(float NdotL, float NdotV)
{
    // Use soft visibility term introduce in: Crafting a Next-Gen Material Pipeline for The Order : 1886
    return 1.0 / (4.0 * (NdotL + NdotV - NdotL * NdotV));
}

// A diffuse term use with fabric done by tech artist - empirical
float FabricLambertNoPI(float roughness)
{
    return lerp(1.0, 0.5, roughness);
}

float FabricLambert(float roughness)
{
    return INV_PI * FabricLambertNoPI(roughness);
}

float G_CookTorrance(float NdotH, float NdotV, float NdotL, float HdotV)
{
    return min(1.0, 2.0 * NdotH * min(NdotV, NdotL) / HdotV);
}

//-----------------------------------------------------------------------------
// Hair
//-----------------------------------------------------------------------------

//http://web.engr.oregonstate.edu/~mjb/cs519/Projects/Papers/HairRendering.pdf
float3 ShiftTangent(float3 T, float3 N, float shift)
{
    return normalize(T + N * shift);
}

// Note: this is Blinn-Phong, the original paper uses Phong.
float3 D_KajiyaKay(float3 T, float3 H, float specularExponent)
{
    float TdotH = dot(T, H);
    float sinTHSq = saturate(1.0 - TdotH * TdotH);

    float dirAttn = saturate(TdotH + 1.0); // Evgenii: this seems like a hack? Do we floatly need this?

                                           // Note: Kajiya-Kay is not energy conserving.
                                           // We attempt at least some energy conservation by approximately normalizing Blinn-Phong NDF.
                                           // We use the formulation with the NdotL.
                                           // See http://www.thetenthplanet.de/archives/255.
    float n = specularExponent;
    float norm = (n + 2) * rcp(2 * PI);

    return dirAttn * norm * PositivePow(sinTHSq, 0.5 * n);
}

//================================================================================================

struct PositionInputs
{
    float3 positionWS;  // World space position (could be camera-relative)
    float2 positionNDC; // Normalized screen coordinates within the viewport    : [0, 1) (with the half-pixel offset)
    uint2  positionSS;  // Screen space pixel coordinates                       : [0, NumPixels)
    uint2  tileCoord;   // Screen tile coordinates                              : [0, NumTiles)
    float  deviceDepth; // Depth from the depth buffer                          : [0, 1] (typically reversed)
    float  linearDepth; // View space Z coordinate                              : [Near, Far]
};

// Precomputed lighting data to send to the various lighting functions
struct PreLightData
{
    float NdotV; // Could be negative due to normal mapping, use ClampNdotV()

    // GGX
    float partLambdaV;
    float energyCompensation;

    // IBL
    float3 iblR; // Reflected specular direction, used for IBL in EvaluateBSDF_Env()
    float iblPerceptualRoughness;

    float3 specularFGD; // Store preintegrated BSDF for both specular and diffuse
    float diffuseFGD;

    // Area lights (17 VGPRs)
    // TODO: 'orthoBasisViewNormal' is just a rotation around the normal and should thus be just 1x VGPR.
    float3x3 orthoBasisViewNormal; // Right-handed view-dependent orthogonal basis around the normal (6x VGPRs)
    float3x3 ltcTransformDiffuse; // Inverse transformation for Lambertian or Disney Diffuse        (4x VGPRs)
    float3x3 ltcTransformSpecular; // Inverse transformation for GGX                                 (4x VGPRs)

    // Clear coat
    float coatPartLambdaV;
    float3 coatIblR;
    float coatIblF; // Fresnel term for view vector
    float coatReflectionWeight; // like reflectionHierarchyWeight but used to distinguish coat contribution between SSR/IBL lighting
    float3x3 ltcTransformCoat; // Inverse transformation for GGX                                 (4x VGPRs)
};

struct SurfaceData
{
    uint materialFeatures;
    float3 baseColor;
    float specularOcclusion;
    float3 normalWS;
    float perceptualSmoothness;
    float ambientOcclusion;
    float metallic;
    float coatMask;
    float3 specularColor;
    uint diffusionProfileHash;
    float subsurfaceMask;
    float transmissionMask;
    float thickness;
    float3 tangentWS;
    float anisotropy;
    float iridescenceThickness;
    float iridescenceMask;
    float3 geomNormalWS;
    float ior;
    float3 transmittanceColor;
    float atDistance;
    float transmittanceMask;
};

struct BSDFData
{
    uint materialFeatures;
    float3 diffuseColor;
    float3 fresnel0;
    float ambientOcclusion;
    float specularOcclusion;
    float3 normalWS;
    float perceptualRoughness;
    float coatMask;
    uint diffusionProfileIndex;
    float subsurfaceMask;
    float thickness;
    bool useThickObjectMode;
    float3 transmittance;
    float3 tangentWS;
    float3 bitangentWS;
    float roughnessT;
    float roughnessB;
    float anisotropy;
    float iridescenceThickness;
    float iridescenceMask;
    float coatRoughness;
    float3 geomNormalWS;
    float ior;
    float3 absorptionCoefficient;
    float transmittanceMask;
};

struct BuiltinData
{
    float opacity;
    float alphaClipTreshold;
    float3 bakeDiffuseLighting;
    float3 backBakeDiffuseLighting;
    float shadowMask0;
    float shadowMask1;
    float shadowMask2;
    float shadowMask3;
    float3 emissiveColor;
    float2 motionVector;
    float2 distortion;
    float distortionBlur;
    uint isLightmap;
    uint renderingLayers;
    float depthOffset;
};

struct HDShadowData
{
    float3 rot0;
    float3 rot1;
    float3 rot2;
    float3 pos;
    float4 proj;
    float2 atlasOffset;
    float worldTexelSize;
    float normalBias;
    float4 zBufferParam;
    float4 shadowMapSize;
    float4 shadowFilterParams0;
    float3 cacheTranslationDelta;
    float4x4 shadowToWorld;
};

struct HDDirectionalShadowData
{
    float4 sphereCascades[4];
    float4 cascadeDirection;
    float cascadeBorders[4];
};

struct HDShadowContext
{
    StructuredBuffer<HDShadowData> shadowDatas;
    HDDirectionalShadowData directionalShadowData;
};

struct LightLoopContext
{
    int sampleReflection;
    float3 positionWS; // For sky reflection, we need position to evalute height base fog

    HDShadowContext shadowContext;

    uint contactShadow; // a bit mask of 24 bits that tell if the pixel is in a contact shadow or not
    float contactShadowFade; // combined fade factor of all contact shadows
    float shadowValue; // Stores the value of the cascade shadow map
    float splineVisibility; // Stores the value of the cascade shadow map (unbiased for splines)
};

struct LightLoopOutput
{
    float3 diffuseLighting;
    float3 specularLighting;
};

struct DirectionalLightData
{
    float3 positionRWS;
    uint lightLayers;
    float lightDimmer;
    float volumetricLightDimmer;
    float3 forward;
    int cookieMode;
    float4 cookieScaleOffset;
    float3 right;
    int shadowIndex;
    float3 up;
    int contactShadowIndex;
    float3 color;
    int contactShadowMask;
    float3 shadowTint;
    float shadowDimmer;
    float volumetricShadowDimmer;
    int nonLightMappedOnly;
    float minRoughness;
    int screenSpaceShadowIndex;
    float4 shadowMaskSelector;
    float diffuseDimmer;
    float specularDimmer;
    float penumbraTint;
    float isRayTracedContactShadow;
    float distanceFromCamera;
    float angularDiameter;
    float flareFalloff;
    float flareCosInner;
    float flareCosOuter;
    float __unused__;
    float3 flareTint;
    float flareSize;
    float3 surfaceTint;
    float4 surfaceTextureScaleOffset;
};

struct DirectLighting
{
    float3 diffuse;
    float3 specular;
};

struct IndirectLighting
{
    float3 specularReflected;
    float3 specularTransmitted;
};

struct AggregateLighting
{
    DirectLighting direct;
    IndirectLighting indirect;
};

struct LightData
{
    float3 positionRWS;
    uint lightLayers;
    float lightDimmer;
    float volumetricLightDimmer;
    float angleScale;
    float angleOffset;
    float3 forward;
    float iesCut;
    int lightType;
    float3 right;
    float penumbraTint;
    float range;
    int cookieMode;
    int shadowIndex;
    float3 up;
    float rangeAttenuationScale;
    float3 color;
    float rangeAttenuationBias;
    float4 cookieScaleOffset;
    float3 shadowTint;
    float shadowDimmer;
    float volumetricShadowDimmer;
    int nonLightMappedOnly;
    float minRoughness;
    int screenSpaceShadowIndex;
    float4 shadowMaskSelector;
    float4 size;
    int contactShadowMask;
    float diffuseDimmer;
    float specularDimmer;
    float __unused__;
    float2 padding;
    float isRayTracedContactShadow;
    float boxLightSafeExtent;
};

void AccumulateDirectLighting(DirectLighting src, inout AggregateLighting dst)
{
    dst.direct.diffuse += src.diffuse;
    dst.direct.specular += src.specular;
}

void AccumulateIndirectLighting(IndirectLighting src, inout AggregateLighting dst)
{
    dst.indirect.specularReflected += src.specularReflected;
    dst.indirect.specularTransmitted += src.specularTransmitted;
}

HDShadowContext InitShadowContext()
{
    HDShadowContext sc;

    sc.shadowDatas = _HDShadowDatas;
    sc.directionalShadowData = _HDDirectionalShadowData[0];

    return sc;
}

// We always fetch the screen space shadow texture to reduce the number of shader variant, overhead is negligible,
// it is a 1x1 white texture if deferred directional shadow and/or contact shadow are disabled
// We perform a single featch a the beginning of the lightloop
void InitContactShadow(PositionInputs posInput, inout LightLoopContext context)
{
    // Note: When we ImageLoad outside of texture size, the value returned by Load is 0 (Note: On Metal maybe it clamp to value of texture which is also fine)
    // We use this property to have a neutral value for contact shadows that doesn't consume a sampler and work also with compute shader (i.e use ImageLoad)
    // We store inverse contact shadow so neutral is white. So either we sample inside or outside the texture it return 1 in case of neutral
    uint packedContactShadow = LOAD_TEXTURE2D(_ContactShadowTexture, posInput.positionSS).x;
    UnpackContactShadowData(packedContactShadow, context.contactShadowFade, context.contactShadow);
}

bool IsNonZeroBSDF(float3 V, float3 L, PreLightData preLightData, BSDFData bsdfData)
{
    float NdotL = dot(bsdfData.normalWS, L);

    return HasFlag(bsdfData.materialFeatures, MATERIALFEATUREFLAGS_LIT_TRANSMISSION) || (NdotL > 0.0);
}

bool ShouldEvaluateThickObjectTransmission(float3 V, float3 L, PreLightData preLightData,
                                           BSDFData bsdfData, int shadowIndex)
{
    // Currently, we don't consider (NdotV < 0) as transmission.
    // TODO: ignore normal map? What about double sided-surfaces with one-sided normals?
    float NdotL = dot(bsdfData.normalWS, L);

    // If a material does not support transmission, it will never have this flag, and
    // the optimization pass of the compiler will remove all of the associated code.
    // However, this will take a lot more CPU time than doing the same thing using
    // the preprocessor.
    return HasFlag(bsdfData.materialFeatures, MATERIALFEATUREFLAGS_TRANSMISSION_MODE_THICK_OBJECT) &&
           (shadowIndex >= 0) && (NdotL < 0.0);
}

float EvalShadow_CascadedDepth_Blend_SplitIndex(inout HDShadowContext shadowContext, Texture2D tex, SamplerComparisonState samp, float2 positionSS, float3 positionWS, float3 normalWS, int index, float3 L, out int shadowSplitIndex)
{
    float alpha;
    int cascadeCount;
    float shadow = 1.0;
    shadowSplitIndex = EvalShadow_GetSplitIndex(shadowContext, index, positionWS, alpha, cascadeCount);
#ifdef SHADOWS_SHADOWMASK
    shadowContext.shadowSplitIndex = shadowSplitIndex;
    shadowContext.fade = alpha;
#endif

    float3 basePositionWS = positionWS;

    if (shadowSplitIndex >= 0.0)
    {
        HDShadowData sd = shadowContext.shadowDatas[index];
        LoadDirectionalShadowDatas(sd, shadowContext, index + shadowSplitIndex);
        positionWS = basePositionWS + sd.cacheTranslationDelta.xyz;

        /* normal based bias */
        float worldTexelSize = sd.worldTexelSize;
        float3 orig_pos = positionWS;
        float3 normalBias = EvalShadow_NormalBiasOrtho(sd.worldTexelSize, sd.normalBias, normalWS);
        positionWS += normalBias;

        /* get shadowmap texcoords */
        float3 posTC = EvalShadow_GetTexcoordsAtlas(sd, _CascadeShadowAtlasSize.zw, positionWS, false);
        /* evalute the first cascade */
        shadow = DIRECTIONAL_FILTER_ALGORITHM(sd, positionSS, posTC, tex, samp, FIXED_UNIFORM_BIAS);
        float shadow1 = 1.0;

        shadowSplitIndex++;
        if (shadowSplitIndex < cascadeCount)
        {
            shadow1 = shadow;

            if (alpha > 0.0)
            {
                LoadDirectionalShadowDatas(sd, shadowContext, index + shadowSplitIndex);

                // We need to modify the bias as the world texel size changes between splits and an update is needed.
                float biasModifier = (sd.worldTexelSize / worldTexelSize);
                normalBias *= biasModifier;

                float3 evaluationPosWS = basePositionWS + sd.cacheTranslationDelta.xyz + normalBias;
                float3 posNDC;
                posTC = EvalShadow_GetTexcoordsAtlas(sd, _CascadeShadowAtlasSize.zw, evaluationPosWS, posNDC, false);
                /* sample the texture */
                if (all(abs(posNDC.xy) <= (1.0 - sd.shadowMapSize.zw * 0.5)))
                    shadow1 = DIRECTIONAL_FILTER_ALGORITHM(sd, positionSS, posTC, tex, samp, FIXED_UNIFORM_BIAS);
            }
        }
        shadow = lerp(shadow, shadow1, alpha);
    }

    return shadow;
}

float EvalShadow_CascadedDepth_Blend(inout HDShadowContext shadowContext, Texture2D tex, SamplerComparisonState samp, float2 positionSS, float3 positionWS, float3 normalWS, int index, float3 L)
{
    int unusedSplitIndex;
    return EvalShadow_CascadedDepth_Blend_SplitIndex(shadowContext, tex, samp, positionSS, positionWS, normalWS, index, L, unusedSplitIndex);
}

float GetDirectionalShadowAttenuation(inout HDShadowContext shadowContext, float2 positionSS, float3 positionWS, float3 normalWS, int shadowDataIndex, float3 L)
{
#if SHADOW_AUTO_FLIP_NORMAL
    normalWS *= FastSign(dot(normalWS, L));
#endif
    return EvalShadow_CascadedDepth_Blend(shadowContext, _ShadowmapCascadeAtlas, s_linear_clamp_compare_sampler, positionSS, positionWS, normalWS, shadowDataIndex, L);
}

float3 GetNormalForShadowBias(BSDFData bsdfData)
{
    // In forward we can used geometric normal for shadow bias which improve quality
#if (SHADERPASS == SHADERPASS_FORWARD)
    return bsdfData.geomNormalWS;
#else
    return bsdfData.normalWS;
#endif
}

// Non physically based hack to limit light influence to attenuationRadius.
// Square the result to smoothen the function.
float DistanceWindowing(float distSquare, float rangeAttenuationScale, float rangeAttenuationBias)
{
    // If (range attenuation is enabled)
    //   rangeAttenuationScale = 1 / r^2
    //   rangeAttenuationBias  = 1
    // Else
    //   rangeAttenuationScale = 2^12 / r^2
    //   rangeAttenuationBias  = 2^24
    return saturate(rangeAttenuationBias - Sq(distSquare * rangeAttenuationScale));
}

// Square the result to smoothen the function.
float AngleAttenuation(float cosFwd, float lightAngleScale, float lightAngleOffset)
{
    return saturate(cosFwd * lightAngleScale + lightAngleOffset);
}

// Combines SmoothWindowedDistanceAttenuation() and SmoothAngleAttenuation() in an efficient manner.
// distances = {d, d^2, 1/d, d_proj}, where d_proj = dot(lightToSample, lightData.forward).
float PunctualLightAttenuation(float4 distances, float rangeAttenuationScale, float rangeAttenuationBias,
                              float lightAngleScale, float lightAngleOffset)
{
    float distSq = distances.y;
    float distRcp = distances.z;
    float distProj = distances.w;
    float cosFwd = distProj * distRcp;

    float attenuation = min(distRcp, 1.0 / PUNCTUAL_LIGHT_THRESHOLD);
    attenuation *= DistanceWindowing(distSq, rangeAttenuationScale, rangeAttenuationBias);
    attenuation *= AngleAttenuation(cosFwd, lightAngleScale, lightAngleOffset);

    // Effectively results in SmoothWindowedDistanceAttenuation(...) * SmoothAngleAttenuation(...).
    return Sq(attenuation);
}

// Returns unassociated (non-premultiplied) color with alpha (attenuation).
// The calling code must perform alpha-compositing.
// distances = {d, d^2, 1/d, d_proj}, where d_proj = dot(lightToSample, light.forward).
float4 EvaluateLight_Punctual(LightLoopContext lightLoopContext, PositionInputs posInput,
    LightData light, float3 L, float4 distances)
{
    float4 color = float4(light.color, 1.0);

    color.a *= PunctualLightAttenuation(distances, light.rangeAttenuationScale, light.rangeAttenuationBias,
                                        light.angleScale, light.angleOffset);

#ifndef LIGHT_EVALUATION_NO_HEIGHT_FOG
    // Height fog attenuation.
    // TODO: add an if()?
    {
        float cosZenithAngle = L.y;
        float fragmentHeight = posInput.positionWS.y;
        color.a *= TransmittanceHeightFog(_HeightFogBaseExtinction, _HeightFogBaseHeight,
                                          _HeightFogExponents, cosZenithAngle,
                                          fragmentHeight, distances.x);
    }
#endif

    // Projector lights (box, pyramid) always have cookies, so we can perform clipping inside the if().
    // Thus why we don't disable the code here based on LIGHT_EVALUATION_NO_COOKIE but we do it
    // inside the EvaluateCookie_Punctual call
    if (light.cookieMode != COOKIEMODE_NONE)
    {
        float3 lightToSample = posInput.positionWS - light.positionRWS;
        float4 cookie = EvaluateCookie_Punctual(lightLoopContext, light, lightToSample);

        color *= cookie;
    }

    return color;
}

// distances = {d, d^2, 1/d, d_proj}, where d_proj = dot(lightToSample, light.forward).
SHADOW_TYPE EvaluateShadow_Punctual(LightLoopContext lightLoopContext, PositionInputs posInput,
                                    LightData light, BuiltinData builtinData, float3 N, float3 L, float4 distances)
{
#ifndef LIGHT_EVALUATION_NO_SHADOWS
    float shadow = 1.0;
    float shadowMask = 1.0;
    float NdotL = dot(N, L); // Disable contact shadow and shadow mask when facing away from light (i.e transmission)

#ifdef SHADOWS_SHADOWMASK
    // shadowMaskSelector.x is -1 if there is no shadow mask
    // Note that we override shadow value (in case we don't have any dynamic shadow)
    shadow = shadowMask = (light.shadowMaskSelector.x >= 0.0 && NdotL > 0.0) ? dot(BUILTIN_DATA_SHADOW_MASK, light.shadowMaskSelector) : 1.0;
#endif

#if defined(SCREEN_SPACE_SHADOWS_ON) && !defined(_SURFACE_TYPE_TRANSPARENT)
    if ((light.screenSpaceShadowIndex & SCREEN_SPACE_SHADOW_INDEX_MASK) != INVALID_SCREEN_SPACE_SHADOW)
    {
        shadow = GetScreenSpaceShadow(posInput, light.screenSpaceShadowIndex);
        shadow = lerp(shadowMask, shadow, light.shadowDimmer);
    }
    else
#endif
    if ((light.shadowIndex >= 0) && (light.shadowDimmer > 0))
    {
        shadow = GetPunctualShadowAttenuation(lightLoopContext.shadowContext, posInput.positionSS, posInput.positionWS, N, light.shadowIndex, L, distances.x, light.lightType == GPULIGHTTYPE_POINT, light.lightType != GPULIGHTTYPE_PROJECTOR_BOX);

#ifdef SHADOWS_SHADOWMASK
        // Note: Legacy Unity have two shadow mask mode. ShadowMask (ShadowMask contain static objects shadow and ShadowMap contain only dynamic objects shadow, final result is the minimun of both value)
        // and ShadowMask_Distance (ShadowMask contain static objects shadow and ShadowMap contain everything and is blend with ShadowMask based on distance (Global distance setup in QualitySettigns)).
        // HDRenderPipeline change this behavior. Only ShadowMask mode is supported but we support both blend with distance AND minimun of both value. Distance is control by light.
        // The following code do this.
        // The min handle the case of having only dynamic objects in the ShadowMap
        // The second case for blend with distance is handled with ShadowDimmer. ShadowDimmer is define manually and by shadowDistance by light.
        // With distance, ShadowDimmer become one and only the ShadowMask appear, we get the blend with distance behavior.
        shadow = light.nonLightMappedOnly ? min(shadowMask, shadow) : shadow;
#endif

        shadow = lerp(shadowMask, shadow, light.shadowDimmer);
    }

    // Transparents have no contact shadow information
#if !defined(_SURFACE_TYPE_TRANSPARENT) && !defined(LIGHT_EVALUATION_NO_CONTACT_SHADOWS)
    {
    // In certain cases (like hair) we allow to force the contact shadow sample.
#ifdef LIGHT_EVALUATION_CONTACT_SHADOW_DISABLE_NDOTL
        const bool allowContactShadow = true;
#else
        const bool allowContactShadow = NdotL > 0.0;
#endif

        shadow = min(shadow, allowContactShadow ? GetContactShadow(lightLoopContext, light.contactShadowMask, light.isRayTracedContactShadow) : 1.0);
    }
#endif

#ifdef DEBUG_DISPLAY
    if (_DebugShadowMapMode == SHADOWMAPDEBUGMODE_SINGLE_SHADOW && light.shadowIndex == _DebugSingleShadowIndex)
        g_DebugShadowAttenuation = shadow;
#endif
    return shadow;
#else // LIGHT_EVALUATION_NO_SHADOWS
    return 1.0;
#endif
}

float3 ComputeShadowColor(float shadow, float3 shadowTint, float penumbraFlag)
{
    // The origin expression is
    // lerp(float3(1.0, 1.0, 1.0) - ((1.0 - shadow) * (float3(1.0, 1.0, 1.0) - shadowTint))
    //            , shadow * lerp(shadowTint, lerp(shadowTint, float3(1.0, 1.0, 1.0), shadow), shadow)
    //            , penumbraFlag);
    // it has been simplified to this
    float3 invTint = float3(1.0, 1.0, 1.0) - shadowTint;
    float shadow3 = shadow * shadow * shadow;
    return lerp(float3(1.0, 1.0, 1.0) - ((1.0 - shadow) * invTint)
                , shadow3 * invTint + shadow * shadowTint,
                penumbraFlag);

}

//
// ClampRoughness helper specific to this material
//
void ClampRoughness(inout PreLightData preLightData, inout BSDFData bsdfData, float minRoughness)
{
    bsdfData.roughnessT = max(minRoughness, bsdfData.roughnessT);
    bsdfData.roughnessB = max(minRoughness, bsdfData.roughnessB);
    bsdfData.coatRoughness = max(minRoughness, bsdfData.coatRoughness);
}

//-----------------------------------------------------------------------------
// BSDF share between directional light, punctual light and area light (reference)
//-----------------------------------------------------------------------------

bool IsNonZeroBSDF(float3 V, float3 L, PreLightData preLightData, BSDFData bsdfData)
{
    float NdotL = dot(bsdfData.normalWS, L);

    return HasFlag(bsdfData.materialFeatures, MATERIALFEATUREFLAGS_LIT_TRANSMISSION) || (NdotL > 0.0);
}

CBSDF EvaluateBSDF(float3 V, float3 L, PreLightData preLightData, BSDFData bsdfData)
{
    CBSDF cbsdf;
    ZERO_INITIALIZE(CBSDF, cbsdf);

    float3 N = bsdfData.normalWS;

    float NdotV = preLightData.NdotV;
    float NdotL = dot(N, L);
    float clampedNdotV = ClampNdotV(NdotV);
    float clampedNdotL = saturate(NdotL);
    float flippedNdotL = ComputeWrappedDiffuseLighting(-NdotL, TRANSMISSION_WRAP_LIGHT);

    float LdotV, NdotH, LdotH, invLenLV;
    GetBSDFAngle(V, L, NdotL, NdotV, LdotV, NdotH, LdotH, invLenLV);

    float3 F = F_Schlick(bsdfData.fresnel0, LdotH);
    // Remark: Fresnel must be use with LdotH angle. But Fresnel for iridescence is expensive to compute at each light.
    // Instead we use the incorrect angle NdotV as an approximation for LdotH for Fresnel evaluation.
    // The Fresnel with iridescence and NDotV angle is precomputed ahead and here we jsut reuse the result.
    // Thus why we shouldn't apply a second time Fresnel on the value if iridescence is enabled.
    if (HasFlag(bsdfData.materialFeatures, MATERIALFEATUREFLAGS_LIT_IRIDESCENCE))
    {
        F = lerp(F, bsdfData.fresnel0, bsdfData.iridescenceMask);
    }

    float DV;
    if (HasFlag(bsdfData.materialFeatures, MATERIALFEATUREFLAGS_LIT_ANISOTROPY))
    {
        float3 H = (L + V) * invLenLV;

        // For anisotropy we must not saturate these values
        float TdotH = dot(bsdfData.tangentWS, H);
        float TdotL = dot(bsdfData.tangentWS, L);
        float BdotH = dot(bsdfData.bitangentWS, H);
        float BdotL = dot(bsdfData.bitangentWS, L);

        // TODO: Do comparison between this correct version and the one from isotropic and see if there is any visual difference
        // We use abs(NdotL) to handle the none case of double sided
        DV = DV_SmithJointGGXAniso(TdotH, BdotH, NdotH, clampedNdotV, TdotL, BdotL, abs(NdotL),
                                   bsdfData.roughnessT, bsdfData.roughnessB, preLightData.partLambdaV);
    }
    else
    {
        // We use abs(NdotL) to handle the none case of double sided
        DV = DV_SmithJointGGX(NdotH, abs(NdotL), clampedNdotV, bsdfData.roughnessT, preLightData.partLambdaV);
    }

    float3 specTerm = F * DV;

#ifdef USE_DIFFUSE_LAMBERT_BRDF
    float diffTerm = Lambert();
#else
    // A note on subsurface scattering: [SSS-NOTE-TRSM]
    // The correct way to handle SSS is to transmit light inside the surface, perform SSS,
    // and then transmit it outside towards the viewer.
    // Transmit(X) = F_Transm_Schlick(F0, F90, NdotX), where F0 = 0, F90 = 1.
    // Therefore, the diffuse BSDF should be decomposed as follows:
    // f_d = A / Pi * F_Transm_Schlick(0, 1, NdotL) * F_Transm_Schlick(0, 1, NdotV) + f_d_reflection,
    // with F_Transm_Schlick(0, 1, NdotV) applied after the SSS pass.
    // The alternative (artistic) formulation of Disney is to set F90 = 0.5:
    // f_d = A / Pi * F_Transm_Schlick(0, 0.5, NdotL) * F_Transm_Schlick(0, 0.5, NdotV) + f_retro_reflection.
    // That way, darkening at grading angles is reduced to 0.5.
    // In practice, applying F_Transm_Schlick(F0, F90, NdotV) after the SSS pass is expensive,
    // as it forces us to read the normal buffer at the end of the SSS pass.
    // Separating f_retro_reflection also has a small cost (mostly due to energy compensation
    // for multi-bounce GGX), and the visual difference is negligible.
    // Therefore, we choose not to separate diffuse lighting into reflected and transmitted.

    // Use abs NdotL to evaluate diffuse term also for transmission
    // TODO: See with Evgenii about the clampedNdotV here. This is what we use before the refactor
    // but now maybe we want to revisit it for transmission
    float diffTerm = DisneyDiffuse(clampedNdotV, abs(NdotL), LdotV, bsdfData.perceptualRoughness);
#endif

    if (HasFlag(bsdfData.materialFeatures, MATERIALFEATUREFLAGS_LIT_CLEAR_COAT))
    {
        // Apply isotropic GGX for clear coat
        // Note: coat F is scalar as it is a dieletric
        float coatF = F_Schlick(CLEAR_COAT_F0, LdotH) * bsdfData.coatMask;
        // Scale base specular
        specTerm *= Sq(1.0 - coatF);

        // Add top specular
        // TODO: Should we call just D_GGX here ?
        // We use abs(NdotL) to handle the none case of double sided
        float DV = DV_SmithJointGGX(NdotH, abs(NdotL), clampedNdotV, bsdfData.coatRoughness, preLightData.coatPartLambdaV);
        specTerm += coatF * DV;

        // Note: The modification of the base roughness and fresnel0 by the clear coat is already handled in FillMaterialClearCoatData

        // Very coarse attempt at doing energy conservation for the diffuse layer based on NdotL. No science.
        diffTerm *= lerp(1, 1.0 - coatF, bsdfData.coatMask);
    }

    // The compiler should optimize these. Can revisit later if necessary.
    cbsdf.diffR = diffTerm * clampedNdotL;
    cbsdf.diffT = diffTerm * flippedNdotL;

    // Probably worth branching here for perf reasons.
    // This branch will be optimized away if there's no transmission.
    if (NdotL > 0)
    {
        cbsdf.specR = specTerm * clampedNdotL;
    }

    // We don't multiply by 'bsdfData.diffuseColor' here. It's done only once in PostEvaluateBSDF().
    return cbsdf;
}

DirectLighting ShadeSurface_Infinitesimal(PreLightData preLightData, BSDFData bsdfData,
                                          float3 V, float3 L, float3 lightColor,
                                          float diffuseDimmer, float specularDimmer)
{
    DirectLighting lighting;
    ZERO_INITIALIZE(DirectLighting, lighting);

    if (Max3(lightColor.r, lightColor.g, lightColor.b) > 0)
    {
        CBSDF cbsdf = EvaluateBSDF(V, L, preLightData, bsdfData);

#if defined(MATERIAL_INCLUDE_TRANSMISSION) || defined(MATERIAL_INCLUDE_PRECOMPUTED_TRANSMISSION)
        float3 transmittance = bsdfData.transmittance;
#else
        float3 transmittance = float3(0.0, 0.0, 0.0);
#endif
        // If transmittance or the CBSDF's transmission components are known to be 0,
        // the optimization pass of the compiler will remove all of the associated code.
        // However, this will take a lot more CPU time than doing the same thing using
        // the preprocessor.
        lighting.diffuse = (cbsdf.diffR + cbsdf.diffT * transmittance) * lightColor * diffuseDimmer;
        lighting.specular = (cbsdf.specR + cbsdf.specT * transmittance) * lightColor * specularDimmer;
    }
    
    return lighting;
}

DirectLighting ShadeSurface_Punctual(LightLoopContext lightLoopContext,
                                     PositionInputs posInput, BuiltinData builtinData,
                                     PreLightData preLightData, LightData light,
                                     BSDFData bsdfData, float3 V)
{
    DirectLighting lighting = (DirectLighting)0;

    float3 L;
    float4 distances; // {d, d^2, 1/d, d_proj}
    GetPunctualLightVectors(posInput.positionWS, light, L, distances);

    // Is it worth evaluating the light?
    if ((light.lightDimmer > 0) && IsNonZeroBSDF(V, L, preLightData, bsdfData))
    {
        float4 lightColor = EvaluateLight_Punctual(lightLoopContext, posInput, light, L, distances);
        lightColor.rgb *= lightColor.a; // Composite

#ifdef MATERIAL_INCLUDE_TRANSMISSION
        if (ShouldEvaluateThickObjectTransmission(V, L, preLightData, bsdfData, light.shadowIndex))
        {
            // Replace the 'baked' value using 'thickness from shadow'.
            bsdfData.transmittance = EvaluateTransmittance_Punctual(lightLoopContext, posInput,
                                                                    bsdfData, light, L, distances);
        }
        else
#endif
        {
            PositionInputs shadowPositionInputs = posInput;

#ifdef LIGHT_EVALUATION_SPLINE_SHADOW_BIAS
            shadowPositionInputs.positionWS += L * GetSplineOffsetForShadowBias(bsdfData);
#endif
            // This code works for both surface reflection and thin object transmission.
            SHADOW_TYPE shadow = EvaluateShadow_Punctual(lightLoopContext, shadowPositionInputs, light, builtinData, GetNormalForShadowBias(bsdfData), L, distances);
            lightColor.rgb *= ComputeShadowColor(shadow, light.shadowTint, light.penumbraTint);
        }

        // Simulate a sphere/disk light with this hack.
        // Note that it is not correct with our precomputation of PartLambdaV
        // (means if we disable the optimization it will not have the
        // same result) but we don't care as it is a hack anyway.
        ClampRoughness(preLightData, bsdfData, light.minRoughness);

        lighting = ShadeSurface_Infinitesimal(preLightData, bsdfData, V, L, lightColor.rgb,
                                              light.diffuseDimmer, light.specularDimmer);
    }

    return lighting;
}

//-----------------------------------------------------------------------------
// EvaluateBSDF_Directional
//-----------------------------------------------------------------------------

DirectLighting EvaluateBSDF_Directional(LightLoopContext lightLoopContext,
                                        float3 V, PositionInputs posInput,
                                        PreLightData preLightData, DirectionalLightData lightData,
                                        BSDFData bsdfData, BuiltinData builtinData)
{
    return ShadeSurface_Directional(lightLoopContext, posInput, builtinData, preLightData, lightData, bsdfData, V);
}

//-----------------------------------------------------------------------------
// EvaluateBSDF_Punctual (supports spot, point and projector lights)
//-----------------------------------------------------------------------------

DirectLighting EvaluateBSDF_Punctual(LightLoopContext lightLoopContext,
                                     float3 V, PositionInputs posInput,
                                     PreLightData preLightData, LightData lightData,
                                     BSDFData bsdfData, BuiltinData builtinData)
{
    return ShadeSurface_Punctual(lightLoopContext, posInput, builtinData, preLightData, lightData, bsdfData, V);
}


void LightLoop(float3 V, PositionInputs posInput, PreLightData preLightData, BSDFData bsdfData, BuiltinData builtinData, uint featureFlags,
                out LightLoopOutput lightLoopOutput)
{
    // Init LightLoop output structure
    lightLoopOutput = (LightLoopOutput)0;

    LightLoopContext context;

    context.shadowContext = InitShadowContext();
    context.shadowValue = 1;
    context.sampleReflection = 0;
    context.splineVisibility = -1;
    context.positionWS = posInput.positionWS;

    // Initialize the contactShadow and contactShadowFade fields
    InitContactShadow(posInput, context);

    // First of all we compute the shadow value of the directional light to reduce the VGPR pressure
    if (featureFlags & LIGHTFEATUREFLAGS_DIRECTIONAL)
    {
        // Evaluate sun shadows.
        if (_DirectionalShadowIndex >= 0)
        {
            DirectionalLightData light = _DirectionalLightDatas[_DirectionalShadowIndex];

            {
                // TODO: this will cause us to load from the normal buffer first. Does this cause a performance problem?
                float3 L = -light.forward;

                // Is it worth sampling the shadow map?
                if ((light.lightDimmer > 0) && (light.shadowDimmer > 0) && // Note: Volumetric can have different dimmer, thus why we test it here
                    IsNonZeroBSDF(V, L, preLightData, bsdfData) &&
                    !ShouldEvaluateThickObjectTransmission(V, L, preLightData, bsdfData, light.shadowIndex))
                {
                    float3 positionWS = posInput.positionWS;

                    context.shadowValue = GetDirectionalShadowAttenuation(context.shadowContext,
                                                                          posInput.positionSS, positionWS, GetNormalForShadowBias(bsdfData),
                                                                          light.shadowIndex, L);
                }
            }
        }
    }

    // This struct is define in the material. the Lightloop must not access it
    // PostEvaluateBSDF call at the end will convert Lighting to diffuse and specular lighting
    AggregateLighting aggregateLighting = (AggregateLighting) 0; // LightLoop is in charge of initializing the struct

    if (featureFlags & LIGHTFEATUREFLAGS_PUNCTUAL)
    {
        uint lightCount, lightStart;

#ifndef LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
        GetCountAndStart(posInput, LIGHTCATEGORY_PUNCTUAL, lightStart, lightCount);
#else   // LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
        lightCount = _PunctualLightCount;
        lightStart = 0;
#endif

        bool fastPath = false;
#if SCALARIZE_LIGHT_LOOP
        uint lightStartLane0;
        fastPath = IsFastPath(lightStart, lightStartLane0);

        if (fastPath)
        {
            lightStart = lightStartLane0;
        }
#endif

        // Scalarized loop. All lights that are in a tile/cluster touched by any pixel in the wave are loaded (scalar load), only the one relevant to current thread/pixel are processed.
        // For clarity, the following code will follow the convention: variables starting with s_ are meant to be wave uniform (meant for scalar register),
        // v_ are variables that might have different value for each thread in the wave (meant for vector registers).
        // This will perform more loads than it is supposed to, however, the benefits should offset the downside, especially given that light data accessed should be largely coherent.
        // Note that the above is valid only if wave intriniscs are supported.
        uint v_lightListOffset = 0;
        uint v_lightIdx = lightStart;

#if NEED_TO_CHECK_HELPER_LANE
        // On some platform helper lanes don't behave as we'd expect, therefore we prevent them from entering the loop altogether.
        // IMPORTANT! This has implications if ddx/ddy is used on results derived from lighting, however given Lightloop is called in compute we should be
        // sure it will not happen. 
        bool isHelperLane = WaveIsHelperLane();
        while (!isHelperLane && v_lightListOffset < lightCount)
#else
        while (v_lightListOffset < lightCount)
#endif
        {
            v_lightIdx = FetchIndex(lightStart, v_lightListOffset);
#if SCALARIZE_LIGHT_LOOP
            uint s_lightIdx = ScalarizeElementIndex(v_lightIdx, fastPath);
#else
            uint s_lightIdx = v_lightIdx;
#endif
            if (s_lightIdx == -1)
                break;

            LightData s_lightData = FetchLight(s_lightIdx);

            // If current scalar and vector light index match, we process the light. The v_lightListOffset for current thread is increased.
            // Note that the following should floatly be ==, however, since helper lanes are not considered by WaveActiveMin, such helper lanes could
            // end up with a unique v_lightIdx value that is smaller than s_lightIdx hence being stuck in a loop. All the active lanes will not have this problem.
            if (s_lightIdx >= v_lightIdx)
            {
                v_lightListOffset++;
                if (IsMatchingLightLayer(s_lightData.lightLayers, builtinData.renderingLayers))
                {
                    DirectLighting lighting = EvaluateBSDF_Punctual(context, V, posInput, preLightData, s_lightData, bsdfData, builtinData);
                    AccumulateDirectLighting(lighting, aggregateLighting);
                }
            }
        }
    }


    // Define macro for a better understanding of the loop
    // TODO: this code is now much harder to understand...
#define EVALUATE_BSDF_ENV_SKY(envLightData, TYPE, type) \
        IndirectLighting lighting = EvaluateBSDF_Env(context, V, posInput, preLightData, envLightData, bsdfData, envLightData.influenceShapeType, MERGE_NAME(GPUIMAGEBASEDLIGHTINGTYPE_, TYPE), MERGE_NAME(type, HierarchyWeight)); \
        AccumulateIndirectLighting(lighting, aggregateLighting);

// Environment cubemap test lightlayers, sky don't test it
#define EVALUATE_BSDF_ENV(envLightData, TYPE, type) if (IsMatchingLightLayer(envLightData.lightLayers, builtinData.renderingLayers)) { EVALUATE_BSDF_ENV_SKY(envLightData, TYPE, type) }

    // First loop iteration
    if (featureFlags & (LIGHTFEATUREFLAGS_ENV | LIGHTFEATUREFLAGS_SKY | LIGHTFEATUREFLAGS_SSREFRACTION | LIGHTFEATUREFLAGS_SSREFLECTION))
    {
        float reflectionHierarchyWeight = 0.0; // Max: 1.0
        float refractionHierarchyWeight = _EnableSSRefraction ? 0.0 : 1.0; // Max: 1.0

        uint envLightStart, envLightCount;

        // Fetch first env light to provide the scene proxy for screen space computation
#ifndef LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
        GetCountAndStart(posInput, LIGHTCATEGORY_ENV, envLightStart, envLightCount);
#else   // LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
        envLightCount = _EnvLightCount;
        envLightStart = 0;
#endif

        bool fastPath = false;
    #if SCALARIZE_LIGHT_LOOP
        uint envStartFirstLane;
        fastPath = IsFastPath(envLightStart, envStartFirstLane);
    #endif

        // Reflection / Refraction hierarchy is
        //  1. Screen Space Refraction / Reflection
        //  2. Environment Reflection / Refraction
        //  3. Sky Reflection / Refraction

        // Apply SSR.
    #if (defined(_SURFACE_TYPE_TRANSPARENT) && !defined(_DISABLE_SSR_TRANSPARENT)) || (!defined(_SURFACE_TYPE_TRANSPARENT) && !defined(_DISABLE_SSR))
        {
            IndirectLighting indirect = EvaluateBSDF_ScreenSpaceReflection(posInput, preLightData, bsdfData,
                                                                           reflectionHierarchyWeight);
            AccumulateIndirectLighting(indirect, aggregateLighting);
        }
#endif

#if HAS_REFRACTION
        uint perPixelEnvStart = envLightStart;
        uint perPixelEnvCount = envLightCount;
        // For refraction to be stable, we should reuse the same refraction probe for the whole object.
        // Otherwise as if the object span different tiles it could produce a different refraction probe picking and thus have visual artifacts.
        // For this we need to find the tile that is at the center of the object that is being rendered.
        // And then select the first refraction probe of this list.
        if ((featureFlags & LIGHTFEATUREFLAGS_SSREFRACTION) && (_EnableSSRefraction > 0))
        {
            // grab current object position and retrieve in which tile it belongs too
            float4x4 modelMat = GetObjectToWorldMatrix();
            float3 objPos = modelMat._m03_m13_m23;
            float4 posClip = TransformWorldToHClip(objPos);
            posClip.xyz = posClip.xyz / posClip.w;

            uint2 tileObj = (saturate(posClip.xy * 0.5f + 0.5f) * _ScreenSize.xy);

            uint envLightStart, envLightCount;

            // Fetch first env light to provide the scene proxy for screen space refraction computation
            PositionInputs localInput;
            ZERO_INITIALIZE(PositionInputs, localInput);
            localInput.tileCoord = tileObj.xy;
            localInput.linearDepth = posClip.w;

            GetCountAndStart(localInput, LIGHTCATEGORY_ENV, envLightStart, envLightCount);

            EnvLightData envLightData;
            if (envLightCount > 0)
            {
                envLightData = FetchEnvLight(FetchIndex(envLightStart, 0));
            }
            else if (perPixelEnvCount > 0) // If no refraction probe at the object center, we either fallback on the per pixel result.
            {
                envLightData = FetchEnvLight(FetchIndex(perPixelEnvStart, 0));
            }
            else // .. or the sky
            {
                envLightData = InitDefaultRefractionEnvLightData(0);
            }

            IndirectLighting lighting = EvaluateBSDF_ScreenspaceRefraction(context, V, posInput, preLightData, bsdfData, envLightData, refractionHierarchyWeight);
            AccumulateIndirectLighting(lighting, aggregateLighting);
        }
#endif

        //---------------------------------------
        // Sum up of operations on indirect diffuse lighting
        // Let's define SSGI as SSGI/RTGI/Mixed or APV without lightmaps
        // Let's define GI as Lightmaps/Lightprobe/APV with lightmaps

        // By default we do those operations in deferred
        // GBuffer pass : GI * AO + Emissive -> lightingbuffer
        // Lightloop : indirectDiffuse = lightingbuffer; indirectDiffuse * SSAO
        // Note that SSAO is apply on emissive in this case and we have double occlusion between AO and SSAO on indirectDiffuse

        // By default we do those operation in forward
        // Lightloop : indirectDiffuse = GI; indirectDiffuse * min(AO, SSAO) + Emissive

        // With any SSGI effect we are performing those operations in deferred
        // GBuffer pass : Emissive == 0 ? AmbientOcclusion -> EncodeIn(lightingbuffer) : Emissive -> lightingbuffer
        // Lightloop : indirectDiffuse = SSGI; Emissive = lightingbuffer; AmbientOcclusion = Extract(lightingbuffer) or 1.0;
        // indirectDiffuse * min(AO, SSAO) + Emissive
        // Note that mean that we have the same behavior than forward path if Emissive is 0

        // With any SSGI effect we are performing those operations in Forward
        // Lightloop : indirectDiffuse = SSGI; indirectDiffuse * min(AO, SSAO) + Emissive
        //---------------------------------------

        // Explanation about APV and SSGI/RTGI/Mixed effects steps in the rendering pipeline.
        // All effects will output only Emissive inside the lighting buffer (gbuffer3) in case of deferred (For APV this is done only if we are not a lightmap).
        // The Lightmaps/Lightprobes contribution is 0 for those cases. Code enforce it in SampleBakedGI(). The remaining code of Material pass (and the debug code)
        // is exactly the same with or without effects on, including the EncodeToGbuffer.
        // builtinData.isLightmap is used by APV to know if we have lightmap or not and is harcoded based on preprocessor in InitBuiltinData()
        // AO is also store with a hack in Gbuffer3 if possible. Otherwise it is set to 1.
        // In case of regular deferred path (when effects aren't enable) AO is already apply on lightmap and emissive is added on to of it. All is inside bakeDiffuseLighting and emissiveColor is 0. AO must be 1
        // When effects are enabled and for APV we don't have lightmaps, bakeDiffuseLighting is 0 and emissiveColor contain emissive (Either in deferred or forward) and AO should be the float AO value.
        // Then in the lightloop in below code we will evalaute APV or read the indirectDiffuseTexture to fill bakeDiffuseLighting.
        // We will then just do all the regular step we do with bakeDiffuseLighting in PostInitBuiltinData()
        // No code change is required to handle AO, it is the same for all path.
        // Note: Decals Emissive and Transparent Emissve aren't taken into account by RTGI/Mixed.
        // Forward opaque emissive work in all cases. The current code flow with Emissive store in GBuffer3 is only to manage the case of Opaque Lit Material with Emissive in case of deferred
        // Only APV can handle backFace lighting, all other effects are front face only.

        // If we use SSGI/RTGI/Mixed effect, we are fully replacing the value of builtinData.bakeDiffuseLighting which is 0 at this step.
        // If we are APV we only replace the non lightmaps part.
        bool replaceBakeDiffuseLighting = false;
#if defined(PROBE_VOLUMES_L1) || defined(PROBE_VOLUMES_L2)
        float3 lightInReflDir = float3(-1, -1, -1); // This variable is used with APV for reflection probe normalization - see code for LIGHTFEATUREFLAGS_ENV
#endif

#if !defined(_SURFACE_TYPE_TRANSPARENT) // No SSGI/RTGI/Mixed effect on transparent
        if (_IndirectDiffuseMode != INDIRECTDIFFUSEMODE_OFF)
            replaceBakeDiffuseLighting = true;
#endif
#if defined(PROBE_VOLUMES_L1) || defined(PROBE_VOLUMES_L2)
        if (!builtinData.isLightmap)
            replaceBakeDiffuseLighting = true;
#endif

        if (replaceBakeDiffuseLighting)
        {
            BuiltinData tempBuiltinData;
            ZERO_INITIALIZE(BuiltinData, tempBuiltinData);

#if !defined(_SURFACE_TYPE_TRANSPARENT) && !defined(SCREEN_SPACE_INDIRECT_DIFFUSE_DISABLED)
            if (_IndirectDiffuseMode != INDIRECTDIFFUSEMODE_OFF)
            {
                tempBuiltinData.bakeDiffuseLighting = LOAD_TEXTURE2D(_IndirectDiffuseTexture, posInput.positionSS).xyz * GetInverseCurrentExposureMultiplier();
            }
            else
#endif
            {
#if defined(PROBE_VOLUMES_L1) || defined(PROBE_VOLUMES_L2)
                if (_EnableProbeVolumes)
                {
                    // Reflect normal to get lighting for reflection probe tinting
                    float3 R = reflect(-V, bsdfData.normalWS);

                    EvaluateAdaptiveProbeVolume(GetAbsolutePositionWS(posInput.positionWS),
                        bsdfData.normalWS,
                        -bsdfData.normalWS,
                        R,
                        V,
                        posInput.positionSS,
                        tempBuiltinData.bakeDiffuseLighting,
                        tempBuiltinData.backBakeDiffuseLighting,
                        lightInReflDir);
                }
                else // If probe volume is disabled we fallback on the ambient probes
                {
                    tempBuiltinData.bakeDiffuseLighting = EvaluateAmbientProbe(bsdfData.normalWS);
                    tempBuiltinData.backBakeDiffuseLighting = EvaluateAmbientProbe(-bsdfData.normalWS);
                }
#endif
            }

#ifdef MODIFY_BAKED_DIFFUSE_LIGHTING
#ifdef DEBUG_DISPLAY
            // When the lux meter is enabled, we don't want the albedo of the material to modify the diffuse baked lighting
            if (_DebugLightingMode != DEBUGLIGHTINGMODE_LUX_METER)
#endif
                ModifyBakedDiffuseLighting(V, posInput, preLightData, bsdfData, tempBuiltinData);
#endif
            // This is applied only on bakeDiffuseLighting as ModifyBakedDiffuseLighting combine both bakeDiffuseLighting and backBakeDiffuseLighting
            tempBuiltinData.bakeDiffuseLighting *= GetIndirectDiffuseMultiplier(builtinData.renderingLayers);

            ApplyDebugToBuiltinData(tempBuiltinData); // This will not affect emissive as we don't use it

#if defined(DEBUG_DISPLAY) && (SHADERPASS == SHADERPASS_DEFERRED_LIGHTING)
            // We need to handle the specific case of deferred for debug lighting mode here
            if (_DebugLightingMode == DEBUGLIGHTINGMODE_EMISSIVE_LIGHTING)
            {
                tempBuiltinData.bakeDiffuseLighting = float3(0.0, 0.0, 0.0);
            }
#endif

            // Replace original data
            builtinData.bakeDiffuseLighting = tempBuiltinData.bakeDiffuseLighting;

        } // if (replaceBakeDiffuseLighting)

        // Reflection probes are sorted by volume (in the increasing order).
        if (featureFlags & LIGHTFEATUREFLAGS_ENV)
        {
            context.sampleReflection = SINGLE_PASS_CONTEXT_SAMPLE_REFLECTION_PROBES;

#if SCALARIZE_LIGHT_LOOP
            if (fastPath)
            {
                envLightStart = envStartFirstLane;
            }
#endif

            // Scalarized loop, same rationale of the punctual light version
            uint v_envLightListOffset = 0;
            uint v_envLightIdx = envLightStart;
#if NEED_TO_CHECK_HELPER_LANE
            // On some platform helper lanes don't behave as we'd expect, therefore we prevent them from entering the loop altogether.
            // IMPORTANT! This has implications if ddx/ddy is used on results derived from lighting, however given Lightloop is called in compute we should be
            // sure it will not happen. 
            bool isHelperLane = WaveIsHelperLane();
            while (!isHelperLane && v_envLightListOffset < envLightCount)
#else
            while (v_envLightListOffset < envLightCount)
#endif
            {
                v_envLightIdx = FetchIndex(envLightStart, v_envLightListOffset);
#if SCALARIZE_LIGHT_LOOP
                uint s_envLightIdx = ScalarizeElementIndex(v_envLightIdx, fastPath);
#else
                uint s_envLightIdx = v_envLightIdx;
#endif
                if (s_envLightIdx == -1)
                    break;

                EnvLightData s_envLightData = FetchEnvLight(s_envLightIdx); // Scalar load.

                // If current scalar and vector light index match, we process the light. The v_envLightListOffset for current thread is increased.
                // Note that the following should floatly be ==, however, since helper lanes are not considered by WaveActiveMin, such helper lanes could
                // end up with a unique v_envLightIdx value that is smaller than s_envLightIdx hence being stuck in a loop. All the active lanes will not have this problem.
                if (s_envLightIdx >= v_envLightIdx)
                {
                    v_envLightListOffset++;
                    if (reflectionHierarchyWeight < 1.0)
                    {
                        if (IsMatchingLightLayer(s_envLightData.lightLayers, builtinData.renderingLayers))
                        {
                            IndirectLighting lighting = EvaluateBSDF_Env(context, V, posInput, preLightData, s_envLightData, bsdfData, s_envLightData.influenceShapeType, GPUIMAGEBASEDLIGHTINGTYPE_REFLECTION, reflectionHierarchyWeight);
#if defined(PROBE_VOLUMES_L1) || defined(PROBE_VOLUMES_L2)

                            if (s_envLightData.normalizeWithAPV > 0 && all(lightInReflDir >= 0))
                            {
                                float factor = GetReflectionProbeNormalizationFactor(lightInReflDir, bsdfData.normalWS, s_envLightData.L0L1, s_envLightData.L2_1, s_envLightData.L2_2);
                                lighting.specularReflected *= factor;
                            }
#endif

                            AccumulateIndirectLighting(lighting, aggregateLighting);
                        }
                    }
                    // Refraction probe and reflection probe will process exactly the same weight. It will be good for performance to be able to share this computation
                    // However it is hard to deal with the fact that reflectionHierarchyWeight and refractionHierarchyWeight have not the same values, they are independent
                    // The refraction probe is rarely used and happen only with sphere shape and high IOR. So we accept the slow path that use more simple code and
                    // doesn't affect the performance of the reflection which is more important.
                    // We reuse LIGHTFEATUREFLAGS_SSREFRACTION flag as refraction is mainly base on the screen. Would be a waste to not use screen and only cubemap.
                    if ((featureFlags & LIGHTFEATUREFLAGS_SSREFRACTION) && (refractionHierarchyWeight < 1.0))
                    {
                        EVALUATE_BSDF_ENV(s_envLightData, REFRACTION, refraction);
                    }
                }

            }
        }

        // Only apply the sky IBL if the sky texture is available
        if ((featureFlags & LIGHTFEATUREFLAGS_SKY) && _EnvLightSkyEnabled)
        {
            // The sky is a single cubemap texture separate from the reflection probe texture array (different resolution and compression)
            context.sampleReflection = SINGLE_PASS_CONTEXT_SAMPLE_SKY;

            // The sky data are generated on the fly so the compiler can optimize the code
            EnvLightData envLightSky = InitSkyEnvLightData(0);

            // Only apply the sky if we haven't yet accumulated enough IBL lighting.
            if (reflectionHierarchyWeight < 1.0)
            {
                EVALUATE_BSDF_ENV_SKY(envLightSky, REFLECTION, reflection);
            }

            if ((featureFlags & LIGHTFEATUREFLAGS_SSREFRACTION) && (refractionHierarchyWeight < 1.0))
            {
                EVALUATE_BSDF_ENV_SKY(envLightSky, REFRACTION, refraction);
            }
        }
    }
#undef EVALUATE_BSDF_ENV
#undef EVALUATE_BSDF_ENV_SKY

    uint i = 0; // Declare once to avoid the D3D11 compiler warning.
    if (featureFlags & LIGHTFEATUREFLAGS_DIRECTIONAL)
    {
        for (i = 0; i < _DirectionalLightCount; ++i)
        {
            if (IsMatchingLightLayer(_DirectionalLightDatas[i].lightLayers, builtinData.renderingLayers))
            {
                DirectLighting lighting = EvaluateBSDF_Directional(context, V, posInput, preLightData, _DirectionalLightDatas[i], bsdfData, builtinData);
                AccumulateDirectLighting(lighting, aggregateLighting);
            }
        }
    }

#if SHADEROPTIONS_AREA_LIGHTS
    if (featureFlags & LIGHTFEATUREFLAGS_AREA)
    {
        uint lightCount, lightStart;

#ifndef LIGHTLOOP_DISABLE_TILE_AND_CLUSTER
        GetCountAndStart(posInput, LIGHTCATEGORY_AREA, lightStart, lightCount);
#else
        lightCount = _AfloatightCount;
        lightStart = _PunctualLightCount;
#endif

        // COMPILER BEHAVIOR WARNING!
        // If rectangle lights are before line lights, the compiler will duplicate light matrices in VGPR because they are used differently between the two types of lights.
        // By keeping line lights first we avoid this behavior and save substantial register pressure.
        // TODO: This is based on the current Lit.shader and can be different for any other way of implementing area lights, how to be generic and ensure performance ?

        if (lightCount > 0)
        {
            i = 0;

            uint      last      = lightCount - 1;
            LightData lightData = FetchLight(lightStart, i);

            while (i <= last && lightData.lightType == GPULIGHTTYPE_TUBE)
            {
                lightData.lightType = GPULIGHTTYPE_TUBE; // Enforce constant propagation
                lightData.cookieMode = COOKIEMODE_NONE;  // Enforce constant propagation

                if (IsMatchingLightLayer(lightData.lightLayers, builtinData.renderingLayers))
                {
                    DirectLighting lighting = EvaluateBSDF_Area(context, V, posInput, preLightData, lightData, bsdfData, builtinData);
                    AccumulateDirectLighting(lighting, aggregateLighting);
                }

                lightData = FetchLight(lightStart, min(++i, last));
            }

            while (i <= last) // GPULIGHTTYPE_RECTANGLE
            {
                lightData.lightType = GPULIGHTTYPE_RECTANGLE; // Enforce constant propagation

                if (IsMatchingLightLayer(lightData.lightLayers, builtinData.renderingLayers))
                {
                    DirectLighting lighting = EvaluateBSDF_Area(context, V, posInput, preLightData, lightData, bsdfData, builtinData);
                    AccumulateDirectLighting(lighting, aggregateLighting);
                }

                lightData = FetchLight(lightStart, min(++i, last));
            }
        }
    }
#endif

    ApplyDebugToLighting(context, builtinData, aggregateLighting);

    // Note: We can't apply the IndirectDiffuseMultiplier here as with GBuffer, Emissive is part of the bakeDiffuseLighting.
    // so IndirectDiffuseMultiplier is apply in PostInitBuiltinData or related location (like for probe volume)
    aggregateLighting.indirect.specularReflected *= GetIndirectSpecularMultiplier(builtinData.renderingLayers);

    // Also Apply indiret diffuse (GI)
    // PostEvaluateBSDF will perform any operation wanted by the material and sum everything into diffuseLighting and specularLighting
    PostEvaluateBSDF(context, V, posInput, preLightData, bsdfData, builtinData, aggregateLighting, lightLoopOutput);

    ApplyDebug(context, posInput, bsdfData, lightLoopOutput);
}
