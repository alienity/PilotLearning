#include "ATDefinitions.hlsli"
#include "ATFunctions.hlsli"

cbuffer Constants : register(b0)
{
    int atmosphereUniformIndex;
	int transmittance2DSRVIndex;
	int deltaRayleighUAVIndex;
	int deltaMieUAVIndex;
	int scatteringUAVIndex;
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
	RWTexture3D<float3> deltaRayleighUAV = ResourceDescriptorHeap[deltaRayleighUAVIndex];
	RWTexture3D<float3> deltaMieUAV = ResourceDescriptorHeap[deltaMieUAVIndex];
	RWTexture3D<float4> scatteringUAV = ResourceDescriptorHeap[scatteringUAVIndex];

	AtmosphereParameters atmosphereParameters;
	AtmosphereConstants atmosphereConstants;
	InitAtmosphereParameters(atmosphereUniform, atmosphereParameters, atmosphereConstants);
	AtmosphereSampler atmosphereSampler;
	InitAtmosphereSampler(sampler_LinearClamp, sampler_LinearRepeat, sampler_PointClamp, sampler_PointRepeat, atmosphereSampler);

	int layer = dispatchThreadID.z;

	float3 delta_rayleigh, delta_mie;
	ComputeSingleScatteringTexture(atmosphereParameters, atmosphereConstants, atmosphereSampler, 
		transmittanceTexture, float3(dispatchThreadID.xy + 0.5.xx, layer + 0.5), delta_rayleigh, delta_mie);
    float4 scattering = float4(delta_rayleigh.rgb, delta_mie.r);
	// float3 single_mie_scattering = mul(luminance_from_radiance, delta_mie);

    deltaRayleighUAV[dispatchThreadID.xyz] = delta_rayleigh;
    deltaMieUAV[dispatchThreadID.xyz] = delta_mie;
    scatteringUAV[dispatchThreadID.xyz] = scattering;
}
