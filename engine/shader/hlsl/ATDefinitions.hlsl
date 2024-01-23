#ifndef ATMOSPHERIC_SCATTERING_DEFINITION_INCLUDE
#define ATMOSPHERIC_SCATTERING_DEFINITION_INCLUDE

#include "Packages/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl"

#define Length float
#define Wavelength float
#define Angle float
#define SolidAngle float
#define Power float
#define LuminousPower float

#define Number float
#define InverseLength float
#define Area float
#define Volume float
#define NumberDensity float
#define Irradiance float
#define Radiance float
#define SpectralPower float
#define SpectralIrradiance float
#define SpectralRadiance float
#define SpectralRadianceDensity float
#define ScatteringCoefficient float
#define InverseSolidAngle float
#define LuminousIntensity float
#define Luminance float
#define Illuminance float

// A generic function from Wavelength to some other type.
#define AbstractSpectrum float3
// A function from Wavelength to Number.
#define DimensionlessSpectrum float3
// A function from Wavelength to SpectralPower.
#define PowerSpectrum float3
// A function from Wavelength to SpectralIrradiance.
#define IrradianceSpectrum float3
// A function from Wavelength to SpectralRadiance.
#define RadianceSpectrum float3
// A function from Wavelength to SpectralRadianceDensity.
#define RadianceDensitySpectrum float3
// A function from Wavelength to ScaterringCoefficient.
#define ScatteringSpectrum float3

// A position in 3D (3 length values).
#define Position float3
// A unit direction vector in 3D (3 unitless values).
#define Direction float3
// A vector of 3 luminance values.
#define Luminance3 float3
// A vector of 3 illuminance values.
#define Illuminance3 float3

#define TransmittanceTexture Texture2D
#define AbstractScatteringTexture Texture3D
#define ReducedScatteringTexture Texture3D
#define ScatteringTexture Texture3D
#define ScatteringDensityTexture Texture3D
#define IrradianceTexture Texture2D

#define IN(x) x
#define OUT(x) out x
#define TEMPLATE(x)
#define TEMPLATE_ARGUMENT(x)
#define assert(x) ;

static const Length m = 1.0;
static const Wavelength nm = 1.0;
static const Angle rad = 1.0;
static const SolidAngle sr = 1.0;
static const Power watt = 1.0;
static const LuminousPower lm = 1.0;

//static const float PI = 3.14159265358979323846f;
//#define PI 3.14159265358979323846f

static const Length km = 1000.0f * m;
static const Area m2 = m * m;
static const Volume m3 = m * m * m;
static const Angle pi = PI * rad;
static const Angle deg = pi / 180.0;
static const Irradiance watt_per_square_meter = watt / m2;
static const Radiance watt_per_square_meter_per_sr = watt / (m2 * sr);
static const SpectralIrradiance watt_per_square_meter_per_nm = watt / (m2 * nm);
static const SpectralRadiance watt_per_square_meter_per_sr_per_nm = watt / (m2 * sr * nm);
static const SpectralRadianceDensity watt_per_cubic_meter_per_sr_per_nm = watt / (m3 * sr * nm);
static const LuminousIntensity cd = lm / sr;
static const LuminousIntensity kcd = 1000.0 * cd;
static const Luminance cd_per_square_meter = cd / m2;
static const Luminance kcd_per_square_meter = kcd / m2;

// ----------------------------------------------------------------------------------
// Parameters

// An atmosphere layer of width 'width', and whose density is defined as
//   'exp_term' * exp('exp_scale' * h) + 'linear_term' * h + 'constant_term',
// clamped to [0,1], and where h is the altitude.
struct DensityProfileLayer {
	Length width;
	Number exp_term;
	InverseLength exp_scale;
	InverseLength linear_term;
	Number constant_term;
};

// An atmosphere density profile made of several layers on top of each other
// (from bottom to top). The width of the last layer is ignored, i.e. it always
// extend to the top atmosphere boundary. The profile values vary between 0
// (null density) to 1 (maximum density).
struct DensityProfile {
	DensityProfileLayer layers[2];
};

/*
The atmosphere parameters are then defined by the following struct:
*/

struct AtmosphereParameters {
	// The solar irradiance at the top of the atmosphere.
	IrradianceSpectrum solar_irradiance;
	// The sun's angular radius. Warning: the implementation uses approximations
	// that are valid only if this angle is smaller than 0.1 radians.
	Angle sun_angular_radius;
	// The distance between the planet center and the bottom of the atmosphere.
	Length bottom_radius;
	// The distance between the planet center and the top of the atmosphere.
	Length top_radius;
	// The density profile of air molecules, i.e. a function from altitude to
	// dimensionless values between 0 (null density) and 1 (maximum density).
	DensityProfile rayleigh_density;
	// The scattering coefficient of air molecules at the altitude where their
	// density is maximum (usually the bottom of the atmosphere), as a function of
	// wavelength. The scattering coefficient at altitude h is equal to
	// 'rayleigh_scattering' times 'rayleigh_density' at this altitude.
	ScatteringSpectrum rayleigh_scattering;
	// The density profile of aerosols, i.e. a function from altitude to
	// dimensionless values between 0 (null density) and 1 (maximum density).
	DensityProfile mie_density;
	// The scattering coefficient of aerosols at the altitude where their density
	// is maximum (usually the bottom of the atmosphere), as a function of
	// wavelength. The scattering coefficient at altitude h is equal to
	// 'mie_scattering' times 'mie_density' at this altitude.
	ScatteringSpectrum mie_scattering;
	// The extinction coefficient of aerosols at the altitude where their density
	// is maximum (usually the bottom of the atmosphere), as a function of
	// wavelength. The extinction coefficient at altitude h is equal to
	// 'mie_extinction' times 'mie_density' at this altitude.
	ScatteringSpectrum mie_extinction;
	// The asymetry parameter for the Cornette-Shanks phase function for the
	// aerosols.
	Number mie_phase_function_g;
	// The density profile of air molecules that absorb light (e.g. ozone), i.e.
	// a function from altitude to dimensionless values between 0 (null density)
	// and 1 (maximum density).
	DensityProfile absorption_density;
	// The extinction coefficient of molecules that absorb light (e.g. ozone) at
	// the altitude where their density is maximum, as a function of wavelength.
	// The extinction coefficient at altitude h is equal to
	// 'absorption_extinction' times 'absorption_density' at this altitude.
	ScatteringSpectrum absorption_extinction;
	// The average albedo of the ground.
	DimensionlessSpectrum ground_albedo;
	// The cosine of the maximum Sun zenith angle for which atmospheric scattering
	// must be precomputed (for maximum precision, use the smallest Sun zenith
	// angle yielding negligible sky light radiance values. For instance, for the
	// Earth case, 102 degrees is a good choice - yielding mu_s_min = -0.2).
	Number mu_s_min;
};

// ----------------------------------------------------------------------------------
// Samplers

SAMPLER(sampler_LinearClamp);
SAMPLER(sampler_LinearRepeat);
SAMPLER(sampler_PointClamp);
SAMPLER(sampler_PointRepeat);

// ----------------------------------------------------------------------------------
// Uniform Parameters

IrradianceSpectrum solar_irradiance;
Angle sun_angular_radius;
Length bottom_radius;
Length top_radius;

Length rayleigh_density_layers_0_width;
Number rayleigh_density_layers_0_exp_term;
InverseLength rayleigh_density_layers_0_exp_scale;
InverseLength rayleigh_density_layers_0_linear_term;
Number rayleigh_density_layers_0_constant_term;

Length rayleigh_density_layers_1_width;
Number rayleigh_density_layers_1_exp_term;
InverseLength rayleigh_density_layers_1_exp_scale;
InverseLength rayleigh_density_layers_1_linear_term;
Number rayleigh_density_layers_1_constant_term;

ScatteringSpectrum rayleigh_scattering;

Length mie_density_layers_0_width;
Number mie_density_layers_0_exp_term;
InverseLength mie_density_layers_0_exp_scale;
InverseLength mie_density_layers_0_linear_term;
Number mie_density_layers_0_constant_term;

Length mie_density_layers_1_width;
Number mie_density_layers_1_exp_term;
InverseLength mie_density_layers_1_exp_scale;
InverseLength mie_density_layers_1_linear_term;
Number mie_density_layers_1_constant_term;

ScatteringSpectrum mie_scattering;
ScatteringSpectrum mie_extinction;
Number mie_phase_function_g;

Length absorption_density_layers_0_width;
Number absorption_density_layers_0_exp_term;
InverseLength absorption_density_layers_0_exp_scale;
InverseLength absorption_density_layers_0_linear_term;
Number absorption_density_layers_0_constant_term;

Length absorption_density_layers_1_width;
Number absorption_density_layers_1_exp_term;
InverseLength absorption_density_layers_1_exp_scale;
InverseLength absorption_density_layers_1_linear_term;
Number absorption_density_layers_1_constant_term;

ScatteringSpectrum absorption_extinction;

DimensionlessSpectrum ground_albedo;

Number mu_s_min;

int TRANSMITTANCE_TEXTURE_WIDTH;
int TRANSMITTANCE_TEXTURE_HEIGHT;
int SCATTERING_TEXTURE_R_SIZE;
int SCATTERING_TEXTURE_MU_SIZE;
int SCATTERING_TEXTURE_MU_S_SIZE;
int SCATTERING_TEXTURE_NU_SIZE;
int SCATTERING_TEXTURE_WIDTH;
int SCATTERING_TEXTURE_HEIGHT;
int SCATTERING_TEXTURE_DEPTH;
int IRRADIANCE_TEXTURE_WIDTH;
int IRRADIANCE_TEXTURE_HEIGHT;
float3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
float3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;

AtmosphereParameters GetAtmosphereParameters() {
	//-------------------------rayleigh_density---------------------------------
	DensityProfileLayer rayleighProfileLayer0;
	rayleighProfileLayer0.width = rayleigh_density_layers_0_width;
	rayleighProfileLayer0.exp_term = rayleigh_density_layers_0_exp_term;
	rayleighProfileLayer0.exp_scale = rayleigh_density_layers_0_exp_scale;
	rayleighProfileLayer0.linear_term = rayleigh_density_layers_0_linear_term;
	rayleighProfileLayer0.constant_term = rayleigh_density_layers_0_constant_term;

	DensityProfileLayer rayleighProfileLayer1;
	rayleighProfileLayer1.width = rayleigh_density_layers_1_width;
	rayleighProfileLayer1.exp_term = rayleigh_density_layers_1_exp_term;
	rayleighProfileLayer1.exp_scale = rayleigh_density_layers_1_exp_scale;
	rayleighProfileLayer1.linear_term = rayleigh_density_layers_1_linear_term;
	rayleighProfileLayer1.constant_term = rayleigh_density_layers_1_constant_term;

	DensityProfile rayleigh_density;
	rayleigh_density.layers[0] = rayleighProfileLayer0;
	rayleigh_density.layers[1] = rayleighProfileLayer1;

	//-------------------------mie_density---------------------------------
	DensityProfileLayer absorptionProfileLayer0;
	absorptionProfileLayer0.width = absorption_density_layers_0_width;
	absorptionProfileLayer0.exp_term = absorption_density_layers_0_exp_term;
	absorptionProfileLayer0.exp_scale = absorption_density_layers_0_exp_scale;
	absorptionProfileLayer0.linear_term = absorption_density_layers_0_linear_term;
	absorptionProfileLayer0.constant_term = absorption_density_layers_0_constant_term;

	DensityProfileLayer absorptionProfileLayer1;
	absorptionProfileLayer1.width = absorption_density_layers_1_width;
	absorptionProfileLayer1.exp_term = absorption_density_layers_1_exp_term;
	absorptionProfileLayer1.exp_scale = absorption_density_layers_1_exp_scale;
	absorptionProfileLayer1.linear_term = absorption_density_layers_1_linear_term;
	absorptionProfileLayer1.constant_term = absorption_density_layers_1_constant_term;

	DensityProfile mie_density;
	mie_density.layers[0] = absorptionProfileLayer0;
	mie_density.layers[1] = absorptionProfileLayer1;

	//-------------------------absorption_density---------------------------------
	DensityProfileLayer mieProfileLayer0;
	mieProfileLayer0.width = mie_density_layers_0_width;
	mieProfileLayer0.exp_term = mie_density_layers_0_exp_term;
	mieProfileLayer0.exp_scale = mie_density_layers_0_exp_scale;
	mieProfileLayer0.linear_term = mie_density_layers_0_linear_term;
	mieProfileLayer0.constant_term = mie_density_layers_0_constant_term;

	DensityProfileLayer mieProfileLayer1;
	mieProfileLayer1.width = mie_density_layers_1_width;
	mieProfileLayer1.exp_term = mie_density_layers_1_exp_term;
	mieProfileLayer1.exp_scale = mie_density_layers_1_exp_scale;
	mieProfileLayer1.linear_term = mie_density_layers_1_linear_term;
	mieProfileLayer1.constant_term = mie_density_layers_1_constant_term;

	DensityProfile absorption_density;
	absorption_density.layers[0] = mieProfileLayer0;
	absorption_density.layers[1] = mieProfileLayer1;


	//-------------------------atmosphereParameters---------------------------------
	AtmosphereParameters atmosphereParameters = (AtmosphereParameters)0;
	atmosphereParameters.solar_irradiance = solar_irradiance;
	atmosphereParameters.sun_angular_radius = sun_angular_radius;
	atmosphereParameters.bottom_radius = bottom_radius;
	atmosphereParameters.top_radius = top_radius;
	atmosphereParameters.rayleigh_density = rayleigh_density;
	atmosphereParameters.rayleigh_scattering = rayleigh_scattering;
	atmosphereParameters.mie_density = mie_density;
	atmosphereParameters.mie_scattering = mie_scattering;
	atmosphereParameters.mie_extinction = mie_extinction;
	atmosphereParameters.mie_phase_function_g = mie_phase_function_g;
	atmosphereParameters.absorption_density = absorption_density;
	atmosphereParameters.absorption_extinction = absorption_extinction;
	atmosphereParameters.ground_albedo = ground_albedo;
	atmosphereParameters.mu_s_min = mu_s_min;

	return atmosphereParameters;
}

#endif
