#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ATDefinitions.hlsli"
#include "ATFunctions.hlsli"

cbuffer PSConstants : register(b0)
{
	int perFrameBufferIndex;
    int atmosphereUniformIndex;
    int transmittance2DSRVIndex;
    int scattering3DSRVIndex;
	int irradiance2DSRVIndex;

	float _Exposure;
	float4 _White_point;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 viewDir : TEXCOORD3;
};

SamplerState sampler_LinearClamp : register(s10);
SamplerState sampler_LinearRepeat : register(s11);
SamplerState sampler_PointClamp : register(s12);
SamplerState sampler_PointRepeat : register(s13);

RadianceSpectrum GetSolarRadiance(AtmosphereParameters atmosphere) {
	return atmosphere.solar_irradiance /
		(PI * atmosphere.sun_angular_radius * atmosphere.sun_angular_radius);
}

const static float3 kGroundAlbedo = float3(0.0, 0.0, 0.04);

float4 PSMain(VSOutput vsOutput) : SV_Target0
{
	ConstantBuffer<FrameUniforms> mPerFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
	ConstantBuffer<AtmosphereUniformCB> atmosphereUniformCB = ResourceDescriptorHeap[atmosphereUniformIndex];
	AtmosphereUniform atmosphereUniform = atmosphereUniformCB.atmosphereUniform;
    Texture2D<float3> transmittance2DTexture = ResourceDescriptorHeap[transmittance2DSRVIndex];
    Texture3D<float4> scattering3DTexture = ResourceDescriptorHeap[scattering3DSRVIndex];
	Texture2D<float3> irradiance2DSRV = ResourceDescriptorHeap[irradiance2DSRVIndex];
	
	AtmosphereParameters atmosphereParameters;
	AtmosphereConstants atmosphereConstants;
	InitAtmosphereParameters(atmosphereUniform, atmosphereParameters, atmosphereConstants);
	AtmosphereSampler atmosphereSampler;
	InitAtmosphereSampler(sampler_LinearClamp, sampler_LinearRepeat, sampler_PointClamp, sampler_PointRepeat, atmosphereSampler);

    float3 sunDirection = mPerFrameUniforms.directionalLight.lightDirection;
    float3 cameraPosWS = mPerFrameUniforms.cameraUniform.curFrameUniform.cameraPosition;

    float3 cameraPos = cameraPosWS / 1000.0f;
	float3 earthCenter = float3(0, -atmosphereParameters.bottom_radius, 0);

	float3 viewRay = normalize(vsOutput.viewDir);// normalize(IN.positionWS - _WorldSpaceCameraPos);
	// Normalized view direction vector.
    float3 viewDirection = normalize(viewRay);
	// Tangent of the angle subtended by this fragment.
    float fragment_angular_size = length(ddx(viewRay) + ddy(viewRay)) / length(viewRay);

	//// Hack to fade out light shafts when the Sun is very close to the horizon.
	//float lightshaft_fadein_hack = smoothstep(
	//	0.02, 0.04, dot(normalize(_Camera - _Earth_center), _Sun_direction));

	// Compute the distance between the view ray line and the sphere center,
	// and the distance between the camera and the intersection of the view
	// ray with the sphere (or NaN if there is no intersection).
    float3 p = cameraPos - earthCenter;
    float p_dot_v = dot(p, viewDirection);
	float p_dot_p = dot(p, p);
	float ray_sphere_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
    float distance_to_intersection = -p_dot_v - sqrt(
		earthCenter.y * earthCenter.y - ray_sphere_center_squared_distance);

	// Compute the radiance reflected by the ground, if the ray intersects it.
	float ground_alpha = 0.0;
	float3 ground_radiance = float3(0.0, 0.0, 0.0);
    if (distance_to_intersection > 0.0)
    {
        float3 _point = cameraPos + viewDirection * distance_to_intersection;
        float3 normal = normalize(_point - earthCenter);
		// Compute the radiance reflected by the ground.
		float3 sky_irradiance;

		float3 sun_irradiance = GetSunAndSkyIrradiance(
			atmosphereParameters, atmosphereConstants, atmosphereSampler, 
			transmittance2DTexture, irradiance2DSRV,
			_point - earthCenter, normal, sunDirection, sky_irradiance);
		ground_radiance = kGroundAlbedo * (1.0 / PI) * (sun_irradiance + sky_irradiance);
		//float shadow_length =
		//	max(0.0, min(shadow_out, distance_to_intersection) - shadow_in) *
		//	lightshaft_fadein_hack;
		float shadow_length = 0;
		float3 transmittance;

		float3 in_scatter = GetSkyRadianceToPoint(
			atmosphereParameters, atmosphereConstants, atmosphereSampler, 
			transmittance2DTexture, scattering3DTexture,
			cameraPos - earthCenter, _point - earthCenter, shadow_length, sunDirection, transmittance);
		ground_radiance = ground_radiance * transmittance + in_scatter;
		ground_alpha = 1.0;
	}

	// Compute the radiance of the sky.
	//float shadow_length = max(0.0, shadow_out - shadow_in) *
	//	lightshaft_fadein_hack;
	float shadow_length = 0.0;
	float3 transmittance;
	
	float3 radiance = GetSkyRadiance(
		atmosphereParameters, atmosphereConstants, atmosphereSampler, 
		transmittance2DTexture, scattering3DTexture,
		cameraPos - earthCenter, viewDirection, shadow_length, sunDirection, transmittance);

	// If the view ray intersects the Sun, add the Sun radiance.
	float _Sun_size = cos(atmosphereParameters.sun_angular_radius);
    if (dot(viewDirection, sunDirection) > _Sun_size)
    {
		radiance = radiance + transmittance * GetSolarRadiance(atmosphereParameters);
	}
	radiance = lerp(radiance, ground_radiance, ground_alpha);
	//radiance = mix(radiance, sphere_radiance, sphere_alpha);
	float4 color;
    color.rgb = abs(float3(1.0, 1.0, 1.0) - exp(-radiance / _White_point.rgb)) * _Exposure;
	//pow(abs(float3(1.0, 1.0, 1.0) - exp(-radiance / _White_point * _Exposure)), float3(0.456, 0.456, 0.456));
	color.a = 1.0;

	return color;
}
