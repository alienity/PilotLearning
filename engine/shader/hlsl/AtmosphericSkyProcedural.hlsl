#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ATFunctions.hlsli"

cbuffer PSConstants : register(b0)
{
	int _perFrameBuffer_index;
    int _transmittance_2d_texture_index;
	int _scattering_3d_texture_index;
	int _irradiance_2d_texture_index;
	int _single_mie_scattering_3d_texture_index;
	int _atmosphere_uniform_index;

	float _Exposure;
	float4 _White_point;
};

SamplerState cubeMapSampler : register(s10);

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 viewDir : TEXCOORD3;
};

/*
RadianceSpectrum GetSolarRadiance(AtmosphereParameters atmosphere) {
	return atmosphere.solar_irradiance /
		(PI * atmosphere.sun_angular_radius * atmosphere.sun_angular_radius);
}
RadianceSpectrum GetSkyRadiance(
	AtmosphereParameters atmosphere,
	Position camera, Direction view_ray, Length shadow_length,
	Direction sun_direction, out DimensionlessSpectrum transmittance) {
	return GetSkyRadiance(atmosphere, _transmittance_texture,
		_scattering_texture, _single_mie_scattering_texture,
		camera, view_ray, shadow_length, sun_direction, transmittance);
}
RadianceSpectrum GetSkyRadianceToPoint(
	AtmosphereParameters atmosphere,
	Position camera, Position _point, Length shadow_length,
	Direction sun_direction, out DimensionlessSpectrum transmittance) {
	return GetSkyRadianceToPoint(atmosphere, _transmittance_texture,
		_scattering_texture, _single_mie_scattering_texture,
		camera, _point, shadow_length, sun_direction, transmittance);
}
IrradianceSpectrum GetSunAndSkyIrradiance(
	AtmosphereParameters atmosphere,
	Position p, Direction normal, Direction sun_direction,
	out IrradianceSpectrum sky_irradiance) {
	return GetSunAndSkyIrradiance(atmosphere, _transmittance_texture,
		_irradiance_texture, p, normal, sun_direction, sky_irradiance);
}
*/

const static float3 kGroundAlbedo = float3(0.0, 0.0, 0.04);

float4 PSMain(VSOutput vsOutput) : SV_Target0
{
	ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[_perFrameBuffer_index];
	ConstantBuffer<AtmosphereUniform> _atmosphere_uniform = ResourceDescriptorHeap[_atmosphere_uniform_index];
	Texture2D _transmittance_texture = ResourceDescriptorHeap[_transmittance_2d_texture_index];
	Texture3D _scattering_texture = ResourceDescriptorHeap[_scattering_3d_texture_index];
	Texture2D _irradiance_texture = ResourceDescriptorHeap[_irradiance_2d_texture_index];
	Texture3D _single_mie_scattering_texture = ResourceDescriptorHeap[_single_mie_scattering_3d_texture_index];

	AtmosphereParameters atmosphereParameters = GetAtmosphereParameters(_atmosphere_uniform);

	float3 _Sun_direction = mFrameUniforms.directionalLight.lightPosition;
	float3 _WorldSpaceCameraPos = mFrameUniforms.cameraUniform.curFrameUniform.cameraPosition;

	float3 _Camera = _WorldSpaceCameraPos / 1000.0;
	float3 _Earth_center = float3(0, -atmosphereParameters.bottom_radius, 0);

	float3 _View_ray = normalize(vsOutput.viewDir);// normalize(IN.positionWS - _WorldSpaceCameraPos);
	// Normalized view direction vector.
	float3 view_direction = normalize(_View_ray);
	// Tangent of the angle subtended by this fragment.
	float fragment_angular_size = length(ddx(_View_ray) + ddy(_View_ray)) / length(_View_ray);

	//// Hack to fade out light shafts when the Sun is very close to the horizon.
	//float lightshaft_fadein_hack = smoothstep(
	//	0.02, 0.04, dot(normalize(_Camera - _Earth_center), _Sun_direction));

	// Compute the distance between the view ray line and the sphere center,
	// and the distance between the camera and the intersection of the view
	// ray with the sphere (or NaN if there is no intersection).
	float3 p = _Camera - _Earth_center;
	float p_dot_v = dot(p, view_direction);
	float p_dot_p = dot(p, p);
	float ray_sphere_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
	float distance_to_intersection = -p_dot_v - sqrt(
		_Earth_center.y * _Earth_center.y - ray_sphere_center_squared_distance);

	// Compute the radiance reflected by the ground, if the ray intersects it.
	float ground_alpha = 0.0;
	float3 ground_radiance = float3(0.0, 0.0, 0.0);
	if (distance_to_intersection > 0.0) {
		float3 _point = _Camera + view_direction * distance_to_intersection;
		float3 normal = normalize(_point - _Earth_center);
		// Compute the radiance reflected by the ground.
		float3 sky_irradiance;

		float3 sun_irradiance = GetSunAndSkyIrradiance(
			atmosphereParameters, _transmittance_texture, _irradiance_texture, 
			_point - _Earth_center, normal, _Sun_direction, sky_irradiance);
		ground_radiance = kGroundAlbedo * (1.0 / PI) * (sun_irradiance + sky_irradiance);
		//float shadow_length =
		//	max(0.0, min(shadow_out, distance_to_intersection) - shadow_in) *
		//	lightshaft_fadein_hack;
		float shadow_length = 0;
		float3 transmittance;

		float3 in_scatter = GetSkyRadianceToPoint(
			atmosphereParameters, _transmittance_texture, _scattering_texture, _single_mie_scattering_texture,
			_Camera - _Earth_center, _point - _Earth_center, shadow_length, _Sun_direction, transmittance);
		ground_radiance = ground_radiance * transmittance + in_scatter;
		ground_alpha = 1.0;
	}

	// Compute the radiance of the sky.
	//float shadow_length = max(0.0, shadow_out - shadow_in) *
	//	lightshaft_fadein_hack;
	float shadow_length = 0.0;
	float3 transmittance;
	
	float3 radiance = GetSkyRadiance(
		atmosphereParameters, _transmittance_texture, _scattering_texture, _single_mie_scattering_texture,
		_Camera - _Earth_center, view_direction, shadow_length, _Sun_direction, transmittance);

	// If the view ray intersects the Sun, add the Sun radiance.
	float _Sun_size = cos(atmosphereParameters.sun_angular_radius);
	if (dot(view_direction, _Sun_direction) > _Sun_size) {
		radiance = radiance + transmittance * GetSolarRadiance(atmosphereParameters);
	}
	radiance = lerp(radiance, ground_radiance, ground_alpha);
	//radiance = mix(radiance, sphere_radiance, sphere_alpha);
	float4 color;
	color.rgb = abs(float3(1.0, 1.0, 1.0) - exp(-radiance / _White_point)) * _Exposure;
	//pow(abs(float3(1.0, 1.0, 1.0) - exp(-radiance / _White_point * _Exposure)), float3(0.456, 0.456, 0.456));
	color.a = 1.0;

	return color;
}
