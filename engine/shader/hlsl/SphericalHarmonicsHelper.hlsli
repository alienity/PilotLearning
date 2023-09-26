#include "CommonMath.hlsli"
#include "ColorSpaceUtility.hlsli"


//*****************************************************************
// Spherical Harmonics
//*****************************************************************

// https://www.ppsloan.org/publications/StupidSH36.pdf

// SH Basis coefs
#define kSHBasis0  0.28209479177387814347f // {0, 0} : 1/2 * sqrt(1/Pi)
#define kSHBasis1  0.48860251190291992159f // {1, 0} : 1/2 * sqrt(3/Pi)
#define kSHBasis2  1.09254843059207907054f // {2,-2} : 1/2 * sqrt(15/Pi)
#define kSHBasis3  0.31539156525252000603f // {2, 0} : 1/4 * sqrt(5/Pi)
#define kSHBasis4  0.54627421529603953527f // {2, 2} : 1/4 * sqrt(15/Pi)

static const float kSHBasisCoef[] = { kSHBasis0, -kSHBasis1, kSHBasis1, -kSHBasis1, kSHBasis2, -kSHBasis2, kSHBasis3, -kSHBasis2, kSHBasis4 };

// Clamped cosine convolution coefs (pre-divided by PI)
// See https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
#define kClampedCosine0 1.0f
#define kClampedCosine1 2.0f / 3.0f
#define kClampedCosine2 1.0f / 4.0f

static const float kClampedCosineCoefs[] = { kClampedCosine0, kClampedCosine1, kClampedCosine1, kClampedCosine1, kClampedCosine2, kClampedCosine2, kClampedCosine2, kClampedCosine2, kClampedCosine2 };

// Ref: "Efficient Evaluation of Irradiance Environment Maps" from ShaderX 2
float3 SHEvalLinearL0L1(float3 N, float4 shAr, float4 shAg, float4 shAb)
{
    float4 vA = float4(N, 1.0);

    float3 x1;
    // Linear (L1) + constant (L0) polynomial terms
    x1.r = dot(shAr, vA);
    x1.g = dot(shAg, vA);
    x1.b = dot(shAb, vA);

    return x1;
}

float3 SHEvalLinearL1(float3 N, float3 shAr, float3 shAg, float3 shAb)
{
    float3 x1;
    x1.r = dot(shAr, N);
    x1.g = dot(shAg, N);
    x1.b = dot(shAb, N);

    return x1;
}

float3 SHEvalLinearL2(float3 N, float4 shBr, float4 shBg, float4 shBb, float4 shC)
{
    float3 x2;
    // 4 of the quadratic (L2) polynomials
    float4 vB = N.xyzz * N.yzzx;
    x2.r = dot(shBr, vB);
    x2.g = dot(shBg, vB);
    x2.b = dot(shBb, vB);

    // Final (5th) quadratic (L2) polynomial
    float vC = N.x * N.x - N.y * N.y;
    float3 x3 = shC.rgb * vC;

    return x2 + x3;
}

float3 SampleSH9(float4 SHCoefficients[7], float3 N)
{
    float4 shAr = SHCoefficients[0];
    float4 shAg = SHCoefficients[1];
    float4 shAb = SHCoefficients[2];
    float4 shBr = SHCoefficients[3];
    float4 shBg = SHCoefficients[4];
    float4 shBb = SHCoefficients[5];
    float4 shCr = SHCoefficients[6];

    // Linear + constant polynomial terms
    float3 res = SHEvalLinearL0L1(N, shAr, shAg, shAb);

    // Quadratic polynomials
    res += SHEvalLinearL2(N, shBr, shBg, shBb, shCr);

    return res;
}


/*
 * returns n! / d!
 */
uint SHindex(uint m, uint l)
{
    return l * (l + 1) + m;
}

/*
 * Calculates non-normalized SH bases, i.e.:
 *  m > 0, cos(m*phi)   * P(m,l)
 *  m < 0, sin(|m|*phi) * P(|m|,l)
 *  m = 0, P(0,l)
 */
void computeShBasisBand2(out float SHb[9], const float3 s)
{
    /*
     * TODO: all the Legendre computation below is identical for all faces, so it
     * might make sense to pre-compute it once. Also note that there is
     * a fair amount of symmetry within a face (which we could take advantage of
     * to reduce the pre-compute table).
     */

    /*
     * Below, we compute the associated Legendre polynomials using recursion.
     * see: http://mathworld.wolfram.com/AssociatedLegendrePolynomial.html
     *
     * Note [0]: s.z == cos(theta) ==> we only need to compute P(s.z)
     *
     * Note [1]: We in fact compute P(s.z) / sin(theta)^|m|, by removing
     * the "sqrt(1 - s.z*s.z)" [i.e.: sin(theta)] factor from the recursion.
     * This is later corrected in the ( cos(m*phi), sin(m*phi) ) recursion.
     */

    // s = (x, y, z) = (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta))

    static const uint numBands = 3;

    // handle m=0 separately, since it produces only one coefficient
    float Pml_2 = 0;
    float Pml_1 = 1;
    SHb[0] =  Pml_1;
    for (uint l=1; l<numBands; l++) {
        float Pml = ((2*l-1.0f)*Pml_1*s.z - (l-1.0f)*Pml_2) / l;
        Pml_2 = Pml_1;
        Pml_1 = Pml;
        SHb[SHindex(0, l)] = Pml;
    }
    float Pmm = 1;
    uint m;
    for (m=1 ; m<numBands ; m++) {
        Pmm = (1.0f - 2*m) * Pmm;      // See [1], divide by sqrt(1 - s.z*s.z);
        Pml_2 = Pmm;
        Pml_1 = (2*m + 1.0f)*Pmm*s.z;
        // l == m
        SHb[SHindex(-m, m)] = Pml_2;
        SHb[SHindex( m, m)] = Pml_2;
        if (m+1 < numBands) {
            // l == m+1
            SHb[SHindex(-m, m+1)] = Pml_1;
            SHb[SHindex( m, m+1)] = Pml_1;
            for (uint l=m+2 ; l<numBands ; l++) {
                float Pml = ((2*l - 1.0f)*Pml_1*s.z - (l + m - 1.0f)*Pml_2) / (l-m);
                Pml_2 = Pml_1;
                Pml_1 = Pml;
                SHb[SHindex(-m, l)] = Pml;
                SHb[SHindex( m, l)] = Pml;
            }
        }
    }

    // At this point, SHb contains the associated Legendre polynomials divided
    // by sin(theta)^|m|. Below we compute the SH basis.
    //
    // ( cos(m*phi), sin(m*phi) ) recursion:
    // cos(m*phi + phi) == cos(m*phi)*cos(phi) - sin(m*phi)*sin(phi)
    // sin(m*phi + phi) == sin(m*phi)*cos(phi) + cos(m*phi)*sin(phi)
    // cos[m+1] == cos[m]*s.x - sin[m]*s.y
    // sin[m+1] == sin[m]*s.x + cos[m]*s.y
    //
    // Note that (d.x, d.y) == (cos(phi), sin(phi)) * sin(theta), so the
    // code below actually evaluates:
    //      (cos((m*phi), sin(m*phi)) * sin(theta)^|m|
    float Cm = s.x;
    float Sm = s.y;
    for (m = 1; m <= numBands; m++) {
        for (uint l = m; l < numBands; l++) {
            SHb[SHindex(-m, l)] *= Sm;
            SHb[SHindex( m, l)] *= Cm;
        }
        float Cm1 = Cm * s.x - Sm * s.y;
        float Sm1 = Sm * s.x + Cm * s.y;
        Cm = Cm1;
        Sm = Sm1;
    }
}

/*
 * returns n! / d!
 */
float factorial(uint n, uint d = 1)
{
   d = max(uint(1), d);
   n = max(uint(1), n);
   float r = 1.0;
   if (n == d) {
       // intentionally left blank
   } else if (n > d) {
       for ( ; n>d ; n--) {
           r *= n;
       }
   } else {
       for ( ; d>n ; d--) {
           r *= d;
       }
       r = 1.0f / r;
   }
   return r;
}

/*
 * SH scaling factors:
 *  returns sqrt((2*l + 1) / 4*pi) * sqrt( (l-|m|)! / (l+|m|)! )
 */
float Kml(uint m, uint l)
{
    m = m < 0 ? -m : m;  // abs() is not constexpr
    const float K = (2 * l + 1) * factorial(uint(l - m), uint(l + m));
    return sqrt(K) * (F_2_SQRTPI * 0.25);
}

void KiBand2(out float ki[9])
{
    for (uint l = 0; l < 3; l++) {
        ki[SHindex(0, l)] = Kml(0, l);
        for (uint m = 1; m <= l; m++) {
            ki[SHindex(m, l)] =
            ki[SHindex(-m, l)] = F_SQRT2 * Kml(m, l);
        }
    }
}

// < cos(theta) > SH coefficients pre-multiplied by 1 / K(0,l)
float computeTruncatedCosSh(uint l)
{
    if (l == 0) {
        return F_PI;
    } else if (l == 1) {
        return 2 * F_PI / 3;
    } else if (l & 1u) {
        return 0;
    }
    const uint l_2 = l / 2;
    float A0 = ((l_2 & 1u) ? 1.0f : -1.0f) / ((l + 2) * int(l - 1));
    float A1 = factorial(l, l_2) / (factorial(l_2) * uint(uint(1) << l));
    return 2 * F_PI * A0 * A1;
}

/*
 * This computes the 3-bands SH coefficients of the Cubemap convoluted by the
 * truncated cos(theta) (i.e.: saturate(s.z)), pre-scaled by the reconstruction
 * factors.
 */
void preprocessSH(inout float3 SH[9])
{
    const uint numBands = 3;
    const uint numCoefs = numBands * numBands;

    // Coefficient for the polynomial form of the SH functions -- these were taken from
    // "Stupid Spherical Harmonics (SH)" by Peter-Pike Sloan
    // They simply come for expanding the computation of each SH function.
    //
    // To render spherical harmonics we can use the polynomial form, like this:
    //          c += sh[0] * A[0];
    //          c += sh[1] * A[1] * s.y;
    //          c += sh[2] * A[2] * s.z;
    //          c += sh[3] * A[3] * s.x;
    //          c += sh[4] * A[4] * s.y * s.x;
    //          c += sh[5] * A[5] * s.y * s.z;
    //          c += sh[6] * A[6] * (3 * s.z * s.z - 1);
    //          c += sh[7] * A[7] * s.z * s.x;
    //          c += sh[8] * A[8] * (s.x * s.x - s.y * s.y);
    //
    // To save math in the shader, we pre-multiply our SH coefficient by the A[i] factors.
    // Additionally, we include the lambertian diffuse BRDF 1/pi.

    const float M_SQRT_PI = 1.7724538509f;
    const float M_SQRT_3  = 1.7320508076f;
    const float M_SQRT_5  = 2.2360679775f;
    const float M_SQRT_15 = 3.8729833462f;
    const float A[numCoefs] = {
                  1.0f / (2.0f * M_SQRT_PI),    // 0  0
            -M_SQRT_3  / (2.0f * M_SQRT_PI),    // 1 -1
             M_SQRT_3  / (2.0f * M_SQRT_PI),    // 1  0
            -M_SQRT_3  / (2.0f * M_SQRT_PI),    // 1  1
             M_SQRT_15 / (2.0f * M_SQRT_PI),    // 2 -2
            -M_SQRT_15 / (2.0f * M_SQRT_PI),    // 3 -1
             M_SQRT_5  / (4.0f * M_SQRT_PI),    // 3  0
            -M_SQRT_15 / (2.0f * M_SQRT_PI),    // 3  1
             M_SQRT_15 / (4.0f * M_SQRT_PI)     // 3  2
    };

    for (uint i = 0; i < numCoefs; i++) {
        SH[i] *= A[i] * F_1_PI;
    }
}


//-------------------------------------------------------------------
// Packs coefficients so that we can use Peter-Pike Sloan's shader code.
// The function does not perform premultiplication with coefficients of SH basis functions, caller need to do it
// See SetSHEMapConstants() in "Stupid Spherical Harmonics Tricks".
// Constant + linear
// 输入 sh[9*3] 表示9个球鞋系数的3个通道
void PackSH(float sh[27], out float4 buffer[7])
{
    int c = 0;
    for (c = 0; c < 3; c++)

    {
        buffer[c] = float4(sh[c * 9 + 3], sh[c * 9 + 1], sh[c * 9 + 2], sh[c * 9 + 0] - sh[c * 9 + 6]);
    }

    // Quadratic (4/5)
    for (c = 0; c < 3; c++)
    {
        buffer[3 + c] = float4(sh[c * 9 + 4], sh[c * 9 + 5], sh[c * 9 + 6] * 3.0f, sh[c * 9 + 7]);
    }

    // Quadratic (5)
    buffer[6] = float4(sh[0 * 9 + 8], sh[1 * 9 + 8], sh[2 * 9 + 8], 1.0f);
}


