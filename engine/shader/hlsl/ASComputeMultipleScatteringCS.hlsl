#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ATDefinitions.hlsli"
#include "ATFunctions.hlsli"

cbuffer Constants : register(b0)
{
    int atmosphereUniformIndex;
    int transmittance2DSRVIndex;
    int scatteringDensity3DSRVIndex;
    int deltaMultipleScattering3DUAVIndex;
    int scattering3DUAVIndex;

    int scattering_order;
};

SamplerState sampler_LinearClamp : register(s10);
SamplerState sampler_LinearRepeat : register(s11);
SamplerState sampler_PointClamp : register(s12);
SamplerState sampler_PointRepeat : register(s13);

[numthreads(8, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    ConstantBuffer<AtmosphereUniformCB> atmosphereUniformCB = ResourceDescriptorHeap[atmosphereUniformIndex];
    AtmosphereUniform atmosphereUniform = atmosphereUniformCB.atmosphereUniform;
    Texture2D<float3> transmittanceTexture = ResourceDescriptorHeap[transmittance2DSRVIndex];
    Texture3D<float3> scatteringDensity3DSRV = ResourceDescriptorHeap[scatteringDensity3DSRVIndex];
    RWTexture3D<float3> deltaMultipleScattering3DUAV = ResourceDescriptorHeap[deltaMultipleScattering3DUAVIndex];
    RWTexture3D<float4> scattering3DUAV = ResourceDescriptorHeap[scattering3DUAVIndex];

    AtmosphereParameters atmosphereParameters;
    AtmosphereConstants atmosphereConstants;
    InitAtmosphereParameters(atmosphereUniform, atmosphereParameters, atmosphereConstants);
    AtmosphereSampler atmosphereSampler;
    InitAtmosphereSampler(sampler_LinearClamp, sampler_LinearRepeat, sampler_PointClamp, sampler_PointRepeat, atmosphereSampler);

    int layer = dispatchThreadID.z;

    float nu;
    float3 delta_multiple_scattering = ComputeMultipleScatteringTexture(
		atmosphereParameters, atmosphereConstants, atmosphereSampler,
		transmittanceTexture, scatteringDensity3DSRV,
		float3(dispatchThreadID.xy + 0.5.xx, layer + 0.5), nu);
    
    float4 scattering = float4(delta_multiple_scattering.rgb / RayleighPhaseFunction(nu), 0);

    deltaMultipleScattering3DUAV[dispatchThreadID.xyz] = delta_multiple_scattering;
    
    scattering3DUAV[dispatchThreadID.xyz] = scattering3DUAV[dispatchThreadID.xyz].rgba + scattering;
}
