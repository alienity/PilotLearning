#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    namespace AtmosphereScattering
    {
#define Length float
#define Wavelength float
#define Angle float
#define SolidAngle float
#define Power float
#define LuminousPower float

#define Number float
#define InverseLength float
//#define Area float
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
#define AbstractSpectrum glm::float3
// A function from Wavelength to Number.
#define DimensionlessSpectrum glm::float3
// A function from Wavelength to SpectralPower.
#define PowerSpectrum glm::float3
// A function from Wavelength to SpectralIrradiance.
#define IrradianceSpectrum glm::float3
// A function from Wavelength to SpectralRadiance.
#define RadianceSpectrum glm::float3
// A function from Wavelength to SpectralRadianceDensity.
#define RadianceDensitySpectrum glm::float3
// A function from Wavelength to ScaterringCoefficient.
#define ScatteringSpectrum glm::float3

// A position in 3D (3 length values).
#define Position glm::float3
// A unit direction vector in 3D (3 unitless values).
#define Direction glm::float3
// A vector of 3 luminance values.
#define Luminance3 glm::float3
// A vector of 3 illuminance values.
#define Illuminance3 glm::float3

        constexpr int TRANSMITTANCE_TEXTURE_WIDTH  = 256;
        constexpr int TRANSMITTANCE_TEXTURE_HEIGHT = 64;

        constexpr int SCATTERING_TEXTURE_R_SIZE    = 32;
        constexpr int SCATTERING_TEXTURE_MU_SIZE   = 128;
        constexpr int SCATTERING_TEXTURE_MU_S_SIZE = 32;
        constexpr int SCATTERING_TEXTURE_NU_SIZE   = 8;

        constexpr int SCATTERING_TEXTURE_WIDTH  = SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
        constexpr int SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
        constexpr int SCATTERING_TEXTURE_DEPTH  = SCATTERING_TEXTURE_R_SIZE;

        constexpr int IRRADIANCE_TEXTURE_WIDTH  = 64;
        constexpr int IRRADIANCE_TEXTURE_HEIGHT = 16;

        // The conversion factor between watts and lumens.
        constexpr double MAX_LUMINOUS_EFFICACY = 683.0;
        
        // Values from "CIE (1931) 2-deg color matching functions", see
        // "http://web.archive.org/web/20081228084047/
        //    http://www.cvrl.org/database/data/cmfs/ciexyz31.txt".
        
        constexpr double CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[380] = {
          360, 0.000129900000, 0.000003917000, 0.000606100000,
          365, 0.000232100000, 0.000006965000, 0.001086000000,
          370, 0.000414900000, 0.000012390000, 0.001946000000,
          375, 0.000741600000, 0.000022020000, 0.003486000000,
          380, 0.001368000000, 0.000039000000, 0.006450001000,
          385, 0.002236000000, 0.000064000000, 0.010549990000,
          390, 0.004243000000, 0.000120000000, 0.020050010000,
          395, 0.007650000000, 0.000217000000, 0.036210000000,
          400, 0.014310000000, 0.000396000000, 0.067850010000,
          405, 0.023190000000, 0.000640000000, 0.110200000000,
          410, 0.043510000000, 0.001210000000, 0.207400000000,
          415, 0.077630000000, 0.002180000000, 0.371300000000,
          420, 0.134380000000, 0.004000000000, 0.645600000000,
          425, 0.214770000000, 0.007300000000, 1.039050100000,
          430, 0.283900000000, 0.011600000000, 1.385600000000,
          435, 0.328500000000, 0.016840000000, 1.622960000000,
          440, 0.348280000000, 0.023000000000, 1.747060000000,
          445, 0.348060000000, 0.029800000000, 1.782600000000,
          450, 0.336200000000, 0.038000000000, 1.772110000000,
          455, 0.318700000000, 0.048000000000, 1.744100000000,
          460, 0.290800000000, 0.060000000000, 1.669200000000,
          465, 0.251100000000, 0.073900000000, 1.528100000000,
          470, 0.195360000000, 0.090980000000, 1.287640000000,
          475, 0.142100000000, 0.112600000000, 1.041900000000,
          480, 0.095640000000, 0.139020000000, 0.812950100000,
          485, 0.057950010000, 0.169300000000, 0.616200000000,
          490, 0.032010000000, 0.208020000000, 0.465180000000,
          495, 0.014700000000, 0.258600000000, 0.353300000000,
          500, 0.004900000000, 0.323000000000, 0.272000000000,
          505, 0.002400000000, 0.407300000000, 0.212300000000,
          510, 0.009300000000, 0.503000000000, 0.158200000000,
          515, 0.029100000000, 0.608200000000, 0.111700000000,
          520, 0.063270000000, 0.710000000000, 0.078249990000,
          525, 0.109600000000, 0.793200000000, 0.057250010000,
          530, 0.165500000000, 0.862000000000, 0.042160000000,
          535, 0.225749900000, 0.914850100000, 0.029840000000,
          540, 0.290400000000, 0.954000000000, 0.020300000000,
          545, 0.359700000000, 0.980300000000, 0.013400000000,
          550, 0.433449900000, 0.994950100000, 0.008749999000,
          555, 0.512050100000, 1.000000000000, 0.005749999000,
          560, 0.594500000000, 0.995000000000, 0.003900000000,
          565, 0.678400000000, 0.978600000000, 0.002749999000,
          570, 0.762100000000, 0.952000000000, 0.002100000000,
          575, 0.842500000000, 0.915400000000, 0.001800000000,
          580, 0.916300000000, 0.870000000000, 0.001650001000,
          585, 0.978600000000, 0.816300000000, 0.001400000000,
          590, 1.026300000000, 0.757000000000, 0.001100000000,
          595, 1.056700000000, 0.694900000000, 0.001000000000,
          600, 1.062200000000, 0.631000000000, 0.000800000000,
          605, 1.045600000000, 0.566800000000, 0.000600000000,
          610, 1.002600000000, 0.503000000000, 0.000340000000,
          615, 0.938400000000, 0.441200000000, 0.000240000000,
          620, 0.854449900000, 0.381000000000, 0.000190000000,
          625, 0.751400000000, 0.321000000000, 0.000100000000,
          630, 0.642400000000, 0.265000000000, 0.000049999990,
          635, 0.541900000000, 0.217000000000, 0.000030000000,
          640, 0.447900000000, 0.175000000000, 0.000020000000,
          645, 0.360800000000, 0.138200000000, 0.000010000000,
          650, 0.283500000000, 0.107000000000, 0.000000000000,
          655, 0.218700000000, 0.081600000000, 0.000000000000,
          660, 0.164900000000, 0.061000000000, 0.000000000000,
          665, 0.121200000000, 0.044580000000, 0.000000000000,
          670, 0.087400000000, 0.032000000000, 0.000000000000,
          675, 0.063600000000, 0.023200000000, 0.000000000000,
          680, 0.046770000000, 0.017000000000, 0.000000000000,
          685, 0.032900000000, 0.011920000000, 0.000000000000,
          690, 0.022700000000, 0.008210000000, 0.000000000000,
          695, 0.015840000000, 0.005723000000, 0.000000000000,
          700, 0.011359160000, 0.004102000000, 0.000000000000,
          705, 0.008110916000, 0.002929000000, 0.000000000000,
          710, 0.005790346000, 0.002091000000, 0.000000000000,
          715, 0.004109457000, 0.001484000000, 0.000000000000,
          720, 0.002899327000, 0.001047000000, 0.000000000000,
          725, 0.002049190000, 0.000740000000, 0.000000000000,
          730, 0.001439971000, 0.000520000000, 0.000000000000,
          735, 0.000999949300, 0.000361100000, 0.000000000000,
          740, 0.000690078600, 0.000249200000, 0.000000000000,
          745, 0.000476021300, 0.000171900000, 0.000000000000,
          750, 0.000332301100, 0.000120000000, 0.000000000000,
          755, 0.000234826100, 0.000084800000, 0.000000000000,
          760, 0.000166150500, 0.000060000000, 0.000000000000,
          765, 0.000117413000, 0.000042400000, 0.000000000000,
          770, 0.000083075270, 0.000030000000, 0.000000000000,
          775, 0.000058706520, 0.000021200000, 0.000000000000,
          780, 0.000041509940, 0.000014990000, 0.000000000000,
          785, 0.000029353260, 0.000010600000, 0.000000000000,
          790, 0.000020673830, 0.000007465700, 0.000000000000,
          795, 0.000014559770, 0.000005257800, 0.000000000000,
          800, 0.000010253980, 0.000003702900, 0.000000000000,
          805, 0.000007221456, 0.000002607800, 0.000000000000,
          810, 0.000005085868, 0.000001836600, 0.000000000000,
          815, 0.000003581652, 0.000001293400, 0.000000000000,
          820, 0.000002522525, 0.000000910930, 0.000000000000,
          825, 0.000001776509, 0.000000641530, 0.000000000000,
          830, 0.000001251141, 0.000000451810, 0.000000000000,
        };

        // The conversion matrix from XYZ to linear sRGB color spaces.
        // Values from https://en.wikipedia.org/wiki/SRGB.
        constexpr double XYZ_TO_SRGB[9] = {
          +3.2406, -1.5372, -0.4986,
          -0.9689, +1.8758, +0.0415,
          +0.0557, -0.2040, +1.0570
        };

        constexpr double kLambdaR = 680.0;
        constexpr double kLambdaG = 550.0;
        constexpr double kLambdaB = 440.0;

        //=============================================================================

        // An atmosphere layer of width 'width' (in m), and whose density is defined as
        //   'exp_term' * exp('exp_scale' * h) + 'linear_term' * h + 'constant_term',
        // clamped to [0,1], and where h is the altitude (in m). 'exp_term' and
        // 'constant_term' are unitless, while 'exp_scale' and 'linear_term' are in
        // m^-1.
        struct DensityProfileLayer
        {
            double width;
            double exp_term;
            double exp_scale;
            double linear_term;
            double constant_term;
        };

        struct DensityProfile
        {
            DensityProfileLayer layers0;
            DensityProfileLayer layers1;
        };

        struct AtmosphereParameters
        {
            IrradianceSpectrum    solar_irradiance;
            Angle                 sun_angular_radius;
            Length                bottom_radius;
            Length                top_radius;
            DensityProfile        rayleigh_density;
            ScatteringSpectrum    rayleigh_scattering;
            DensityProfile        mie_density;
            ScatteringSpectrum    mie_scattering;
            ScatteringSpectrum    mie_extinction;
            Number                mie_phase_function_g;
            DensityProfile        absorption_density;
            ScatteringSpectrum    absorption_extinction;
            DimensionlessSpectrum ground_albedo;
            Number                mu_s_min;
        };

        struct AtmosphereUniform
        {
            IrradianceSpectrum solar_irradiance;
            Angle              sun_angular_radius;

            Length bottom_radius;
            Length top_radius;

            Length        rayleigh_density_layers_0_width;
            Number        rayleigh_density_layers_0_exp_term;
            InverseLength rayleigh_density_layers_0_exp_scale;
            InverseLength rayleigh_density_layers_0_linear_term;
            Number        rayleigh_density_layers_0_constant_term;

            Length        rayleigh_density_layers_1_width;
            Number        rayleigh_density_layers_1_exp_term;
            InverseLength rayleigh_density_layers_1_exp_scale;
            InverseLength rayleigh_density_layers_1_linear_term;
            Number        rayleigh_density_layers_1_constant_term;

            ScatteringSpectrum rayleigh_scattering;
            float              _padding0;

            float _padding1;
            float _padding2;

            Length        mie_density_layers_0_width;
            Number        mie_density_layers_0_exp_term;
            InverseLength mie_density_layers_0_exp_scale;
            InverseLength mie_density_layers_0_linear_term;
            Number        mie_density_layers_0_constant_term;

            Length        mie_density_layers_1_width;
            Number        mie_density_layers_1_exp_term;
            InverseLength mie_density_layers_1_exp_scale;
            InverseLength mie_density_layers_1_linear_term;
            Number        mie_density_layers_1_constant_term;

            ScatteringSpectrum mie_scattering;
            float              _padding3;
            ScatteringSpectrum mie_extinction;
            Number             mie_phase_function_g;

            float _padding5;
            float _padding6;

            Length        absorption_density_layers_0_width;
            Number        absorption_density_layers_0_exp_term;
            InverseLength absorption_density_layers_0_exp_scale;
            InverseLength absorption_density_layers_0_linear_term;
            Number        absorption_density_layers_0_constant_term;

            Length        absorption_density_layers_1_width;
            Number        absorption_density_layers_1_exp_term;
            InverseLength absorption_density_layers_1_exp_scale;
            InverseLength absorption_density_layers_1_linear_term;
            Number        absorption_density_layers_1_constant_term;

            ScatteringSpectrum absorption_extinction;
            float              _padding7;

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
            int _padding8;

            glm::float3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
            float       _padding9;
            glm::float3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
            float       _padding10;
        };

        class AtmosphericScatteringGenerator
        {
        public:
            static AtmosphereUniform Init();

            static AtmosphereUniform InitAtmosphereUniform(std::vector<double>              wavelengths,
                                                           std::vector<double>              solar_irradiance,
                                                           double                           kLambdaMin,
                                                           double                           kLambdaMax,
                                                           double                           sun_angular_radius,
                                                           double                           bottom_radius,
                                                           double                           top_radius,
                                                           std::vector<DensityProfileLayer> rayleigh_density,
                                                           std::vector<double>              rayleigh_scattering,
                                                           std::vector<DensityProfileLayer> mie_density,
                                                           std::vector<double>              mie_scattering,
                                                           std::vector<double>              mie_extinction,
                                                           double                           mie_phase_function_g,
                                                           std::vector<DensityProfileLayer> absorption_density,
                                                           std::vector<double>              absorption_extinction,
                                                           std::vector<double>              ground_albedo,
                                                           double                           max_sun_zenith_angle,
                                                           double                           length_unit_in_meters);

        private:
            static glm::float3 ToVector3(std::vector<double> wavelengths, std::vector<double> v, glm::float3 lambdas, double scale);
            static DensityProfileLayer ToDensityLayer(double length_unit_in_meters, DensityProfileLayer layer);
        };

    }

}

