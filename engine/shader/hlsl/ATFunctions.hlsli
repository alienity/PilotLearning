#ifndef ATMOSPHERIC_SCATTERING_FUNCTION_INCLUDE
#define ATMOSPHERIC_SCATTERING_FUNCTION_INCLUDE

#include "ATDefinitions.hlsli"

Number ClampCosine(Number mu) {
	return clamp(mu, Number(-1.0), Number(1.0));
}

Length ClampDistance(Length d) {
	return max(d, 0.0 * m);
}

Length ClampRadius(IN(AtmosphereParameters) atmosphere, Length r) {
	return clamp(r, atmosphere.bottom_radius, atmosphere.top_radius);
}

//Length SafeSqrt(Area a) {
//	return sqrt(max(a, 0.0 * m2));
//}

//------------------------------------------------------------------------------------------------------------------
// Transmittance
//------------------------------------------------------------------------------------------------------------------

// Distance to the top atmosphere boundary
Length DistanceToTopAtmosphereBoundary(IN(AtmosphereParameters) atmosphere,
	Length r, Number mu) {
	assert(r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	Area discriminant = r * r * (mu * mu - 1.0) +
		atmosphere.top_radius * atmosphere.top_radius;
	return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

// Distance to the bottom atmosphere boundary
Length DistanceToBottomAtmosphereBoundary(IN(AtmosphereParameters) atmosphere,
	Length r, Number mu) {
	assert(r >= atmosphere.bottom_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	Area discriminant = r * r * (mu * mu - 1.0) +
		atmosphere.bottom_radius * atmosphere.bottom_radius;
	return ClampDistance(-r * mu - SafeSqrt(discriminant));
}

// Intersections with the ground
bool RayIntersectsGround(IN(AtmosphereParameters) atmosphere,
	Length r, Number mu) {
	assert(r >= atmosphere.bottom_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	return mu < 0.0 && r * r * (mu * mu - 1.0) +
		atmosphere.bottom_radius * atmosphere.bottom_radius >= 0.0 * m2;
}

// Transmittance to the top atmosphere boundary
Number GetLayerDensity(IN(DensityProfileLayer) layer, Length altitude) {
	Number density = layer.exp_term * exp(layer.exp_scale * altitude) +
		layer.linear_term * altitude + layer.constant_term;
	return clamp(density, Number(0.0), Number(1.0));
}

Number GetProfileDensity(IN(DensityProfile) profile, Length altitude) {
	return altitude < profile.layers[0].width ?
		GetLayerDensity(profile.layers[0], altitude) :
		GetLayerDensity(profile.layers[1], altitude);
}

Length ComputeOpticalLengthToTopAtmosphereBoundary(
	IN(AtmosphereParameters) atmosphere, IN(DensityProfile) profile,
	Length r, Number mu) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	// Number of intervals for the numerical integration.
	const int SAMPLE_COUNT = 500;
	// The integration step, i.e. the length of each integration interval.
	Length dx =
		DistanceToTopAtmosphereBoundary(atmosphere, r, mu) / Number(SAMPLE_COUNT);
	// Integration loop.
	Length result = 0.0 * m;
	for (int i = 0; i <= SAMPLE_COUNT; ++i) {
		Length d_i = Number(i) * dx;
		// Distance between the current sample point and the planet center.
		Length r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
		// Number density at the current sample point (divided by the number density
		// at the bottom of the atmosphere, yielding a dimensionless number).
		Number y_i = GetProfileDensity(profile, r_i - atmosphere.bottom_radius);
		// Sample weight (from the trapezoidal rule).
		Number weight_i = i == 0 || i == SAMPLE_COUNT ? 0.5 : 1.0;
		result += y_i * weight_i * dx;
	}
	return result;
}

DimensionlessSpectrum ComputeTransmittanceToTopAtmosphereBoundary(
	IN(AtmosphereParameters) atmosphere, Length r, Number mu) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	return exp(-(
		atmosphere.rayleigh_scattering *
		ComputeOpticalLengthToTopAtmosphereBoundary(
			atmosphere, atmosphere.rayleigh_density, r, mu) +
		atmosphere.mie_extinction *
		ComputeOpticalLengthToTopAtmosphereBoundary(
			atmosphere, atmosphere.mie_density, r, mu) +
		atmosphere.absorption_extinction *
		ComputeOpticalLengthToTopAtmosphereBoundary(
			atmosphere, atmosphere.absorption_density, r, mu)));
}

// Transmittance LUT Precomputation
Number GetTextureCoordFromUnitRange(Number x, int texture_size) {
	return 0.5 / Number(texture_size) + x * (1.0 - 1.0 / Number(texture_size));
}

Number GetUnitRangeFromTextureCoord(Number u, int texture_size) {
	return (u - 0.5 / Number(texture_size)) / (1.0 - 1.0 / Number(texture_size));
}

float2 GetTransmittanceTextureUvFromRMu(IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	Length r, Number mu) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	// Distance to top atmosphere boundary for a horizontal ray at ground level.
	Length H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
		atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the horizon.
	Length rho =
		SafeSqrt(r * r - atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
	// and maximum values over all mu - obtained for (r,1) and (r,mu_horizon).
	Length d = DistanceToTopAtmosphereBoundary(atmosphere, r, mu);
	Length d_min = atmosphere.top_radius - r;
	Length d_max = rho + H;
	Number x_mu = (d - d_min) / (d_max - d_min);
	Number x_r = rho / H;
	return float2(GetTextureCoordFromUnitRange(x_mu, atmoscons.TRANSMITTANCE_TEXTURE_WIDTH),
		GetTextureCoordFromUnitRange(x_r, atmoscons.TRANSMITTANCE_TEXTURE_HEIGHT));
}

void GetRMuFromTransmittanceTextureUv(IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(float2) uv, OUT(Length) r, OUT(Number) mu) {
	assert(uv.x >= 0.0 && uv.x <= 1.0);
	assert(uv.y >= 0.0 && uv.y <= 1.0);
	Number x_mu = GetUnitRangeFromTextureCoord(uv.x, atmoscons.TRANSMITTANCE_TEXTURE_WIDTH);
	Number x_r = GetUnitRangeFromTextureCoord(uv.y, atmoscons.TRANSMITTANCE_TEXTURE_HEIGHT);
	// Distance to top atmosphere boundary for a horizontal ray at ground level.
	Length H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
		atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the horizon, from which we can compute r:
	Length rho = H * x_r;
	r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
	// and maximum values over all mu - obtained for (r,1) and (r,mu_horizon) -
	// from which we can recover mu:
	Length d_min = atmosphere.top_radius - r;
	Length d_max = rho + H;
	Length d = d_min + x_mu * (d_max - d_min);
	mu = d == 0.0 * m ? Number(1.0) : (H * H - rho * rho - d * d) / (2.0 * r * d);
	mu = ClampCosine(mu);
}

DimensionlessSpectrum ComputeTransmittanceToTopAtmosphereBoundaryTexture(
	IN(AtmosphereParameters) atmosphere, IN(AtmosphereConstants) atmoscons, IN(float2) frag_coord) {
	const float2 TRANSMITTANCE_TEXTURE_SIZE =
		float2(atmoscons.TRANSMITTANCE_TEXTURE_WIDTH, atmoscons.TRANSMITTANCE_TEXTURE_HEIGHT);
	Length r;
	Number mu;
	GetRMuFromTransmittanceTextureUv(
		atmosphere, atmoscons, frag_coord / TRANSMITTANCE_TEXTURE_SIZE, r, mu);
	return ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r, mu);
}

// Transmittance LUT Lookup
DimensionlessSpectrum GetTransmittanceToTopAtmosphereBoundary(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	Length r, Number mu) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	float2 uv = GetTransmittanceTextureUvFromRMu(atmosphere, atmoscons, r, mu);
	return DimensionlessSpectrum(transmittance_texture.SampleLevel(atmossampler.sampler_LinearClamp, uv, 0).rgb);
}

DimensionlessSpectrum GetTransmittance(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	Length r, Number mu, Length d, bool ray_r_mu_intersects_ground) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	assert(d >= 0.0 * m);

	Length r_d = ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
	Number mu_d = ClampCosine((r * mu + d) / r_d);

	if (ray_r_mu_intersects_ground) {
		return min(
			GetTransmittanceToTopAtmosphereBoundary(
				atmosphere, atmoscons, atmossampler, transmittance_texture, r_d, -mu_d) /
			GetTransmittanceToTopAtmosphereBoundary(
				atmosphere, atmoscons, atmossampler, transmittance_texture, r, -mu),
			DimensionlessSpectrum(1.0f, 1.0f, 1.0f));
	}
	else {
		return min(
			GetTransmittanceToTopAtmosphereBoundary(
				atmosphere, atmoscons, atmossampler, transmittance_texture, r, mu) /
			GetTransmittanceToTopAtmosphereBoundary(
				atmosphere, atmoscons, atmossampler, transmittance_texture, r_d, mu_d),
			DimensionlessSpectrum(1.0, 1.0f, 1.0f));
	}
}

DimensionlessSpectrum GetTransmittanceToSun(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	Length r, Number mu_s) {
	Number sin_theta_h = atmosphere.bottom_radius / r;
	Number cos_theta_h = -sqrt(max(1.0 - sin_theta_h * sin_theta_h, 0.0));
	return GetTransmittanceToTopAtmosphereBoundary(
		atmosphere, atmoscons, atmossampler, transmittance_texture, r, mu_s) *
		smoothstep(-sin_theta_h * atmosphere.sun_angular_radius / rad,
			sin_theta_h * atmosphere.sun_angular_radius / rad,
			mu_s - cos_theta_h);
}

//------------------------------------------------------------------------------------------------------------------
// Single scattering
//------------------------------------------------------------------------------------------------------------------

void ComputeSingleScatteringIntegrand(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	Length r, Number mu, Number mu_s, Number nu, Length d,
	bool ray_r_mu_intersects_ground,
	OUT(DimensionlessSpectrum) rayleigh, OUT(DimensionlessSpectrum) mie) {
	Length r_d = ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
	Number mu_s_d = ClampCosine((r * mu_s + d * nu) / r_d);
	DimensionlessSpectrum transmittance =
		GetTransmittance(
			atmosphere, atmoscons, atmossampler, transmittance_texture, r, mu, d,
			ray_r_mu_intersects_ground) *
		GetTransmittanceToSun(
			atmosphere, atmoscons, atmossampler, transmittance_texture, r_d, mu_s_d);
	rayleigh = transmittance * GetProfileDensity(
		atmosphere.rayleigh_density, r_d - atmosphere.bottom_radius);
	mie = transmittance * GetProfileDensity(
		atmosphere.mie_density, r_d - atmosphere.bottom_radius);
}

Length DistanceToNearestAtmosphereBoundary(IN(AtmosphereParameters) atmosphere,
	Length r, Number mu, bool ray_r_mu_intersects_ground) {
	if (ray_r_mu_intersects_ground) {
		return DistanceToBottomAtmosphereBoundary(atmosphere, r, mu);
	}
	else {
		return DistanceToTopAtmosphereBoundary(atmosphere, r, mu);
	}
}

void ComputeSingleScattering(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground,
	OUT(IrradianceSpectrum) rayleigh, OUT(IrradianceSpectrum) mie) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	assert(nu >= -1.0 && nu <= 1.0);

	// Number of intervals for the numerical integration.
	const int SAMPLE_COUNT = 50;
	// The integration step, i.e. the length of each integration interval.
	Length dx =
		DistanceToNearestAtmosphereBoundary(atmosphere, r, mu,
			ray_r_mu_intersects_ground) / Number(SAMPLE_COUNT);
	// Integration loop.
	DimensionlessSpectrum rayleigh_sum = DimensionlessSpectrum(0.0f, 0.0f, 0.0f);
	DimensionlessSpectrum mie_sum = DimensionlessSpectrum(0.0f, 0.0f, 0.0f);
	for (int i = 0; i <= SAMPLE_COUNT; ++i) {
		Length d_i = Number(i) * dx;
		// The Rayleigh and Mie single scattering at the current sample point.
		DimensionlessSpectrum rayleigh_i;
		DimensionlessSpectrum mie_i;
		ComputeSingleScatteringIntegrand(atmosphere, atmoscons, atmossampler, transmittance_texture,
			r, mu, mu_s, nu, d_i, ray_r_mu_intersects_ground, rayleigh_i, mie_i);
		// Sample weight (from the trapezoidal rule).
		Number weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
		rayleigh_sum += rayleigh_i * weight_i;
		mie_sum += mie_i * weight_i;
	}
	rayleigh = rayleigh_sum * dx * atmosphere.solar_irradiance *
		atmosphere.rayleigh_scattering;
	mie = mie_sum * dx * atmosphere.solar_irradiance * atmosphere.mie_scattering;
}

InverseSolidAngle RayleighPhaseFunction(Number nu) {
	InverseSolidAngle k = 3.0 / (16.0 * VAPI * sr);
	return k * (1.0 + nu * nu);
}

InverseSolidAngle MiePhaseFunction(Number g, Number nu) {
	InverseSolidAngle k = 3.0 / (8.0 * VAPI * sr) * (1.0 - g * g) / (2.0 + g * g);
	return k * (1.0 + nu * nu) / pow(abs(1.0 + g * g - 2.0 * g * nu), 1.5);
}

// Single Scattering Precomputation
float4 GetScatteringTextureUvwzFromRMuMuSNu(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	assert(nu >= -1.0 && nu <= 1.0);

	// Distance to top atmosphere boundary for a horizontal ray at ground level.
	Length H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
		atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the horizon.
	Length rho =
		SafeSqrt(r * r - atmosphere.bottom_radius * atmosphere.bottom_radius);
	Number u_r = GetTextureCoordFromUnitRange(rho / H, atmoscons.SCATTERING_TEXTURE_R_SIZE);

	// Discriminant of the quadratic equation for the intersections of the ray
	// (r,mu) with the ground (see RayIntersectsGround).
	Length r_mu = r * mu;
	Area discriminant =
		r_mu * r_mu - r * r + atmosphere.bottom_radius * atmosphere.bottom_radius;
	Number u_mu;
	if (ray_r_mu_intersects_ground) {
		// Distance to the ground for the ray (r,mu), and its minimum and maximum
		// values over all mu - obtained for (r,-1) and (r,mu_horizon).
		Length d = -r_mu - SafeSqrt(discriminant);
		Length d_min = r - atmosphere.bottom_radius;
		Length d_max = rho;
		u_mu = 0.5 - 0.5 * GetTextureCoordFromUnitRange(d_max == d_min ? 0.0 :
			(d - d_min) / (d_max - d_min), atmoscons.SCATTERING_TEXTURE_MU_SIZE / 2.0);
	}
	else {
		// Distance to the top atmosphere boundary for the ray (r,mu), and its
		// minimum and maximum values over all mu - obtained for (r,1) and
		// (r,mu_horizon).
		Length d = -r_mu + SafeSqrt(discriminant + H * H);
		Length d_min = atmosphere.top_radius - r;
		Length d_max = rho + H;
		u_mu = 0.5 + 0.5 * GetTextureCoordFromUnitRange(
			(d - d_min) / (d_max - d_min), atmoscons.SCATTERING_TEXTURE_MU_SIZE / 2.0);
	}

	Length d = DistanceToTopAtmosphereBoundary(
		atmosphere, atmosphere.bottom_radius, mu_s);
	Length d_min = atmosphere.top_radius - atmosphere.bottom_radius;
	Length d_max = H;
	Number a = (d - d_min) / (d_max - d_min);
	Length D = DistanceToTopAtmosphereBoundary(
		atmosphere, atmosphere.bottom_radius, atmosphere.mu_s_min);
	Number A = (D - d_min) / (d_max - d_min);
	// An ad-hoc function equal to 0 for mu_s = mu_s_min (because then d = D and
	// thus a = A), equal to 1 for mu_s = 1 (because then d = d_min and thus
	// a = 0), and with a large slope around mu_s = 0, to get more texture 
	// samples near the horizon.
	Number u_mu_s = GetTextureCoordFromUnitRange(
		max(1.0 - a / A, 0.0) / (1.0 + a), atmoscons.SCATTERING_TEXTURE_MU_S_SIZE);

	Number u_nu = (nu + 1.0) / 2.0;
	return float4(u_nu, u_mu_s, u_mu, u_r);
}

void GetRMuMuSNuFromScatteringTextureUvwz(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(float4) uvwz, OUT(Length) r, OUT(Number) mu, OUT(Number) mu_s,
	OUT(Number) nu, OUT(bool) ray_r_mu_intersects_ground) {
	assert(uvwz.x >= 0.0 && uvwz.x <= 1.0);
	assert(uvwz.y >= 0.0 && uvwz.y <= 1.0);
	assert(uvwz.z >= 0.0 && uvwz.z <= 1.0);
	assert(uvwz.w >= 0.0 && uvwz.w <= 1.0);

	// Distance to top atmosphere boundary for a horizontal ray at ground level.
	Length H = sqrt(atmosphere.top_radius * atmosphere.top_radius -
		atmosphere.bottom_radius * atmosphere.bottom_radius);
	// Distance to the horizon.
	Length rho =
		H * GetUnitRangeFromTextureCoord(uvwz.w, atmoscons.SCATTERING_TEXTURE_R_SIZE);
	r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);

	if (uvwz.z < 0.5) {
		// Distance to the ground for the ray (r,mu), and its minimum and maximum
		// values over all mu - obtained for (r,-1) and (r,mu_horizon) - from which
		// we can recover mu:
		Length d_min = r - atmosphere.bottom_radius;
		Length d_max = rho;
		Length d = d_min + (d_max - d_min) * GetUnitRangeFromTextureCoord(
			1.0 - 2.0 * uvwz.z, atmoscons.SCATTERING_TEXTURE_MU_SIZE / 2.0f);
		mu = d == 0.0 * m ? Number(-1.0) :
			ClampCosine(-(rho * rho + d * d) / (2.0 * r * d));
		ray_r_mu_intersects_ground = true;
	}
	else {
		// Distance to the top atmosphere boundary for the ray (r,mu), and its
		// minimum and maximum values over all mu - obtained for (r,1) and
		// (r,mu_horizon) - from which we can recover mu:
		Length d_min = atmosphere.top_radius - r;
		Length d_max = rho + H;
		Length d = d_min + (d_max - d_min) * GetUnitRangeFromTextureCoord(
			2.0 * uvwz.z - 1.0, atmoscons.SCATTERING_TEXTURE_MU_SIZE / 2.0f);
		mu = d == 0.0 * m ? Number(1.0) :
			ClampCosine((H * H - rho * rho - d * d) / (2.0 * r * d));
		ray_r_mu_intersects_ground = false;
	}

	Number x_mu_s =
		GetUnitRangeFromTextureCoord(uvwz.y, atmoscons.SCATTERING_TEXTURE_MU_S_SIZE);
	Length d_min = atmosphere.top_radius - atmosphere.bottom_radius;
	Length d_max = H;
	Length D = DistanceToTopAtmosphereBoundary(
		atmosphere, atmosphere.bottom_radius, atmosphere.mu_s_min);
	Number A = (D - d_min) / (d_max - d_min);
	Number a = (A - x_mu_s * A) / (1.0 + x_mu_s * A);
	Length d = d_min + min(a, A) * (d_max - d_min);
	mu_s = d == 0.0 * m ? Number(1.0) :
		ClampCosine((H * H - d * d) / (2.0 * atmosphere.bottom_radius * d));

	nu = ClampCosine(uvwz.x * 2.0 - 1.0);
}

// mapping from 3D texel coordinate to 4D texture coordinate
void GetRMuMuSNuFromScatteringTextureFragCoord(
	IN(AtmosphereParameters) atmosphere, 
	IN(AtmosphereConstants) atmoscons, 
	IN(float3) frag_coord,
	OUT(Length) r, OUT(Number) mu, OUT(Number) mu_s, OUT(Number) nu,
	OUT(bool) ray_r_mu_intersects_ground) {
	const float4 SCATTERING_TEXTURE_SIZE = float4(
		atmoscons.SCATTERING_TEXTURE_NU_SIZE - 1,
		atmoscons.SCATTERING_TEXTURE_MU_S_SIZE,
		atmoscons.SCATTERING_TEXTURE_MU_SIZE,
		atmoscons.SCATTERING_TEXTURE_R_SIZE);
	Number frag_coord_nu =
		floor(frag_coord.x / Number(atmoscons.SCATTERING_TEXTURE_MU_S_SIZE));
	Number frag_coord_mu_s =
		fmod(frag_coord.x, Number(atmoscons.SCATTERING_TEXTURE_MU_S_SIZE));
	float4 uvwz =
		float4(frag_coord_nu, frag_coord_mu_s, frag_coord.y, frag_coord.z) /
		SCATTERING_TEXTURE_SIZE;
	GetRMuMuSNuFromScatteringTextureUvwz(
		atmosphere, atmoscons, uvwz, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	// Clamp nu to its valid range of values, given mu and mu_s.
	nu = clamp(nu, mu * mu_s - sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)),
		mu * mu_s + sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)));
}

// precompute a texel of the single scattering in a 3D texture
void ComputeSingleScatteringTexture(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture, IN(float3) frag_coord,
	OUT(IrradianceSpectrum) rayleigh, OUT(IrradianceSpectrum) mie) {
	Length r;
	Number mu;
	Number mu_s;
	Number nu;
	bool ray_r_mu_intersects_ground;
	GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, atmoscons, frag_coord,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	ComputeSingleScattering(atmosphere, atmoscons, atmossampler, transmittance_texture,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground, rayleigh, mie);
}

// Single Scattering Lookup. 
// we need two 3D texture lookups to emulate a single 4D texture lookup with quadrilinear interpolation; 
// the 3D texture coordinates are computed using the inverse of the 3D-4D mapping GetRMuMuSNuFromScatteringTextureFragCoord
TEMPLATE(AbstractSpectrum)
AbstractSpectrum GetScattering(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(AbstractScatteringTexture TEMPLATE_ARGUMENT(AbstractSpectrum)) scattering_texture,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground) {
	float4 uvwz = GetScatteringTextureUvwzFromRMuMuSNu(
		atmosphere, atmoscons, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	Number tex_coord_x = uvwz.x * Number(atmoscons.SCATTERING_TEXTURE_NU_SIZE - 1);
	Number tex_x = floor(tex_coord_x);
	Number lerp = tex_coord_x - tex_x;
	float3 uvw0 = float3((tex_x + uvwz.y) / Number(atmoscons.SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	float3 uvw1 = float3((tex_x + 1.0 + uvwz.y) / Number(atmoscons.SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	return AbstractSpectrum(scattering_texture.SampleLevel(atmossampler.sampler_LinearClamp, uvw0, 0).rgb * (1.0 - lerp) +
		scattering_texture.SampleLevel(atmossampler.sampler_LinearClamp, uvw1, 0).rgb * lerp);
}

RadianceSpectrum GetScattering(
	IN(AtmosphereParameters) atmosphere, IN(AtmosphereConstants) atmoscons, IN(AtmosphereSampler) atmossampler,
	IN(ReducedScatteringTexture) single_rayleigh_scattering_texture,
	IN(ReducedScatteringTexture) single_mie_scattering_texture,
	IN(ScatteringTexture) multiple_scattering_texture,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground,
	int scattering_order) {
	if (scattering_order == 1) {
		IrradianceSpectrum rayleigh = GetScattering(
			atmosphere, atmoscons, atmossampler, single_rayleigh_scattering_texture, r, mu, mu_s, nu,
			ray_r_mu_intersects_ground);
		IrradianceSpectrum mie = GetScattering(
			atmosphere, atmoscons, atmossampler, single_mie_scattering_texture, r, mu, mu_s, nu,
			ray_r_mu_intersects_ground);
		return rayleigh * RayleighPhaseFunction(nu) +
			mie * MiePhaseFunction(atmosphere.mie_phase_function_g, nu);
	}
	else {
		return GetScattering(
			atmosphere, atmoscons, atmossampler, multiple_scattering_texture, r, mu, mu_s, nu,
			ray_r_mu_intersects_ground);
	}
}

//------------------------------------------------------------------------------------------------------------------
// Multiple scattering
//------------------------------------------------------------------------------------------------------------------

IrradianceSpectrum GetIrradiance(
	IN(AtmosphereParameters) atmosphere, IN(AtmosphereConstants) atmoscons, IN(AtmosphereSampler) atmossampler,
	IN(IrradianceTexture) irradiance_texture,
	Length r, Number mu_s);

// ------First step------
// The first step computes the radiance which is scattered at some point q inside the atmosphere, towards some function -w.
// Furthermore, we assume that this scattering event is the n-th bounce.
// multiple_scattering_texture is supposed to contain the (n-1)-th order of scattering, if n > 2, 
// irradiance_texture is the irradiance reveived on the ground after n-2 bounces, and scattering_order is equal to m
RadianceDensitySpectrum ComputeScatteringDensity(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	IN(ReducedScatteringTexture) single_rayleigh_scattering_texture,
	IN(ReducedScatteringTexture) single_mie_scattering_texture,
	IN(ScatteringTexture) multiple_scattering_texture,
	IN(IrradianceTexture) irradiance_texture,
	Length r, Number mu, Number mu_s, Number nu, int scattering_order) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	assert(nu >= -1.0 && nu <= 1.0);
	assert(scattering_order >= 2);

	// Compute unit direction vectors for the zenith, the view direction omega and
	// and the sun direction omega_s, such that the cosine of the view-zenith
	// angle is mu, the cosine of the sun-zenith angle is mu_s, and the cosine of
	// the view-sun angle is nu. The goal is to simplify computations below.
	float3 zenith_direction = float3(0.0, 0.0, 1.0);
	float3 omega = float3(sqrt(1.0 - mu * mu), 0.0, mu);
	Number sun_dir_x = omega.x == 0.0 ? 0.0 : (nu - mu * mu_s) / omega.x;
	Number sun_dir_y = sqrt(max(1.0 - sun_dir_x * sun_dir_x - mu_s * mu_s, 0.0));
	float3 omega_s = float3(sun_dir_x, sun_dir_y, mu_s);

	const int SAMPLE_COUNT = 16;
	const Angle dphi = pi / Number(SAMPLE_COUNT);
	const Angle dtheta = pi / Number(SAMPLE_COUNT);
	RadianceDensitySpectrum rayleigh_mie =
		RadianceDensitySpectrum(0.0f, 0.0f, 0.0f) * watt_per_cubic_meter_per_sr_per_nm;

	// Nested loops for the integral over all the incident directions omega_i.
	for (int l = 0; l < SAMPLE_COUNT; ++l) {
		Angle theta = (Number(l) + 0.5) * dtheta;
		Number cos_theta = cos(theta);
		Number sin_theta = sin(theta);
		bool ray_r_theta_intersects_ground =
			RayIntersectsGround(atmosphere, r, cos_theta);

		// The distance and transmittance to the ground only depend on theta, so we
		// can compute them in the outer loop for efficiency.
		Length distance_to_ground = 0.0 * m;
		DimensionlessSpectrum transmittance_to_ground = DimensionlessSpectrum(0.0f, 0.0f, 0.0f);
		DimensionlessSpectrum ground_albedo = DimensionlessSpectrum(0.0f, 0.0f, 0.0f);
		if (ray_r_theta_intersects_ground) {
			distance_to_ground =
				DistanceToBottomAtmosphereBoundary(atmosphere, r, cos_theta);
			transmittance_to_ground =
				GetTransmittance(atmosphere, atmoscons, atmossampler, transmittance_texture, r, cos_theta,
					distance_to_ground, true /* ray_intersects_ground */);
			ground_albedo = atmosphere.ground_albedo;
		}

		for (int m = 0; m < 2 * SAMPLE_COUNT; ++m) {
			Angle phi = (Number(m) + 0.5) * dphi;
			float3 omega_i =
				float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
			SolidAngle domega_i = (dtheta / rad) * (dphi / rad) * sin(theta) * sr;

			// The radiance L_i arriving from direction omega_i after n-1 bounces is
			// the sum of a term given by the precomputed scattering texture for the
			// (n-1)-th order:
			Number nu1 = dot(omega_s, omega_i);
			RadianceSpectrum incident_radiance = GetScattering(
				atmosphere, atmoscons, atmossampler,
				single_rayleigh_scattering_texture,
				single_mie_scattering_texture,
				multiple_scattering_texture,
				r, omega_i.z, mu_s, nu1,
				ray_r_theta_intersects_ground, scattering_order - 1);

			// and of the contribution from the light paths with n-1 bounces and whose
			// last bounce is on the ground. This contribution is the product of the
			// transmittance to the ground, the ground albedo, the ground BRDF, and
			// the irradiance received on the ground after n-2 bounces.
			float3 ground_normal =
				normalize(zenith_direction * r + omega_i * distance_to_ground);
			IrradianceSpectrum ground_irradiance = GetIrradiance(
				atmosphere, atmoscons, atmossampler, irradiance_texture, atmosphere.bottom_radius,
				dot(ground_normal, omega_s));
			incident_radiance += transmittance_to_ground *
				ground_albedo * (1.0 / (VAPI * sr)) * ground_irradiance;

			// The radiance finally scattered from direction omega_i towards direction
			// -omega is the product of the incident radiance, the scattering
			// coefficient, and the phase function for directions omega and omega_i
			// (all this summed over all particle types, i.e. Rayleigh and Mie).
			Number nu2 = dot(omega, omega_i);
			Number rayleigh_density = GetProfileDensity(
				atmosphere.rayleigh_density, r - atmosphere.bottom_radius);
			Number mie_density = GetProfileDensity(
				atmosphere.mie_density, r - atmosphere.bottom_radius);
			rayleigh_mie += incident_radiance * (
				atmosphere.rayleigh_scattering * rayleigh_density *
				RayleighPhaseFunction(nu2) +
				atmosphere.mie_scattering * mie_density *
				MiePhaseFunction(atmosphere.mie_phase_function_g, nu2)) *
				domega_i;
		}
	}
	return rayleigh_mie;
}

// ------Second step------
// The second step to compute the n-th order of scattering is to compute for each point p and direction w, 
// the radiance coming from direction w after n bounces, using a texture precomputed with the previous function.
RadianceSpectrum ComputeMultipleScattering(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	IN(ScatteringDensityTexture) scattering_density_texture,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu >= -1.0 && mu <= 1.0);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	assert(nu >= -1.0 && nu <= 1.0);

	// Number of intervals for the numerical integration.
	const int SAMPLE_COUNT = 50;
	// The integration step, i.e. the length of each integration interval.
	Length dx =
		DistanceToNearestAtmosphereBoundary(
			atmosphere, r, mu, ray_r_mu_intersects_ground) /
		Number(SAMPLE_COUNT);
	// Integration loop.
	RadianceSpectrum rayleigh_mie_sum =
		RadianceSpectrum(0.0f, 0.0f, 0.0f) * watt_per_square_meter_per_sr_per_nm;
	for (int i = 0; i <= SAMPLE_COUNT; ++i) {
		Length d_i = Number(i) * dx;

		// The r, mu and mu_s parameters at the current integration point (see the
		// single scattering section for a detailed explanation).
		Length r_i =
			ClampRadius(atmosphere, sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r));
		Number mu_i = ClampCosine((r * mu + d_i) / r_i);
		Number mu_s_i = ClampCosine((r * mu_s + d_i * nu) / r_i);

		// The Rayleigh and Mie multiple scattering at the current sample point.
		RadianceSpectrum rayleigh_mie_i =
			GetScattering(
				atmosphere, atmoscons, atmossampler, scattering_density_texture, r_i, mu_i, mu_s_i, nu,
				ray_r_mu_intersects_ground) *
			GetTransmittance(
				atmosphere, atmoscons, atmossampler, transmittance_texture, r, mu, d_i,
				ray_r_mu_intersects_ground) *
			dx;
		// Sample weight (from the trapezoidal rule).
		Number weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
		rayleigh_mie_sum += rayleigh_mie_i * weight_i;
	}
	return rayleigh_mie_sum;
}

// Multiscattering Precomputation
// This immediately leads to the following simple functions to precompute a texel of the textures
// for the first and second steps of each iteration over the number of bounces: 

RadianceDensitySpectrum ComputeScatteringDensityTexture(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	IN(ReducedScatteringTexture) single_rayleigh_scattering_texture,
	IN(ReducedScatteringTexture) single_mie_scattering_texture,
	IN(ScatteringTexture) multiple_scattering_texture,
	IN(IrradianceTexture) irradiance_texture,
	IN(float3) frag_coord, int scattering_order) {
	Length r;
	Number mu;
	Number mu_s;
	Number nu;
	bool ray_r_mu_intersects_ground;
	GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, atmoscons, frag_coord,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	return ComputeScatteringDensity(atmosphere, atmoscons, atmossampler, transmittance_texture,
		single_rayleigh_scattering_texture, single_mie_scattering_texture,
		multiple_scattering_texture, irradiance_texture, r, mu, mu_s, nu,
		scattering_order);
}

RadianceSpectrum ComputeMultipleScatteringTexture(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	IN(ScatteringDensityTexture) scattering_density_texture,
	IN(float3) frag_coord, OUT(Number) nu) {
	Length r;
	Number mu;
	Number mu_s;
	bool ray_r_mu_intersects_ground;
	GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, atmoscons, frag_coord,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	return ComputeMultipleScattering(atmosphere, atmoscons, atmossampler, transmittance_texture,
		scattering_density_texture, r, mu, mu_s, nu,
		ray_r_mu_intersects_ground);
}

//------------------------------------------------------------------------------------------------------------------
// Ground irradiance
//------------------------------------------------------------------------------------------------------------------
// The ground irradiance is the Sun light received on the ground after n ≥ 0 bounces 
// (where a bounce is either a scattering event or a reflection on the ground).

IrradianceSpectrum ComputeDirectIrradiance(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	Length r, Number mu_s) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu_s >= -1.0 && mu_s <= 1.0);

	Number alpha_s = atmosphere.sun_angular_radius / rad;
	// Approximate average of the cosine factor mu_s over the visible fraction of
	// the Sun disc.
	Number average_cosine_factor =
		mu_s < -alpha_s ? 0.0 : (mu_s > alpha_s ? mu_s :
			(mu_s + alpha_s) * (mu_s + alpha_s) / (4.0 * alpha_s));

	return atmosphere.solar_irradiance *
		GetTransmittanceToTopAtmosphereBoundary(
			atmosphere, atmoscons, atmossampler, transmittance_texture, r, mu_s) * average_cosine_factor;

}

IrradianceSpectrum ComputeIndirectIrradiance(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(ReducedScatteringTexture) single_rayleigh_scattering_texture,
	IN(ReducedScatteringTexture) single_mie_scattering_texture,
	IN(ScatteringTexture) multiple_scattering_texture,
	Length r, Number mu_s, int scattering_order) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	assert(scattering_order >= 1);

	const int SAMPLE_COUNT = 32;
	const Angle dphi = pi / Number(SAMPLE_COUNT);
	const Angle dtheta = pi / Number(SAMPLE_COUNT);

	IrradianceSpectrum result =
		IrradianceSpectrum(0.0f, 0.0f, 0.0f) * watt_per_square_meter_per_nm;
	float3 omega_s = float3(sqrt(1.0 - mu_s * mu_s), 0.0, mu_s);
	for (int j = 0; j < SAMPLE_COUNT / 2; ++j) {
		Angle theta = (Number(j) + 0.5) * dtheta;
		for (int i = 0; i < 2 * SAMPLE_COUNT; ++i) {
			Angle phi = (Number(i) + 0.5) * dphi;
			float3 omega =
				float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
			SolidAngle domega = (dtheta / rad) * (dphi / rad) * sin(theta) * sr;

			Number nu = dot(omega, omega_s);
			result += GetScattering(atmosphere, atmoscons, atmossampler, single_rayleigh_scattering_texture,
				single_mie_scattering_texture, multiple_scattering_texture,
				r, omega.z, mu_s, nu, false /* ray_r_theta_intersects_ground */,
				scattering_order) *
				omega.z * domega;
		}
	}
	return result;
}

// Ground irradiance Precomputation

float2 GetIrradianceTextureUvFromRMuS(
	IN(AtmosphereParameters) atmosphere, IN(AtmosphereConstants) atmoscons,
	Length r, Number mu_s) {
	assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
	assert(mu_s >= -1.0 && mu_s <= 1.0);
	Number x_r = (r - atmosphere.bottom_radius) /
		(atmosphere.top_radius - atmosphere.bottom_radius);
	Number x_mu_s = mu_s * 0.5 + 0.5;
	return float2(GetTextureCoordFromUnitRange(x_mu_s, atmoscons.IRRADIANCE_TEXTURE_WIDTH),
		GetTextureCoordFromUnitRange(x_r, atmoscons.IRRADIANCE_TEXTURE_HEIGHT));
}

void GetRMuSFromIrradianceTextureUv(
	IN(AtmosphereParameters) atmosphere, IN(AtmosphereConstants) atmoscons,
	IN(float2) uv, OUT(Length) r, OUT(Number) mu_s) {
	assert(uv.x >= 0.0 && uv.x <= 1.0);
	assert(uv.y >= 0.0 && uv.y <= 1.0);
	Number x_mu_s = GetUnitRangeFromTextureCoord(uv.x, atmoscons.IRRADIANCE_TEXTURE_WIDTH);
	Number x_r = GetUnitRangeFromTextureCoord(uv.y, atmoscons.IRRADIANCE_TEXTURE_HEIGHT);
	r = atmosphere.bottom_radius +
		x_r * (atmosphere.top_radius - atmosphere.bottom_radius);
	mu_s = ClampCosine(2.0 * x_mu_s - 1.0);
}

//const float2 IRRADIANCE_TEXTURE_SIZE =
//float2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);

IrradianceSpectrum ComputeDirectIrradianceTexture(
	IN(AtmosphereParameters) atmosphere, IN(AtmosphereConstants) atmoscons, IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	IN(float2) frag_coord) {
	Length r;
	Number mu_s;
	float2 IRRADIANCE_TEXTURE_SIZE =
		float2(atmoscons.IRRADIANCE_TEXTURE_WIDTH, atmoscons.IRRADIANCE_TEXTURE_HEIGHT);
	GetRMuSFromIrradianceTextureUv(
		atmosphere, atmoscons, frag_coord / IRRADIANCE_TEXTURE_SIZE, r, mu_s);
	return ComputeDirectIrradiance(atmosphere, atmoscons, atmossampler, transmittance_texture, r, mu_s);
}

IrradianceSpectrum ComputeIndirectIrradianceTexture(
	IN(AtmosphereParameters) atmosphere, IN(AtmosphereConstants) atmoscons, IN(AtmosphereSampler) atmossampler,
	IN(ReducedScatteringTexture) single_rayleigh_scattering_texture,
	IN(ReducedScatteringTexture) single_mie_scattering_texture,
	IN(ScatteringTexture) multiple_scattering_texture,
	IN(float2) frag_coord, int scattering_order) {
	Length r;
	Number mu_s;
	float2 IRRADIANCE_TEXTURE_SIZE =
		float2(atmoscons.IRRADIANCE_TEXTURE_WIDTH, atmoscons.IRRADIANCE_TEXTURE_HEIGHT);
	GetRMuSFromIrradianceTextureUv(
		atmosphere, atmoscons, frag_coord / IRRADIANCE_TEXTURE_SIZE, r, mu_s);
	return ComputeIndirectIrradiance(atmosphere, atmoscons, atmossampler,
		single_rayleigh_scattering_texture, single_mie_scattering_texture,
		multiple_scattering_texture, r, mu_s, scattering_order);
}

// Ground irradiance Lookup

IrradianceSpectrum GetIrradiance(
	IN(AtmosphereParameters) atmosphere, IN(AtmosphereConstants) atmoscons, IN(AtmosphereSampler) atmossampler,
	IN(IrradianceTexture) irradiance_texture,
	Length r, Number mu_s) {
	float2 uv = GetIrradianceTextureUvFromRMuS(atmosphere, atmoscons, r, mu_s);
	//return IrradianceSpectrum(tex2D(irradiance_texture, uv).rgb);
	return IrradianceSpectrum(irradiance_texture.SampleLevel(atmossampler.sampler_LinearClamp, uv, 0).rgb);
}

//------------------------------------------------------------------------------------------------------------------
// Rendering
//------------------------------------------------------------------------------------------------------------------
// Here we assume that the transmittance, scattering and irradiance textures have been precomputed, 
// and we provide functions using them to compute the sky color, the aerial perspective, and the ground radiance. 

// More precisely, we assume that the single Rayleigh scattering, without its phase function term, plus the 
// multiple scattering terms (divided by the Rayleigh phase function for dimensional homogeneity) are stored 
// in a scattering_texture. We also assume that the single Mie scattering is stored, without its phase function term

// if the COMBINED_SCATTERING_TEXTURES preprocessor macro is defined, in the scattering_texture. In this case, 
// which is only available with a GLSL compiler, Rayleigh and multiple scattering are stored in the RGB channels, 
// and the red component of the single Mie scattering is stored in the alpha channel.

float3 GetExtrapolatedSingleMieScattering(
	IN(AtmosphereParameters) atmosphere, IN(float4) scattering) {
	// Algebraically this can never be negative, but rounding errors can produce
	// that effect for sufficiently short view rays.
	if (scattering.r <= 0.0) {
		return float3(0.0f, 0.0f, 0.0f);
	}
	return scattering.rgb * scattering.a / scattering.r *
		(atmosphere.rayleigh_scattering.r / atmosphere.mie_scattering.r) *
		(atmosphere.mie_scattering / atmosphere.rayleigh_scattering);
}

IrradianceSpectrum GetCombinedScattering(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(CombinedScatteringTexture) scattering_texture,
	Length r, Number mu, Number mu_s, Number nu,
	bool ray_r_mu_intersects_ground,
	OUT(IrradianceSpectrum) single_mie_scattering) {
	float4 uvwz = GetScatteringTextureUvwzFromRMuMuSNu(
		atmosphere, atmoscons, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	Number tex_coord_x = uvwz.x * Number(atmoscons.SCATTERING_TEXTURE_NU_SIZE - 1);
	Number tex_x = floor(tex_coord_x);
	Number lerp = tex_coord_x - tex_x;
	float3 uvw0 = float3((tex_x + uvwz.y) / Number(atmoscons.SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	float3 uvw1 = float3((tex_x + 1.0 + uvwz.y) / Number(atmoscons.SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	float4 combined_scattering =
		scattering_texture.Sample(atmossampler.sampler_LinearClamp, uvw0) * (1.0 - lerp) + 
		scattering_texture.Sample(atmossampler.sampler_LinearClamp, uvw1) * lerp;
	IrradianceSpectrum scattering = IrradianceSpectrum(combined_scattering.xyz);
	single_mie_scattering =
		GetExtrapolatedSingleMieScattering(atmosphere, combined_scattering);
	return scattering;
}

// ------------------Sky Rendering------------------
RadianceSpectrum GetSkyRadiance(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	IN(CombinedScatteringTexture) scattering_texture,
	Position camera, IN(Direction) view_ray, Length shadow_length,
	IN(Direction) sun_direction, OUT(DimensionlessSpectrum) transmittance) {
	// Compute the distance to the top atmosphere boundary along the view ray,
	// assuming the viewer is in space (or NaN if the view ray does not intersect
	// the atmosphere).
	Length r = length(camera);
	Length rmu = dot(camera, view_ray);
	Length distance_to_top_atmosphere_boundary = -rmu -
		sqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);
	// If the viewer is in space and the view ray intersects the atmosphere, move
	// the viewer to the top atmosphere boundary (along the view ray):
	if (distance_to_top_atmosphere_boundary > 0.0 * m) {
		camera = camera + view_ray * distance_to_top_atmosphere_boundary;
		r = atmosphere.top_radius;
		rmu += distance_to_top_atmosphere_boundary;
	}
	else if (r > atmosphere.top_radius) {
		// If the view ray does not intersect the atmosphere, simply return 0.
		transmittance = DimensionlessSpectrum(1.0f, 1.0f, 1.0f);
		return RadianceSpectrum(0.0f, 0.0f, 0.0f) * watt_per_square_meter_per_sr_per_nm;
	}
	// Compute the r, mu, mu_s and nu parameters needed for the texture lookups.
	Number mu = rmu / r;
	Number mu_s = dot(camera, sun_direction) / r;
	Number nu = dot(view_ray, sun_direction);
	bool ray_r_mu_intersects_ground = RayIntersectsGround(atmosphere, r, mu);

	transmittance = ray_r_mu_intersects_ground ? DimensionlessSpectrum(0.0f, 0.0f, 0.0f) :
		GetTransmittanceToTopAtmosphereBoundary(
			atmosphere, atmoscons, atmossampler, transmittance_texture, r, mu);
	IrradianceSpectrum single_mie_scattering;
	IrradianceSpectrum scattering;
	if (shadow_length == 0.0 * m) {
		scattering = GetCombinedScattering(
			atmosphere, atmoscons, atmossampler, scattering_texture, 
			r, mu, mu_s, nu, ray_r_mu_intersects_ground,
			single_mie_scattering);
	}
	else {
		// Case of light shafts (shadow_length is the total length noted l in our
		// paper): we omit the scattering between the camera and the point at
		// distance l, by implementing Eq. (18) of the paper (shadow_transmittance
		// is the T(x,x_s) term, scattering is the S|x_s=x+lv term).
		Length d = shadow_length;
		Length r_p =
			ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
		Number mu_p = (r * mu + d) / r_p;
		Number mu_s_p = (r * mu_s + d * nu) / r_p;

		scattering = GetCombinedScattering(
			atmosphere, atmoscons, atmossampler, scattering_texture, 
			r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,
			single_mie_scattering);
		DimensionlessSpectrum shadow_transmittance =
			GetTransmittance(atmosphere, atmoscons, atmossampler, transmittance_texture,
				r, mu, shadow_length, ray_r_mu_intersects_ground);
		scattering = scattering * shadow_transmittance;
		single_mie_scattering = single_mie_scattering * shadow_transmittance;
	}
	return scattering * RayleighPhaseFunction(nu) + single_mie_scattering *
		MiePhaseFunction(atmosphere.mie_phase_function_g, nu);
}

// ------------------Aerial perspective Rendering------------------
RadianceSpectrum GetSkyRadianceToPoint(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	IN(CombinedScatteringTexture) scattering_texture,
	Position camera, IN(Position) _point, Length shadow_length,
	IN(Direction) sun_direction, OUT(DimensionlessSpectrum) transmittance) {
	// Compute the distance to the top atmosphere boundary along the view ray,
	// assuming the viewer is in space (or NaN if the view ray does not intersect
	// the atmosphere).
	Direction view_ray = normalize(_point - camera);
	Length r = length(camera);
	Length rmu = dot(camera, view_ray);
	Length distance_to_top_atmosphere_boundary = -rmu -
		sqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);
	// If the viewer is in space and the view ray intersects the atmosphere, move
	// the viewer to the top atmosphere boundary (along the view ray):
	if (distance_to_top_atmosphere_boundary > 0.0 * m) {
		camera = camera + view_ray * distance_to_top_atmosphere_boundary;
		r = atmosphere.top_radius;
		rmu += distance_to_top_atmosphere_boundary;
	}

	// Compute the r, mu, mu_s and nu parameters for the first texture lookup.
	Number mu = rmu / r;
	Number mu_s = dot(camera, sun_direction) / r;
	Number nu = dot(view_ray, sun_direction);
	Length d = length(_point - camera);
	bool ray_r_mu_intersects_ground = RayIntersectsGround(atmosphere, r, mu);

	transmittance = GetTransmittance(atmosphere, atmoscons, atmossampler, transmittance_texture,
		r, mu, d, ray_r_mu_intersects_ground);

	IrradianceSpectrum single_mie_scattering;
	IrradianceSpectrum scattering = GetCombinedScattering(
		atmosphere, atmoscons, atmossampler, scattering_texture, 
		r, mu, mu_s, nu, ray_r_mu_intersects_ground,
		single_mie_scattering);

	// Compute the r, mu, mu_s and nu parameters for the second texture lookup.
	// If shadow_length is not 0 (case of light shafts), we want to ignore the
	// scattering along the last shadow_length meters of the view ray, which we
	// do by subtracting shadow_length from d (this way scattering_p is equal to
	// the S|x_s=x_0-lv term in Eq. (17) of our paper).
	d = max(d - shadow_length, 0.0 * m);
	Length r_p = ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
	Number mu_p = (r * mu + d) / r_p;
	Number mu_s_p = (r * mu_s + d * nu) / r_p;

	IrradianceSpectrum single_mie_scattering_p;
	IrradianceSpectrum scattering_p = GetCombinedScattering(
		atmosphere, atmoscons, atmossampler, scattering_texture, 
		r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,
		single_mie_scattering_p);

	// Combine the lookup results to get the scattering between camera and point.
	DimensionlessSpectrum shadow_transmittance = transmittance;
	if (shadow_length > 0.0 * m) {
		// This is the T(x,x_s) term in Eq. (17) of our paper, for light shafts.
		shadow_transmittance = GetTransmittance(atmosphere, atmoscons, atmossampler, transmittance_texture,
			r, mu, d, ray_r_mu_intersects_ground);
	}
	scattering = scattering - shadow_transmittance * scattering_p;
	single_mie_scattering =
		single_mie_scattering - shadow_transmittance * single_mie_scattering_p;
	
	single_mie_scattering = GetExtrapolatedSingleMieScattering(
		atmosphere, float4(scattering, single_mie_scattering.r));

	// Hack to avoid rendering artifacts when the sun is below the horizon.
	single_mie_scattering = single_mie_scattering *
		smoothstep(Number(0.0), Number(0.01), mu_s);

	return scattering * RayleighPhaseFunction(nu) + single_mie_scattering *
		MiePhaseFunction(atmosphere.mie_phase_function_g, nu);
}

// ------------------Ground Rendering------------------
IrradianceSpectrum GetSunAndSkyIrradiance(
	IN(AtmosphereParameters) atmosphere,
	IN(AtmosphereConstants) atmoscons,
	IN(AtmosphereSampler) atmossampler,
	IN(TransmittanceTexture) transmittance_texture,
	IN(IrradianceTexture) irradiance_texture,
	IN(Position) _point, IN(Direction) normal, IN(Direction) sun_direction,
	OUT(IrradianceSpectrum) sky_irradiance) {
	Length r = length(_point);
	Number mu_s = dot(_point, sun_direction) / r;

	// Indirect irradiance (approximated if the surface is not horizontal).
	sky_irradiance = GetIrradiance(atmosphere, atmoscons, atmossampler, irradiance_texture, r, mu_s) *
		(1.0 + dot(normal, _point) / r) * 0.5;

	// Direct irradiance.
	return atmosphere.solar_irradiance *
		GetTransmittanceToSun(
			atmosphere, atmoscons, atmossampler, transmittance_texture, r, mu_s) *
		max(dot(normal, sun_direction), 0.0);
}

#endif
