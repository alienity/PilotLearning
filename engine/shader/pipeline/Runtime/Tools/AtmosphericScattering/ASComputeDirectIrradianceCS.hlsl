#include "ATDefinitions.hlsli"
#include "ATFunctions.hlsli"

cbuffer Constants : register(b0)
{
    int atmosphereUniformCBVIndex;
	int transmittance2DSRVIndex;
	int deltaIrradiance2DUAVIndex;
	int irradiance2DUAVIndex;
};

SamplerState sampler_LinearClamp : register(s10);
SamplerState sampler_LinearRepeat : register(s11);
SamplerState sampler_PointClamp : register(s12);
SamplerState sampler_PointRepeat : register(s13);

[numthreads(8, 8, 1)]
void CSMain(uint2 dispatchThreadID : SV_DispatchThreadID, uint2 groupThreadID : SV_GroupThreadID)
{
	ConstantBuffer<AtmosphereUniformCB> atmosphereUniformCB = ResourceDescriptorHeap[atmosphereUniformCBVIndex];
	AtmosphereUniform atmosphereUniform = atmosphereUniformCB.atmosphereUniform;
	Texture2D<float3> transmittanceTexture = ResourceDescriptorHeap[transmittance2DSRVIndex];
	RWTexture2D<float3> deltaIrradianceTexture = ResourceDescriptorHeap[deltaIrradiance2DUAVIndex];
	RWTexture2D<float3> irradianceTexture = ResourceDescriptorHeap[irradiance2DUAVIndex];

	AtmosphereParameters atmosphereParameters;
	AtmosphereConstants atmosphereConstants;
	InitAtmosphereParameters(atmosphereUniform, atmosphereParameters, atmosphereConstants);
	AtmosphereSampler atmosphereSampler;
	InitAtmosphereSampler(sampler_LinearClamp, sampler_LinearRepeat, sampler_PointClamp, sampler_PointRepeat, atmosphereSampler);

	float2 coord = dispatchThreadID.xy + 0.5f.xx;

    deltaIrradianceTexture[dispatchThreadID.xy] =
		ComputeDirectIrradianceTexture(atmosphereParameters, atmosphereConstants, atmosphereSampler, transmittanceTexture, coord);
	
	irradianceTexture[dispatchThreadID.xy] = 0.0f.xxx;
}
