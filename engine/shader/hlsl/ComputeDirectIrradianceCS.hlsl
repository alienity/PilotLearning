#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ATDefinitions.hlsli"
#include "ATFunctions.hlsli"

cbuffer Constants : register(b0)
{
    int atmosphereUniformIndex;
	int transmittanceTextureIndex;
	int deltaIrradianceTextureIndex;
};

SamplerState sampler_LinearClamp : register(s10);
SamplerState sampler_LinearRepeat : register(s11);
SamplerState sampler_PointClamp : register(s12);
SamplerState sampler_PointRepeat : register(s13);

[numthreads(8, 8, 1)]
void CSMain(uint2 dispatchThreadID : SV_DispatchThreadID, uint2 groupThreadID : SV_GroupThreadID)
{
	ConstantBuffer<AtmosphereUniformCB> atmosphereUniformCB = ResourceDescriptorHeap[atmosphereUniformIndex];
	AtmosphereUniform atmosphereUniform = atmosphereUniformCB.atmosphereUniform;
	Texture2D<float3> transmittanceTexture = ResourceDescriptorHeap[transmittanceTextureIndex];
	RWTexture2D<float3> deltaIrradianceTexture = ResourceDescriptorHeap[deltaIrradianceTextureIndex];

	AtmosphereParameters atmosphereParameters;
	AtmosphereConstants atmosphereConstants;
	InitAtmosphereParameters(atmosphereUniform, atmosphereParameters, atmosphereConstants);
	AtmosphereSampler atmosphereSampler;
	InitAtmosphereSampler(sampler_LinearClamp, sampler_LinearRepeat, sampler_PointClamp, sampler_PointRepeat, atmosphereSampler);

	uint width, height;
	deltaIrradianceTexture.GetDimensions(width, height);
	float2 coord = (dispatchThreadID.xy + 0.5f.xx) / float2(width, height);

	deltaIrradianceTexture[dispatchThreadID.xy] = 
		ComputeDirectIrradianceTexture(atmosphereParameters, atmosphereConstants, atmosphereSampler, 
			transmittanceTexture, coord);
}
