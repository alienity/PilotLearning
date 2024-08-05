//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include "runtime/core/math/moyu_math2.h"
#include "Textures.h"

namespace SampleFramework11
{

// Constants
static const float CosineA0 = MoYu::f::PI;
static const float CosineA1 = (2.0f * MoYu::f::PI) / 3.0f;
static const float CosineA2 = MoYu::f::PI / 4.0f;

template<typename T, glm::uint64 N> class SH
{

protected:

    void Assign(const SH& other)
    {
        for(glm::uint64 i = 0; i < N; ++i)
            Coefficients[i] = other.Coefficients[i];
    }

public:

    T Coefficients[N];

    SH()
    {
        for(glm::uint64 i = 0; i < N; ++i)
            Coefficients[i] = 0.0f;
    }

    // Operator overloads
    T& operator[](glm::uint64 idx)
    {
        return Coefficients[idx];
    }

    T operator[](glm::uint64 idx) const
    {
        return Coefficients[idx];
    }

    SH& operator+=(const SH& other)
    {
        for(glm::uint64 i = 0; i < N; ++i)
            Coefficients[i] += other.Coefficients[i];
        return *this;
    }

    SH operator+(const SH& other) const
    {
        SH result;
        for(glm::uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] + other.Coefficients[i];
        return result;
    }

    SH& operator-=(const SH& other)
    {
        for(glm::uint64 i = 0; i < N; ++i)
            Coefficients[i] -= other.Coefficients[i];
        return *this;
    }

    SH operator-(const SH& other) const
    {
        SH result;
        for(glm::uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] - other.Coefficients[i];
        return result;
    }

    SH& operator*=(const T& scale)
    {
        for(glm::uint64 i = 0; i < N; ++i)
            Coefficients[i] *= scale;
        return *this;
    }

    SH& operator*=(float scale)
    {
        for(glm::uint64 i = 0; i < N; ++i)
            Coefficients[i] *= scale;
        return *this;
    }

    SH operator*(const SH& other) const
    {
        SH result;
        for(glm::uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] * other.Coefficients[i];
        return result;
    }

    SH& operator*=(const SH& other)
    {
        for(glm::uint64 i = 0; i < N; ++i)
            Coefficients[i] *= other.Coefficients[i];
        return *this;
    }

    SH operator*(const T& scale) const
    {
        SH result;
        for(glm::uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] * scale;
        return result;
    }

    SH operator*(const T scale) const
    {
        SH result;
        for(glm::uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] * scale;
        return result;
    }

    SH operator*(float scale) const
    {
        SH result;
        for(glm::uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] * scale;
        return result;
    }

    SH& operator/=(const T& scale)
    {
        for(glm::uint32 i = 0; i < N; ++i)
            Coefficients[i] /= scale;
        return *this;
    }

    SH operator/(const T& scale) const
    {
        SH result;
        for(glm::uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] / scale;
        return result;
    }

    SH operator/(const SH& other) const
    {
        SH result;
        for(glm::uint64 i = 0; i < N; ++i)
            result.Coefficients[i] = Coefficients[i] / other.Coefficients[i];
        return result;
    }

    SH& operator/=(const SH& other)
    {
        for(glm::uint64 i = 0; i < N; ++i)
            Coefficients[i] /= other.Coefficients[i];
        return *this;
    }

    // Dot products
    T Dot(const SH& other) const
    {
        T result = 0.0f;
        for(glm::uint64 i = 0; i < N; ++i)
            result += Coefficients[i] * other.Coefficients[i];
        return result;
    }

    static T Dot(const SH& a, const SH& b)
    {
        T result = 0.0f;
        for(glm::uint64 i = 0; i < N; ++i)
            result += a.Coefficients[i] * b.Coefficients[i];
        return result;
    }

    // Convolution with cosine kernel
    void ConvolveWithCosineKernel()
    {
        Coefficients[0] *= CosineA0;

        for(glm::uint64 i = 1; i < N; ++i)
            if(i < 4)
                Coefficients[i] *= CosineA1;
            else if(i < 9)
                Coefficients[i] *= CosineA2;
    }
};

// Spherical Harmonics
typedef SH<float, 4> SH4;
typedef SH<glm::float3, 4> SH4Color;
typedef SH<float, 9> SH9;
typedef SH<glm::float3, 9> SH9Color;

// H-basis
typedef SH<float, 4> H4;
typedef SH<glm::float3, 4> H4Color;
typedef SH<float, 6> H6;
typedef SH<glm::float3, 6> H6Color;

// For proper alignment with shader constant buffers
struct ShaderSH9Color
{
    glm::float4 Coefficients[9];

    ShaderSH9Color()
    {
    }

    ShaderSH9Color(const SH9Color& sh9Clr)
    {
        for(glm::uint32 i = 0; i < 9; ++i)
            Coefficients[i] = glm::float4(sh9Clr.Coefficients[i], 0.0f);
    }
};

SH4 ProjectOntoSH4(const glm::float3& dir);
SH4Color ProjectOntoSH4Color(const glm::float3& dir, const glm::float3& color);
glm::float3 EvalSH4Cosine(const glm::float3& dir, const SH4Color& sh);

SH9 ProjectOntoSH9(const glm::float3& dir);
SH9Color ProjectOntoSH9Color(const glm::float3& dir, const glm::float3& color);
glm::float3 EvalSH9Cosine(const glm::float3& dir, const SH9Color& sh);

// H-basis functions
H4 ProjectOntoH4(const glm::float3& dir);
H4Color ProjectOntoH4Color(const glm::float3& dir, const glm::float3& color);
float EvalH4(const H4& h, const glm::float3& dir);
H4 ConvertToH4(const SH9& sh);
H4Color ConvertToH4(const SH9Color& sh);
H6 ConvertToH6(const SH9& sh);
H6Color ConvertToH6(const SH9Color& sh);

 // Lighting environment generation functions
SH9Color ProjectCubemapToSH(MoYu::MoYuScratchImage* cubeMap);

}