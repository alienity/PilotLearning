#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ATDefinitions.hlsli"
#include "ATFunctions.hlsli"

cbuffer Constants : register(b0)
{
    int atmosphereUniformIndex;
	int singleRayleighScattering3DSRVIndex;
	int singleMieScattering3DSRVIndex;
	int multipleScattering3DSRVIndex;
	int deltaIrradiance2DUAVIndex;
	int irradiance2DUAVIndex;

	float3x3 luminance_from_radiance;
	int scattering_order;
};

SamplerState sampler_LinearClamp : register(s10);
SamplerState sampler_LinearRepeat : register(s11);
SamplerState sampler_PointClamp : register(s12);
SamplerState sampler_PointRepeat : register(s13);

[numthreads(8, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID)
{
	ConstantBuffer<AtmosphereUniformCB> atmosphereUniformCB = ResourceDescriptorHeap[atmosphereUniformIndex];
	AtmosphereUniform atmosphereUniform = atmosphereUniformCB.atmosphereUniform;
	Texture3D<float3> singleRayleighScattering3DSRV = ResourceDescriptorHeap[singleRayleighScattering3DSRVIndex];
	Texture3D<float3> singleMieScattering3DSRV = ResourceDescriptorHeap[singleMieScattering3DSRVIndex];
	Texture3D<float3> multipleScattering3DSRV = ResourceDescriptorHeap[multipleScattering3DSRVIndex];
	RWTexture2D<float3> deltaIrradiance2DUAV = ResourceDescriptorHeap[deltaIrradiance2DUAVIndex];
	RWTexture2D<float3> irradiance2DUAV = ResourceDescriptorHeap[irradiance2DUAVIndex];

	AtmosphereParameters atmosphereParameters;
	AtmosphereConstants atmosphereConstants;
	InitAtmosphereParameters(atmosphereUniform, atmosphereParameters, atmosphereConstants);
	AtmosphereSampler atmosphereSampler;
	InitAtmosphereSampler(sampler_LinearClamp, sampler_LinearRepeat, sampler_PointClamp, sampler_PointRepeat, atmosphereSampler);

	int layer = dispatchThreadID.z;

	float3 delta_irradiance = ComputeIndirectIrradianceTexture(
		atmosphereParameters, atmosphereConstants, atmosphereSampler, singleRayleighScattering3DSRV,
		singleMieScattering3DSRV, multipleScattering3DSRV,
		float2(dispatchThreadID.xy + 0.5.xx), scattering_order);

	float3 irradiance = mul(luminance_from_radiance, delta_irradiance);

    deltaIrradiance2DUAV[dispatchThreadID.xy] = delta_irradiance;
    irradiance2DUAV[dispatchThreadID.xy] = irradiance;
}
