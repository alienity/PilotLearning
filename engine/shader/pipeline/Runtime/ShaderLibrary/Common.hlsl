#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

#include "./API/D3D12.hlsl"

#include "Macros.hlsl"
#include "Random.hlsl"

// Unsigned integer bit field extraction.
// Note that the intrinsic itself generates a vector instruction.
// Wrap this function with WaveReadLaneFirst() to get scalar output.
uint BitFieldExtract(uint data, uint offset, uint numBits)
{
    uint mask = (1u << numBits) - 1u;
    return (data >> offset) & mask;
}

// Integer bit field extraction with sign extension.
// Note that the intrinsic itself generates a vector instruction.
// Wrap this function with WaveReadLaneFirst() to get scalar output.
int BitFieldExtractSignExtend(int data, uint offset, uint numBits)
{
    int shifted = data >> offset; // Sign-extending (arithmetic) shift
    int signBit = shifted & (1u << (numBits - 1u));
    uint mask = (1u << numBits) - 1u;

    return -signBit | (shifted & mask); // Use 2-complement for negation to replicate the sign bit
}

// Inserts the bits indicated by 'mask' from 'src' into 'dst'.
uint BitFieldInsert(uint mask, uint src, uint dst)
{
    return (src & mask) | (dst & ~mask);
}

bool IsBitSet(uint data, uint offset)
{
    return BitFieldExtract(data, offset, 1u) != 0;
}

void SetBit(inout uint data, uint offset)
{
    data |= 1u << offset;
}

void ClearBit(inout uint data, uint offset)
{
    data &= ~(1u << offset);
}

void ToggleBit(inout uint data, uint offset)
{
    data ^= 1u << offset;
}

// Warning: for correctness, the argument's value must be the same across all lanes of the wave.
TEMPLATE_1_FLT(WaveReadLaneFirst, scalarValue, return scalarValue)
TEMPLATE_1_INT(WaveReadLaneFirst, scalarValue, return scalarValue)

TEMPLATE_2_INT(Mul24, a, b, return a * b)

TEMPLATE_3_INT(Mad24, a, b, c, return a * b + c)

TEMPLATE_3_FLT(Min3, a, b, c, return min(min(a, b), c))
TEMPLATE_3_INT(Min3, a, b, c, return min(min(a, b), c))
TEMPLATE_3_FLT(Max3, a, b, c, return max(max(a, b), c))
TEMPLATE_3_INT(Max3, a, b, c, return max(max(a, b), c))

TEMPLATE_3_FLT(Avg3, a, b, c, return (a + b + c) * 0.33333333)

// Important! Quad functions only valid in pixel shaders!
float2 GetQuadOffset(int2 screenPos)
{
    return float2(float(screenPos.x & 1) * 2.0 - 1.0, float(screenPos.y & 1) * 2.0 - 1.0);
}

float QuadReadAcrossX(float value, int2 screenPos)
{
    return value - (ddx_fine(value) * (float(screenPos.x & 1) * 2.0 - 1.0));
}

float QuadReadAcrossY(float value, int2 screenPos)
{
    return value - (ddy_fine(value) * (float(screenPos.y & 1) * 2.0 - 1.0));
}

float QuadReadAcrossDiagonal(float value, int2 screenPos)
{
    float2 quadDir = GetQuadOffset(screenPos);
    float dX = ddx_fine(value);
    float X = value - (dX * quadDir.x);
    return X - (ddy_fine(X) * quadDir.y);
}

float3 QuadReadFloat3AcrossX(float3 val, int2 positionSS)
{
    return float3(QuadReadAcrossX(val.x, positionSS), QuadReadAcrossX(val.y, positionSS), QuadReadAcrossX(val.z, positionSS));
}

float4 QuadReadFloat4AcrossX(float4 val, int2 positionSS)
{
    return float4(QuadReadAcrossX(val.x, positionSS), QuadReadAcrossX(val.y, positionSS), QuadReadAcrossX(val.z, positionSS), QuadReadAcrossX(val.w, positionSS));
}

float3 QuadReadFloat3AcrossY(float3 val, int2 positionSS)
{
    return float3(QuadReadAcrossY(val.x, positionSS), QuadReadAcrossY(val.y, positionSS), QuadReadAcrossY(val.z, positionSS));
}

float4 QuadReadFloat4AcrossY(float4 val, int2 positionSS)
{
    return float4(QuadReadAcrossY(val.x, positionSS), QuadReadAcrossY(val.y, positionSS), QuadReadAcrossY(val.z, positionSS), QuadReadAcrossY(val.w, positionSS));
}

float3 QuadReadFloat3AcrossDiagonal(float3 val, int2 positionSS)
{
    return float3(QuadReadAcrossDiagonal(val.x, positionSS), QuadReadAcrossDiagonal(val.y, positionSS), QuadReadAcrossDiagonal(val.z, positionSS));
}

float4 QuadReadFloat4AcrossDiagonal(float4 val, int2 positionSS)
{
    return float4(QuadReadAcrossDiagonal(val.x, positionSS), QuadReadAcrossDiagonal(val.y, positionSS), QuadReadAcrossDiagonal(val.z, positionSS), QuadReadAcrossDiagonal(val.w, positionSS));
}

TEMPLATE_SWAP(Swap) // Define a Swap(a, b) function for all types

#define CUBEMAPFACE_POSITIVE_X 0
#define CUBEMAPFACE_NEGATIVE_X 1
#define CUBEMAPFACE_POSITIVE_Y 2
#define CUBEMAPFACE_NEGATIVE_Y 3
#define CUBEMAPFACE_POSITIVE_Z 4
#define CUBEMAPFACE_NEGATIVE_Z 5

float CubeMapFaceID(float3 dir)
{
    float faceID;

    if (abs(dir.z) >= abs(dir.x) && abs(dir.z) >= abs(dir.y))
    {
        faceID = (dir.z < 0.0) ? CUBEMAPFACE_NEGATIVE_Z : CUBEMAPFACE_POSITIVE_Z;
    }
    else if (abs(dir.y) >= abs(dir.x))
    {
        faceID = (dir.y < 0.0) ? CUBEMAPFACE_NEGATIVE_Y : CUBEMAPFACE_POSITIVE_Y;
    }
    else
    {
        faceID = (dir.x < 0.0) ? CUBEMAPFACE_NEGATIVE_X : CUBEMAPFACE_POSITIVE_X;
    }

    return faceID;
}

// Intrinsic isnan can't be used because it require /Gic to be enabled on fxc that we can't do. So use AnyIsNan instead
bool IsNaN(float x)
{
    return (asuint(x) & 0x7FFFFFFF) > 0x7F800000;
}

bool AnyIsNaN(float2 v)
{
    return (IsNaN(v.x) || IsNaN(v.y));
}

bool AnyIsNaN(float3 v)
{
    return (IsNaN(v.x) || IsNaN(v.y) || IsNaN(v.z));
}

bool AnyIsNaN(float4 v)
{
    return (IsNaN(v.x) || IsNaN(v.y) || IsNaN(v.z) || IsNaN(v.w));
}

bool IsInf(float x)
{
    return (asuint(x) & 0x7FFFFFFF) == 0x7F800000;
}

bool AnyIsInf(float2 v)
{
    return (IsInf(v.x) || IsInf(v.y));
}

bool AnyIsInf(float3 v)
{
    return (IsInf(v.x) || IsInf(v.y) || IsInf(v.z));
}

bool AnyIsInf(float4 v)
{
    return (IsInf(v.x) || IsInf(v.y) || IsInf(v.z) || IsInf(v.w));
}

bool IsFinite(float x)
{
    return (asuint(x) & 0x7F800000) != 0x7F800000;
}

float SanitizeFinite(float x)
{
    return IsFinite(x) ? x : 0;
}

bool IsPositiveFinite(float x)
{
    return asuint(x) < 0x7F800000;
}

float SanitizePositiveFinite(float x)
{
    return IsPositiveFinite(x) ? x : 0;
}

// ----------------------------------------------------------------------------
// Common math functions
// ----------------------------------------------------------------------------

float DegToRad(float deg)
{
    return deg * (PI / 180.0);
}

float RadToDeg(float rad)
{
    return rad * (180.0 / PI);
}

// Square functions for cleaner code
TEMPLATE_1_FLT(Sq, x, return (x) * (x))
TEMPLATE_1_INT(Sq, x, return (x) * (x))

bool IsPower2(uint x)
{
    return (x & (x - 1)) == 0;
}

// Input [0, 1] and output [0, PI/2]
// 9 VALU
float FastACosPos(float inX)
{
    float x = abs(inX);
    float res = (0.0468878 * x + -0.203471) * x + 1.570796; // p(x)
    res *= sqrt(1.0 - x);

    return res;
}

// Ref: https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
// Input [-1, 1] and output [0, PI]
// 12 VALU
float FastACos(float inX)
{
    float res = FastACosPos(inX);

    return (inX >= 0) ? res : PI - res; // Undo range reduction
}

// Same cost as Acos + 1 FR
// Same error
// input [-1, 1] and output [-PI/2, PI/2]
float FastASin(float x)
{
    return HALF_PI - FastACos(x);
}

// max absolute error 1.3x10^-3
// Eberly's odd polynomial degree 5 - respect bounds
// 4 VGPR, 14 FR (10 FR, 1 QR), 2 scalar
// input [0, infinity] and output [0, PI/2]
float FastATanPos(float x)
{
    float t0 = (x < 1.0) ? x : 1.0 / x;
    float t1 = t0 * t0;
    float poly = 0.0872929;
    poly = -0.301895 + poly * t1;
    poly = 1.0 + poly * t1;
    poly = poly * t0;
    return (x < 1.0) ? poly : HALF_PI - poly;
}

// 4 VGPR, 16 FR (12 FR, 1 QR), 2 scalar
// input [-infinity, infinity] and output [-PI/2, PI/2]
float FastATan(float x)
{
    float t0 = FastATanPos(abs(x));
    return (x < 0.0) ? -t0 : t0;
}

float FastAtan2(float y, float x)
{
    return FastATan(y / x) + (y >= 0.0 ? PI : -PI) * (x < 0.0);
}

uint FastLog2(uint x)
{
    return firstbithigh(x);
}

// Using pow often result to a warning like this
// "pow(f, e) will not work for negative f, use abs(f) or conditionally handle negative values if you expect them"
// PositivePow remove this warning when you know the value is positive or 0 and avoid inf/NAN.
// Note: https://msdn.microsoft.com/en-us/library/windows/desktop/bb509636(v=vs.85).aspx pow(0, >0) == 0
TEMPLATE_2_FLT(PositivePow, base, power, return pow(abs(base), power))

// SafePositivePow: Same as pow(x,y) but considers x always positive and never exactly 0 such that
// SafePositivePow(0,y) will numerically converge to 1 as y -> 0, including SafePositivePow(0,0) returning 1.
//
// First, like PositivePow, SafePositivePow removes this warning for when you know the x value is positive or 0 and you know
// you avoid a NaN:
// ie you know that x == 0 and y > 0, such that pow(x,y) == pow(0, >0) == 0
// SafePositivePow(0, y) will however return close to 1 as y -> 0, see below.
//
// Also, pow(x,y) is most probably approximated as exp2(log2(x) * y), so pow(0,0) will give exp2(-inf * 0) == exp2(NaN) == NaN.
//
// SafePositivePow avoids NaN in allowing SafePositivePow(x,y) where (x,y) == (0,y) for any y including 0 by clamping x to a
// minimum of FLT_EPS. The consequences are:
//
// -As a replacement for pow(0,y) where y >= 1, the result of SafePositivePow(x,y) should be close enough to 0.
// -For cases where we substitute for pow(0,y) where 0 < y < 1, SafePositivePow(x,y) will quickly reach 1 as y -> 0, while
// normally pow(0,y) would give 0 instead of 1 for all 0 < y.
// eg: if we #define FLT_EPS  5.960464478e-8 (for fp32),
// SafePositivePow(0, 0.1)   = 0.1894646
// SafePositivePow(0, 0.01)  = 0.8467453
// SafePositivePow(0, 0.001) = 0.9835021
//
// Depending on the intended usage of pow(), this difference in behavior might be a moot point since:
// 1) by leaving "y" free to get to 0, we get a NaNs
// 2) the behavior of SafePositivePow() has more continuity when both x and y get closer together to 0, since
// when x is assured to be positive non-zero, pow(x,x) -> 1 as x -> 0.
//
// TL;DR: SafePositivePow(x,y) avoids NaN and is safe for positive (x,y) including (x,y) == (0,0),
//        but SafePositivePow(0, y) will return close to 1 as y -> 0, instead of 0, so watch out
//        for behavior depending on pow(0, y) giving always 0, especially for 0 < y < 1.
//
// Ref: https://msdn.microsoft.com/en-us/library/windows/desktop/bb509636(v=vs.85).aspx
TEMPLATE_2_FLT(SafePositivePow, base, power, return pow(max(abs(base), float(FLT_EPS)), power))

// Helpers for making shadergraph functions consider precision spec through the same $precision token used for variable types
TEMPLATE_2_FLT(SafePositivePow_float, base, power, return pow(max(abs(base), float(FLT_EPS)), power))
TEMPLATE_2_HALF(SafePositivePow_half, base, power, return pow(max(abs(base), half(HALF_EPS)), power))

float Eps_float()
{
    return FLT_EPS;
}
float Min_float()
{
    return FLT_MIN;
}
float Max_float()
{
    return FLT_MAX;
}
half Eps_half()
{
    return HALF_EPS;
}
half Min_half()
{
    return HALF_MIN;
}
half Max_half()
{
    return HALF_MAX;
}

// Composes a floating point value with the magnitude of 'x' and the sign of 's'.
// See the comment about FastSign() below.
float CopySign(float x, float s, bool ignoreNegZero = true)
{
    if (ignoreNegZero)
    {
        return (s >= 0) ? abs(x) : -abs(x);
    }
    else
    {
        uint negZero = 0x80000000u;
        uint signBit = negZero & asuint(s);
        return asfloat(BitFieldInsert(negZero, signBit, asuint(x)));
    }
}

// Returns -1 for negative numbers and 1 for positive numbers.
// 0 can be handled in 2 different ways.
// The IEEE floating point standard defines 0 as signed: +0 and -0.
// However, mathematics typically treats 0 as unsigned.
// Therefore, we treat -0 as +0 by default: FastSign(+0) = FastSign(-0) = 1.
// If (ignoreNegZero = false), FastSign(-0, false) = -1.
// Note that the sign() function in HLSL implements signum, which returns 0 for 0.
float FastSign(float s, bool ignoreNegZero = true)
{
    return CopySign(1.0, s, ignoreNegZero);
}

// Orthonormalizes the tangent frame using the Gram-Schmidt process.
// We assume that the normal is normalized and that the two vectors
// aren't collinear.
// Returns the new tangent (the normal is unaffected).
float3 Orthonormalize(float3 tangent, float3 normal)
{
    // TODO: use SafeNormalize()?
    return normalize(tangent - dot(tangent, normal) * normal);
}

// [start, end] -> [0, 1] : (x - start) / (end - start) = x * rcpLength - (start * rcpLength)
TEMPLATE_3_FLT(Remap01, x, rcpLength, startTimesRcpLength, return saturate(x * rcpLength - startTimesRcpLength))

// [start, end] -> [1, 0] : (end - x) / (end - start) = (end * rcpLength) - x * rcpLength
TEMPLATE_3_FLT(Remap10, x, rcpLength, endTimesRcpLength, return saturate(endTimesRcpLength - x * rcpLength))

// Remap: [0.5 / size, 1 - 0.5 / size] -> [0, 1]
float2 RemapHalfTexelCoordTo01(float2 coord, float2 size)
{
    const float2 rcpLen = size * rcp(size - 1);
    const float2 startTimesRcpLength = 0.5 * rcp(size - 1);

    return Remap01(coord, rcpLen, startTimesRcpLength);
}

// Remap: [0, 1] -> [0.5 / size, 1 - 0.5 / size]
float2 Remap01ToHalfTexelCoord(float2 coord, float2 size)
{
    const float2 start = 0.5 * rcp(size);
    const float2 len = 1 - rcp(size);

    return coord * len + start;
}

// smoothstep that assumes that 'x' lies within the [0, 1] interval.
float Smoothstep01(float x)
{
    return x * x * (3 - (2 * x));
}

float Smootherstep01(float x)
{
    return x * x * x * (x * (x * 6 - 15) + 10);
}

float Smootherstep(float a, float b, float t)
{
    float r = rcp(b - a);
    float x = Remap01(t, r, a * r);
    return Smootherstep01(x);
}

float3 NLerp(float3 A, float3 B, float t)
{
    return normalize(lerp(A, B, t));
}

float Length2(float3 v)
{
    return dot(v, v);
}

float Pow4(float x)
{
    return (x * x) * (x * x);
}

TEMPLATE_3_FLT(RangeRemap, min, max, t, return saturate((t - min) / (max - min)))

float4x4 Inverse(float4x4 m)
{
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}

// ----------------------------------------------------------------------------
// Texture utilities
// ----------------------------------------------------------------------------

float ComputeTextureLOD(float2 uvdx, float2 uvdy, float2 scale, float bias = 0.0)
{
    float2 ddx_ = scale * uvdx;
    float2 ddy_ = scale * uvdy;
    float d = max(dot(ddx_, ddx_), dot(ddy_, ddy_));

    return max(0.5 * log2(d) - bias, 0.0);
}

float ComputeTextureLOD(float2 uv, float bias = 0.0)
{
    float2 ddx_ = ddx(uv);
    float2 ddy_ = ddy(uv);

    return ComputeTextureLOD(ddx_, ddy_, 1.0, bias);
}

// x contains width, w contains height
float ComputeTextureLOD(float2 uv, float2 texelSize, float bias = 0.0)
{
    uv *= texelSize;

    return ComputeTextureLOD(uv, bias);
}

// LOD clamp is optional and happens outside the function.
float ComputeTextureLOD(float3 duvw_dx, float3 duvw_dy, float3 duvw_dz, float scale, float bias = 0.0)
{
    float d = Max3(dot(duvw_dx, duvw_dx), dot(duvw_dy, duvw_dy), dot(duvw_dz, duvw_dz));

    return max(0.5f * log2(d * (scale * scale)) - bias, 0.0);
}

uint GetMipCount(TEXTURE2D_PARAM(tex, smp))
{
    uint mipLevel, width, height, mipCount;
    mipLevel = width = height = mipCount = 0;
    tex.GetDimensions(mipLevel, width, height, mipCount);
    return mipCount;
}

// ----------------------------------------------------------------------------
// Texture format sampling
// ----------------------------------------------------------------------------

float2 DirectionToLatLongCoordinate(float3 unDir)
{
    float3 dir = normalize(unDir);
    // coordinate frame is (-Z, X) meaning negative Z is primary axis and X is secondary axis.
    return float2(1.0 - 0.5 * INV_PI * atan2(dir.x, -dir.z), asin(dir.y) * INV_PI + 0.5);
}

float3 LatlongToDirectionCoordinate(float2 coord)
{
    float theta = coord.y * PI;
    float phi = (coord.x * 2.f * PI - PI * 0.5f);

    float cosTheta = cos(theta);
    float sinTheta = sqrt(1.0 - min(1.0, cosTheta * cosTheta));
    float cosPhi = cos(phi);
    float sinPhi = sin(phi);

    float3 direction = float3(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
    direction.xy *= -1.0;
    return direction;
}

// ----------------------------------------------------------------------------
// Depth encoding/decoding
// ----------------------------------------------------------------------------

// Z buffer to linear 0..1 depth (0 at near plane, 1 at far plane).
// Does NOT correctly handle oblique view frustums.
// Does NOT work with orthographic projection.`
// zBufferParam = { (f-n)/n, 1, (f-n)/n*f, 1/f }
float Linear01DepthFromNear(float depth, float4 zBufferParam)
{
    return 1.0 / (zBufferParam.x + zBufferParam.y / depth);
}

// Z buffer to linear 0..1 depth (0 at camera position, 1 at far plane).
// Does NOT work with orthographic projections.
// Does NOT correctly handle oblique view frustums.
// zBufferParam = { (f-n)/n, 1, (f-n)/n*f, 1/f }
float Linear01Depth(float depth, float4 zBufferParam)
{
    return 1.0 / (zBufferParam.x * depth + zBufferParam.y);
}

// Z buffer to linear depth.
// Does NOT correctly handle oblique view frustums.
// Does NOT work with orthographic projection.
// zBufferParam = { (f-n)/n, 1, (f-n)/n*f, 1/f }
float LinearEyeDepth(float depth, float4 zBufferParam)
{
    return 1.0 / (zBufferParam.z * depth + zBufferParam.w);
}

// Z buffer to linear depth.
// Correctly handles oblique view frustums.
// Does NOT work with orthographic projection.
// Ref: An Efficient Depth Linearization Method for Oblique View Frustums, Eq. 6.
float LinearEyeDepth(float2 positionNDC, float deviceDepth, float4 invProjParam)
{
    float4 positionCS = float4(positionNDC * 2.0 - 1.0, deviceDepth, 1.0);
    float viewSpaceZ = rcp(dot(positionCS, invProjParam));

    // If the matrix is right-handed, we have to flip the Z axis to get a positive value.
    return abs(viewSpaceZ);
}

// Z buffer to linear depth.
// Works in all cases.
// Typically, this is the cheapest variant, provided you've already computed 'positionWS'.
// Assumes that the 'positionWS' is in front of the camera.
float LinearEyeDepth(float3 positionWS, float4x4 viewMatrix)
{
    float viewSpaceZ = mul(viewMatrix, float4(positionWS, 1.0)).z;

    // If the matrix is right-handed, we have to flip the Z axis to get a positive value.
    return abs(viewSpaceZ);
}

// 'z' is the view space Z position (linear depth).
// saturate(z) the output of the function to clamp them to the [0, 1] range.
// d = log2(c * (z - n) + 1) / log2(c * (f - n) + 1)
//   = log2(c * (z - n + 1/c)) / log2(c * (f - n) + 1)
//   = log2(c) / log2(c * (f - n) + 1) + log2(z - (n - 1/c)) / log2(c * (f - n) + 1)
//   = E + F * log2(z - G)
// encodingParams = { E, F, G, 0 }
float EncodeLogarithmicDepthGeneralized(float z, float4 encodingParams)
{
    // Use max() to avoid NaNs.
    return encodingParams.x + encodingParams.y * log2(max(0, z - encodingParams.z));
}

// 'd' is the logarithmically encoded depth value.
// saturate(d) to clamp the output of the function to the [n, f] range.
// z = 1/c * (pow(c * (f - n) + 1, d) - 1) + n
//   = 1/c * pow(c * (f - n) + 1, d) + n - 1/c
//   = 1/c * exp2(d * log2(c * (f - n) + 1)) + (n - 1/c)
//   = L * exp2(d * M) + N
// decodingParams = { L, M, N, 0 }
// Graph: https://www.desmos.com/calculator/qrtatrlrba
float DecodeLogarithmicDepthGeneralized(float d, float4 decodingParams)
{
    return decodingParams.x * exp2(d * decodingParams.y) + decodingParams.z;
}

// 'z' is the view-space Z position (linear depth).
// saturate(z) the output of the function to clamp them to the [0, 1] range.
// encodingParams = { n, log2(f/n), 1/n, 1/log2(f/n) }
// This is an optimized version of EncodeLogarithmicDepthGeneralized() for (c = 2).
float EncodeLogarithmicDepth(float z, float4 encodingParams)
{
    // Use max() to avoid NaNs.
    // TODO: optimize to (log2(z) - log2(n)) / (log2(f) - log2(n)).
    return log2(max(0, z * encodingParams.z)) * encodingParams.w;
}

// 'd' is the logarithmically encoded depth value.
// saturate(d) to clamp the output of the function to the [n, f] range.
// encodingParams = { n, log2(f/n), 1/n, 1/log2(f/n) }
// This is an optimized version of DecodeLogarithmicDepthGeneralized() for (c = 2).
// Graph: https://www.desmos.com/calculator/qrtatrlrba
float DecodeLogarithmicDepth(float d, float4 encodingParams)
{
    // TODO: optimize to exp2(d * y + log2(x)).
    return encodingParams.x * exp2(d * encodingParams.y);
}

float4 CompositeOver(float4 front, float4 back)
{
    return front + (1 - front.a) * back;
}

void CompositeOver(float3 colorFront, float3 alphaFront,
                   float3 colorBack, float3 alphaBack,
                   out float3 color, out float3 alpha)
{
    color = colorFront + (1 - alphaFront) * colorBack;
    alpha = alphaFront + (1 - alphaFront) * alphaBack;
}

// ----------------------------------------------------------------------------
// Space transformations
// ----------------------------------------------------------------------------

static const float3x3 k_identity3x3 = {1, 0, 0,
                                       0, 1, 0,
                                       0, 0, 1};

static const float4x4 k_identity4x4 = {1, 0, 0, 0,
                                       0, 1, 0, 0,
                                       0, 0, 1, 0,
                                       0, 0, 0, 1};

float4 ComputeClipSpacePosition(float2 positionNDC, float deviceDepth)
{
    float4 positionCS = float4(positionNDC * 2.0 - 1.0, deviceDepth, 1.0);
    return positionCS;
}

// Use case examples:
// (position = positionCS) => (clipSpaceTransform = use default)
// (position = positionVS) => (clipSpaceTransform = UNITY_MATRIX_P)
// (position = positionWS) => (clipSpaceTransform = UNITY_MATRIX_VP)
float4 ComputeClipSpacePosition(float3 position, float4x4 clipSpaceTransform = k_identity4x4)
{
    return mul(clipSpaceTransform, float4(position, 1.0));
}

// The returned Z value is the depth buffer value (and NOT linear view space Z value).
// Use case examples:
// (position = positionCS) => (clipSpaceTransform = use default)
// (position = positionVS) => (clipSpaceTransform = UNITY_MATRIX_P)
// (position = positionWS) => (clipSpaceTransform = UNITY_MATRIX_VP)
float3 ComputeNormalizedDeviceCoordinatesWithZ(float3 position, float4x4 clipSpaceTransform = k_identity4x4)
{
    float4 positionCS = ComputeClipSpacePosition(position, clipSpaceTransform);
    
    positionCS *= rcp(positionCS.w);
    positionCS.xy = positionCS.xy * 0.5 + 0.5;

    return positionCS.xyz;
}

// Use case examples:
// (position = positionCS) => (clipSpaceTransform = use default)
// (position = positionVS) => (clipSpaceTransform = UNITY_MATRIX_P)
// (position = positionWS) => (clipSpaceTransform = UNITY_MATRIX_VP)
float2 ComputeNormalizedDeviceCoordinates(float3 position, float4x4 clipSpaceTransform = k_identity4x4)
{
    return ComputeNormalizedDeviceCoordinatesWithZ(position, clipSpaceTransform).xy;
}

float3 ComputeViewSpacePosition(float2 positionNDC, float deviceDepth, float4x4 invProjMatrix)
{
    float4 positionCS = ComputeClipSpacePosition(positionNDC, deviceDepth);
    float4 positionVS = mul(invProjMatrix, positionCS);
    // The view space uses a right-handed coordinate system.
    positionVS.z = -positionVS.z;
    return positionVS.xyz / positionVS.w;
}

float3 ComputeWorldSpacePosition(float2 positionNDC, float deviceDepth, float4x4 invViewProjMatrix)
{
    float4 positionCS = ComputeClipSpacePosition(positionNDC, deviceDepth);
    float4 hpositionWS = mul(invViewProjMatrix, positionCS);
    return hpositionWS.xyz / hpositionWS.w;
}

float3 ComputeWorldSpacePosition(float4 positionCS, float4x4 invViewProjMatrix)
{
    float4 hpositionWS = mul(invViewProjMatrix, positionCS);
    return hpositionWS.xyz / hpositionWS.w;
}

// ----------------------------------------------------------------------------
// PositionInputs
// ----------------------------------------------------------------------------

// Note: if you modify this struct, be sure to update the CustomPassFullscreenShader.template
struct PositionInputs
{
    float3 positionWS;  // World space position (could be camera-relative)
    float2 positionNDC; // Normalized screen coordinates within the viewport    : [0, 1) (with the half-pixel offset)
    uint2 positionSS;   // Screen space pixel coordinates                       : [0, NumPixels)
    uint2 tileCoord;    // Screen tile coordinates                              : [0, NumTiles)
    float deviceDepth;  // Depth from the depth buffer                          : [0, 1] (typically reversed)
    float linearDepth;  // View space Z coordinate                              : [Near, Far]
};

// This function is use to provide an easy way to sample into a screen texture, either from a pixel or a compute shaders.
// This allow to easily share code.
// If a compute shader call this function positionSS is an integer usually calculate like: uint2 positionSS = groupId.xy * BLOCK_SIZE + groupThreadId.xy
// else it is current unormalized screen coordinate like return by SV_Position
PositionInputs GetPositionInput(float2 positionSS, float2 invScreenSize, uint2 tileCoord)   // Specify explicit tile coordinates so that we can easily make it lane invariant for compute evaluation.
{
    PositionInputs posInput;
    ZERO_INITIALIZE(PositionInputs, posInput);

    posInput.positionNDC = positionSS;
#if defined(SHADER_STAGE_COMPUTE)
    // In case of compute shader an extra half offset is added to the screenPos to shift the integer position to pixel center.
    posInput.positionNDC.xy += float2(0.5, 0.5);
#endif
    posInput.positionNDC *= invScreenSize;
    posInput.positionSS = uint2(positionSS);
    posInput.tileCoord = tileCoord;

    return posInput;
}

PositionInputs GetPositionInput(float2 positionSS, float2 invScreenSize)
{
    return GetPositionInput(positionSS, invScreenSize, uint2(0, 0));
}

// For Raytracing only
// This function does not initialize deviceDepth and linearDepth
PositionInputs GetPositionInput(float2 positionSS, float2 invScreenSize, float3 positionWS)
{
    PositionInputs posInput = GetPositionInput(positionSS, invScreenSize, uint2(0, 0));
    posInput.positionWS = positionWS;

    return posInput;
}

// From forward
// deviceDepth and linearDepth come directly from .zw of SV_Position
PositionInputs GetPositionInput(float2 positionSS, float2 invScreenSize, float deviceDepth, float linearDepth, float3 positionWS, uint2 tileCoord)
{
    PositionInputs posInput = GetPositionInput(positionSS, invScreenSize, tileCoord);
    posInput.positionWS = positionWS;
    posInput.deviceDepth = deviceDepth;
    posInput.linearDepth = linearDepth;

    return posInput;
}

PositionInputs GetPositionInput(float2 positionSS, float2 invScreenSize, float deviceDepth, float linearDepth, float3 positionWS)
{
    return GetPositionInput(positionSS, invScreenSize, deviceDepth, linearDepth, positionWS, uint2(0, 0));
}

// From deferred or compute shader
// depth must be the depth from the raw depth buffer. This allow to handle all kind of depth automatically with the inverse view projection matrix.
// For information. In Unity Depth is always in range 0..1 (even on OpenGL) but can be reversed.
PositionInputs GetPositionInput(float2 positionSS, float2 invScreenSize, float deviceDepth,
    float4x4 invViewProjMatrix, float4x4 viewMatrix,
    uint2 tileCoord)
{
    PositionInputs posInput = GetPositionInput(positionSS, invScreenSize, tileCoord);
    posInput.positionWS = ComputeWorldSpacePosition(posInput.positionNDC, deviceDepth, invViewProjMatrix);
    posInput.deviceDepth = deviceDepth;
    posInput.linearDepth = LinearEyeDepth(posInput.positionWS, viewMatrix);

    return posInput;
}

PositionInputs GetPositionInput(float2 positionSS, float2 invScreenSize, float deviceDepth,
                                float4x4 invViewProjMatrix, float4x4 viewMatrix)
{
    return GetPositionInput(positionSS, invScreenSize, deviceDepth, invViewProjMatrix, viewMatrix, uint2(0, 0));
}

// The view direction 'V' points towards the camera.
// 'depthOffsetVS' is always applied in the opposite direction (-V).
void ApplyDepthOffsetPositionInput(float3 V, float depthOffsetVS, float3 viewForwardDir, float4x4 viewProjMatrix, inout PositionInputs posInput)
{
    posInput.positionWS += depthOffsetVS * (-V);
    posInput.deviceDepth = ComputeNormalizedDeviceCoordinatesWithZ(posInput.positionWS, viewProjMatrix).z;

    // Transform the displacement along the view vector to the displacement along the forward vector.
    // Use abs() to make sure we get the sign right.
    // 'depthOffsetVS' applies in the direction away from the camera.
    posInput.linearDepth += depthOffsetVS * abs(dot(V, viewForwardDir));
}

// ----------------------------------------------------------------------------
// Misc utilities
// ----------------------------------------------------------------------------

// Simple function to test a bitfield
bool HasFlag(uint bitfield, uint flag)
{
    return (bitfield & flag) != 0;
}

// Normalize that account for vectors with zero length
float3 SafeNormalize(float3 inVec)
{
    float dp3 = max(FLT_MIN, dot(inVec, inVec));
    return inVec * rsqrt(dp3);
}

// Checks if a vector is normalized
bool IsNormalized(float3 inVec)
{
    float l = length(inVec);
    return length(l) < 1.0001 && length(l) > 0.9999;
}

// Division which returns 1 for (inf/inf) and (0/0).
// If any of the input parameters are NaNs, the result is a NaN.
float SafeDiv(float numer, float denom)
{
    return (numer != denom) ? numer / denom : 1;
}

// Perform a square root safe of imaginary number.
float SafeSqrt(float x)
{
    return sqrt(max(0, x));
}

// Assumes that (0 <= x <= Pi).
float SinFromCos(float cosX)
{
    return sqrt(saturate(1 - cosX * cosX));
}

// Dot product in spherical coordinates.
float SphericalDot(float cosTheta1, float phi1, float cosTheta2, float phi2)
{
    return SinFromCos(cosTheta1) * SinFromCos(cosTheta2) * cos(phi1 - phi2) + cosTheta1 * cosTheta2;
}

// Generates a triangle in homogeneous clip space, s.t.
// v0 = (-1, -1, 1), v1 = (3, -1, 1), v2 = (-1, 3, 1).
float2 GetFullScreenTriangleTexCoord(uint vertexID)
{
    return float2((vertexID << 1) & 2, vertexID & 2);
}

float4 GetFullScreenTriangleVertexPosition(uint vertexID, float z = UNITY_NEAR_CLIP_VALUE)
{
    // note: the triangle vertex position coordinates are x2 so the returned UV coordinates are in range -1, 1 on the screen.
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    float4 pos = float4(uv * 2.0 - 1.0, z, 1.0);
    return pos;
}


// draw procedural with 2 triangles has index order (0,1,2)  (0,2,3)

// 0 - 0,0
// 1 - 0,1
// 2 - 1,1
// 3 - 1,0

float2 GetQuadTexCoord(uint vertexID)
{
    uint topBit = vertexID >> 1;
    uint botBit = (vertexID & 1);
    float u = topBit;
    float v = (topBit + botBit) & 1; // produces 0 for indices 0,3 and 1 for 1,2
    return float2(u, v);
}

// 0 - 0,1
// 1 - 0,0
// 2 - 1,0
// 3 - 1,1
float4 GetQuadVertexPosition(uint vertexID, float z = UNITY_NEAR_CLIP_VALUE)
{
    uint topBit = vertexID >> 1;
    uint botBit = (vertexID & 1);
    float x = topBit;
    float y = 1 - (topBit + botBit) & 1; // produces 1 for indices 0,3 and 0 for 1,2
    float4 pos = float4(x, y, z, 1.0);
    return pos;
}

// LOD dithering transition helper
// LOD0 must use this function with ditherFactor 1..0
// LOD1 must use this function with ditherFactor -1..0
// This is what is provided by unity_LODFade
void LODDitheringTransition(uint2 fadeMaskSeed, float ditherFactor)
{
    // Generate a spatially varying pattern.
    // Unfortunately, varying the pattern with time confuses the TAA, increasing the amount of noise.
    float p = GenerateHashedRandomFloat(fadeMaskSeed);

    // This preserves the symmetry s.t. if LOD 0 has f = x, LOD 1 has f = -x.
    float f = ditherFactor - CopySign(p, ditherFactor);
    clip(f);
}

// The resource that is bound when binding a stencil buffer from the depth buffer is two channel. On D3D11 the stencil value is in the green channel,
// while on other APIs is in the red channel. Note that on some platform, always using the green channel might work, but is not guaranteed.
uint GetStencilValue(uint2 stencilBufferVal)
{
    return stencilBufferVal.y;
}

// Sharpens the alpha of a texture to the width of a single pixel
// Used for alpha to coverage
// source: https://medium.com/@bgolus/anti-aliased-alpha-test-the-esoteric-alpha-to-coverage-8b177335ae4f
float SharpenAlpha(float alpha, float alphaClipTreshold)
{
    return saturate((alpha - alphaClipTreshold) / max(fwidth(alpha), 0.0001) + 0.5);
}

// These clamping function to max of floating point 16 bit are use to prevent INF in code in case of extreme value
TEMPLATE_1_FLT(ClampToFloat16Max, value, return min(value, HALF_MAX))

float2 RepeatOctahedralUV(float u, float v)
{
    float2 uv;

    if (u < 0.0f)
    {
        if (v < 0.0f)
            uv = float2(1.0f + u, 1.0f + v);
        else if (v < 1.0f)
            uv = float2(-u, 1.0f - v);
        else
            uv = float2(1.0f + u, v - 1.0f);
    }
    else if (u < 1.0f)
    {
        if (v < 0.0f)
            uv = float2(1.0f - u, -v);
        else if (v < 1.0f)
            uv = float2(u, v);
        else
            uv = float2(1.0f - u, 2.0f - v);
    }
    else
    {
        if (v < 0.0f)
            uv = float2(u - 1.0f, 1.0f + v);
        else if (v < 1.0f)
            uv = float2(2.0f - u, 1.0f - v);
        else
            uv = float2(u - 1.0f, v - 1.0f);
    }

    return uv;
}

// ----------------------------------------------------------------------------
// Custom utilities
// ----------------------------------------------------------------------------

float3x3 Cofactor(const float3x3 m)
{
    float a = m[0][0];
    float b = m[1][0];
    float c = m[2][0];
    float d = m[0][1];
    float e = m[1][1];
    float f = m[2][1];
    float g = m[0][2];
    float h = m[1][2];
    float i = m[2][2];

    float3x3 cof;
    cof[0][0] = e * i - f * h;
    cof[0][1] = c * h - b * i;
    cof[0][2] = b * f - c * e;
    cof[1][0] = f * g - d * i;
    cof[1][1] = a * i - c * g;
    cof[1][2] = c * d - a * f;
    cof[2][0] = d * h - e * g;
    cof[2][1] = b * g - a * h;
    cof[2][2] = a * e - b * d;
    return cof;
}

float3 Faceforward(float3 n, float3 v)
{
    if(dot(n, v) < 0.0f)
        return -n;
    else
        return n;
}

float SphericalTheta(float3 v)
{
    return acos(clamp(v.z, -1.0f, 1.0f));
}

float SphericalPhi(float3 v)
{
    float p = atan2(v.y, v.x);
    if(p < 0)
        return p + TWO_PI;
    else
        return p;
}

void CoordinateSystem(float3 v1, out float3 v2, out float3 v3)
{
    float sign = (v1.z >= 0.0f) * 2.0f - 1.0f; // copysign(1.0f, v1.z); // No HLSL support yet
    float a	   = -1.0f / (sign + v1.z);
    float b	   = v1.x * v1.y * a;
    v2		   = float3(1.0f + sign * v1.x * v1.x * a, sign * b, -sign * v1.x);
    v3		   = float3(b, sign + v1.y * v1.y * a, -v1.y);
}

struct Frame
{
    float3 x;
    float3 y;
    float3 z;

    float3 ToWorld(float3 v) { return x * v.x + y * v.y + z * v.z; }
    float3 ToLocal(float3 v) { return float3(dot(v, x), dot(v, y), dot(v, z)); }
};

Frame InitFrameFromZ(float3 z)
{
    Frame frame;
    frame.z = z;
    CoordinateSystem(frame.z, frame.x, frame.y);
    return frame;
}

Frame InitFrameFromXY(float3 x, float3 y)
{
    Frame frame;
    frame.x = x;
    frame.y = y;
    frame.z = cross(x, y);
    return frame;
}

#define PLANE_INTERSECTION_POSITIVE_HALFSPACE 0
#define PLANE_INTERSECTION_NEGATIVE_HALFSPACE 1
#define PLANE_INTERSECTION_INTERSECTING		  2

#define CONTAINMENT_DISJOINT				  0
#define CONTAINMENT_INTERSECTS				  1
#define CONTAINMENT_CONTAINS				  2

// ax + by + cz = d where d = dot(n, P)
struct Plane
{
    float3 Normal; // Plane normal. Points x on the plane satisfy dot(n, x) = d
    float  Offset; // d = dot(n, p) for a given point p on the plane
};

struct BSphere
{
    float3 Center;
    float  Radius;

    bool Intersects(BSphere Other)
    {
        // The distance between the sphere centers is computed and compared
        // against the sum of the sphere radii. To avoid an square root operation, the
        // squared distances are compared with squared sum radii instead.
        float3 d		 = Center - Other.Center;
        float  dist2	 = dot(d, d);
        float  radiusSum = Radius + Other.Radius;
        float  r2		 = radiusSum * radiusSum;
        return dist2 <= r2;
    }
};

struct BoundingBox
{
    float4 Center;
    float4 Extents;

    bool Intersects(BoundingBox Other)
    {
        float3 minA = Center.xyz - Extents.xyz;
        float3 maxA = Center.xyz + Extents.xyz;

        float3 minB = Other.Center.xyz - Other.Extents.xyz;
        float3 maxB = Other.Center.xyz + Other.Extents.xyz;

        // All axis needs to overlap for a intersection
        return maxA.x >= minB.x && minA.x <= maxB.x && // Overlap on x-axis?
               maxA.y >= minB.y && minA.y <= maxB.y && // Overlap on y-axis?
               maxA.z >= minB.z && minA.z <= maxB.z;   // Overlap on z-axis?
    }

    // https://stackoverflow.com/questions/6053522/how-to-recalculate-axis-aligned-bounding-box-after-translate-rotate
    void Transform(float4x4 m)
    {
        BoundingBox b = (BoundingBox)0;
        float3 t = float3(m[0][3], m[1][3], m[2][3]);
        b.Center  = float4(t, 0);
        b.Extents = float4(0.0f, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                b.Center[i] += m[i][j] * Center[j];
                b.Extents[i] += abs(m[i][j]) * Extents[j];
            }
        }
        Center = b.Center;
        Extents = b.Extents;
    }
};

Plane ComputePlane(float3 a, float3 b, float3 c)
{
    Plane plane;
    plane.Normal = normalize(cross(b - a, c - a));
    plane.Offset = dot(plane.Normal, a);
    return plane;
}

struct Frustum
{
    Plane Left;	  // -x
    Plane Right;  // +x
    Plane Bottom; // -y
    Plane Top;	  // +y
    Plane Near;	  // -z
    Plane Far;	  // +z
};

int BoundingSphereToPlane(BSphere s, Plane p)
{
    // Compute signed distance from plane to sphere center
    float sd = dot(s.Center, p.Normal) - p.Offset;
    if (sd > s.Radius)
    {
        return PLANE_INTERSECTION_POSITIVE_HALFSPACE;
    }
    if (sd < -s.Radius)
    {
        return PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
    }
    return PLANE_INTERSECTION_INTERSECTING;
}

int BoundingBoxToPlane(BoundingBox b, Plane p)
{
	// Compute signed distance from plane to box center
	float sd = dot(b.Center.xyz, p.Normal.xyz) - p.Offset;

	// Compute the projection interval radius of b onto L(t) = b.Center + t * p.Normal
	// Projection radii r_i of the 8 bounding box vertices
	// r_i = dot((V_i - C), n)
	// r_i = dot((C +- e0*u0 +- e1*u1 +- e2*u2 - C), n)
	// Cancel C and distribute dot product
	// r_i = +-(dot(e0*u0, n)) +-(dot(e1*u1, n)) +-(dot(e2*u2, n))
	// We take the maximum position radius by taking the absolute value of the terms, we assume Extents to be positive
	// r = e0*|dot(u0, n)| + e1*|dot(u1, n)| + e2*|dot(u2, n)|
	// When the separating axis vector Normal is not a unit vector, we need to divide the radii by the length(Normal)
	// u0,u1,u2 are the local axes of the box, which is = [(1,0,0), (0,1,0), (0,0,1)] respectively for axis aligned bb
	float r = dot(b.Extents.xyz, abs(p.Normal.xyz));

	if (sd > r)
	{
		return PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	}
	if (sd < -r)
	{
		return PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	}
	return PLANE_INTERSECTION_INTERSECTING;
}

int FrustumContainsBoundingSphere(Frustum f, BSphere s)
{
	int	 p0			= BoundingSphereToPlane(s, f.Left);
	int	 p1			= BoundingSphereToPlane(s, f.Right);
	int	 p2			= BoundingSphereToPlane(s, f.Bottom);
	int	 p3			= BoundingSphereToPlane(s, f.Top);
	int	 p4			= BoundingSphereToPlane(s, f.Near);
	int	 p5			= BoundingSphereToPlane(s, f.Far);
	bool anyOutside = p0 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p1 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p2 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p3 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p4 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p5 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	bool allInside = p0 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p1 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p2 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p3 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p4 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p5 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;

	if (anyOutside)
	{
		return CONTAINMENT_DISJOINT;
	}

	if (allInside)
	{
		return CONTAINMENT_CONTAINS;
	}

	return CONTAINMENT_INTERSECTS;
}

int FrustumContainsBoundingBox(Frustum f, BoundingBox b)
{
	int	 p0			= BoundingBoxToPlane(b, f.Left);
	int	 p1			= BoundingBoxToPlane(b, f.Right);
	int	 p2			= BoundingBoxToPlane(b, f.Bottom);
	int	 p3			= BoundingBoxToPlane(b, f.Top);
	int	 p4			= BoundingBoxToPlane(b, f.Near);
	int	 p5			= BoundingBoxToPlane(b, f.Far);
	bool anyOutside = p0 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p1 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p2 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p3 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p4 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p5 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	bool allInside = p0 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p1 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p2 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p3 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p4 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p5 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;

	if (anyOutside)
	{
		return CONTAINMENT_DISJOINT;
	}

	if (allInside)
	{
		return CONTAINMENT_CONTAINS;
	}

	return CONTAINMENT_INTERSECTS;
}

Frustum ExtractPlanesDX(const float4x4 mvp)
{
    Frustum frustum;

    // Left clipping plane
    frustum.Left.Normal.x = mvp[3][0] + mvp[0][0];
    frustum.Left.Normal.y = mvp[3][1] + mvp[0][1];
    frustum.Left.Normal.z = mvp[3][2] + mvp[0][2];
    frustum.Left.Offset   = -(mvp[3][3] + mvp[0][3]);
    // Right clipping plane
    frustum.Right.Normal.x = mvp[3][0] - mvp[0][0];
    frustum.Right.Normal.y = mvp[3][1] - mvp[0][1];
    frustum.Right.Normal.z = mvp[3][2] - mvp[0][2];
    frustum.Right.Offset   = -(mvp[3][3] - mvp[0][3]);
    // Bottom clipping plane
    frustum.Bottom.Normal.x = mvp[3][0] + mvp[1][0];
    frustum.Bottom.Normal.y = mvp[3][1] + mvp[1][1];
    frustum.Bottom.Normal.z = mvp[3][2] + mvp[1][2];
    frustum.Bottom.Offset   = -(mvp[3][3] + mvp[1][3]);
    // Top clipping plane
    frustum.Top.Normal.x = mvp[3][0] - mvp[1][0];
    frustum.Top.Normal.y = mvp[3][1] - mvp[1][1];
    frustum.Top.Normal.z = mvp[3][2] - mvp[1][2];
    frustum.Top.Offset   = -(mvp[3][3] - mvp[1][3]);
    // Far clipping plane
    frustum.Far.Normal.x = mvp[2][0];
    frustum.Far.Normal.y = mvp[2][1];
    frustum.Far.Normal.z = mvp[2][2];
    frustum.Far.Offset   = -(mvp[2][3]);
    // Near clipping plane
    frustum.Near.Normal.x = mvp[3][0] - mvp[2][0];
    frustum.Near.Normal.y = mvp[3][1] - mvp[2][1];
    frustum.Near.Normal.z = mvp[3][2] - mvp[2][2];
    frustum.Near.Offset   = -(mvp[3][3] - mvp[2][3]);

    return frustum;
}

#define PX  0     // left            +----+
#define NX  1     // right           | PY |
#define PY  2     // bottom     +----+----+----+----+
#define NY  3     // top        | NX | PZ | PX | NZ |
#define PZ  4     // back       +----+----+----+----+
#define NZ  5     // front           | NY |
                  //                 +----+

#define Face uint

struct CubemapAddress
{
    Face face;
    float2 st;
};

float3 GetDirectionForCubemap(uint face, float2 uv)
{
    // map [0, dim] to [-1,1] with (-1,-1) at bottom left
    float cx = (uv.x * 2.0) - 1;
    float cy = 1 - (uv.y * 2.0);    // <- not entirely sure about this bit

    float3 dir;
    const float l = sqrt(cx * cx + cy * cy + 1);
    switch (face) 
    {
    case 0:  dir = float3(   1, cy, -cx ); break;  // PX
    case 1:  dir = float3(  -1, cy,  cx ); break;  // NX
    case 2:  dir = float3(  cx,  1, -cy ); break;  // PY
    case 3:  dir = float3(  cx, -1,  cy ); break;  // NY
    case 4:  dir = float3(  cx, cy,   1 ); break;  // PZ
    case 5:  dir = float3( -cx, cy,  -1 ); break;  // NZ
    default: dir = float3(0, 0, 0); break;
    }
    return dir * (1 / l);
}

float3 GetDirectionForCubemap( uint cubeDim, uint face, uint ux, uint uy )
{
    return GetDirectionForCubemap( face, float2(ux+0.5,uy+0.5) / cubeDim.xx );
}

CubemapAddress GetAddressForCubemap(float3 r)
{
    CubemapAddress addr;
    float sc, tc, ma;
    const float rx = abs(r.x);
    const float ry = abs(r.y);
    const float rz = abs(r.z);
    if (rx >= ry && rx >= rz)
    {
        ma = 1.0f / rx;
        if (r.x >= 0)
        {
            addr.face = PX;
            sc = -r.z;
            tc = -r.y;
        }
        else
        {
            addr.face = NX;
            sc = r.z;
            tc = -r.y;
        }
    }
    else if (ry >= rx && ry >= rz)
    {
        ma = 1.0f / ry;
        if (r.y >= 0)
        {
            addr.face = PY;
            sc = r.x;
            tc = r.z;
        }
        else
        {
            addr.face = NY;
            sc = r.x;
            tc = -r.z;
        }
    }
    else
    {
        ma = 1.0f / rz;
        if (r.z >= 0)
        {
            addr.face = PZ;
            sc = r.x;
            tc = -r.y;
        }
        else
        {
            addr.face = NZ;
            sc = -r.x;
            tc = -r.y;
        }
    }
    // ma is guaranteed to be >= sc and tc
    addr.st = float2((sc * ma + 1.0f) * 0.5f, (tc * ma + 1.0f) * 0.5f);
    return addr;
}

/*
 * Area of a cube face's quadrant projected onto a sphere
 *
 *  1 +---+----------+
 *    |   |          |
 *    |---+----------|
 *    |   |(x,y)     |
 *    |   |          |
 *    |   |          |
 * -1 +---+----------+
 *   -1              1
 *
 *
 * The quadrant (-1,1)-(x,y) is projected onto the unit sphere
 *
 */
float SphereQuadrantArea(float x, float y)
{
    return atan2(x*y, sqrt(x*x + y*y + 1));
}

//! computes the solid angle of a pixel of a face of a cubemap
float SolidAngle(uint dim, uint u, uint v)
{
    const float iDim = 1.0f / dim;
    float s = ((u + 0.5f) * 2 * iDim) - 1;
    float t = ((v + 0.5f) * 2 * iDim) - 1;
    const float x0 = s - iDim;
    const float y0 = t - iDim;
    const float x1 = s + iDim;
    const float y1 = t + iDim;
    float solidAngle =
        SphereQuadrantArea(x0, y0) - SphereQuadrantArea(x0, y1) -
        SphereQuadrantArea(x1, y0) + SphereQuadrantArea(x1, y1);
    return solidAngle;
}

// for Texel to Vertex mapping (3x3 kernel, the texel centers are at quad vertices)
float3 ComputeHeightmapNormal( float h00, float h10, float h20, float h01, float h11, float h21, float h02, float h12, float h22, const float3 pixelWorldSize )
{
    // Sobel 3x3
    //    0,0 | 1,0 | 2,0
    //    ----+-----+----
    //    0,1 | 1,1 | 2,1
    //    ----+-----+----
    //    0,2 | 1,2 | 2,2

    h00 -= h11;
    h10 -= h11;
    h20 -= h11;
    h01 -= h11;
    h21 -= h11;
    h02 -= h11;
    h12 -= h11;
    h22 -= h11;
   
    // The Sobel X kernel is:
    //
    // [ 1.0  0.0  -1.0 ]
    // [ 2.0  0.0  -2.0 ]
    // [ 1.0  0.0  -1.0 ]
	
    float Gx = h00 - h20 + 2.0 * h01 - 2.0 * h21 + h02 - h22;
				
    // The Sobel Y kernel is:
    //
    // [  1.0    2.0    1.0 ]
    // [  0.0    0.0    0.0 ]
    // [ -1.0   -2.0   -1.0 ]
	
    float Gy = h00 + 2.0 * h10 + h20 - h02 - 2.0 * h12 - h22;
	
    // The 0.5f leading coefficient can be used to control
    // how pronounced the bumps are - less than 1.0 enhances
    // and greater than 1.0 smoothes.
	
    //return float4( 0, 0, 0, 0 );
	
    float stepX = pixelWorldSize.x;
    float stepY = pixelWorldSize.y;
    float sizeZ = pixelWorldSize.z;
   
    Gx = Gx * stepY * sizeZ;
    Gy = Gy * stepX * sizeZ;
	
    float Gz = stepX * stepY * 8;
	
    return normalize( float3( Gx, Gy, Gz ) );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A couple of nice gradient for various visualization
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// from https://www.shadertoy.com/view/lt2GDc - New Gradients from (0-1 float) Created by ChocoboBreeder in 2015-Jun-3
float3 GradientPalette( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}
// rainbow gradient
float3 GradientRainbow( in float t )
{
    return GradientPalette( t, float3(0.55,0.4,0.3), float3(0.50,0.51,0.35)+0.1, float3(0.8,0.75,0.8), float3(0.075,0.33,0.67)+0.21 );
}
// from https://www.shadertoy.com/view/llKGWG - Heat map, Created by joshliebe in 2016-Oct-15
float3 GradientHeatMap( in float greyValue )
{
    float3 heat;      
    heat.r = smoothstep(0.5, 0.8, greyValue);
    if(greyValue >= 0.90) {
        heat.r *= (1.1 - greyValue) * 5.0;
    }
    if(greyValue > 0.7) {
        heat.g = smoothstep(1.0, 0.7, greyValue);
    } else {
        heat.g = smoothstep(0.0, 0.7, greyValue);
    }    
    heat.b = smoothstep(1.0, 0.0, greyValue);          
    if(greyValue <= 0.3) {
        heat.b *= greyValue / 0.3;     
    }
    return heat;
}

// Manual bilinear filter: input 'coords' is standard [0, 1] texture uv coords multiplied by [textureWidth, textureHeight] minus [0.5, 0.5]
float BilinearFilter( float c00, float c10, float c01, float c11, float2 coords )
{
    float2 intPt    = floor(coords);
    float2 fractPt  = frac(coords);
    float top       = lerp( c00, c10, fractPt.x );
    float bottom    = lerp( c01, c11, fractPt.x );
    return lerp( top, bottom, fractPt.y );
}
//
float3 BilinearFilter( float3 c00, float3 c10, float3 c01, float3 c11, float2 coords )
{
    float2 intPt    = floor(coords);
    float2 fractPt  = frac(coords);
    float3 top      = lerp( c00, c10, fractPt.x );
    float3 bottom   = lerp( c01, c11, fractPt.x );
    return lerp( top, bottom, fractPt.y );
}
//
float4 BilinearFilter( float4 c00, float4 c10, float4 c01, float4 c11, float2 coords )
{
    float2 intPt    = floor(coords);
    float2 fractPt  = frac(coords);
    float4 top      = lerp( c00, c10, fractPt.x );
    float4 bottom   = lerp( c01, c11, fractPt.x );
    return lerp( top, bottom, fractPt.y );
}

float FLOAT_to_SRGB( float val )
{
    if( val < 0.0031308 )
        val *= float( 12.92 );
    else
        val = float( 1.055 ) * pow( abs( val ), float( 1.0 ) / float( 2.4 ) ) - float( 0.055 );
    return val;
}
//
float3 FLOAT3_to_SRGB( float3 val )
{
    float3 outVal;
    outVal.x = FLOAT_to_SRGB( val.x );
    outVal.y = FLOAT_to_SRGB( val.y );
    outVal.z = FLOAT_to_SRGB( val.z );
    return outVal;
}
//
float SRGB_to_FLOAT( float val )
{
    if( val < 0.04045 )
        val /= float( 12.92 );
    else
        val = pow( abs( val + float( 0.055 ) ) / float( 1.055 ), float( 2.4 ) );
    return val;
}
//
float3 SRGB_to_FLOAT3( float3 val )
{
    float3 outVal;
    outVal.x = SRGB_to_FLOAT( val.x );
    outVal.y = SRGB_to_FLOAT( val.y );
    outVal.z = SRGB_to_FLOAT( val.z );
    return outVal;
}

// Interpolation between two uniformly distributed random values using Cumulative Distribution Function
// (see page 4 http://cwyman.org/papers/i3d17_hashedAlpha.pdf or https://en.wikipedia.org/wiki/Cumulative_distribution_function)
float LerpCDF( float lhs, float rhs, float s )
{
    // Interpolate alpha threshold from noise at two scales 
    float x = (1-s)*lhs + s*rhs;

    // Pass into CDF to compute uniformly distrib threshold 
    float a = min( s, 1-s ); 
    float3 cases = float3( x*x/(2*a*(1-a)), (x-0.5*a)/(1-a), 1.0-((1-x)*(1-x)/(2*a*(1-a))) );

    // Find our final, uniformly distributed alpha threshold 
    return (x < (1-a)) ? ((x < a) ? cases.x : cases.y) : cases.z;
}

// From DXT5_NM standard
float3 UnpackNormalDXT5_NM( float4 packedNormal )
{
    float3 normal;
    normal.xy = packedNormal.wy * 2.0 - 1.0;
    normal.z = sqrt( 1.0 - normal.x*normal.x - normal.y * normal.y );
    return normal;
}

float3 DisplayNormal( float3 normal )
{
    return normal * 0.5 + 0.5;
}

float3 DisplayNormalSRGB( float3 normal )
{
    return pow( abs( normal * 0.5 + 0.5 ), 2.2 );
}

float CalcLuminance( float3 color )
{
    // https://en.wikipedia.org/wiki/Relative_luminance - Rec. 709
    return max( FLT_MIN, dot(color, float3(0.2126f, 0.7152f, 0.0722f) ) );
}
//
// color -> log luma conversion used for edge detection
float RGBToLumaForEdges( float3 linearRGB )
{
#if 0
    // this matches Miniengine luma path
    float Luma = dot( linearRGB, float3(0.212671, 0.715160, 0.072169) );
    return log2(1 + Luma * 15) / 4;
#else
    // this is what original FXAA (and consequently CMAA2) use by default - these coefficients correspond to Rec. 601 and those should be
    // used on gamma-compressed components (see https://en.wikipedia.org/wiki/Luma_(video)#Rec._601_luma_versus_Rec._709_luma_coefficients), 
    float luma = dot( sqrt( linearRGB.rgb ), float3( 0.299, 0.587, 0.114 ) );  // http://en.wikipedia.org/wiki/CCIR_601
    // using sqrt luma for now but log luma like in miniengine provides a nicer curve on the low-end
    return luma;
#endif
}

// The following code is licensed under the MIT license: https://gist.github.com/TheRealMJP/bc503b0b87b643d3505d41eab8b332ae
//
// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ / https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1 for more details
float4 SampleBicubic9(in Texture2D<float4> tex, in SamplerState linearSampler, in float2 uv) // a.k.a. SampleTextureCatmullRom
{
    float2 texSize; tex.GetDimensions( texSize.x, texSize.y );
    float2 invTexSize = 1.f / texSize;

    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    float2 samplePos = uv * texSize;
    float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    float2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    float2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    float2 w12 = w1 + w2;
    float2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    float2 texPos0 = texPos1 - 1;
    float2 texPos3 = texPos1 + 2;
    float2 texPos12 = texPos1 + offset12;

    texPos0  *= invTexSize;
    texPos3  *= invTexSize;
    texPos12 *= invTexSize;

    float4 result = 0.0f;
    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;     // apparently for 5-tap version it's ok to just remove these
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;     // apparently for 5-tap version it's ok to just remove these

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;     // apparently for 5-tap version it's ok to just remove these
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;     // apparently for 5-tap version it's ok to just remove these

    return result;
}

#endif // COMMON_INCLUDED
