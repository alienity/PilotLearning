#pragma once

//------------------------------------------------------------------------------
// Common color operations
//------------------------------------------------------------------------------

/**
 * Computes the luminance of the specified linear RGB color using the
 * luminance coefficients from Rec. 709.
 *
 * @public-api
 */
float luminance(const float3 linear) { return dot(linear, float3(0.2126, 0.7152, 0.0722)); }

/**
 * Computes the pre-exposed intensity using the specified intensity and exposure.
 * This function exists to force highp precision on the two parameters
 */
float computePreExposedIntensity(const float intensity, const float exposure)
{
    return intensity * exposure;
}

void unpremultiply(inout float4 color) { color.rgb /= max(color.a, FLT_EPS); }

/**
 * Applies a full range YCbCr to sRGB conversion and returns an RGB color.
 *
 * @public-api
 */
float3 ycbcrToRgb(float luminance, float2 cbcr)
{
    // Taken from https://developer.apple.com/documentation/arkit/arframe/2867984-capturedimage
    const float4x4 ycbcrToRgbTransform = float4x4(1.0000,
                                                  1.0000,
                                                  1.0000,
                                                  0.0000,
                                                  0.0000,
                                                  -0.3441,
                                                  1.7720,
                                                  0.0000,
                                                  1.4020,
                                                  -0.7141,
                                                  0.0000,
                                                  0.0000,
                                                  -0.7010,
                                                  0.5291,
                                                  -0.8860,
                                                  1.0000);
    return (ycbcrToRgbTransform * float4(luminance, cbcr, 1.0)).rgb;
}

//------------------------------------------------------------------------------
// Tone mapping operations
//------------------------------------------------------------------------------

/*
 * The input must be in the [0, 1] range.
 */
float3 Inverse_Tonemap_Filmic(const float3 x)
{
    return (0.03 - 0.59 * x - sqrt(0.0009 + 1.3702 * x - 1.0127 * x * x)) / (-5.02 + 4.86 * x);
}

/**
 * Applies the inverse of the tone mapping operator to the specified HDR or LDR
 * sRGB (non-linear) color and returns a linear sRGB color. The inverse tone mapping
 * operator may be an approximation of the real inverse operation.
 *
 * @public-api
 */
float3 inverseTonemapSRGB(float3 color)
{
    // sRGB input
    color = clamp(color, 0.0, 1.0);
    return Inverse_Tonemap_Filmic(pow(color, float3(2.2)));
}

/**
 * Applies the inverse of the tone mapping operator to the specified HDR or LDR
 * linear RGB color and returns a linear RGB color. The inverse tone mapping operator
 * may be an approximation of the real inverse operation.
 *
 * @public-api
 */
float3 inverseTonemap(float3 linear)
{
    // Linear input
    return Inverse_Tonemap_Filmic(clamp(linear, 0.0, 1.0));
}

//------------------------------------------------------------------------------
// Common texture operations
//------------------------------------------------------------------------------

/**
 * Decodes the specified RGBM value to linear HDR RGB.
 */
float3 decodeRGBM(float4 c)
{
    c.rgb *= (c.a * 16.0);
    return c.rgb * c.rgb;
}

//------------------------------------------------------------------------------
// Common screen-space operations
//------------------------------------------------------------------------------

// returns the frag coord in the GL convention with (0, 0) at the bottom-left
// resolution : width, height
float2 getFragCoord(const float2 resolution) { return SV_Position.xy; }

//------------------------------------------------------------------------------
// Common debug
//------------------------------------------------------------------------------

float3 heatmap(float v)
{
    float3 r = v * 2.1 - float3(1.8, 1.14, 0.3);
    return 1.0 - r * r;
}
