#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint volumetricParameterIndex; 
    uint noiseTextureIndex;
    uint ditherTextureIndex;
    uint resultTextureIndex;
};

//-----------------------------------------------------------------------------------------
// GetCascadeWeights_SplitSpheres
//-----------------------------------------------------------------------------------------
float4 GetCascadeWeights_SplitSpheres(float3 wpos)
{
    float3 fromCenter0 = wpos.xyz - unity_ShadowSplitSpheres[0].xyz;
    float3 fromCenter1 = wpos.xyz - unity_ShadowSplitSpheres[1].xyz;
    float3 fromCenter2 = wpos.xyz - unity_ShadowSplitSpheres[2].xyz;
    float3 fromCenter3 = wpos.xyz - unity_ShadowSplitSpheres[3].xyz;
    float4 distances2 = float4(dot(fromCenter0, fromCenter0), dot(fromCenter1, fromCenter1), dot(fromCenter2, fromCenter2), dot(fromCenter3, fromCenter3));

    float4 weights = float4(distances2 < unity_ShadowSplitSqRadii);
    weights.yzw = saturate(weights.yzw - weights.xyz);
    return weights;
}

//-----------------------------------------------------------------------------------------
// GetCascadeShadowCoord
//-----------------------------------------------------------------------------------------
inline float4 GetCascadeShadowCoord(float4 wpos, float4 cascadeWeights)
{
    float3 sc0 = mul(unity_World2Shadow[0], wpos).xyz;
    float3 sc1 = mul(unity_World2Shadow[1], wpos).xyz;
    float3 sc2 = mul(unity_World2Shadow[2], wpos).xyz;
    float3 sc3 = mul(unity_World2Shadow[3], wpos).xyz;
    
    float4 shadowMapCoordinate = float4(sc0 * cascadeWeights[0] + sc1 * cascadeWeights[1] + sc2 * cascadeWeights[2] + sc3 * cascadeWeights[3], 1);
    float  noCascadeWeights = 1 - dot(cascadeWeights, float4(1, 1, 1, 1));
    shadowMapCoordinate.z += noCascadeWeights;
    return shadowMapCoordinate;
}

//-----------------------------------------------------------------------------------------
// GetLightAttenuation
//-----------------------------------------------------------------------------------------
float GetLightAttenuation(float3 wpos)
{
    float atten = 0;
#if defined (DIRECTIONAL) || defined (DIRECTIONAL_COOKIE)
    atten = 1;
#if defined (SHADOWS_DEPTH)
    // sample cascade shadow map
    float4 cascadeWeights = GetCascadeWeights_SplitSpheres(wpos);
    bool inside = dot(cascadeWeights, float4(1, 1, 1, 1)) < 4;
    float4 samplePos = GetCascadeShadowCoord(float4(wpos, 1), cascadeWeights);

    atten = inside ? UNITY_SAMPLE_SHADOW(_CascadeShadowMapTexture, samplePos.xyz) : 1.0f;
    atten = _LightShadowData.r + atten * (1 - _LightShadowData.r);
    //atten = inside ? tex2Dproj(_ShadowMapTexture, float4((samplePos).xyz, 1)).r : 1.0f;
#endif
#if defined (DIRECTIONAL_COOKIE)
    // NOT IMPLEMENTED
#endif
#elif defined (SPOT)	
    float3 tolight = _LightPos.xyz - wpos;
    half3 lightDir = normalize(tolight);

    float4 uvCookie = mul(_MyLightMatrix0, float4(wpos, 1));
    // negative bias because http://aras-p.info/blog/2010/01/07/screenspace-vs-mip-mapping/
    atten = tex2Dbias(_LightTexture0, float4(uvCookie.xy / uvCookie.w, 0, -8)).w;
    atten *= uvCookie.w < 0;
    float att = dot(tolight, tolight) * _LightPos.w;
    atten *= tex2D(_LightTextureB0, att.rr).UNITY_ATTEN_CHANNEL;

#if defined(SHADOWS_DEPTH)
    float4 shadowCoord = mul(_MyWorld2Shadow, float4(wpos, 1));
    atten *= saturate(UnitySampleShadowmap(shadowCoord));
#endif
#elif defined (POINT) || defined (POINT_COOKIE)
    float3 tolight = wpos - _LightPos.xyz;
    half3 lightDir = -normalize(tolight);

    float att = dot(tolight, tolight) * _LightPos.w;
    atten = tex2D(_LightTextureB0, att.rr).UNITY_ATTEN_CHANNEL;

    atten *= UnityDeferredComputeShadow(tolight, 0, float2(0, 0));

#if defined (POINT_COOKIE)
    atten *= texCUBEbias(_LightTexture0, float4(mul(_MyLightMatrix0, half4(wpos, 1)).xyz, -8)).w;
#endif //POINT_COOKIE
#endif
    return atten;
}

//-----------------------------------------------------------------------------------------
// ApplyHeightFog
//-----------------------------------------------------------------------------------------
void ApplyHeightFog(float3 wpos, inout float density)
{
#ifdef HEIGHT_FOG
    density *= exp(-(wpos.y + _HeightFog.x) * _HeightFog.y);
#endif
}

//-----------------------------------------------------------------------------------------
// GetDensity
//-----------------------------------------------------------------------------------------
float GetDensity(float3 wpos)
{
    float density = 1;
#ifdef NOISE
    float noise = tex3D(_NoiseTexture, frac(wpos * _NoiseData.x + float3(_Time.y * _NoiseVelocity.x, 0, _Time.y * _NoiseVelocity.y)));
    noise = saturate(noise - _NoiseData.z) * _NoiseData.y;
    density = saturate(noise);
#endif
    ApplyHeightFog(wpos, density);

    return density;
}        

//-----------------------------------------------------------------------------------------
// MieScattering
//-----------------------------------------------------------------------------------------
float MieScattering(float cosAngle, float4 g)
{
    return g.w * (g.x / (pow(g.y - g.z * cosAngle, 1.5)));			
}

//-----------------------------------------------------------------------------------------
// RayMarch
//-----------------------------------------------------------------------------------------
float4 RayMarch(float2 screenPos, float3 rayStart, float3 rayDir, float rayLength)
{
#ifdef DITHER_4_4
    float2 interleavedPos = (fmod(floor(screenPos.xy), 4.0));
    float offset = tex2D(_DitherTexture, interleavedPos / 4.0 + float2(0.5 / 4.0, 0.5 / 4.0)).w;
#else
    float2 interleavedPos = (fmod(floor(screenPos.xy), 8.0));
    float offset = tex2D(_DitherTexture, interleavedPos / 8.0 + float2(0.5 / 8.0, 0.5 / 8.0)).w;
#endif

    int stepCount = _SampleCount;

    float stepSize = rayLength / stepCount;
    float3 step = rayDir * stepSize;

    float3 currentPosition = rayStart + step * offset;

    float4 vlight = 0;

    float cosAngle;
#if defined (DIRECTIONAL) || defined (DIRECTIONAL_COOKIE)
    float extinction = 0;
    cosAngle = dot(_LightDir.xyz, -rayDir);
#else
    // we don't know about density between camera and light's volume, assume 0.5
    float extinction = length(_WorldSpaceCameraPos - currentPosition) * _VolumetricLight.y * 0.5;
#endif
    [loop]
    for (int i = 0; i < stepCount; ++i)
    {
        float atten = GetLightAttenuation(currentPosition);
        float density = GetDensity(currentPosition);

        float scattering = _VolumetricLight.x * stepSize * density;
        extinction += _VolumetricLight.y * stepSize * density;// +scattering;

        float4 light = atten * scattering * exp(-extinction);

//#if PHASE_FUNCTOIN
#if !defined (DIRECTIONAL) && !defined (DIRECTIONAL_COOKIE)
        // phase functino for spot and point lights
        float3 tolight = normalize(currentPosition - _LightPos.xyz);
        cosAngle = dot(tolight, -rayDir);
        light *= MieScattering(cosAngle, _MieG);
#endif          
//#endif
        vlight += light;

        currentPosition += step;				
    }

#if defined (DIRECTIONAL) || defined (DIRECTIONAL_COOKIE)
    // apply phase function for dir light
    vlight *= MieScattering(cosAngle, _MieG);
#endif

    // apply light's color
    vlight *= _LightColor;

    vlight = max(0, vlight);
#if defined (DIRECTIONAL) || defined (DIRECTIONAL_COOKIE) // use "proper" out-scattering/absorption for dir light 
    vlight.w = exp(-extinction);
#else
    vlight.w = 0;
#endif
    return vlight;
}

//-----------------------------------------------------------------------------------------
// RayConeIntersect
//-----------------------------------------------------------------------------------------
float2 RayConeIntersect(in float3 f3ConeApex, in float3 f3ConeAxis, in float fCosAngle, in float3 f3RayStart, in float3 f3RayDir)
{
    float inf = 10000;
    f3RayStart -= f3ConeApex;
    float a = dot(f3RayDir, f3ConeAxis);
    float b = dot(f3RayDir, f3RayDir);
    float c = dot(f3RayStart, f3ConeAxis);
    float d = dot(f3RayStart, f3RayDir);
    float e = dot(f3RayStart, f3RayStart);
    fCosAngle *= fCosAngle;
    float A = a*a - b*fCosAngle;
    float B = 2 * (c*a - d*fCosAngle);
    float C = c*c - e*fCosAngle;
    float D = B*B - 4 * A*C;

    if (D > 0)
    {
        D = sqrt(D);
        float2 t = (-B + sign(A)*float2(-D, +D)) / (2 * A);
        bool2 b2IsCorrect = c + a * t > 0 && t > 0;
        t = t * b2IsCorrect + !b2IsCorrect * (inf);
        return t;
    }
    else // no intersection
        return inf;
}

//-----------------------------------------------------------------------------------------
// RayPlaneIntersect
//-----------------------------------------------------------------------------------------
float RayPlaneIntersect(in float3 planeNormal, in float planeD, in float3 rayOrigin, in float3 rayDir)
{
    float NdotD = dot(planeNormal, rayDir);
    float NdotO = dot(planeNormal, rayOrigin);

    float t = -(NdotO + planeD) / NdotD;
    if (t < 0)
        t = 100000;
    return t;
}










[numthreads( 8, 8, 1 )]
void CSMain( uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID )
{
    Texture2D<float3>   InputBuf = ResourceDescriptorHeap[m_InputBufIndex];
    RWTexture2D<float3> Result   = ResourceDescriptorHeap[m_ResultIndex];

    //
    // Load 4 pixels per thread into LDS
    //
    int2 GroupUL = (Gid.xy << 3) - 4;                // Upper-left pixel coordinate of group read location
    int2 ThreadUL = (GTid.xy << 1) + GroupUL;        // Upper-left pixel coordinate of quad that this thread will read

    //
    // Store 4 unblurred pixels in LDS
    //
    int destIdx = GTid.x + (GTid.y << 4);
    Store2Pixels(destIdx+0, InputBuf[ThreadUL + uint2(0, 0)], InputBuf[ThreadUL + uint2(1, 0)]);
    Store2Pixels(destIdx+8, InputBuf[ThreadUL + uint2(0, 1)], InputBuf[ThreadUL + uint2(1, 1)]);

    GroupMemoryBarrierWithGroupSync();

    //
    // Horizontally blur the pixels in Cache
    //
    uint row = GTid.y << 4;
    BlurHorizontally(row + (GTid.x << 1), row + GTid.x + (GTid.x & 4));

    GroupMemoryBarrierWithGroupSync();

    //
    // Vertically blur the pixels and write the result to memory
    //
    BlurVertically(Result, DTid.xy, (GTid.y << 3) + GTid.x);
}
