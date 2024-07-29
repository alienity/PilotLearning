#include "ATDefinitions.hlsli"
#include "ATFunctions.hlsli"

cbuffer Constants : register(b0)
{
    int atmosphereUniformCBVIndex;
	int transmittance2DUAVIndex;
};

SamplerState sampler_LinearClamp : register(s10);
SamplerState sampler_LinearRepeat : register(s11);
SamplerState sampler_PointClamp : register(s12);
SamplerState sampler_PointRepeat : register(s13);

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	ConstantBuffer<AtmosphereUniformCB> atmosphereUniformCB = ResourceDescriptorHeap[atmosphereUniformCBVIndex];
	AtmosphereUniform atmosphereUniform = atmosphereUniformCB.atmosphereUniform;
	RWTexture2D<float3> transmittanceTexture = ResourceDescriptorHeap[transmittance2DUAVIndex];

	AtmosphereParameters atmosphereParameters;
	AtmosphereConstants atmosphereConstants;
	InitAtmosphereParameters(atmosphereUniform, atmosphereParameters, atmosphereConstants);
	AtmosphereSampler atmosphereSampler;
	InitAtmosphereSampler(sampler_LinearClamp, sampler_LinearRepeat, sampler_PointClamp, sampler_PointRepeat, atmosphereSampler);

	float2 coord = dispatchThreadID.xy + 0.5f.xx;

    transmittanceTexture[dispatchThreadID.xy] = 
		ComputeTransmittanceToTopAtmosphereBoundaryTexture(atmosphereParameters, atmosphereConstants, coord);
		
}
