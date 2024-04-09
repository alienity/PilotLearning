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
	int singleRayleighScattering3DSRVIndex;
	int singleMieScattering3DSRVIndex;
	int multipleScattering3DSRVIndex;
	int irradiance2DSRVIndex;
	int scatteringDensityUAVIndex;

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
	Texture2D<float3> transmittanceTexture = ResourceDescriptorHeap[transmittance2DSRVIndex];
	Texture3D<float3> singleRayleighScattering3DSRV = ResourceDescriptorHeap[singleRayleighScattering3DSRVIndex];
	Texture3D<float3> singleMieScattering3DSRV = ResourceDescriptorHeap[singleMieScattering3DSRVIndex];
	Texture3D<float3> multipleScattering3DSRV = ResourceDescriptorHeap[multipleScattering3DSRVIndex];
	Texture2D<float3> irradiance2DSRV = ResourceDescriptorHeap[irradiance2DSRVIndex];
	RWTexture3D<float3> scatteringDensityUAV = ResourceDescriptorHeap[scatteringDensityUAVIndex];

	AtmosphereParameters atmosphereParameters;
	AtmosphereConstants atmosphereConstants;
	InitAtmosphereParameters(atmosphereUniform, atmosphereParameters, atmosphereConstants);
	AtmosphereSampler atmosphereSampler;
	InitAtmosphereSampler(sampler_LinearClamp, sampler_LinearRepeat, sampler_PointClamp, sampler_PointRepeat, atmosphereSampler);

	int layer = dispatchThreadID.z;

    float3 scattering_density = ComputeScatteringDensityTexture(
		atmosphereParameters, atmosphereConstants, atmosphereSampler,
		transmittanceTexture, singleRayleighScattering3DSRV,
		singleMieScattering3DSRV, multipleScattering3DSRV,
		irradiance2DSRV, float3(dispatchThreadID.xy + 0.5.xx, layer + 0.5),
		scattering_order);

    scatteringDensityUAV[uint3(dispatchThreadID.xy, layer)] = scattering_density;
}
