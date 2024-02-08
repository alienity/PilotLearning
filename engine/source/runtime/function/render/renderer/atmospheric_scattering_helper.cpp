#include "atmospheric_scattering_helper.h"

namespace MoYu
{
 	namespace AtmosphereScattering
	{
        //constexpr int kLambdaMin = 360;
        //constexpr int kLambdaMax = 830;

        double CieColorMatchingFunctionTableValue(double wavelength, int column, double kLambdaMin, double kLambdaMax)
        {
            if (wavelength <= kLambdaMin || wavelength >= kLambdaMax)
            {
                return 0.0;
            }
            double u   = (wavelength - kLambdaMin) / 5.0;
            int    row = static_cast<int>(std::floor(u));
            assert(row >= 0 && row + 1 < 95);
            assert(CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength &&
                   CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
            u -= row;
            return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) +
                   CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
        }

        double Interpolate(const std::vector<double>& wavelengths,
                           const std::vector<double>& wavelength_function,
                           double                     wavelength)
        {
            assert(wavelength_function.size() == wavelengths.size());
            if (wavelength < wavelengths[0])
            {
                return wavelength_function[0];
            }
            for (unsigned int i = 0; i < wavelengths.size() - 1; ++i)
            {
                if (wavelength < wavelengths[i + 1])
                {
                    double u = (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
                    return wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
                }
            }
            return wavelength_function[wavelength_function.size() - 1];
        }

        /*
        <p>We can then implement a utility function to compute the "spectral radiance to
        luminance" conversion constants (see Section 14.3 in <a
        href="https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
        Evaluation of 8 Clear Sky Models</a> for their definitions):
        */

        // The returned constants are in lumen.nm / watt.
        void ComputeSpectralRadianceToLuminanceFactors(const std::vector<double>& wavelengths,
                                                       const std::vector<double>& solar_irradiance,
                                                       double                     kLambdaMin,
                                                       double                     kLambdaMax,
                                                       double                     lambda_power,
                                                       double*                    k_r,
                                                       double*                    k_g,
                                                       double*                    k_b)
        {
            *k_r           = 0.0;
            *k_g           = 0.0;
            *k_b           = 0.0;
            double solar_r = Interpolate(wavelengths, solar_irradiance, kLambdaR);
            double solar_g = Interpolate(wavelengths, solar_irradiance, kLambdaG);
            double solar_b = Interpolate(wavelengths, solar_irradiance, kLambdaB);
            int    dlambda = 1;
            for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda)
            {
                double        x_bar      = CieColorMatchingFunctionTableValue(lambda, 1, kLambdaMin, kLambdaMax);
                double        y_bar      = CieColorMatchingFunctionTableValue(lambda, 2, kLambdaMin, kLambdaMax);
                double        z_bar      = CieColorMatchingFunctionTableValue(lambda, 3, kLambdaMin, kLambdaMax);
                const double* xyz2srgb   = XYZ_TO_SRGB;
                double        r_bar      = xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
                double        g_bar      = xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
                double        b_bar      = xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
                double        irradiance = Interpolate(wavelengths, solar_irradiance, lambda);
                *k_r += r_bar * irradiance / solar_r * pow(lambda / kLambdaR, lambda_power);
                *k_g += g_bar * irradiance / solar_g * pow(lambda / kLambdaG, lambda_power);
                *k_b += b_bar * irradiance / solar_b * pow(lambda / kLambdaB, lambda_power);
            }
            *k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
            *k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
            *k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
        }

        AtmosphereParameters AtmosphericScatteringGenerator::Init()
        {
            double kSunAngularRadius   = 0.00935 / 2.0;
            double kSunSolidAngle      = 3.1415926 * kSunAngularRadius * kSunAngularRadius;
            double kLengthUnitInMeters = 1000.0;

            bool use_constant_solar_spectrum_ = false;
            bool use_ozone_                   = true;

            int kLambdaMin = 360;
            int kLambdaMax = 830;
            double kSolarIrradiance[48] {
                1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.6887, 1.61253,
                1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.8298,
                1.8685, 1.8931, 1.85149, 1.8504, 1.8341, 1.8345, 1.8147, 1.78158, 1.7533,
                1.6965, 1.68194, 1.64654, 1.6048, 1.52143, 1.55622, 1.5113, 1.474, 1.4482,
                1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.2367, 1.2082,
                1.18737, 1.14683, 1.12362, 1.1058, 1.07124, 1.04992
            };
            double kOzoneCrossSection[48] {
                1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
                8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
                1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
                4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
                2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
                6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
                2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
            };

            double kDobsonUnit                = 2.687e20;
            double kMaxOzoneNumberDensity     = 300.0 * kDobsonUnit / 15000.0;
            double kConstantSolarIrradiance   = 1.5;
            double kBottomRadius              = 6360000.0;
            double kTopRadius                 = 6420000.0;
            double kRayleigh                  = 1.24062e-6;
            double kRayleighScaleHeight       = 8000.0;
            double kMieScaleHeight            = 1200.0;
            double kMieAngstromAlpha          = 0.0;
            double kMieAngstromBeta           = 5.328e-3;
            double kMieSingleScatteringAlbedo = 0.9;
            double kMiePhaseFunctionG         = 0.8;
            double kGroundAlbedo              = 0.1;
            double max_sun_zenith_angle       = 120.0 / 180.0 * 3.1415926;

            std::vector<DensityProfileLayer> rayleigh_layer = {
                DensityProfileLayer {0.0, 0.0, 0.0, 0.0, 0.0},
                DensityProfileLayer {0.0, 1.0, -1.0 / kRayleighScaleHeight, 0.0, 0.0}};
            std::vector<DensityProfileLayer> mie_layer = {
                DensityProfileLayer {0.0, 0.0, 0.0, 0.0, 0.0},
                DensityProfileLayer {0.0, 1.0, -1.0 / kMieScaleHeight, 0.0, 0.0}};
            std::vector<DensityProfileLayer> ozone_density = {
                DensityProfileLayer {25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0},
                DensityProfileLayer {0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0}};

            std::vector<double> wavelengths           = {};
            std::vector<double> solar_irradiance      = {};
            std::vector<double> rayleigh_scattering   = {};
            std::vector<double> mie_scattering        = {};
            std::vector<double> mie_extinction        = {};
            std::vector<double> absorption_extinction = {};
            std::vector<double> ground_albedo         = {};

            for (int l = kLambdaMin; l <= kLambdaMax; l += 10)
            {
                double lambda = (double)(l)*1e-3; // micro-meters
                double mie = kMieAngstromBeta / kMieScaleHeight * glm::pow((float)lambda, -(float)kMieAngstromAlpha);
                wavelengths.push_back(l);
                if (use_constant_solar_spectrum_)
                {
                    solar_irradiance.push_back(kConstantSolarIrradiance);
                }
                else
                {
                    solar_irradiance.push_back(kSolarIrradiance[(l - kLambdaMin) / 10]);
                }
                rayleigh_scattering.push_back(kRayleigh * glm::pow((float)lambda, -4));
                mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
                mie_extinction.push_back(mie);
                absorption_extinction.push_back(use_ozone_ ? kMaxOzoneNumberDensity * kOzoneCrossSection[(l - kLambdaMin) / 10] : 0.0);
                ground_albedo.push_back(kGroundAlbedo);
            }

            AtmosphereParameters _atmosphereParameters = InitAtmosphereParameters(wavelengths,
                                                                                  solar_irradiance,
                                                                                  kLambdaMin,
                                                                                  kLambdaMax,
                                                                                  kSunAngularRadius,
                                                                                  kBottomRadius,
                                                                                  kTopRadius,
                                                                                  rayleigh_layer,
                                                                                  rayleigh_scattering,
                                                                                  mie_layer,
                                                                                  mie_scattering,
                                                                                  mie_extinction,
                                                                                  kMiePhaseFunctionG,
                                                                                  ozone_density,
                                                                                  absorption_extinction,
                                                                                  ground_albedo,
                                                                                  max_sun_zenith_angle,
                                                                                  kLengthUnitInMeters);

            return _atmosphereParameters;
        }

        AtmosphereParameters
        AtmosphericScatteringGenerator::InitAtmosphereParameters(std::vector<double>              wavelengths,
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
                                                                 double                           length_unit_in_meters)
        {
            double sky_k_r = 0.0, sky_k_g = 0.0, sky_k_b = 0.0;
            ComputeSpectralRadianceToLuminanceFactors(wavelengths, solar_irradiance, kLambdaMin, kLambdaMax,
                    -3 /* lambda_power */, &sky_k_r, &sky_k_g, &sky_k_b);

            // Compute the values for the SUN_RADIANCE_TO_LUMINANCE constant.
            double sun_k_r = 0.0, sun_k_g = 0.0, sun_k_b = 0.0;
            ComputeSpectralRadianceToLuminanceFactors(wavelengths, solar_irradiance, kLambdaMin, kLambdaMax,
                0 /* lambda_power */, &sun_k_r, &sun_k_g, &sun_k_b);

            glm::float3 kLambdas = glm::float3((float)kLambdaR, (float)kLambdaG, (float)kLambdaB);

            AtmosphereParameters atmosphereParameters = {};

            atmosphereParameters.sun_angular_radius = sun_angular_radius;
            atmosphereParameters.bottom_radius      = bottom_radius / length_unit_in_meters;
            atmosphereParameters.top_radius         = top_radius / length_unit_in_meters;
            atmosphereParameters.rayleigh_density =
                DensityProfile {ToDensityLayer(length_unit_in_meters, rayleigh_density[0]),
                                ToDensityLayer(length_unit_in_meters, rayleigh_density[1])};
            atmosphereParameters.mie_density = DensityProfile {ToDensityLayer(length_unit_in_meters, mie_density[0]),
                                                               ToDensityLayer(length_unit_in_meters, mie_density[1])};
            atmosphereParameters.mie_phase_function_g = mie_phase_function_g;
            atmosphereParameters.absorption_density =
                DensityProfile {ToDensityLayer(length_unit_in_meters, absorption_density[0]),
                                ToDensityLayer(length_unit_in_meters, absorption_density[1])};
            atmosphereParameters.mu_s_min         = glm::cos((float)max_sun_zenith_angle);
            atmosphereParameters.solar_irradiance = ToVector3(wavelengths, solar_irradiance, kLambdas, 1.0);
            atmosphereParameters.rayleigh_scattering =
                ToVector3(wavelengths, rayleigh_scattering, kLambdas, length_unit_in_meters);
            atmosphereParameters.mie_scattering =
                ToVector3(wavelengths, mie_scattering, kLambdas, length_unit_in_meters);
            atmosphereParameters.mie_extinction =
                ToVector3(wavelengths, mie_extinction, kLambdas, length_unit_in_meters);
            atmosphereParameters.absorption_extinction =
                ToVector3(wavelengths, absorption_extinction, kLambdas, length_unit_in_meters);
            atmosphereParameters.ground_albedo = ToVector3(wavelengths, ground_albedo, kLambdas, 1.0);

            return atmosphereParameters;
        }



        glm::float3 AtmosphericScatteringGenerator::ToVector3(std::vector<double> wavelengths, std::vector<double> v, glm::float3 lambdas, double scale)
        {
            float r = (float)(Interpolate(wavelengths, v, lambdas[0]) * scale);
            float g = (float)(Interpolate(wavelengths, v, lambdas[1]) * scale);
            float b = (float)(Interpolate(wavelengths, v, lambdas[2]) * scale);
            return glm::float3(r, g, b);
        }

        DensityProfileLayer AtmosphericScatteringGenerator::ToDensityLayer(double length_unit_in_meters, DensityProfileLayer layer)
        {
            DensityProfileLayer layer = {layer.width / length_unit_in_meters,
                                         layer.exp_term,
                                         layer.exp_scale * length_unit_in_meters,
                                         layer.linear_term * length_unit_in_meters,
                                         layer.constant_term};
            return layer;
        }



	}
	
}
