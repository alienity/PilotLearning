//-------------------------------------------------------------------------------------
// SimpleMath.h -- Simplified C++ Math wrapper for DirectXMath
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//-------------------------------------------------------------------------------------

#pragma once

#include "runtime/core/meta/reflection/reflection.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>

#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

namespace Pilot
{
//------------------------------------------------------------------------------
#define XMStoreFloat2(A, B) XMStoreFloat2(reinterpret_cast<DirectX::XMFLOAT2*>(A), B)
#define XMLoadFloat2(A) XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(A))
#define XMVector2TransformCoordStream(A, B, C, D, E, F) \
    XMVector2TransformCoordStream( \
        reinterpret_cast<DirectX::XMFLOAT2*>(A), B, reinterpret_cast<const DirectX::XMFLOAT2*>(C), D, E, F)
#define XMVector2TransformStream(A, B, C, D, E, F) \
    XMVector2TransformStream( \
        reinterpret_cast<DirectX::XMFLOAT4*>(A), B, reinterpret_cast<const DirectX::XMFLOAT2*>(C), D, E, F)
#define XMVector2TransformNormalStream(A, B, C, D, E, F) \
    XMVector2TransformNormalStream( \
        reinterpret_cast<DirectX::XMFLOAT2*>(A), B, reinterpret_cast<const DirectX::XMFLOAT2*>(C), D, E, F)

#define XMStoreFloat3(A, B) XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(A), B)
#define XMLoadFloat3(A) XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(A))
#define XMVector3TransformCoordStream(A, B, C, D, E, F) \
    XMVector3TransformCoordStream( \
        reinterpret_cast<DirectX::XMFLOAT3*>(A), B, reinterpret_cast<const DirectX::XMFLOAT3*>(C), D, E, F)
#define XMVector3TransformStream(A, B, C, D, E, F) \
    XMVector3TransformStream( \
        reinterpret_cast<DirectX::XMFLOAT4*>(A), B, reinterpret_cast<const DirectX::XMFLOAT3*>(C), D, E, F)
#define XMVector3TransformNormalStream(A, B, C, D, E, F) \
    XMVector3TransformNormalStream( \
        reinterpret_cast<DirectX::XMFLOAT3*>(A), B, reinterpret_cast<const DirectX::XMFLOAT3*>(C), D, E, F)

#define XMStoreFloat4(A, B) XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(A), B)
#define XMLoadFloat4(A) XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT4*>(A))
#define XMVector4TransformStream(A, B, C, D, E, F) \
    XMVector4TransformStream( \
        reinterpret_cast<DirectX::XMFLOAT4*>(A), B, reinterpret_cast<const DirectX::XMFLOAT4*>(C), D, E, F)

//------------------------------------------------------------------------------


    using namespace DirectX;

    struct Float2;
    struct Float4;
    struct Float4x4;
    struct QuaternionN;
    struct PlaneN;

    //------------------------------------------------------------------------------
    // 2D rectangle
    REFLECTION_TYPE(Rectangle)
    STRUCT(Rectangle, Fields)
    {
        REFLECTION_BODY(Rectangle);

        int x;
        int y;
        int width;
        int height;

        // Creators
        Rectangle() noexcept : x(0), y(0), width(0), height(0) {}
        constexpr Rectangle(long ix, long iy, long iw, long ih) noexcept : x(ix), y(iy), width(iw), height(ih) {}

        Rectangle(const Rectangle&) = default;
        Rectangle& operator=(const Rectangle&) = default;

        Rectangle(Rectangle&&) = default;
        Rectangle& operator=(Rectangle&&) = default;

        bool operator==(const Rectangle& r) const noexcept
        {
            return (x == r.x) && (y == r.y) && (width == r.width) && (height == r.height);
        }
        bool operator!=(const Rectangle& r) const noexcept
        {
            return (x != r.x) || (y != r.y) || (width != r.width) || (height != r.height);
        }

        // Rectangle operations
        Float2 Location() const noexcept;
        Float2 Center() const noexcept;

        bool IsEmpty() const noexcept { return (width == 0 && height == 0 && x == 0 && y == 0); }

        bool Contains(long ix, long iy) const noexcept
        {
            return (x <= ix) && (ix < (x + width)) && (y <= iy) && (iy < (y + height));
        }
        bool Contains(const Float2& point) const noexcept;
        bool Contains(const Rectangle& r) const noexcept
        {
            return (x <= r.x) && ((r.x + r.width) <= (x + width)) && (y <= r.y) &&
                    ((r.y + r.height) <= (y + height));
        }
        void Inflate(long horizAmount, long vertAmount) noexcept;

        bool Intersects(const Rectangle& r) const noexcept
        {
            return (r.x < (x + width)) && (x < (r.x + r.width)) && (r.y < (y + height)) && (y < (r.y + r.height));
        }

        void Offset(long ox, long oy) noexcept
        {
            x += ox;
            y += oy;
        }

        // Static functions
        static Rectangle Intersect(const Rectangle& ra, const Rectangle& rb) noexcept;
 
        static Rectangle Union(const Rectangle& ra, const Rectangle& rb) noexcept;
    };

    //------------------------------------------------------------------------------
    // 2D vector
    REFLECTION_TYPE(Float2)
    STRUCT(Float2, Fields)
    {
        REFLECTION_BODY(Rectangle);

        float x;
        float y;

        Float2() noexcept : x(0.f), y(0.f) {}
        constexpr explicit Float2(float ix) noexcept : x(ix), y(ix) {}
        constexpr Float2(float ix, float iy) noexcept : x(ix), y(iy) {}
        explicit Float2(_In_reads_(2) const float* pArray) noexcept : x(pArray[0]), y(pArray[1]) {}
        Float2(FXMVECTOR V) noexcept { XMStoreFloat2(this, V); }
        Float2(const XMFLOAT2& V) noexcept
        {
            this->x = V.x;
            this->y = V.y;
        }
        explicit Float2(const XMVECTORF32& F) noexcept
        {
            this->x = F.f[0];
            this->y = F.f[1];
        }

        Float2(const Float2&) = default;
        Float2& operator=(const Float2&) = default;

        Float2(Float2&&) = default;
        Float2& operator=(Float2&&) = default;

        operator XMVECTOR() const noexcept { return XMLoadFloat2(this); }

        // Comparison operators
        bool operator==(const Float2& V) const noexcept;
        bool operator!=(const Float2& V) const noexcept;

        // Assignment operators
        Float2& operator=(const XMVECTORF32& F) noexcept
        {
            x = F.f[0];
            y = F.f[1];
            return *this;
        }
        Float2& operator+=(const Float2& V) noexcept;
        Float2& operator-=(const Float2& V) noexcept;
        Float2& operator*=(const Float2& V) noexcept;
        Float2& operator*=(float S) noexcept;
        Float2& operator/=(float S) noexcept;

        // Unary operators
        Float2 operator+() const noexcept { return *this; }
        Float2 operator-() const noexcept { return Float2(-x, -y); }

        // Vector operations
        bool InBounds(const Float2& Bounds) const noexcept;

        float Length() const noexcept;
        float LengthSquared() const noexcept;

        float   Dot(const Float2& V) const noexcept;
        void    Cross(const Float2& V, Float2& result) const noexcept;
        Float2 Cross(const Float2& V) const noexcept;

        void Normalize() noexcept;
        void Normalize(Float2& result) const noexcept;

        void Clamp(const Float2& vmin, const Float2& vmax) noexcept;
        void Clamp(const Float2& vmin, const Float2& vmax, Float2& result) const noexcept;

        // Static functions
        static float Distance(const Float2& v1, const Float2& v2) noexcept;
        static float DistanceSquared(const Float2& v1, const Float2& v2) noexcept;

        static void    Min(const Float2& v1, const Float2& v2, Float2& result) noexcept;
        static Float2 Min(const Float2& v1, const Float2& v2) noexcept;

        static void    Max(const Float2& v1, const Float2& v2, Float2& result) noexcept;
        static Float2 Max(const Float2& v1, const Float2& v2) noexcept;

        static void    Lerp(const Float2& v1, const Float2& v2, float t, Float2& result) noexcept;
        static Float2 Lerp(const Float2& v1, const Float2& v2, float t) noexcept;

        static void    SmoothStep(const Float2& v1, const Float2& v2, float t, Float2& result) noexcept;
        static Float2 SmoothStep(const Float2& v1, const Float2& v2, float t) noexcept;

        static void Barycentric(const Float2& v1,
                                const Float2& v2,
                                const Float2& v3,
                                float          f,
                                float          g,
                                Float2&       result) noexcept;
        static Float2
        Barycentric(const Float2& v1, const Float2& v2, const Float2& v3, float f, float g) noexcept;

        static void CatmullRom(const Float2& v1,
                                const Float2& v2,
                                const Float2& v3,
                                const Float2& v4,
                                float          t,
                                Float2&       result) noexcept;
        static Float2
        CatmullRom(const Float2& v1, const Float2& v2, const Float2& v3, const Float2& v4, float t) noexcept;

        static void Hermite(const Float2& v1,
                            const Float2& t1,
                            const Float2& v2,
                            const Float2& t2,
                            float          t,
                            Float2&       result) noexcept;
        static Float2
        Hermite(const Float2& v1, const Float2& t1, const Float2& v2, const Float2& t2, float t) noexcept;

        static void    Reflect(const Float2& ivec, const Float2& nvec, Float2& result) noexcept;
        static Float2 Reflect(const Float2& ivec, const Float2& nvec) noexcept;

        static void
        Refract(const Float2& ivec, const Float2& nvec, float refractionIndex, Float2& result) noexcept;
        static Float2 Refract(const Float2& ivec, const Float2& nvec, float refractionIndex) noexcept;

        static void    Transform(const Float2& v, const QuaternionN& quat, Float2& result) noexcept;
        static Float2 Transform(const Float2& v, const QuaternionN& quat) noexcept;

        static void    Transform(const Float2& v, const Float4x4& m, Float2& result) noexcept;
        static Float2 Transform(const Float2& v, const Float4x4& m) noexcept;
        static void    Transform(_In_reads_(count) const Float2* varray,
                                    size_t                           count,
                                    const Float4x4&                    m,
                                    _Out_writes_(count) Float2*     resultArray) noexcept;

        static void Transform(const Float2& v, const Float4x4& m, Float4& result) noexcept;
        static void Transform(_In_reads_(count) const Float2* varray,
                                size_t                           count,
                                const Float4x4&                    m,
                                _Out_writes_(count) Float4*     resultArray) noexcept;

        static void    TransformNormal(const Float2& v, const Float4x4& m, Float2& result) noexcept;
        static Float2 TransformNormal(const Float2& v, const Float4x4& m) noexcept;
        static void    TransformNormal(_In_reads_(count) const Float2* varray,
                                        size_t                           count,
                                        const Float4x4&                    m,
                                        _Out_writes_(count) Float2*     resultArray) noexcept;

        // Constants
        static const Float2 Zero;
        static const Float2 One;
        static const Float2 UnitX;
        static const Float2 UnitY;
    };

    // Binary operators
    Float2 operator+(const Float2& V1, const Float2& V2) noexcept;
    Float2 operator-(const Float2& V1, const Float2& V2) noexcept;
    Float2 operator*(const Float2& V1, const Float2& V2) noexcept;
    Float2 operator*(const Float2& V, float S) noexcept;
    Float2 operator/(const Float2& V1, const Float2& V2) noexcept;
    Float2 operator/(const Float2& V, float S) noexcept;
    Float2 operator*(float S, const Float2& V) noexcept;

    //------------------------------------------------------------------------------
    // 3D vector
    REFLECTION_TYPE(Float3)
    STRUCT(Float3, Fields)
    {
        REFLECTION_BODY(Float3);

        float x;
        float y;
        float z;

        Float3() noexcept : x(0.f), y(0.f), z(0.f) {}
        constexpr explicit Float3(float ix) noexcept : x(ix), y(ix), z(ix) {}
        constexpr Float3(float ix, float iy, float iz) noexcept : x(ix), y(iy), z(iz) {}
        explicit Float3(_In_reads_(3) const float* pArray) noexcept : x(pArray[0]), y(pArray[1]), z(pArray[2]) {}
        Float3(FXMVECTOR V) noexcept { XMStoreFloat3(this, V); }
        Float3(const XMFLOAT3& V) noexcept
        {
            this->x = V.x;
            this->y = V.y;
            this->z = V.z;
        }
        explicit Float3(const XMVECTORF32& F) noexcept
        {
            this->x = F.f[0];
            this->y = F.f[1];
            this->z = F.f[2];
        }

        Float3(const Float3&) = default;
        Float3& operator=(const Float3&) = default;

        Float3(Float3&&) = default;
        Float3& operator=(Float3&&) = default;

        operator XMVECTOR() const noexcept { return XMLoadFloat3(this); }

        // Comparison operators
        bool operator==(const Float3& V) const noexcept;
        bool operator!=(const Float3& V) const noexcept;

        // Assignment operators
        Float3& operator=(const XMVECTORF32& F) noexcept
        {
            x = F.f[0];
            y = F.f[1];
            z = F.f[2];
            return *this;
        }
        Float3& operator+=(const Float3& V) noexcept;
        Float3& operator-=(const Float3& V) noexcept;
        Float3& operator*=(const Float3& V) noexcept;
        Float3& operator*=(float S) noexcept;
        Float3& operator/=(float S) noexcept;

        // Unary operators
        Float3 operator+() const noexcept { return *this; }
        Float3 operator-() const noexcept;

        // Vector operations
        bool InBounds(const Float3& Bounds) const noexcept;

        float Length() const noexcept;
        float LengthSquared() const noexcept;

        float   Dot(const Float3& V) const noexcept;
        void    Cross(const Float3& V, Float3& result) const noexcept;
        Float3 Cross(const Float3& V) const noexcept;

        void Normalize() noexcept;
        void Normalize(Float3& result) const noexcept;

        void Clamp(const Float3& vmin, const Float3& vmax) noexcept;
        void Clamp(const Float3& vmin, const Float3& vmax, Float3& result) const noexcept;

        // Static functions
        static float Distance(const Float3& v1, const Float3& v2) noexcept;
        static float DistanceSquared(const Float3& v1, const Float3& v2) noexcept;

        static void    Min(const Float3& v1, const Float3& v2, Float3& result) noexcept;
        static Float3 Min(const Float3& v1, const Float3& v2) noexcept;

        static void    Max(const Float3& v1, const Float3& v2, Float3& result) noexcept;
        static Float3 Max(const Float3& v1, const Float3& v2) noexcept;

        static void    Lerp(const Float3& v1, const Float3& v2, float t, Float3& result) noexcept;
        static Float3 Lerp(const Float3& v1, const Float3& v2, float t) noexcept;

        static void    SmoothStep(const Float3& v1, const Float3& v2, float t, Float3& result) noexcept;
        static Float3 SmoothStep(const Float3& v1, const Float3& v2, float t) noexcept;

        static void Barycentric(const Float3& v1,
                                const Float3& v2,
                                const Float3& v3,
                                float          f,
                                float          g,
                                Float3&       result) noexcept;
        static Float3
        Barycentric(const Float3& v1, const Float3& v2, const Float3& v3, float f, float g) noexcept;

        static void CatmullRom(const Float3& v1,
                                const Float3& v2,
                                const Float3& v3,
                                const Float3& v4,
                                float          t,
                                Float3&       result) noexcept;
        static Float3
        CatmullRom(const Float3& v1, const Float3& v2, const Float3& v3, const Float3& v4, float t) noexcept;

        static void Hermite(const Float3& v1,
                            const Float3& t1,
                            const Float3& v2,
                            const Float3& t2,
                            float          t,
                            Float3&       result) noexcept;
        static Float3
        Hermite(const Float3& v1, const Float3& t1, const Float3& v2, const Float3& t2, float t) noexcept;

        static void    Reflect(const Float3& ivec, const Float3& nvec, Float3& result) noexcept;
        static Float3 Reflect(const Float3& ivec, const Float3& nvec) noexcept;

        static void
        Refract(const Float3& ivec, const Float3& nvec, float refractionIndex, Float3& result) noexcept;
        static Float3 Refract(const Float3& ivec, const Float3& nvec, float refractionIndex) noexcept;

        static void    Transform(const Float3& v, const QuaternionN& quat, Float3& result) noexcept;
        static Float3 Transform(const Float3& v, const QuaternionN& quat) noexcept;

        static void    Transform(const Float3& v, const Float4x4& m, Float3& result) noexcept;
        static Float3 Transform(const Float3& v, const Float4x4& m) noexcept;
        static void    Transform(_In_reads_(count) const Float3* varray,
                                    size_t                           count,
                                    const Float4x4&                    m,
                                    _Out_writes_(count) Float3*     resultArray) noexcept;

        static void Transform(const Float3& v, const Float4x4& m, Float4& result) noexcept;
        static void Transform(_In_reads_(count) const Float3* varray,
                                size_t                           count,
                                const Float4x4&                    m,
                                _Out_writes_(count) Float4*     resultArray) noexcept;

        static void    TransformNormal(const Float3& v, const Float4x4& m, Float3& result) noexcept;
        static Float3 TransformNormal(const Float3& v, const Float4x4& m) noexcept;
        static void    TransformNormal(_In_reads_(count) const Float3* varray,
                                        size_t                           count,
                                        const Float4x4&                    m,
                                        _Out_writes_(count) Float3*     resultArray) noexcept;

        // Constants
        static const Float3 Zero;
        static const Float3 One;
        static const Float3 UnitX;
        static const Float3 UnitY;
        static const Float3 UnitZ;
        static const Float3 Up;
        static const Float3 Down;
        static const Float3 Right;
        static const Float3 Left;
        static const Float3 Forward;
        static const Float3 Backward;
    };

    // Binary operators
    Float3 operator+(const Float3& V1, const Float3& V2) noexcept;
    Float3 operator-(const Float3& V1, const Float3& V2) noexcept;
    Float3 operator*(const Float3& V1, const Float3& V2) noexcept;
    Float3 operator*(const Float3& V, float S) noexcept;
    Float3 operator/(const Float3& V1, const Float3& V2) noexcept;
    Float3 operator/(const Float3& V, float S) noexcept;
    Float3 operator*(float S, const Float3& V) noexcept;

    //------------------------------------------------------------------------------
    // 4D vector
    REFLECTION_TYPE(Float4)
    STRUCT(Float4, Fields)
    {
        REFLECTION_BODY(Float4);

        float x;
        float y;
        float z;
        float w;

        Float4() noexcept : x(0.f), y(0.f), z(0.f), w(0.f) {}
        constexpr explicit Float4(float ix) noexcept : x(ix), y(ix), z(ix), w(ix) {}
        constexpr Float4(float ix, float iy, float iz, float iw) noexcept : x(ix), y(iy), z(iz), w(iw) {}
        explicit Float4(_In_reads_(4) const float* pArray) noexcept :
            x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[0])
        {}
        Float4(FXMVECTOR V) noexcept { XMStoreFloat4(this, V); }
        Float4(const XMFLOAT4& V) noexcept
        {
            this->x = V.x;
            this->y = V.y;
            this->z = V.z;
            this->w = V.w;
        }
        explicit Float4(const XMVECTORF32& F) noexcept
        {
            this->x = F.f[0];
            this->y = F.f[1];
            this->z = F.f[2];
            this->w = F.f[3];
        }

        Float4(const Float4&) = default;
        Float4& operator=(const Float4&) = default;

        Float4(Float4&&) = default;
        Float4& operator=(Float4&&) = default;

        operator XMVECTOR() const noexcept { return XMLoadFloat4(this); }

        // Comparison operators
        bool operator==(const Float4& V) const noexcept;
        bool operator!=(const Float4& V) const noexcept;

        // Assignment operators
        Float4& operator=(const XMVECTORF32& F) noexcept
        {
            x = F.f[0];
            y = F.f[1];
            z = F.f[2];
            w = F.f[3];
            return *this;
        }
        Float4& operator+=(const Float4& V) noexcept;
        Float4& operator-=(const Float4& V) noexcept;
        Float4& operator*=(const Float4& V) noexcept;
        Float4& operator*=(float S) noexcept;
        Float4& operator/=(float S) noexcept;

        // Unary operators
        Float4 operator+() const noexcept { return *this; }
        Float4 operator-() const noexcept;

        // Vector operations
        bool InBounds(const Float4& Bounds) const noexcept;

        float Length() const noexcept;
        float LengthSquared() const noexcept;

        float   Dot(const Float4& V) const noexcept;
        void    Cross(const Float4& v1, const Float4& v2, Float4& result) const noexcept;
        Float4 Cross(const Float4& v1, const Float4& v2) const noexcept;

        void Normalize() noexcept;
        void Normalize(Float4& result) const noexcept;

        void Clamp(const Float4& vmin, const Float4& vmax) noexcept;
        void Clamp(const Float4& vmin, const Float4& vmax, Float4& result) const noexcept;

        // Static functions
        static float Distance(const Float4& v1, const Float4& v2) noexcept;
        static float DistanceSquared(const Float4& v1, const Float4& v2) noexcept;

        static void    Min(const Float4& v1, const Float4& v2, Float4& result) noexcept;
        static Float4 Min(const Float4& v1, const Float4& v2) noexcept;

        static void    Max(const Float4& v1, const Float4& v2, Float4& result) noexcept;
        static Float4 Max(const Float4& v1, const Float4& v2) noexcept;

        static void    Lerp(const Float4& v1, const Float4& v2, float t, Float4& result) noexcept;
        static Float4 Lerp(const Float4& v1, const Float4& v2, float t) noexcept;

        static void    SmoothStep(const Float4& v1, const Float4& v2, float t, Float4& result) noexcept;
        static Float4 SmoothStep(const Float4& v1, const Float4& v2, float t) noexcept;

        static void Barycentric(const Float4& v1,
                                const Float4& v2,
                                const Float4& v3,
                                float          f,
                                float          g,
                                Float4&       result) noexcept;
        static Float4
        Barycentric(const Float4& v1, const Float4& v2, const Float4& v3, float f, float g) noexcept;

        static void CatmullRom(const Float4& v1,
                                const Float4& v2,
                                const Float4& v3,
                                const Float4& v4,
                                float          t,
                                Float4&       result) noexcept;
        static Float4
        CatmullRom(const Float4& v1, const Float4& v2, const Float4& v3, const Float4& v4, float t) noexcept;

        static void Hermite(const Float4& v1,
                            const Float4& t1,
                            const Float4& v2,
                            const Float4& t2,
                            float          t,
                            Float4&       result) noexcept;
        static Float4
        Hermite(const Float4& v1, const Float4& t1, const Float4& v2, const Float4& t2, float t) noexcept;

        static void    Reflect(const Float4& ivec, const Float4& nvec, Float4& result) noexcept;
        static Float4 Reflect(const Float4& ivec, const Float4& nvec) noexcept;

        static void
        Refract(const Float4& ivec, const Float4& nvec, float refractionIndex, Float4& result) noexcept;
        static Float4 Refract(const Float4& ivec, const Float4& nvec, float refractionIndex) noexcept;

        static void    Transform(const Float2& v, const QuaternionN& quat, Float4& result) noexcept;
        static Float4 Transform(const Float2& v, const QuaternionN& quat) noexcept;

        static void    Transform(const Float3& v, const QuaternionN& quat, Float4& result) noexcept;
        static Float4 Transform(const Float3& v, const QuaternionN& quat) noexcept;

        static void    Transform(const Float4& v, const QuaternionN& quat, Float4& result) noexcept;
        static Float4 Transform(const Float4& v, const QuaternionN& quat) noexcept;

        static void    Transform(const Float4& v, const Float4x4& m, Float4& result) noexcept;
        static Float4 Transform(const Float4& v, const Float4x4& m) noexcept;
        static void    Transform(_In_reads_(count) const Float4* varray,
                                    size_t                           count,
                                    const Float4x4&                    m,
                                    _Out_writes_(count) Float4*     resultArray) noexcept;

        // Constants
        static const Float4 Zero;
        static const Float4 One;
        static const Float4 UnitX;
        static const Float4 UnitY;
        static const Float4 UnitZ;
        static const Float4 UnitW;
    };

    // Binary operators
    Float4 operator+(const Float4& V1, const Float4& V2) noexcept;
    Float4 operator-(const Float4& V1, const Float4& V2) noexcept;
    Float4 operator*(const Float4& V1, const Float4& V2) noexcept;
    Float4 operator*(const Float4& V, float S) noexcept;
    Float4 operator/(const Float4& V1, const Float4& V2) noexcept;
    Float4 operator/(const Float4& V, float S) noexcept;
    Float4 operator*(float S, const Float4& V) noexcept;

    //------------------------------------------------------------------------------
    REFLECTION_TYPE(Float4x4_)
    STRUCT(Float4x4_, Fields)
    {
        REFLECTION_BODY(Float4x4_);

        Float4x4_() {}
        float v0 {1.f};
        float v1 {0};
        float v2 {0};
        float v3 {0};
        float v4 {0};
        float v5 {1.f};
        float v6 {0};
        float v7 {0};
        float v8 {0};
        float v9 {0};
        float v10 {1.f};
        float v11 {0};
        float v12 {0};
        float v13 {0};
        float v14 {0};
        float v15 {1.f};
    };
    // 4x4 Float4x4 (assumes right-handed cooordinates)
    struct Float4x4 : public XMFLOAT4X4
    {
        Float4x4(const Float4x4_& mat)
        {
            m[0][0] = mat.v0;
            m[0][1] = mat.v1;
            m[0][2] = mat.v2;
            m[0][3] = mat.v3;
            m[1][0] = mat.v4;
            m[1][1] = mat.v5;
            m[1][2] = mat.v6;
            m[1][3] = mat.v7;
            m[2][0] = mat.v8;
            m[2][1] = mat.v9;
            m[2][2] = mat.v10;
            m[2][3] = mat.v11;
            m[3][0] = mat.v12;
            m[3][1] = mat.v13;
            m[3][2] = mat.v14;
            m[3][3] = mat.v15;
        }

        Float4x4_ toFloat4x4_()
        {
            Float4x4_ res;

            res.v0  = m[0][0];
            res.v1  = m[0][1];
            res.v2  = m[0][2];
            res.v3  = m[0][3];
            res.v4  = m[1][0];
            res.v5  = m[1][1];
            res.v6  = m[1][2];
            res.v7  = m[1][3];
            res.v8  = m[2][0];
            res.v9  = m[2][1];
            res.v10 = m[2][2];
            res.v11 = m[2][3];
            res.v12 = m[3][0];
            res.v13 = m[3][1];
            res.v14 = m[3][2];
            res.v15 = m[3][3];
            return res;
        }

        Float4x4() noexcept : XMFLOAT4X4(1.f, 0, 0, 0, 0, 1.f, 0, 0, 0, 0, 1.f, 0, 0, 0, 0, 1.f) {}
        constexpr Float4x4(float m00,
                            float m01,
                            float m02,
                            float m03,
                            float m10,
                            float m11,
                            float m12,
                            float m13,
                            float m20,
                            float m21,
                            float m22,
                            float m23,
                            float m30,
                            float m31,
                            float m32,
                            float m33) noexcept :
            XMFLOAT4X4(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33)
        {}
        explicit Float4x4(const Float3& r0, const Float3& r1, const Float3& r2) noexcept :
            XMFLOAT4X4(r0.x, r0.y, r0.z, 0, r1.x, r1.y, r1.z, 0, r2.x, r2.y, r2.z, 0, 0, 0, 0, 1.f)
        {}
        explicit Float4x4(const Float4& r0, const Float4& r1, const Float4& r2, const Float4& r3) noexcept :
            XMFLOAT4X4(r0.x,
                        r0.y,
                        r0.z,
                        r0.w,
                        r1.x,
                        r1.y,
                        r1.z,
                        r1.w,
                        r2.x,
                        r2.y,
                        r2.z,
                        r2.w,
                        r3.x,
                        r3.y,
                        r3.z,
                        r3.w)
        {}
        Float4x4(const XMFLOAT4X4& M) noexcept { memcpy(this, &M, sizeof(XMFLOAT4X4)); }
        Float4x4(const XMFLOAT3X3& M) noexcept;
        Float4x4(const XMFLOAT4X3& M) noexcept;

        explicit Float4x4(_In_reads_(16) const float* pArray) noexcept : XMFLOAT4X4(pArray) {}
        Float4x4(CXMMATRIX M) noexcept { XMStoreFloat4x4(this, M); }

        Float4x4(const Float4x4&) = default;
        Float4x4& operator=(const Float4x4&) = default;

        Float4x4(Float4x4&&) = default;
        Float4x4& operator=(Float4x4&&) = default;

        operator XMMATRIX() const noexcept { return XMLoadFloat4x4(this); }

        // Comparison operators
        bool operator==(const Float4x4& M) const noexcept;
        bool operator!=(const Float4x4& M) const noexcept;

        // Assignment operators
        Float4x4& operator=(const XMFLOAT3X3& M) noexcept;
        Float4x4& operator=(const XMFLOAT4X3& M) noexcept;
        Float4x4& operator+=(const Float4x4& M) noexcept;
        Float4x4& operator-=(const Float4x4& M) noexcept;
        Float4x4& operator*=(const Float4x4& M) noexcept;
        Float4x4& operator*=(float S) noexcept;
        Float4x4& operator/=(float S) noexcept;

        Float4x4& operator/=(const Float4x4& M) noexcept;
        // Element-wise divide

        // Unary operators
        Float4x4 operator+() const noexcept { return *this; }
        Float4x4 operator-() const noexcept;

        // Properties
        Float3 Up() const noexcept { return Float3(_21, _22, _23); }
        void    Up(const Float3& v) noexcept
        {
            _21 = v.x;
            _22 = v.y;
            _23 = v.z;
        }

        Float3 Down() const noexcept { return Float3(-_21, -_22, -_23); }
        void    Down(const Float3& v) noexcept
        {
            _21 = -v.x;
            _22 = -v.y;
            _23 = -v.z;
        }

        Float3 Right() const noexcept { return Float3(_11, _12, _13); }
        void    Right(const Float3& v) noexcept
        {
            _11 = v.x;
            _12 = v.y;
            _13 = v.z;
        }

        Float3 Left() const noexcept { return Float3(-_11, -_12, -_13); }
        void    Left(const Float3& v) noexcept
        {
            _11 = -v.x;
            _12 = -v.y;
            _13 = -v.z;
        }

        Float3 Forward() const noexcept { return Float3(-_31, -_32, -_33); }
        void    Forward(const Float3& v) noexcept
        {
            _31 = -v.x;
            _32 = -v.y;
            _33 = -v.z;
        }

        Float3 Backward() const noexcept { return Float3(_31, _32, _33); }
        void    Backward(const Float3& v) noexcept
        {
            _31 = v.x;
            _32 = v.y;
            _33 = v.z;
        }

        Float3 Translation() const noexcept { return Float3(_41, _42, _43); }
        void    Translation(const Float3& v) noexcept
        {
            _41 = v.x;
            _42 = v.y;
            _43 = v.z;
        }

        // Float4x4 operations
        bool Decompose(Float3& scale, QuaternionN& rotation, Float3& translation) noexcept;

        Float4x4 Transpose() const noexcept;
        void   Transpose(Float4x4& result) const noexcept;

        Float4x4 Invert() const noexcept;
        void   Invert(Float4x4& result) const noexcept;

        float Determinant() const noexcept;

        // Computes rotation about y-axis (y), then x-axis (x), then z-axis (z)
        Float3 ToEuler() const noexcept;

        // Static functions
        static Float4x4 CreateBillboard(const Float3&          object,
                                        const Float3&          cameraPosition,
                                        const Float3&          cameraUp,
                                        _In_opt_ const Float3* cameraForward = nullptr) noexcept;

        static Float4x4 CreateConstrainedBillboard(const Float3&          object,
                                                    const Float3&          cameraPosition,
                                                    const Float3&          rotateAxis,
                                                    _In_opt_ const Float3* cameraForward = nullptr,
                                                    _In_opt_ const Float3* objectForward = nullptr) noexcept;

        static Float4x4 CreateTranslation(const Float3& position) noexcept;
        static Float4x4 CreateTranslation(float x, float y, float z) noexcept;

        static Float4x4 CreateScale(const Float3& scales) noexcept;
        static Float4x4 CreateScale(float xs, float ys, float zs) noexcept;
        static Float4x4 CreateScale(float scale) noexcept;

        static Float4x4 CreateRotationX(float radians) noexcept;
        static Float4x4 CreateRotationY(float radians) noexcept;
        static Float4x4 CreateRotationZ(float radians) noexcept;

        static Float4x4 CreateFromAxisAngle(const Float3& axis, float angle) noexcept;

        static Float4x4
        CreatePerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane) noexcept;
        static Float4x4 CreatePerspective(float width, float height, float nearPlane, float farPlane) noexcept;
        static Float4x4 CreatePerspectiveOffCenter(float left,
                                                    float right,
                                                    float bottom,
                                                    float top,
                                                    float nearPlane,
                                                    float farPlane) noexcept;
        static Float4x4 CreateOrthographic(float width, float height, float zNearPlane, float zFarPlane) noexcept;
        static Float4x4 CreateOrthographicOffCenter(float left,
                                                    float right,
                                                    float bottom,
                                                    float top,
                                                    float zNearPlane,
                                                    float zFarPlane) noexcept;

        static Float4x4 CreateLookAt(const Float3& position, const Float3& target, const Float3& up) noexcept;
        static Float4x4 CreateWorld(const Float3& position, const Float3& forward, const Float3& up) noexcept;

        static Float4x4 CreateFromQuaternion(const QuaternionN& quat) noexcept;

        // Rotates about y-axis (yaw), then x-axis (pitch), then z-axis (roll)
        static Float4x4 CreateFromYawPitchRoll(float yaw, float pitch, float roll) noexcept;

        // Rotates about y-axis (angles.y), then x-axis (angles.x), then z-axis (angles.z)
        static Float4x4 CreateFromYawPitchRoll(const Float3& angles) noexcept;

        static Float4x4 CreateShadow(const Float3& lightDir, const PlaneN& plane) noexcept;

        static Float4x4 CreateReflection(const PlaneN& plane) noexcept;

        static void   Lerp(const Float4x4& M1, const Float4x4& M2, float t, Float4x4& result) noexcept;
        static Float4x4 Lerp(const Float4x4& M1, const Float4x4& M2, float t) noexcept;

        static void   Transform(const Float4x4& M, const QuaternionN& rotation, Float4x4& result) noexcept;
        static Float4x4 Transform(const Float4x4& M, const QuaternionN& rotation) noexcept;

        // Constants
        static const Float4x4 Identity;
    };

    // Binary operators
    Float4x4 operator+(const Float4x4& M1, const Float4x4& M2) noexcept;
    Float4x4 operator-(const Float4x4& M1, const Float4x4& M2) noexcept;
    Float4x4 operator*(const Float4x4& M1, const Float4x4& M2) noexcept;
    Float4x4 operator*(const Float4x4& M, float S) noexcept;
    Float4x4 operator/(const Float4x4& M, float S) noexcept;
    Float4x4 operator/(const Float4x4& M1, const Float4x4& M2) noexcept;
    // Element-wise divide
    Float4x4 operator*(float S, const Float4x4& M) noexcept;

    //-----------------------------------------------------------------------------
    // PlaneN
    REFLECTION_TYPE(PlaneN)
    STRUCT(PlaneN, Fields)
    {
        REFLECTION_BODY(PlaneN);

        float x;
        float y;
        float z;
        float w;

        PlaneN() noexcept : x(0.f), y(1.f), z(0.f), w(0.f) {}
        constexpr PlaneN(float ix, float iy, float iz, float iw) noexcept : x(ix), y(iy), z(iz), w(iw) {}
        PlaneN(const Float3& normal, float d) noexcept : x(normal.x), y(normal.y), z(normal.z), w(d) {}
        PlaneN(const Float3& point1, const Float3& point2, const Float3& point3) noexcept;
        PlaneN(const Float3& point, const Float3& normal) noexcept;
        explicit PlaneN(const Float4& v) noexcept : x(v.x), y(v.y), z(v.z), w(v.w) {}
        explicit PlaneN(_In_reads_(4) const float* pArray) noexcept :
            x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[0])
        {}
        PlaneN(FXMVECTOR V) noexcept { XMStoreFloat4(this, V); }
        PlaneN(const XMFLOAT4& p) noexcept
        {
            this->x = p.x;
            this->y = p.y;
            this->z = p.z;
            this->w = p.w;
        }
        explicit PlaneN(const XMVECTORF32& F) noexcept
        {
            this->x = F.f[0];
            this->y = F.f[1];
            this->z = F.f[2];
            this->w = F.f[3];
        }

        PlaneN(const PlaneN&) = default;
        PlaneN& operator=(const PlaneN&) = default;

        PlaneN(PlaneN&&) = default;
        PlaneN& operator=(PlaneN&&) = default;

        operator XMVECTOR() const noexcept { return XMLoadFloat4(this); }

        // Comparison operators
        bool operator==(const PlaneN& p) const noexcept;
        bool operator!=(const PlaneN& p) const noexcept;

        // Assignment operators
        PlaneN& operator=(const XMVECTORF32& F) noexcept
        {
            x = F.f[0];
            y = F.f[1];
            z = F.f[2];
            w = F.f[3];
            return *this;
        }

        // Properties
        Float3 Normal() const noexcept { return Float3(x, y, z); }
        void    Normal(const Float3& normal) noexcept
        {
            x = normal.x;
            y = normal.y;
            z = normal.z;
        }

        float D() const noexcept { return w; }
        void  D(float d) noexcept { w = d; }

        // PlaneN operations
        void Normalize() noexcept;
        void Normalize(PlaneN& result) const noexcept;

        float Dot(const Float4& v) const noexcept;
        float DotCoordinate(const Float3& position) const noexcept;
        float DotNormal(const Float3& normal) const noexcept;

        // Static functions
        static void  Transform(const PlaneN& plane, const Float4x4& M, PlaneN& result) noexcept;
        static PlaneN Transform(const PlaneN& plane, const Float4x4& M) noexcept;

        static void  Transform(const PlaneN& plane, const QuaternionN& rotation, PlaneN& result) noexcept;
        static PlaneN Transform(const PlaneN& plane, const QuaternionN& rotation) noexcept;
        // Input quaternion must be the inverse transpose of the transformation
    };

    //------------------------------------------------------------------------------
    // QuaternionN
    REFLECTION_TYPE(QuaternionN)
    STRUCT(QuaternionN, Fields)
    {
        REFLECTION_BODY(QuaternionN);

        float x;
        float y;
        float z;
        float w;

        QuaternionN() noexcept : x(0), y(0), z(0), w(1.f) {}
        constexpr QuaternionN(float ix, float iy, float iz, float iw) noexcept : x(ix), y(iy), z(iz), w(iw) {}
        QuaternionN(const Float3& v, float scalar) noexcept : x(v.x), y(v.y), z(v.z), w(scalar) {}
        explicit QuaternionN(const Float4& v) noexcept : x(v.x), y(v.y), z(v.z), w(v.w) {}
        explicit QuaternionN(_In_reads_(4) const float* pArray) noexcept :
            x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[0])
        {}
        QuaternionN(FXMVECTOR V) noexcept { XMStoreFloat4(this, V); }
        QuaternionN(const XMFLOAT4& q) noexcept
        {
            this->x = q.x;
            this->y = q.y;
            this->z = q.z;
            this->w = q.w;
        }
        explicit QuaternionN(const XMVECTORF32& F) noexcept
        {
            this->x = F.f[0];
            this->y = F.f[1];
            this->z = F.f[2];
            this->w = F.f[3];
        }

        QuaternionN(const QuaternionN&) = default;
        QuaternionN& operator=(const QuaternionN&) = default;

        QuaternionN(QuaternionN&&) = default;
        QuaternionN& operator=(QuaternionN&&) = default;

        operator XMVECTOR() const noexcept { return XMLoadFloat4(this); }

        // Comparison operators
        bool operator==(const QuaternionN& q) const noexcept;
        bool operator!=(const QuaternionN& q) const noexcept;

        // Assignment operators
        QuaternionN& operator=(const XMVECTORF32& F) noexcept
        {
            x = F.f[0];
            y = F.f[1];
            z = F.f[2];
            w = F.f[3];
            return *this;
        }
        QuaternionN& operator+=(const QuaternionN& q) noexcept;
        QuaternionN& operator-=(const QuaternionN& q) noexcept;
        QuaternionN& operator*=(const QuaternionN& q) noexcept;
        QuaternionN& operator*=(float S) noexcept;
        QuaternionN& operator/=(const QuaternionN& q) noexcept;

        // Unary operators
        QuaternionN operator+() const noexcept { return *this; }
        QuaternionN operator-() const noexcept;

        // QuaternionN operations
        float Length() const noexcept;
        float LengthSquared() const noexcept;

        void Normalize() noexcept;
        void Normalize(QuaternionN& result) const noexcept;

        void Conjugate() noexcept;
        void Conjugate(QuaternionN& result) const noexcept;

        void Inverse(QuaternionN& result) const noexcept;

        float Dot(const QuaternionN& Q) const noexcept;

        void RotateTowards(const QuaternionN& target, float maxAngle) noexcept;
        void __cdecl RotateTowards(const QuaternionN& target, float maxAngle, QuaternionN& result) const noexcept;

        // Computes rotation about y-axis (y), then x-axis (x), then z-axis (z)
        Float3 ToEuler() const noexcept;

        // Static functions
        static QuaternionN CreateFromAxisAngle(const Float3& axis, float angle) noexcept;

        // Rotates about y-axis (yaw), then x-axis (pitch), then z-axis (roll)
        static QuaternionN CreateFromYawPitchRoll(float yaw, float pitch, float roll) noexcept;

        // Rotates about y-axis (angles.y), then x-axis (angles.x), then z-axis (angles.z)
        static QuaternionN CreateFromYawPitchRoll(const Float3& angles) noexcept;

        static QuaternionN CreateFromRotationMatrix(const Float4x4& M) noexcept;

        static void       Lerp(const QuaternionN& q1, const QuaternionN& q2, float t, QuaternionN& result) noexcept;
        static QuaternionN Lerp(const QuaternionN& q1, const QuaternionN& q2, float t) noexcept;

        static void       Slerp(const QuaternionN& q1, const QuaternionN& q2, float t, QuaternionN& result) noexcept;
        static QuaternionN Slerp(const QuaternionN& q1, const QuaternionN& q2, float t) noexcept;

        static void       Concatenate(const QuaternionN& q1, const QuaternionN& q2, QuaternionN& result) noexcept;
        static QuaternionN Concatenate(const QuaternionN& q1, const QuaternionN& q2) noexcept;

        static void __cdecl FromToRotation(const Float3& fromDir,
                                            const Float3& toDir,
                                            QuaternionN&    result) noexcept;
        static QuaternionN FromToRotation(const Float3& fromDir, const Float3& toDir) noexcept;

        static void __cdecl LookRotation(const Float3& forward, const Float3& up, QuaternionN& result) noexcept;
        static QuaternionN LookRotation(const Float3& forward, const Float3& up) noexcept;

        static float Angle(const QuaternionN& q1, const QuaternionN& q2) noexcept;

        // Constants
        static const QuaternionN Identity;
    };

    // Binary operators
    QuaternionN operator+(const QuaternionN& Q1, const QuaternionN& Q2) noexcept;
    QuaternionN operator-(const QuaternionN& Q1, const QuaternionN& Q2) noexcept;
    QuaternionN operator*(const QuaternionN& Q1, const QuaternionN& Q2) noexcept;
    QuaternionN operator*(const QuaternionN& Q, float S) noexcept;
    QuaternionN operator/(const QuaternionN& Q1, const QuaternionN& Q2) noexcept;
    QuaternionN operator*(float S, const QuaternionN& Q) noexcept;

    //------------------------------------------------------------------------------
    // ColorN
    REFLECTION_TYPE(ColorN)
    STRUCT(ColorN, Fields)
    {
        REFLECTION_BODY(ColorN);

        float x;
        float y;
        float z;
        float w;

        ColorN() noexcept : x(0), y(0), z(0), w(1.f) {}
        constexpr ColorN(float _r, float _g, float _b) noexcept : x(_r), y(_g), z(_b), w(1.f) {}
        constexpr ColorN(float _r, float _g, float _b, float _a) noexcept : x(_r), y(_g), z(_b), w(_a) {}
        explicit ColorN(const Float3& clr) noexcept : x(clr.x), y(clr.y), z(clr.z), w(1.f) {}
        explicit ColorN(const Float4& clr) noexcept : x(clr.x), y(clr.y), z(clr.z), w(clr.w) {}
        explicit ColorN(_In_reads_(4) const float* pArray) noexcept :
            x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[0])
        {}
        ColorN(FXMVECTOR V) noexcept { XMStoreFloat4(this, V); }
        ColorN(const XMFLOAT4& c) noexcept
        {
            this->x = c.x;
            this->y = c.y;
            this->z = c.z;
            this->w = c.w;
        }
        explicit ColorN(const XMVECTORF32& F) noexcept
        {
            this->x = F.f[0];
            this->y = F.f[1];
            this->z = F.f[2];
            this->w = F.f[3];
        }

        // BGRA Direct3D 9 D3DCOLOR packed color
        explicit ColorN(const DirectX::PackedVector::XMCOLOR& Packed) noexcept;

        // RGBA XNA Game Studio packed color
        explicit ColorN(const DirectX::PackedVector::XMUBYTEN4& Packed) noexcept;

        ColorN(const ColorN&) = default;
        ColorN& operator=(const ColorN&) = default;

        ColorN(ColorN&&) = default;
        ColorN& operator=(ColorN&&) = default;

        operator XMVECTOR() const noexcept { return XMLoadFloat4(this); }
        operator const float*() const noexcept { return reinterpret_cast<const float*>(this); }

        // Comparison operators
        bool operator==(const ColorN& c) const noexcept;
        bool operator!=(const ColorN& c) const noexcept;

        // Assignment operators
        ColorN& operator=(const XMVECTORF32& F) noexcept
        {
            x = F.f[0];
            y = F.f[1];
            z = F.f[2];
            w = F.f[3];
            return *this;
        }
        ColorN& operator=(const DirectX::PackedVector::XMCOLOR& Packed) noexcept;
        ColorN& operator=(const DirectX::PackedVector::XMUBYTEN4& Packed) noexcept;
        ColorN& operator+=(const ColorN& c) noexcept;
        ColorN& operator-=(const ColorN& c) noexcept;
        ColorN& operator*=(const ColorN& c) noexcept;
        ColorN& operator*=(float S) noexcept;
        ColorN& operator/=(const ColorN& c) noexcept;

        // Unary operators
        ColorN operator+() const noexcept { return *this; }
        ColorN operator-() const noexcept;

        // Properties
        float R() const noexcept { return x; }
        void  R(float r) noexcept { x = r; }

        float G() const noexcept { return y; }
        void  G(float g) noexcept { y = g; }

        float B() const noexcept { return z; }
        void  B(float b) noexcept { z = b; }

        float A() const noexcept { return w; }
        void  A(float a) noexcept { w = a; }

        // ColorN operations
        DirectX::PackedVector::XMCOLOR   BGRA() const noexcept;
        DirectX::PackedVector::XMUBYTEN4 RGBA() const noexcept;

        Float3 ToVector3() const noexcept;
        Float4 ToVector4() const noexcept;

        void Negate() noexcept;
        void Negate(ColorN& result) const noexcept;

        void Saturate() noexcept;
        void Saturate(ColorN& result) const noexcept;

        void Premultiply() noexcept;
        void Premultiply(ColorN& result) const noexcept;

        void AdjustSaturation(float sat) noexcept;
        void AdjustSaturation(float sat, ColorN& result) const noexcept;

        void AdjustContrast(float contrast) noexcept;
        void AdjustContrast(float contrast, ColorN& result) const noexcept;

        // Static functions
        static void  Modulate(const ColorN& c1, const ColorN& c2, ColorN& result) noexcept;
        static ColorN Modulate(const ColorN& c1, const ColorN& c2) noexcept;

        static void  Lerp(const ColorN& c1, const ColorN& c2, float t, ColorN& result) noexcept;
        static ColorN Lerp(const ColorN& c1, const ColorN& c2, float t) noexcept;
    };

    // Binary operators
    ColorN operator+(const ColorN& C1, const ColorN& C2) noexcept;
    ColorN operator-(const ColorN& C1, const ColorN& C2) noexcept;
    ColorN operator*(const ColorN& C1, const ColorN& C2) noexcept;
    ColorN operator*(const ColorN& C, float S) noexcept;
    ColorN operator/(const ColorN& C1, const ColorN& C2) noexcept;
    ColorN operator*(float S, const ColorN& C) noexcept;

    //------------------------------------------------------------------------------
    // RayN
    REFLECTION_TYPE(RayN)
    CLASS(RayN, Fields)
    {
        REFLECTION_BODY(RayN);

    public:
        Float3 position;
        Float3 direction;

        RayN() noexcept : position(0, 0, 0), direction(0, 0, 1) {}
        RayN(const Float3& pos, const Float3& dir) noexcept : position(pos), direction(dir) {}

        RayN(const RayN&) = default;
        RayN& operator=(const RayN&) = default;

        RayN(RayN&&)   = default;
        RayN& operator=(RayN&&) = default;

        // Comparison operators
        bool operator==(const RayN& r) const noexcept;
        bool operator!=(const RayN& r) const noexcept;

        // RayN operations
        bool Intersects(const BoundingSphere& sphere, _Out_ float& Dist) const noexcept;
        bool Intersects(const BoundingBox& box, _Out_ float& Dist) const noexcept;
        bool
        Intersects(const Float3& tri0, const Float3& tri1, const Float3& tri2, _Out_ float& Dist) const noexcept;
        bool Intersects(const PlaneN& plane, _Out_ float& Dist) const noexcept;
    };

    //------------------------------------------------------------------------------
    // ViewportN
    REFLECTION_TYPE(ViewportN)
    CLASS(ViewportN, Fields)
    {
        REFLECTION_BODY(ViewportN);

    public:
        float x;
        float y;
        float width;
        float height;
        float minDepth;
        float maxDepth;

        ViewportN() noexcept : x(0.f), y(0.f), width(0.f), height(0.f), minDepth(0.f), maxDepth(1.f) {}
        constexpr ViewportN(float ix, float iy, float iw, float ih, float iminz = 0.f, float imaxz = 1.f) noexcept :
            x(ix), y(iy), width(iw), height(ih), minDepth(iminz), maxDepth(imaxz)
        {}

        ViewportN(const ViewportN&) = default;
        ViewportN& operator=(const ViewportN&) = default;

        ViewportN(ViewportN&&) = default;
        ViewportN& operator=(ViewportN&&) = default;

        bool operator==(const ViewportN& vp) const noexcept;
        bool operator!=(const ViewportN& vp) const noexcept;

        // ViewportN operations
        float AspectRatio() const noexcept;

        Float3
                Project(const Float3& p, const Float4x4& proj, const Float4x4& view, const Float4x4& world) const noexcept;
        void Project(const Float3& p,
                        const Float4x4&  proj,
                        const Float4x4&  view,
                        const Float4x4&  world,
                        Float3&       result) const noexcept;

        Float3
        Unproject(const Float3& p, const Float4x4& proj, const Float4x4& view, const Float4x4& world) const noexcept;
        void Unproject(const Float3& p,
                        const Float4x4&  proj,
                        const Float4x4&  view,
                        const Float4x4&  world,
                        Float3&       result) const noexcept;

    };

    #include "simple_math_custom.inl"

} // namespace DirectX

//------------------------------------------------------------------------------
// Support for SimpleMath and Standard C++ Library containers
namespace std
{

    template<>
    struct less<Pilot::Rectangle>
    {
        bool operator()(const Pilot::Rectangle& r1,
                        const Pilot::Rectangle& r2) const noexcept
        {
            return ((r1.x < r2.x) || ((r1.x == r2.x) && (r1.y < r2.y)) ||
                    ((r1.x == r2.x) && (r1.y == r2.y) && (r1.width < r2.width)) ||
                    ((r1.x == r2.x) && (r1.y == r2.y) && (r1.width == r2.width) && (r1.height < r2.height)));
        }
    };

    template<>
    struct less<Pilot::Float2>
    {
        bool operator()(const Pilot::Float2& V1, const Pilot::Float2& V2) const noexcept
        {
            return ((V1.x < V2.x) || ((V1.x == V2.x) && (V1.y < V2.y)));
        }
    };

    template<>
    struct less<Pilot::Float3>
    {
        bool operator()(const Pilot::Float3& V1, const Pilot::Float3& V2) const noexcept
        {
            return ((V1.x < V2.x) || ((V1.x == V2.x) && (V1.y < V2.y)) ||
                    ((V1.x == V2.x) && (V1.y == V2.y) && (V1.z < V2.z)));
        }
    };

    template<>
    struct less<Pilot::Float4>
    {
        bool operator()(const Pilot::Float4& V1, const Pilot::Float4& V2) const noexcept
        {
            return ((V1.x < V2.x) || ((V1.x == V2.x) && (V1.y < V2.y)) ||
                    ((V1.x == V2.x) && (V1.y == V2.y) && (V1.z < V2.z)) ||
                    ((V1.x == V2.x) && (V1.y == V2.y) && (V1.z == V2.z) && (V1.w < V2.w)));
        }
    };

    template<>
    struct less<Pilot::Float4x4>
    {
        bool operator()(const Pilot::Float4x4& M1, const Pilot::Float4x4& M2) const noexcept
        {
            if (M1._11 != M2._11)
                return M1._11 < M2._11;
            if (M1._12 != M2._12)
                return M1._12 < M2._12;
            if (M1._13 != M2._13)
                return M1._13 < M2._13;
            if (M1._14 != M2._14)
                return M1._14 < M2._14;
            if (M1._21 != M2._21)
                return M1._21 < M2._21;
            if (M1._22 != M2._22)
                return M1._22 < M2._22;
            if (M1._23 != M2._23)
                return M1._23 < M2._23;
            if (M1._24 != M2._24)
                return M1._24 < M2._24;
            if (M1._31 != M2._31)
                return M1._31 < M2._31;
            if (M1._32 != M2._32)
                return M1._32 < M2._32;
            if (M1._33 != M2._33)
                return M1._33 < M2._33;
            if (M1._34 != M2._34)
                return M1._34 < M2._34;
            if (M1._41 != M2._41)
                return M1._41 < M2._41;
            if (M1._42 != M2._42)
                return M1._42 < M2._42;
            if (M1._43 != M2._43)
                return M1._43 < M2._43;
            if (M1._44 != M2._44)
                return M1._44 < M2._44;

            return false;
        }
    };

    template<>
    struct less<Pilot::PlaneN>
    {
        bool operator()(const Pilot::PlaneN& P1, const Pilot::PlaneN& P2) const noexcept
        {
            return ((P1.x < P2.x) || ((P1.x == P2.x) && (P1.y < P2.y)) ||
                    ((P1.x == P2.x) && (P1.y == P2.y) && (P1.z < P2.z)) ||
                    ((P1.x == P2.x) && (P1.y == P2.y) && (P1.z == P2.z) && (P1.w < P2.w)));
        }
    };

    template<>
    struct less<Pilot::QuaternionN>
    {
        bool operator()(const Pilot::QuaternionN& Q1,
                        const Pilot::QuaternionN& Q2) const noexcept
        {
            return ((Q1.x < Q2.x) || ((Q1.x == Q2.x) && (Q1.y < Q2.y)) ||
                    ((Q1.x == Q2.x) && (Q1.y == Q2.y) && (Q1.z < Q2.z)) ||
                    ((Q1.x == Q2.x) && (Q1.y == Q2.y) && (Q1.z == Q2.z) && (Q1.w < Q2.w)));
        }
    };

    template<>
    struct less<Pilot::ColorN>
    {
        bool operator()(const Pilot::ColorN& C1, const Pilot::ColorN& C2) const noexcept
        {
            return ((C1.x < C2.x) || ((C1.x == C2.x) && (C1.y < C2.y)) ||
                    ((C1.x == C2.x) && (C1.y == C2.y) && (C1.z < C2.z)) ||
                    ((C1.x == C2.x) && (C1.y == C2.y) && (C1.z == C2.z) && (C1.w < C2.w)));
        }
    };

    template<>
    struct less<Pilot::RayN>
    {
        bool operator()(const Pilot::RayN& R1, const Pilot::RayN& R2) const noexcept
        {
            if (R1.position.x != R2.position.x)
                return R1.position.x < R2.position.x;
            if (R1.position.y != R2.position.y)
                return R1.position.y < R2.position.y;
            if (R1.position.z != R2.position.z)
                return R1.position.z < R2.position.z;

            if (R1.direction.x != R2.direction.x)
                return R1.direction.x < R2.direction.x;
            if (R1.direction.y != R2.direction.y)
                return R1.direction.y < R2.direction.y;
            if (R1.direction.z != R2.direction.z)
                return R1.direction.z < R2.direction.z;

            return false;
        }
    };

    template<>
    struct less<Pilot::ViewportN>
    {
        bool operator()(const Pilot::ViewportN& vp1,
                        const Pilot::ViewportN& vp2) const noexcept
        {
            if (vp1.x != vp2.x)
                return (vp1.x < vp2.x);
            if (vp1.y != vp2.y)
                return (vp1.y < vp2.y);

            if (vp1.width != vp2.width)
                return (vp1.width < vp2.width);
            if (vp1.height != vp2.height)
                return (vp1.height < vp2.height);

            if (vp1.minDepth != vp2.minDepth)
                return (vp1.minDepth < vp2.minDepth);
            if (vp1.maxDepth != vp2.maxDepth)
                return (vp1.maxDepth < vp2.maxDepth);

            return false;
        }
    };

} // namespace std
