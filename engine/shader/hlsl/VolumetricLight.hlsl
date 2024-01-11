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
    // sample cascade shadow map
    float4 cascadeWeights = GetCascadeWeights_SplitSpheres(wpos);
    bool inside = dot(cascadeWeights, float4(1, 1, 1, 1)) < 4;
    float4 samplePos = GetCascadeShadowCoord(float4(wpos, 1), cascadeWeights);
    float atten = inside ? UNITY_SAMPLE_SHADOW(_CascadeShadowMapTexture, samplePos.xyz) : 1.0f;
    return atten;
}

//-----------------------------------------------------------------------------------------
// ApplyHeightFog
//-----------------------------------------------------------------------------------------
void ApplyHeightFog(float3 wpos, inout float density)
{
    density *= exp(-(wpos.y + _HeightFog.x) * _HeightFog.y);
}

//-----------------------------------------------------------------------------------------
// GetDensity
//-----------------------------------------------------------------------------------------
float GetDensity(float3 wpos)
{
    float density = 1;

    float noise = tex3D(_NoiseTexture, frac(wpos * _NoiseData.x + float3(_Time.y * _NoiseVelocity.x, 0, _Time.y * _NoiseVelocity.y)));
    noise = saturate(noise - _NoiseData.z) * _NoiseData.y;
    density = saturate(noise);

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
    float2 interleavedPos = (fmod(floor(screenPos.xy), 8.0));
    float offset = tex2D(_DitherTexture, interleavedPos / 8.0 + float2(0.5 / 8.0, 0.5 / 8.0)).w;

    int stepCount = _SampleCount;

    float stepSize = rayLength / stepCount;
    float3 step = rayDir * stepSize;

    float3 currentPosition = rayStart + step * offset;

    float4 vlight = 0;

    float extinction = 0;
    float cosAngle = dot(_LightDir.xyz, -rayDir);

    [loop]
    for (int i = 0; i < stepCount; ++i)
    {
        float atten = GetLightAttenuation(currentPosition);
        float density = GetDensity(currentPosition);

        float scattering = _VolumetricLight.x * stepSize * density;
        extinction += _VolumetricLight.y * stepSize * density;// +scattering;

        float4 light = atten * scattering * exp(-extinction);

        vlight += light;

        currentPosition += step;				
    }

    // apply phase function for dir light
    vlight *= MieScattering(cosAngle, _MieG);

    // apply light's color
    vlight *= _LightColor;

    vlight = max(0, vlight);

    // use "proper" out-scattering/absorption for dir light 
    vlight.w = exp(-extinction);

    return vlight;
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/volume-rendering-for-developers/ray-marching-get-it-right.html
// https://github.com/scratchapixel/code

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
