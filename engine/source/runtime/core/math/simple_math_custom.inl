//-------------------------------------------------------------------------------------
// SimpleMath.inl -- Simplified C++ Math wrapper for DirectXMath
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//-------------------------------------------------------------------------------------

#pragma once

/****************************************************************************
*
* Rectangle
*
****************************************************************************/

//------------------------------------------------------------------------------
// Rectangle operations
//------------------------------------------------------------------------------
inline Float2 Rectangle::Location() const noexcept
{
    return Float2(float(x), float(y));
}

inline Float2 Rectangle::Center() const noexcept
{
    return Float2(float(x) + (float(width) / 2.f), float(y) + (float(height) / 2.f));
}

inline bool Rectangle::Contains(const Float2& point) const noexcept
{
    return (float(x) <= point.x) && (point.x < float(x + width)) && (float(y) <= point.y) && (point.y < float(y + height));
}

inline void Rectangle::Inflate(long horizAmount, long vertAmount) noexcept
{
    x -= horizAmount;
    y -= vertAmount;
    width += horizAmount;
    height += vertAmount;
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline Pilot::Rectangle Rectangle::Intersect(const Pilot::Rectangle& ra,
                                                         const Pilot::Rectangle& rb) noexcept
{
    const long righta = ra.x + ra.width;
    const long rightb = rb.x + rb.width;

    const long bottoma = ra.y + ra.height;
    const long bottomb = rb.y + rb.height;

    const long maxX = ra.x > rb.x ? ra.x : rb.x;
    const long maxY = ra.y > rb.y ? ra.y : rb.y;

    const long minRight = righta < rightb ? righta : rightb;
    const long minBottom = bottoma < bottomb ? bottoma : bottomb;

    Pilot::Rectangle result;

    if ((minRight > maxX) && (minBottom > maxY))
    {
        result.x = maxX;
        result.y = maxY;
        result.width = minRight - maxX;
        result.height = minBottom - maxY;
    }
    else
    {
        result.x = 0;
        result.y = 0;
        result.width = 0;
        result.height = 0;
    }

    return result;
}

inline Pilot::Rectangle Rectangle::Union(const Pilot::Rectangle& ra,
                                                     const Pilot::Rectangle& rb) noexcept
{
    const long righta = ra.x + ra.width;
    const long rightb = rb.x + rb.width;

    const long bottoma = ra.y + ra.height;
    const long bottomb = rb.y + rb.height;

    const int minX = ra.x < rb.x ? ra.x : rb.x;
    const int minY = ra.y < rb.y ? ra.y : rb.y;

    const int maxRight = righta > rightb ? righta : rightb;
    const int maxBottom = bottoma > bottomb ? bottoma : bottomb;

    Pilot::Rectangle result;
    result.x = minX;
    result.y = minY;
    result.width = maxRight - minX;
    result.height = maxBottom - minY;
    return result;
}


/****************************************************************************
 *
 * Float2
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool Float2::operator == (const Float2& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&V);
    return XMVector2Equal(v1, v2);
}

inline bool Float2::operator != (const Float2& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&V);
    return XMVector2NotEqual(v1, v2);
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline Float2& Float2::operator+= (const Float2& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&V);
    const XMVECTOR X = XMVectorAdd(v1, v2);
    XMStoreFloat2(this, X);
    return *this;
}

inline Float2& Float2::operator-= (const Float2& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&V);
    const XMVECTOR X = XMVectorSubtract(v1, v2);
    XMStoreFloat2(this, X);
    return *this;
}

inline Float2& Float2::operator*= (const Float2& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&V);
    const XMVECTOR X = XMVectorMultiply(v1, v2);
    XMStoreFloat2(this, X);
    return *this;
}

inline Float2& Float2::operator*= (float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR X = XMVectorScale(v1, S);
    XMStoreFloat2(this, X);
    return *this;
}

inline Float2& Float2::operator/= (float S) noexcept
{
    using namespace DirectX;
    assert(S != 0.0f);
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR X = XMVectorScale(v1, 1.f / S);
    XMStoreFloat2(this, X);
    return *this;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline Float2 operator+ (const Float2& V1, const Float2& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&V1);
    const XMVECTOR v2 = XMLoadFloat2(&V2);
    const XMVECTOR X = XMVectorAdd(v1, v2);
    Float2 R;
    XMStoreFloat2(&R, X);
    return R;
}

inline Float2 operator- (const Float2& V1, const Float2& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&V1);
    const XMVECTOR v2 = XMLoadFloat2(&V2);
    const XMVECTOR X = XMVectorSubtract(v1, v2);
    Float2 R;
    XMStoreFloat2(&R, X);
    return R;
}

inline Float2 operator* (const Float2& V1, const Float2& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&V1);
    const XMVECTOR v2 = XMLoadFloat2(&V2);
    const XMVECTOR X = XMVectorMultiply(v1, v2);
    Float2 R;
    XMStoreFloat2(&R, X);
    return R;
}

inline Float2 operator* (const Float2& V, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&V);
    const XMVECTOR X = XMVectorScale(v1, S);
    Float2 R;
    XMStoreFloat2(&R, X);
    return R;
}

inline Float2 operator/ (const Float2& V1, const Float2& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&V1);
    const XMVECTOR v2 = XMLoadFloat2(&V2);
    const XMVECTOR X = XMVectorDivide(v1, v2);
    Float2 R;
    XMStoreFloat2(&R, X);
    return R;
}

inline Float2 operator/ (const Float2& V, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&V);
    const XMVECTOR X = XMVectorScale(v1, 1.f / S);
    Float2 R;
    XMStoreFloat2(&R, X);
    return R;
}

inline Float2 operator* (float S, const Float2& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&V);
    const XMVECTOR X = XMVectorScale(v1, S);
    Float2 R;
    XMStoreFloat2(&R, X);
    return R;
}

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

inline bool Float2::InBounds(const Float2& Bounds) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&Bounds);
    return XMVector2InBounds(v1, v2);
}

inline float Float2::Length() const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR X = XMVector2Length(v1);
    return XMVectorGetX(X);
}

inline float Float2::LengthSquared() const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR X = XMVector2LengthSq(v1);
    return XMVectorGetX(X);
}

inline float Float2::Dot(const Float2& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&V);
    const XMVECTOR X = XMVector2Dot(v1, v2);
    return XMVectorGetX(X);
}

inline void Float2::Cross(const Float2& V, Float2& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&V);
    const XMVECTOR R = XMVector2Cross(v1, v2);
    XMStoreFloat2(&result, R);
}

inline Float2 Float2::Cross(const Float2& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&V);
    const XMVECTOR R = XMVector2Cross(v1, v2);

    Float2 result;
    XMStoreFloat2(&result, R);
    return result;
}

inline void Float2::Normalize() noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR X = XMVector2Normalize(v1);
    XMStoreFloat2(this, X);
}

inline void Float2::Normalize(Float2& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR X = XMVector2Normalize(v1);
    XMStoreFloat2(&result, X);
}

inline void Float2::Clamp(const Float2& vmin, const Float2& vmax) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&vmin);
    const XMVECTOR v3 = XMLoadFloat2(&vmax);
    const XMVECTOR X = XMVectorClamp(v1, v2, v3);
    XMStoreFloat2(this, X);
}

inline void Float2::Clamp(const Float2& vmin, const Float2& vmax, Float2& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(this);
    const XMVECTOR v2 = XMLoadFloat2(&vmin);
    const XMVECTOR v3 = XMLoadFloat2(&vmax);
    const XMVECTOR X = XMVectorClamp(v1, v2, v3);
    XMStoreFloat2(&result, X);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline float Float2::Distance(const Float2& v1, const Float2& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR V = XMVectorSubtract(x2, x1);
    const XMVECTOR X = XMVector2Length(V);
    return XMVectorGetX(X);
}

inline float Float2::DistanceSquared(const Float2& v1, const Float2& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR V = XMVectorSubtract(x2, x1);
    const XMVECTOR X = XMVector2LengthSq(V);
    return XMVectorGetX(X);
}

inline void Float2::Min(const Float2& v1, const Float2& v2, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X = XMVectorMin(x1, x2);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::Min(const Float2& v1, const Float2& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X = XMVectorMin(x1, x2);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Float2::Max(const Float2& v1, const Float2& v2, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X = XMVectorMax(x1, x2);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::Max(const Float2& v1, const Float2& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X = XMVectorMax(x1, x2);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Float2::Lerp(const Float2& v1, const Float2& v2, float t, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::Lerp(const Float2& v1, const Float2& v2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Float2::SmoothStep(const Float2& v1, const Float2& v2, float t, Float2& result) noexcept
{
    using namespace DirectX;
    t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
    t = t * t*(3.f - 2.f*t);
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::SmoothStep(const Float2& v1, const Float2& v2, float t) noexcept
{
    using namespace DirectX;
    t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
    t = t * t*(3.f - 2.f*t);
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Float2::Barycentric(const Float2& v1, const Float2& v2, const Float2& v3, float f, float g, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR x3 = XMLoadFloat2(&v3);
    const XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::Barycentric(const Float2& v1, const Float2& v2, const Float2& v3, float f, float g) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR x3 = XMLoadFloat2(&v3);
    const XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Float2::CatmullRom(const Float2& v1, const Float2& v2, const Float2& v3, const Float2& v4, float t, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR x3 = XMLoadFloat2(&v3);
    const XMVECTOR x4 = XMLoadFloat2(&v4);
    const XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::CatmullRom(const Float2& v1, const Float2& v2, const Float2& v3, const Float2& v4, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&v2);
    const XMVECTOR x3 = XMLoadFloat2(&v3);
    const XMVECTOR x4 = XMLoadFloat2(&v4);
    const XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Float2::Hermite(const Float2& v1, const Float2& t1, const Float2& v2, const Float2& t2, float t, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&t1);
    const XMVECTOR x3 = XMLoadFloat2(&v2);
    const XMVECTOR x4 = XMLoadFloat2(&t2);
    const XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::Hermite(const Float2& v1, const Float2& t1, const Float2& v2, const Float2& t2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat2(&v1);
    const XMVECTOR x2 = XMLoadFloat2(&t1);
    const XMVECTOR x3 = XMLoadFloat2(&v2);
    const XMVECTOR x4 = XMLoadFloat2(&t2);
    const XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Float2::Reflect(const Float2& ivec, const Float2& nvec, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat2(&ivec);
    const XMVECTOR n = XMLoadFloat2(&nvec);
    const XMVECTOR X = XMVector2Reflect(i, n);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::Reflect(const Float2& ivec, const Float2& nvec) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat2(&ivec);
    const XMVECTOR n = XMLoadFloat2(&nvec);
    const XMVECTOR X = XMVector2Reflect(i, n);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Float2::Refract(const Float2& ivec, const Float2& nvec, float refractionIndex, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat2(&ivec);
    const XMVECTOR n = XMLoadFloat2(&nvec);
    const XMVECTOR X = XMVector2Refract(i, n, refractionIndex);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::Refract(const Float2& ivec, const Float2& nvec, float refractionIndex) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat2(&ivec);
    const XMVECTOR n = XMLoadFloat2(&nvec);
    const XMVECTOR X = XMVector2Refract(i, n, refractionIndex);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Float2::Transform(const Float2& v, const QuaternionN& quat, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMVECTOR q = XMLoadFloat4(&quat);
    const XMVECTOR X = XMVector3Rotate(v1, q);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::Transform(const Float2& v, const QuaternionN& quat) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMVECTOR q = XMLoadFloat4(&quat);
    const XMVECTOR X = XMVector3Rotate(v1, q);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

inline void Float2::Transform(const Float2& v, const Float4x4& m, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector2TransformCoord(v1, M);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::Transform(const Float2& v, const Float4x4& m) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector2TransformCoord(v1, M);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

_Use_decl_annotations_
inline void Float2::Transform(const Float2* varray, size_t count, const Float4x4& m, Float2* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector2TransformCoordStream(resultArray, sizeof(XMFLOAT2), varray, sizeof(XMFLOAT2), count, M);
}

inline void Float2::Transform(const Float2& v, const Float4x4& m, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector2Transform(v1, M);
    XMStoreFloat4(&result, X);
}

_Use_decl_annotations_
inline void Float2::Transform(const Float2* varray, size_t count, const Float4x4& m, Float4* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector2TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT2), count, M);
}

inline void Float2::TransformNormal(const Float2& v, const Float4x4& m, Float2& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector2TransformNormal(v1, M);
    XMStoreFloat2(&result, X);
}

inline Float2 Float2::TransformNormal(const Float2& v, const Float4x4& m) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector2TransformNormal(v1, M);

    Float2 result;
    XMStoreFloat2(&result, X);
    return result;
}

_Use_decl_annotations_
inline void Float2::TransformNormal(const Float2* varray, size_t count, const Float4x4& m, Float2* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector2TransformNormalStream(resultArray, sizeof(XMFLOAT2), varray, sizeof(XMFLOAT2), count, M);
}


/****************************************************************************
 *
 * Float3
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool Float3::operator == (const Float3& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&V);
    return XMVector3Equal(v1, v2);
}

inline bool Float3::operator != (const Float3& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&V);
    return XMVector3NotEqual(v1, v2);
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline Float3& Float3::operator+= (const Float3& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&V);
    const XMVECTOR X = XMVectorAdd(v1, v2);
    XMStoreFloat3(this, X);
    return *this;
}

inline Float3& Float3::operator-= (const Float3& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&V);
    const XMVECTOR X = XMVectorSubtract(v1, v2);
    XMStoreFloat3(this, X);
    return *this;
}

inline Float3& Float3::operator*= (const Float3& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&V);
    const XMVECTOR X = XMVectorMultiply(v1, v2);
    XMStoreFloat3(this, X);
    return *this;
}

inline Float3& Float3::operator*= (float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR X = XMVectorScale(v1, S);
    XMStoreFloat3(this, X);
    return *this;
}

inline Float3& Float3::operator/= (float S) noexcept
{
    using namespace DirectX;
    assert(S != 0.0f);
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR X = XMVectorScale(v1, 1.f / S);
    XMStoreFloat3(this, X);
    return *this;
}

//------------------------------------------------------------------------------
// Urnary operators
//------------------------------------------------------------------------------

inline Float3 Float3::operator- () const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR X = XMVectorNegate(v1);
    Float3 R;
    XMStoreFloat3(&R, X);
    return R;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline Float3 operator+ (const Float3& V1, const Float3& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&V1);
    const XMVECTOR v2 = XMLoadFloat3(&V2);
    const XMVECTOR X = XMVectorAdd(v1, v2);
    Float3 R;
    XMStoreFloat3(&R, X);
    return R;
}

inline Float3 operator- (const Float3& V1, const Float3& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&V1);
    const XMVECTOR v2 = XMLoadFloat3(&V2);
    const XMVECTOR X = XMVectorSubtract(v1, v2);
    Float3 R;
    XMStoreFloat3(&R, X);
    return R;
}

inline Float3 operator* (const Float3& V1, const Float3& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&V1);
    const XMVECTOR v2 = XMLoadFloat3(&V2);
    const XMVECTOR X = XMVectorMultiply(v1, v2);
    Float3 R;
    XMStoreFloat3(&R, X);
    return R;
}

inline Float3 operator* (const Float3& V, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&V);
    const XMVECTOR X = XMVectorScale(v1, S);
    Float3 R;
    XMStoreFloat3(&R, X);
    return R;
}

inline Float3 operator/ (const Float3& V1, const Float3& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&V1);
    const XMVECTOR v2 = XMLoadFloat3(&V2);
    const XMVECTOR X = XMVectorDivide(v1, v2);
    Float3 R;
    XMStoreFloat3(&R, X);
    return R;
}

inline Float3 operator/ (const Float3& V, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&V);
    const XMVECTOR X = XMVectorScale(v1, 1.f / S);
    Float3 R;
    XMStoreFloat3(&R, X);
    return R;
}

inline Float3 operator* (float S, const Float3& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&V);
    const XMVECTOR X = XMVectorScale(v1, S);
    Float3 R;
    XMStoreFloat3(&R, X);
    return R;
}

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

inline bool Float3::InBounds(const Float3& Bounds) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&Bounds);
    return XMVector3InBounds(v1, v2);
}

inline float Float3::Length() const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR X = XMVector3Length(v1);
    return XMVectorGetX(X);
}

inline float Float3::LengthSquared() const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR X = XMVector3LengthSq(v1);
    return XMVectorGetX(X);
}

inline float Float3::Dot(const Float3& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&V);
    const XMVECTOR X = XMVector3Dot(v1, v2);
    return XMVectorGetX(X);
}

inline void Float3::Cross(const Float3& V, Float3& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&V);
    const XMVECTOR R = XMVector3Cross(v1, v2);
    XMStoreFloat3(&result, R);
}

inline Float3 Float3::Cross(const Float3& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&V);
    const XMVECTOR R = XMVector3Cross(v1, v2);

    Float3 result;
    XMStoreFloat3(&result, R);
    return result;
}

inline void Float3::Normalize() noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR X = XMVector3Normalize(v1);
    XMStoreFloat3(this, X);
}

inline void Float3::Normalize(Float3& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR X = XMVector3Normalize(v1);
    XMStoreFloat3(&result, X);
}

inline void Float3::Clamp(const Float3& vmin, const Float3& vmax) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&vmin);
    const XMVECTOR v3 = XMLoadFloat3(&vmax);
    const XMVECTOR X = XMVectorClamp(v1, v2, v3);
    XMStoreFloat3(this, X);
}

inline void Float3::Clamp(const Float3& vmin, const Float3& vmax, Float3& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(this);
    const XMVECTOR v2 = XMLoadFloat3(&vmin);
    const XMVECTOR v3 = XMLoadFloat3(&vmax);
    const XMVECTOR X = XMVectorClamp(v1, v2, v3);
    XMStoreFloat3(&result, X);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline float Float3::Distance(const Float3& v1, const Float3& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR V = XMVectorSubtract(x2, x1);
    const XMVECTOR X = XMVector3Length(V);
    return XMVectorGetX(X);
}

inline float Float3::DistanceSquared(const Float3& v1, const Float3& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR V = XMVectorSubtract(x2, x1);
    const XMVECTOR X = XMVector3LengthSq(V);
    return XMVectorGetX(X);
}

inline void Float3::Min(const Float3& v1, const Float3& v2, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X = XMVectorMin(x1, x2);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::Min(const Float3& v1, const Float3& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X = XMVectorMin(x1, x2);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Float3::Max(const Float3& v1, const Float3& v2, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X = XMVectorMax(x1, x2);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::Max(const Float3& v1, const Float3& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X = XMVectorMax(x1, x2);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Float3::Lerp(const Float3& v1, const Float3& v2, float t, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::Lerp(const Float3& v1, const Float3& v2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Float3::SmoothStep(const Float3& v1, const Float3& v2, float t, Float3& result) noexcept
{
    using namespace DirectX;
    t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
    t = t * t*(3.f - 2.f*t);
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::SmoothStep(const Float3& v1, const Float3& v2, float t) noexcept
{
    using namespace DirectX;
    t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
    t = t * t*(3.f - 2.f*t);
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Float3::Barycentric(const Float3& v1, const Float3& v2, const Float3& v3, float f, float g, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR x3 = XMLoadFloat3(&v3);
    const XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::Barycentric(const Float3& v1, const Float3& v2, const Float3& v3, float f, float g) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR x3 = XMLoadFloat3(&v3);
    const XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Float3::CatmullRom(const Float3& v1, const Float3& v2, const Float3& v3, const Float3& v4, float t, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR x3 = XMLoadFloat3(&v3);
    const XMVECTOR x4 = XMLoadFloat3(&v4);
    const XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::CatmullRom(const Float3& v1, const Float3& v2, const Float3& v3, const Float3& v4, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&v2);
    const XMVECTOR x3 = XMLoadFloat3(&v3);
    const XMVECTOR x4 = XMLoadFloat3(&v4);
    const XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Float3::Hermite(const Float3& v1, const Float3& t1, const Float3& v2, const Float3& t2, float t, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&t1);
    const XMVECTOR x3 = XMLoadFloat3(&v2);
    const XMVECTOR x4 = XMLoadFloat3(&t2);
    const XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::Hermite(const Float3& v1, const Float3& t1, const Float3& v2, const Float3& t2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat3(&v1);
    const XMVECTOR x2 = XMLoadFloat3(&t1);
    const XMVECTOR x3 = XMLoadFloat3(&v2);
    const XMVECTOR x4 = XMLoadFloat3(&t2);
    const XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Float3::Reflect(const Float3& ivec, const Float3& nvec, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat3(&ivec);
    const XMVECTOR n = XMLoadFloat3(&nvec);
    const XMVECTOR X = XMVector3Reflect(i, n);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::Reflect(const Float3& ivec, const Float3& nvec) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat3(&ivec);
    const XMVECTOR n = XMLoadFloat3(&nvec);
    const XMVECTOR X = XMVector3Reflect(i, n);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Float3::Refract(const Float3& ivec, const Float3& nvec, float refractionIndex, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat3(&ivec);
    const XMVECTOR n = XMLoadFloat3(&nvec);
    const XMVECTOR X = XMVector3Refract(i, n, refractionIndex);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::Refract(const Float3& ivec, const Float3& nvec, float refractionIndex) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat3(&ivec);
    const XMVECTOR n = XMLoadFloat3(&nvec);
    const XMVECTOR X = XMVector3Refract(i, n, refractionIndex);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Float3::Transform(const Float3& v, const QuaternionN& quat, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMVECTOR q = XMLoadFloat4(&quat);
    const XMVECTOR X = XMVector3Rotate(v1, q);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::Transform(const Float3& v, const QuaternionN& quat) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMVECTOR q = XMLoadFloat4(&quat);
    const XMVECTOR X = XMVector3Rotate(v1, q);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

inline void Float3::Transform(const Float3& v, const Float4x4& m, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector3TransformCoord(v1, M);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::Transform(const Float3& v, const Float4x4& m) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector3TransformCoord(v1, M);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

_Use_decl_annotations_
inline void Float3::Transform(const Float3* varray, size_t count, const Float4x4& m, Float3* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector3TransformCoordStream(resultArray, sizeof(XMFLOAT3), varray, sizeof(XMFLOAT3), count, M);
}

inline void Float3::Transform(const Float3& v, const Float4x4& m, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector3Transform(v1, M);
    XMStoreFloat4(&result, X);
}

_Use_decl_annotations_
inline void Float3::Transform(const Float3* varray, size_t count, const Float4x4& m, Float4* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector3TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT3), count, M);
}

inline void Float3::TransformNormal(const Float3& v, const Float4x4& m, Float3& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector3TransformNormal(v1, M);
    XMStoreFloat3(&result, X);
}

inline Float3 Float3::TransformNormal(const Float3& v, const Float4x4& m) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector3TransformNormal(v1, M);

    Float3 result;
    XMStoreFloat3(&result, X);
    return result;
}

_Use_decl_annotations_
inline void Float3::TransformNormal(const Float3* varray, size_t count, const Float4x4& m, Float3* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector3TransformNormalStream(resultArray, sizeof(XMFLOAT3), varray, sizeof(XMFLOAT3), count, M);
}


/****************************************************************************
 *
 * Float4
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool Float4::operator == (const Float4& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    return XMVector4Equal(v1, v2);
}

inline bool Float4::operator != (const Float4& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    return XMVector4NotEqual(v1, v2);
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline Float4& Float4::operator+= (const Float4& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    const XMVECTOR X = XMVectorAdd(v1, v2);
    XMStoreFloat4(this, X);
    return *this;
}

inline Float4& Float4::operator-= (const Float4& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    const XMVECTOR X = XMVectorSubtract(v1, v2);
    XMStoreFloat4(this, X);
    return *this;
}

inline Float4& Float4::operator*= (const Float4& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    const XMVECTOR X = XMVectorMultiply(v1, v2);
    XMStoreFloat4(this, X);
    return *this;
}

inline Float4& Float4::operator*= (float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X = XMVectorScale(v1, S);
    XMStoreFloat4(this, X);
    return *this;
}

inline Float4& Float4::operator/= (float S) noexcept
{
    using namespace DirectX;
    assert(S != 0.0f);
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X = XMVectorScale(v1, 1.f / S);
    XMStoreFloat4(this, X);
    return *this;
}

//------------------------------------------------------------------------------
// Urnary operators
//------------------------------------------------------------------------------

inline Float4 Float4::operator- () const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X = XMVectorNegate(v1);
    Float4 R;
    XMStoreFloat4(&R, X);
    return R;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline Float4 operator+ (const Float4& V1, const Float4& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V1);
    const XMVECTOR v2 = XMLoadFloat4(&V2);
    const XMVECTOR X = XMVectorAdd(v1, v2);
    Float4 R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Float4 operator- (const Float4& V1, const Float4& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V1);
    const XMVECTOR v2 = XMLoadFloat4(&V2);
    const XMVECTOR X = XMVectorSubtract(v1, v2);
    Float4 R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Float4 operator* (const Float4& V1, const Float4& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V1);
    const XMVECTOR v2 = XMLoadFloat4(&V2);
    const XMVECTOR X = XMVectorMultiply(v1, v2);
    Float4 R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Float4 operator* (const Float4& V, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V);
    const XMVECTOR X = XMVectorScale(v1, S);
    Float4 R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Float4 operator/ (const Float4& V1, const Float4& V2) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V1);
    const XMVECTOR v2 = XMLoadFloat4(&V2);
    const XMVECTOR X = XMVectorDivide(v1, v2);
    Float4 R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Float4 operator/ (const Float4& V, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V);
    const XMVECTOR X = XMVectorScale(v1, 1.f / S);
    Float4 R;
    XMStoreFloat4(&R, X);
    return R;
}

inline Float4 operator* (float S, const Float4& V) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&V);
    const XMVECTOR X = XMVectorScale(v1, S);
    Float4 R;
    XMStoreFloat4(&R, X);
    return R;
}

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

inline bool Float4::InBounds(const Float4& Bounds) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&Bounds);
    return XMVector4InBounds(v1, v2);
}

inline float Float4::Length() const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X = XMVector4Length(v1);
    return XMVectorGetX(X);
}

inline float Float4::LengthSquared() const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X = XMVector4LengthSq(v1);
    return XMVectorGetX(X);
}

inline float Float4::Dot(const Float4& V) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&V);
    const XMVECTOR X = XMVector4Dot(v1, v2);
    return XMVectorGetX(X);
}

inline void Float4::Cross(const Float4& v1, const Float4& v2, Float4& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(this);
    const XMVECTOR x2 = XMLoadFloat4(&v1);
    const XMVECTOR x3 = XMLoadFloat4(&v2);
    const XMVECTOR R = XMVector4Cross(x1, x2, x3);
    XMStoreFloat4(&result, R);
}

inline Float4 Float4::Cross(const Float4& v1, const Float4& v2) const noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(this);
    const XMVECTOR x2 = XMLoadFloat4(&v1);
    const XMVECTOR x3 = XMLoadFloat4(&v2);
    const XMVECTOR R = XMVector4Cross(x1, x2, x3);

    Float4 result;
    XMStoreFloat4(&result, R);
    return result;
}

inline void Float4::Normalize() noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X = XMVector4Normalize(v1);
    XMStoreFloat4(this, X);
}

inline void Float4::Normalize(Float4& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR X = XMVector4Normalize(v1);
    XMStoreFloat4(&result, X);
}

inline void Float4::Clamp(const Float4& vmin, const Float4& vmax) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&vmin);
    const XMVECTOR v3 = XMLoadFloat4(&vmax);
    const XMVECTOR X = XMVectorClamp(v1, v2, v3);
    XMStoreFloat4(this, X);
}

inline void Float4::Clamp(const Float4& vmin, const Float4& vmax, Float4& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(this);
    const XMVECTOR v2 = XMLoadFloat4(&vmin);
    const XMVECTOR v3 = XMLoadFloat4(&vmax);
    const XMVECTOR X = XMVectorClamp(v1, v2, v3);
    XMStoreFloat4(&result, X);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline float Float4::Distance(const Float4& v1, const Float4& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR V = XMVectorSubtract(x2, x1);
    const XMVECTOR X = XMVector4Length(V);
    return XMVectorGetX(X);
}

inline float Float4::DistanceSquared(const Float4& v1, const Float4& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR V = XMVectorSubtract(x2, x1);
    const XMVECTOR X = XMVector4LengthSq(V);
    return XMVectorGetX(X);
}

inline void Float4::Min(const Float4& v1, const Float4& v2, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X = XMVectorMin(x1, x2);
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Min(const Float4& v1, const Float4& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X = XMVectorMin(x1, x2);

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::Max(const Float4& v1, const Float4& v2, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X = XMVectorMax(x1, x2);
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Max(const Float4& v1, const Float4& v2) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X = XMVectorMax(x1, x2);

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::Lerp(const Float4& v1, const Float4& v2, float t, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Lerp(const Float4& v1, const Float4& v2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::SmoothStep(const Float4& v1, const Float4& v2, float t, Float4& result) noexcept
{
    using namespace DirectX;
    t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
    t = t * t*(3.f - 2.f*t);
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::SmoothStep(const Float4& v1, const Float4& v2, float t) noexcept
{
    using namespace DirectX;
    t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
    t = t * t*(3.f - 2.f*t);
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR X = XMVectorLerp(x1, x2, t);

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::Barycentric(const Float4& v1, const Float4& v2, const Float4& v3, float f, float g, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR x3 = XMLoadFloat4(&v3);
    const XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Barycentric(const Float4& v1, const Float4& v2, const Float4& v3, float f, float g) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR x3 = XMLoadFloat4(&v3);
    const XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::CatmullRom(const Float4& v1, const Float4& v2, const Float4& v3, const Float4& v4, float t, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR x3 = XMLoadFloat4(&v3);
    const XMVECTOR x4 = XMLoadFloat4(&v4);
    const XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::CatmullRom(const Float4& v1, const Float4& v2, const Float4& v3, const Float4& v4, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&v2);
    const XMVECTOR x3 = XMLoadFloat4(&v3);
    const XMVECTOR x4 = XMLoadFloat4(&v4);
    const XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::Hermite(const Float4& v1, const Float4& t1, const Float4& v2, const Float4& t2, float t, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&t1);
    const XMVECTOR x3 = XMLoadFloat4(&v2);
    const XMVECTOR x4 = XMLoadFloat4(&t2);
    const XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Hermite(const Float4& v1, const Float4& t1, const Float4& v2, const Float4& t2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(&v1);
    const XMVECTOR x2 = XMLoadFloat4(&t1);
    const XMVECTOR x3 = XMLoadFloat4(&v2);
    const XMVECTOR x4 = XMLoadFloat4(&t2);
    const XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::Reflect(const Float4& ivec, const Float4& nvec, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat4(&ivec);
    const XMVECTOR n = XMLoadFloat4(&nvec);
    const XMVECTOR X = XMVector4Reflect(i, n);
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Reflect(const Float4& ivec, const Float4& nvec) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat4(&ivec);
    const XMVECTOR n = XMLoadFloat4(&nvec);
    const XMVECTOR X = XMVector4Reflect(i, n);

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::Refract(const Float4& ivec, const Float4& nvec, float refractionIndex, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat4(&ivec);
    const XMVECTOR n = XMLoadFloat4(&nvec);
    const XMVECTOR X = XMVector4Refract(i, n, refractionIndex);
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Refract(const Float4& ivec, const Float4& nvec, float refractionIndex) noexcept
{
    using namespace DirectX;
    const XMVECTOR i = XMLoadFloat4(&ivec);
    const XMVECTOR n = XMLoadFloat4(&nvec);
    const XMVECTOR X = XMVector4Refract(i, n, refractionIndex);

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::Transform(const Float2& v, const QuaternionN& quat, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMVECTOR q = XMLoadFloat4(&quat);
    XMVECTOR X = XMVector3Rotate(v1, q);
    X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Transform(const Float2& v, const QuaternionN& quat) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat2(&v);
    const XMVECTOR q = XMLoadFloat4(&quat);
    XMVECTOR X = XMVector3Rotate(v1, q);
    X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::Transform(const Float3& v, const QuaternionN& quat, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMVECTOR q = XMLoadFloat4(&quat);
    XMVECTOR X = XMVector3Rotate(v1, q);
    X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Transform(const Float3& v, const QuaternionN& quat) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat3(&v);
    const XMVECTOR q = XMLoadFloat4(&quat);
    XMVECTOR X = XMVector3Rotate(v1, q);
    X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::Transform(const Float4& v, const QuaternionN& quat, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&v);
    const XMVECTOR q = XMLoadFloat4(&quat);
    XMVECTOR X = XMVector3Rotate(v1, q);
    X = XMVectorSelect(v1, X, g_XMSelect1110); // result.w = v.w
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Transform(const Float4& v, const QuaternionN& quat) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&v);
    const XMVECTOR q = XMLoadFloat4(&quat);
    XMVECTOR X = XMVector3Rotate(v1, q);
    X = XMVectorSelect(v1, X, g_XMSelect1110); // result.w = v.w

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

inline void Float4::Transform(const Float4& v, const Float4x4& m, Float4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector4Transform(v1, M);
    XMStoreFloat4(&result, X);
}

inline Float4 Float4::Transform(const Float4& v, const Float4x4& m) noexcept
{
    using namespace DirectX;
    const XMVECTOR v1 = XMLoadFloat4(&v);
    const XMMATRIX M = XMLoadFloat4x4(&m);
    const XMVECTOR X = XMVector4Transform(v1, M);

    Float4 result;
    XMStoreFloat4(&result, X);
    return result;
}

_Use_decl_annotations_
inline void Float4::Transform(const Float4* varray, size_t count, const Float4x4& m, Float4* resultArray) noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(&m);
    XMVector4TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT4), count, M);
}


/****************************************************************************
 *
 * Float4x4
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool Float4x4::operator == (const Float4x4& M) const noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    const XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    const XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    const XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    return (XMVector4Equal(x1, y1)
        && XMVector4Equal(x2, y2)
        && XMVector4Equal(x3, y3)
        && XMVector4Equal(x4, y4)) != 0;
}

inline bool Float4x4::operator != (const Float4x4& M) const noexcept
{
    using namespace DirectX;
    const XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    const XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    const XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    const XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    return (XMVector4NotEqual(x1, y1)
        || XMVector4NotEqual(x2, y2)
        || XMVector4NotEqual(x3, y3)
        || XMVector4NotEqual(x4, y4)) != 0;
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline Float4x4::Float4x4(const XMFLOAT3X3& M) noexcept
{
    _11 = M._11; _12 = M._12; _13 = M._13; _14 = 0.f;
    _21 = M._21; _22 = M._22; _23 = M._23; _24 = 0.f;
    _31 = M._31; _32 = M._32; _33 = M._33; _34 = 0.f;
    _41 = 0.f;   _42 = 0.f;   _43 = 0.f;   _44 = 1.f;
}

inline Float4x4::Float4x4(const XMFLOAT4X3& M) noexcept
{
    _11 = M._11; _12 = M._12; _13 = M._13; _14 = 0.f;
    _21 = M._21; _22 = M._22; _23 = M._23; _24 = 0.f;
    _31 = M._31; _32 = M._32; _33 = M._33; _34 = 0.f;
    _41 = M._41; _42 = M._42; _43 = M._43; _44 = 1.f;
}

inline Float4x4& Float4x4::operator= (const XMFLOAT3X3& M) noexcept
{
    _11 = M._11; _12 = M._12; _13 = M._13; _14 = 0.f;
    _21 = M._21; _22 = M._22; _23 = M._23; _24 = 0.f;
    _31 = M._31; _32 = M._32; _33 = M._33; _34 = 0.f;
    _41 = 0.f;   _42 = 0.f;   _43 = 0.f;   _44 = 1.f;
    return *this;
}

inline Float4x4& Float4x4::operator= (const XMFLOAT4X3& M) noexcept
{
    _11 = M._11; _12 = M._12; _13 = M._13; _14 = 0.f;
    _21 = M._21; _22 = M._22; _23 = M._23; _24 = 0.f;
    _31 = M._31; _32 = M._32; _33 = M._33; _34 = 0.f;
    _41 = M._41; _42 = M._42; _43 = M._43; _44 = 1.f;
    return *this;
}

inline Float4x4& Float4x4::operator+= (const Float4x4& M) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    x1 = XMVectorAdd(x1, y1);
    x2 = XMVectorAdd(x2, y2);
    x3 = XMVectorAdd(x3, y3);
    x4 = XMVectorAdd(x4, y4);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
    return *this;
}

inline Float4x4& Float4x4::operator-= (const Float4x4& M) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    x1 = XMVectorSubtract(x1, y1);
    x2 = XMVectorSubtract(x2, y2);
    x3 = XMVectorSubtract(x3, y3);
    x4 = XMVectorSubtract(x4, y4);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
    return *this;
}

inline Float4x4& Float4x4::operator*= (const Float4x4& M) noexcept
{
    using namespace DirectX;
    const XMMATRIX M1 = XMLoadFloat4x4(this);
    const XMMATRIX M2 = XMLoadFloat4x4(&M);
    const XMMATRIX X = XMMatrixMultiply(M1, M2);
    XMStoreFloat4x4(this, X);
    return *this;
}

inline Float4x4& Float4x4::operator*= (float S) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    x1 = XMVectorScale(x1, S);
    x2 = XMVectorScale(x2, S);
    x3 = XMVectorScale(x3, S);
    x4 = XMVectorScale(x4, S);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
    return *this;
}

inline Float4x4& Float4x4::operator/= (float S) noexcept
{
    using namespace DirectX;
    assert(S != 0.f);
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const float rs = 1.f / S;

    x1 = XMVectorScale(x1, rs);
    x2 = XMVectorScale(x2, rs);
    x3 = XMVectorScale(x3, rs);
    x4 = XMVectorScale(x4, rs);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
    return *this;
}

inline Float4x4& Float4x4::operator/= (const Float4x4& M) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    x1 = XMVectorDivide(x1, y1);
    x2 = XMVectorDivide(x2, y2);
    x3 = XMVectorDivide(x3, y3);
    x4 = XMVectorDivide(x4, y4);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
    return *this;
}

//------------------------------------------------------------------------------
// Urnary operators
//------------------------------------------------------------------------------

inline Float4x4 Float4x4::operator- () const noexcept
{
    using namespace DirectX;
    XMVECTOR v1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
    XMVECTOR v2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
    XMVECTOR v3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
    XMVECTOR v4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

    v1 = XMVectorNegate(v1);
    v2 = XMVectorNegate(v2);
    v3 = XMVectorNegate(v3);
    v4 = XMVectorNegate(v4);

    Float4x4 R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), v1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), v2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), v3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), v4);
    return R;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline Float4x4 operator+ (const Float4x4& M1, const Float4x4& M2) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

    x1 = XMVectorAdd(x1, y1);
    x2 = XMVectorAdd(x2, y2);
    x3 = XMVectorAdd(x3, y3);
    x4 = XMVectorAdd(x4, y4);

    Float4x4 R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Float4x4 operator- (const Float4x4& M1, const Float4x4& M2) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

    x1 = XMVectorSubtract(x1, y1);
    x2 = XMVectorSubtract(x2, y2);
    x3 = XMVectorSubtract(x3, y3);
    x4 = XMVectorSubtract(x4, y4);

    Float4x4 R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Float4x4 operator* (const Float4x4& M1, const Float4x4& M2) noexcept
{
    using namespace DirectX;
    const XMMATRIX m1 = XMLoadFloat4x4(&M1);
    const XMMATRIX m2 = XMLoadFloat4x4(&M2);
    const XMMATRIX X = XMMatrixMultiply(m1, m2);

    Float4x4 R;
    XMStoreFloat4x4(&R, X);
    return R;
}

inline Float4x4 operator* (const Float4x4& M, float S) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    x1 = XMVectorScale(x1, S);
    x2 = XMVectorScale(x2, S);
    x3 = XMVectorScale(x3, S);
    x4 = XMVectorScale(x4, S);

    Float4x4 R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Float4x4 operator/ (const Float4x4& M, float S) noexcept
{
    using namespace DirectX;
    assert(S != 0.f);

    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    const float rs = 1.f / S;

    x1 = XMVectorScale(x1, rs);
    x2 = XMVectorScale(x2, rs);
    x3 = XMVectorScale(x3, rs);
    x4 = XMVectorScale(x4, rs);

    Float4x4 R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Float4x4 operator/ (const Float4x4& M1, const Float4x4& M2) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

    x1 = XMVectorDivide(x1, y1);
    x2 = XMVectorDivide(x2, y2);
    x3 = XMVectorDivide(x3, y3);
    x4 = XMVectorDivide(x4, y4);

    Float4x4 R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

inline Float4x4 operator* (float S, const Float4x4& M) noexcept
{
    using namespace DirectX;

    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

    x1 = XMVectorScale(x1, S);
    x2 = XMVectorScale(x2, S);
    x3 = XMVectorScale(x3, S);
    x4 = XMVectorScale(x4, S);

    Float4x4 R;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
    return R;
}

//------------------------------------------------------------------------------
// Float4x4 operations
//------------------------------------------------------------------------------

inline bool Float4x4::Decompose(Float3& scale, QuaternionN& rotation, Float3& translation) noexcept
{
    using namespace DirectX;

    XMVECTOR s, r, t;

    if (!XMMatrixDecompose(&s, &r, &t, *this))
        return false;

    XMStoreFloat3(&scale, s);
    XMStoreFloat4(&rotation, r);
    XMStoreFloat3(&translation, t);

    return true;
}

inline Float4x4 Float4x4::Transpose() const noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(this);
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixTranspose(M));
    return R;
}

inline void Float4x4::Transpose(Float4x4& result) const noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(this);
    XMStoreFloat4x4(&result, XMMatrixTranspose(M));
}

inline Float4x4 Float4x4::Invert() const noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(this);
    Float4x4 R;
    XMVECTOR det;
    XMStoreFloat4x4(&R, XMMatrixInverse(&det, M));
    return R;
}

inline void Float4x4::Invert(Float4x4& result) const noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(this);
    XMVECTOR det;
    XMStoreFloat4x4(&result, XMMatrixInverse(&det, M));
}

inline float Float4x4::Determinant() const noexcept
{
    using namespace DirectX;
    const XMMATRIX M = XMLoadFloat4x4(this);
    return XMVectorGetX(XMMatrixDeterminant(M));
}

inline Float3 Float4x4::ToEuler() const noexcept
{
    const float cy = sqrtf(_33 * _33 + _31 * _31);
    const float cx = atan2f(-_32, cy);
    if (cy > 16.f * FLT_EPSILON)
    {
        return Float3(cx, atan2f(_31, _33), atan2f(_12, _22));
    }
    else
    {
        return Float3(cx, 0.f, atan2f(-_21, _11));
    }
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

_Use_decl_annotations_
inline Float4x4 Float4x4::CreateBillboard(
    const Float3& object,
    const Float3& cameraPosition,
    const Float3& cameraUp,
    const Float3* cameraForward) noexcept
{
    using namespace DirectX;
    const XMVECTOR O = XMLoadFloat3(&object);
    const XMVECTOR C = XMLoadFloat3(&cameraPosition);
    XMVECTOR Z = XMVectorSubtract(O, C);

    const XMVECTOR N = XMVector3LengthSq(Z);
    if (XMVector3Less(N, g_XMEpsilon))
    {
        if (cameraForward)
        {
            const XMVECTOR F = XMLoadFloat3(cameraForward);
            Z = XMVectorNegate(F);
        }
        else
            Z = g_XMNegIdentityR2;
    }
    else
    {
        Z = XMVector3Normalize(Z);
    }

    const XMVECTOR up = XMLoadFloat3(&cameraUp);
    XMVECTOR X = XMVector3Cross(up, Z);
    X = XMVector3Normalize(X);

    const XMVECTOR Y = XMVector3Cross(Z, X);

    XMMATRIX M;
    M.r[0] = X;
    M.r[1] = Y;
    M.r[2] = Z;
    M.r[3] = XMVectorSetW(O, 1.f);

    Float4x4 R;
    XMStoreFloat4x4(&R, M);
    return R;
}

_Use_decl_annotations_
inline Float4x4 Float4x4::CreateConstrainedBillboard(
    const Float3& object,
    const Float3& cameraPosition,
    const Float3& rotateAxis,
    const Float3* cameraForward,
    const Float3* objectForward) noexcept
{
    using namespace DirectX;

    static const XMVECTORF32 s_minAngle = { { { 0.99825467075f, 0.99825467075f, 0.99825467075f, 0.99825467075f } } }; // 1.0 - XMConvertToRadians( 0.1f );

    const XMVECTOR O = XMLoadFloat3(&object);
    const XMVECTOR C = XMLoadFloat3(&cameraPosition);
    XMVECTOR faceDir = XMVectorSubtract(O, C);

    const XMVECTOR N = XMVector3LengthSq(faceDir);
    if (XMVector3Less(N, g_XMEpsilon))
    {
        if (cameraForward)
        {
            const XMVECTOR F = XMLoadFloat3(cameraForward);
            faceDir = XMVectorNegate(F);
        }
        else
            faceDir = g_XMNegIdentityR2;
    }
    else
    {
        faceDir = XMVector3Normalize(faceDir);
    }

    const XMVECTOR Y = XMLoadFloat3(&rotateAxis);
    XMVECTOR X, Z;

    XMVECTOR dot = XMVectorAbs(XMVector3Dot(Y, faceDir));
    if (XMVector3Greater(dot, s_minAngle))
    {
        if (objectForward)
        {
            Z = XMLoadFloat3(objectForward);
            dot = XMVectorAbs(XMVector3Dot(Y, Z));
            if (XMVector3Greater(dot, s_minAngle))
            {
                dot = XMVectorAbs(XMVector3Dot(Y, g_XMNegIdentityR2));
                Z = (XMVector3Greater(dot, s_minAngle)) ? g_XMIdentityR0 : g_XMNegIdentityR2;
            }
        }
        else
        {
            dot = XMVectorAbs(XMVector3Dot(Y, g_XMNegIdentityR2));
            Z = (XMVector3Greater(dot, s_minAngle)) ? g_XMIdentityR0 : g_XMNegIdentityR2;
        }

        X = XMVector3Cross(Y, Z);
        X = XMVector3Normalize(X);

        Z = XMVector3Cross(X, Y);
        Z = XMVector3Normalize(Z);
    }
    else
    {
        X = XMVector3Cross(Y, faceDir);
        X = XMVector3Normalize(X);

        Z = XMVector3Cross(X, Y);
        Z = XMVector3Normalize(Z);
    }

    XMMATRIX M;
    M.r[0] = X;
    M.r[1] = Y;
    M.r[2] = Z;
    M.r[3] = XMVectorSetW(O, 1.f);

    Float4x4 R;
    XMStoreFloat4x4(&R, M);
    return R;
}

inline Float4x4 Float4x4::CreateTranslation(const Float3& position) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixTranslation(position.x, position.y, position.z));
    return R;
}

inline Float4x4 Float4x4::CreateTranslation(float x, float y, float z) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixTranslation(x, y, z));
    return R;
}

inline Float4x4 Float4x4::CreateScale(const Float3& scales) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixScaling(scales.x, scales.y, scales.z));
    return R;
}

inline Float4x4 Float4x4::CreateScale(float xs, float ys, float zs) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixScaling(xs, ys, zs));
    return R;
}

inline Float4x4 Float4x4::CreateScale(float scale) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixScaling(scale, scale, scale));
    return R;
}

inline Float4x4 Float4x4::CreateRotationX(float radians) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixRotationX(radians));
    return R;
}

inline Float4x4 Float4x4::CreateRotationY(float radians) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixRotationY(radians));
    return R;
}

inline Float4x4 Float4x4::CreateRotationZ(float radians) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixRotationZ(radians));
    return R;
}

inline Float4x4 Float4x4::CreateFromAxisAngle(const Float3& axis, float angle) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    const XMVECTOR a = XMLoadFloat3(&axis);
    XMStoreFloat4x4(&R, XMMatrixRotationAxis(a, angle));
    return R;
}

inline Float4x4 Float4x4::CreatePerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixPerspectiveFovRH(fov, aspectRatio, nearPlane, farPlane));
    return R;
}

inline Float4x4 Float4x4::CreatePerspective(float width, float height, float nearPlane, float farPlane) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixPerspectiveRH(width, height, nearPlane, farPlane));
    return R;
}

inline Float4x4 Float4x4::CreatePerspectiveOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixPerspectiveOffCenterRH(left, right, bottom, top, nearPlane, farPlane));
    return R;
}

inline Float4x4 Float4x4::CreateOrthographic(float width, float height, float zNearPlane, float zFarPlane) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixOrthographicRH(width, height, zNearPlane, zFarPlane));
    return R;
}

inline Float4x4 Float4x4::CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixOrthographicOffCenterRH(left, right, bottom, top, zNearPlane, zFarPlane));
    return R;
}

inline Float4x4 Float4x4::CreateLookAt(const Float3& eye, const Float3& target, const Float3& up) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    const XMVECTOR eyev = XMLoadFloat3(&eye);
    const XMVECTOR targetv = XMLoadFloat3(&target);
    const XMVECTOR upv = XMLoadFloat3(&up);
    XMStoreFloat4x4(&R, XMMatrixLookAtRH(eyev, targetv, upv));
    return R;
}

inline Float4x4 Float4x4::CreateWorld(const Float3& position, const Float3& forward, const Float3& up) noexcept
{
    using namespace DirectX;
    const XMVECTOR zaxis = XMVector3Normalize(XMVectorNegate(XMLoadFloat3(&forward)));
    XMVECTOR yaxis = XMLoadFloat3(&up);
    const XMVECTOR xaxis = XMVector3Normalize(XMVector3Cross(yaxis, zaxis));
    yaxis = XMVector3Cross(zaxis, xaxis);

    Float4x4 R;
    XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._11), xaxis);
    XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._21), yaxis);
    XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._31), zaxis);
    R._14 = R._24 = R._34 = 0.f;
    R._41 = position.x; R._42 = position.y; R._43 = position.z;
    R._44 = 1.f;
    return R;
}

inline Float4x4 Float4x4::CreateFromQuaternion(const QuaternionN& rotation) noexcept
{
    using namespace DirectX;
    const XMVECTOR quatv = XMLoadFloat4(&rotation);
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixRotationQuaternion(quatv));
    return R;
}

inline Float4x4 Float4x4::CreateFromYawPitchRoll(float yaw, float pitch, float roll) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
    return R;
}

inline Float4x4 Float4x4::CreateFromYawPitchRoll(const Float3& angles) noexcept
{
    using namespace DirectX;
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixRotationRollPitchYawFromVector(angles));
    return R;
}

inline Float4x4 Float4x4::CreateShadow(const Float3& lightDir, const PlaneN& plane) noexcept
{
    using namespace DirectX;
    const XMVECTOR light = XMLoadFloat3(&lightDir);
    const XMVECTOR planev = XMLoadFloat4(&plane);
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixShadow(planev, light));
    return R;
}

inline Float4x4 Float4x4::CreateReflection(const PlaneN& plane) noexcept
{
    using namespace DirectX;
    const XMVECTOR planev = XMLoadFloat4(&plane);
    Float4x4 R;
    XMStoreFloat4x4(&R, XMMatrixReflect(planev));
    return R;
}

inline void Float4x4::Lerp(const Float4x4& M1, const Float4x4& M2, float t, Float4x4& result) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

    x1 = XMVectorLerp(x1, y1, t);
    x2 = XMVectorLerp(x2, y2, t);
    x3 = XMVectorLerp(x3, y3, t);
    x4 = XMVectorLerp(x4, y4, t);

    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._41), x4);
}

inline Float4x4 Float4x4::Lerp(const Float4x4& M1, const Float4x4& M2, float t) noexcept
{
    using namespace DirectX;
    XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
    XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
    XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
    XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

    const XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
    const XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
    const XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
    const XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

    x1 = XMVectorLerp(x1, y1, t);
    x2 = XMVectorLerp(x2, y2, t);
    x3 = XMVectorLerp(x3, y3, t);
    x4 = XMVectorLerp(x4, y4, t);

    Float4x4 result;
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._11), x1);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._21), x2);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._31), x3);
    XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._41), x4);
    return result;
}

inline void Float4x4::Transform(const Float4x4& M, const QuaternionN& rotation, Float4x4& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR quatv = XMLoadFloat4(&rotation);

    const XMMATRIX M0 = XMLoadFloat4x4(&M);
    const XMMATRIX M1 = XMMatrixRotationQuaternion(quatv);

    XMStoreFloat4x4(&result, XMMatrixMultiply(M0, M1));
}

inline Float4x4 Float4x4::Transform(const Float4x4& M, const QuaternionN& rotation) noexcept
{
    using namespace DirectX;
    const XMVECTOR quatv = XMLoadFloat4(&rotation);

    const XMMATRIX M0 = XMLoadFloat4x4(&M);
    const XMMATRIX M1 = XMMatrixRotationQuaternion(quatv);

    Float4x4 result;
    XMStoreFloat4x4(&result, XMMatrixMultiply(M0, M1));
    return result;
}


/****************************************************************************
 *
 * PlaneN
 *
 ****************************************************************************/

inline PlaneN::PlaneN(const Float3& point1, const Float3& point2, const Float3& point3) noexcept
{
    using namespace DirectX;
    const XMVECTOR P0 = XMLoadFloat3(&point1);
    const XMVECTOR P1 = XMLoadFloat3(&point2);
    const XMVECTOR P2 = XMLoadFloat3(&point3);
    XMStoreFloat4(this, XMPlaneFromPoints(P0, P1, P2));
}

inline PlaneN::PlaneN(const Float3& point, const Float3& normal) noexcept
{
    using namespace DirectX;
    const XMVECTOR P = XMLoadFloat3(&point);
    const XMVECTOR N = XMLoadFloat3(&normal);
    XMStoreFloat4(this, XMPlaneFromPointNormal(P, N));
}

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool PlaneN::operator == (const PlaneN& p) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p1 = XMLoadFloat4(this);
    const XMVECTOR p2 = XMLoadFloat4(&p);
    return XMPlaneEqual(p1, p2);
}

inline bool PlaneN::operator != (const PlaneN& p) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p1 = XMLoadFloat4(this);
    const XMVECTOR p2 = XMLoadFloat4(&p);
    return XMPlaneNotEqual(p1, p2);
}

//------------------------------------------------------------------------------
// PlaneN operations
//------------------------------------------------------------------------------

inline void PlaneN::Normalize() noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(this);
    XMStoreFloat4(this, XMPlaneNormalize(p));
}

inline void PlaneN::Normalize(PlaneN& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMPlaneNormalize(p));
}

inline float PlaneN::Dot(const Float4& v) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(this);
    const XMVECTOR v0 = XMLoadFloat4(&v);
    return XMVectorGetX(XMPlaneDot(p, v0));
}

inline float PlaneN::DotCoordinate(const Float3& position) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(this);
    const XMVECTOR v0 = XMLoadFloat3(&position);
    return XMVectorGetX(XMPlaneDotCoord(p, v0));
}

inline float PlaneN::DotNormal(const Float3& normal) const noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(this);
    const XMVECTOR n0 = XMLoadFloat3(&normal);
    return XMVectorGetX(XMPlaneDotNormal(p, n0));
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline void PlaneN::Transform(const PlaneN& plane, const Float4x4& M, PlaneN& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(&plane);
    const XMMATRIX m0 = XMLoadFloat4x4(&M);
    XMStoreFloat4(&result, XMPlaneTransform(p, m0));
}

inline PlaneN PlaneN::Transform(const PlaneN& plane, const Float4x4& M) noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(&plane);
    const XMMATRIX m0 = XMLoadFloat4x4(&M);

    PlaneN result;
    XMStoreFloat4(&result, XMPlaneTransform(p, m0));
    return result;
}

inline void PlaneN::Transform(const PlaneN& plane, const QuaternionN& rotation, PlaneN& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(&plane);
    const XMVECTOR q = XMLoadFloat4(&rotation);
    XMVECTOR X = XMVector3Rotate(p, q);
    X = XMVectorSelect(p, X, g_XMSelect1110); // result.d = plane.d
    XMStoreFloat4(&result, X);
}

inline PlaneN PlaneN::Transform(const PlaneN& plane, const QuaternionN& rotation) noexcept
{
    using namespace DirectX;
    const XMVECTOR p = XMLoadFloat4(&plane);
    const XMVECTOR q = XMLoadFloat4(&rotation);
    XMVECTOR X = XMVector3Rotate(p, q);
    X = XMVectorSelect(p, X, g_XMSelect1110); // result.d = plane.d

    PlaneN result;
    XMStoreFloat4(&result, X);
    return result;
}


/****************************************************************************
 *
 * QuaternionN
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool QuaternionN::operator == (const QuaternionN& q) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    return XMQuaternionEqual(q1, q2);
}

inline bool QuaternionN::operator != (const QuaternionN& q) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    return XMQuaternionNotEqual(q1, q2);
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline QuaternionN& QuaternionN::operator+= (const QuaternionN& q) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    XMStoreFloat4(this, XMVectorAdd(q1, q2));
    return *this;
}

inline QuaternionN& QuaternionN::operator-= (const QuaternionN& q) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    XMStoreFloat4(this, XMVectorSubtract(q1, q2));
    return *this;
}

inline QuaternionN& QuaternionN::operator*= (const QuaternionN& q) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    XMStoreFloat4(this, XMQuaternionMultiply(q1, q2));
    return *this;
}

inline QuaternionN& QuaternionN::operator*= (float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(this, XMVectorScale(q, S));
    return *this;
}

inline QuaternionN& QuaternionN::operator/= (const QuaternionN& q) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    XMVECTOR q2 = XMLoadFloat4(&q);
    q2 = XMQuaternionInverse(q2);
    XMStoreFloat4(this, XMQuaternionMultiply(q1, q2));
    return *this;
}

//------------------------------------------------------------------------------
// Urnary operators
//------------------------------------------------------------------------------

inline QuaternionN QuaternionN::operator- () const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);

    QuaternionN R;
    XMStoreFloat4(&R, XMVectorNegate(q));
    return R;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline QuaternionN operator+ (const QuaternionN& Q1, const QuaternionN& Q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(&Q1);
    const XMVECTOR q2 = XMLoadFloat4(&Q2);

    QuaternionN R;
    XMStoreFloat4(&R, XMVectorAdd(q1, q2));
    return R;
}

inline QuaternionN operator- (const QuaternionN& Q1, const QuaternionN& Q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(&Q1);
    const XMVECTOR q2 = XMLoadFloat4(&Q2);

    QuaternionN R;
    XMStoreFloat4(&R, XMVectorSubtract(q1, q2));
    return R;
}

inline QuaternionN operator* (const QuaternionN& Q1, const QuaternionN& Q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(&Q1);
    const XMVECTOR q2 = XMLoadFloat4(&Q2);

    QuaternionN R;
    XMStoreFloat4(&R, XMQuaternionMultiply(q1, q2));
    return R;
}

inline QuaternionN operator* (const QuaternionN& Q, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(&Q);

    QuaternionN R;
    XMStoreFloat4(&R, XMVectorScale(q, S));
    return R;
}

inline QuaternionN operator/ (const QuaternionN& Q1, const QuaternionN& Q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(&Q1);
    XMVECTOR q2 = XMLoadFloat4(&Q2);
    q2 = XMQuaternionInverse(q2);

    QuaternionN R;
    XMStoreFloat4(&R, XMQuaternionMultiply(q1, q2));
    return R;
}

inline QuaternionN operator* (float S, const QuaternionN& Q) noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(&Q);

    QuaternionN R;
    XMStoreFloat4(&R, XMVectorScale(q1, S));
    return R;
}

//------------------------------------------------------------------------------
// QuaternionN operations
//------------------------------------------------------------------------------

inline float QuaternionN::Length() const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    return XMVectorGetX(XMQuaternionLength(q));
}

inline float QuaternionN::LengthSquared() const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    return XMVectorGetX(XMQuaternionLengthSq(q));
}

inline void QuaternionN::Normalize() noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(this, XMQuaternionNormalize(q));
}

inline void QuaternionN::Normalize(QuaternionN& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMQuaternionNormalize(q));
}

inline void QuaternionN::Conjugate() noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(this, XMQuaternionConjugate(q));
}

inline void QuaternionN::Conjugate(QuaternionN& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMQuaternionConjugate(q));
}

inline void QuaternionN::Inverse(QuaternionN& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMQuaternionInverse(q));
}

inline float QuaternionN::Dot(const QuaternionN& q) const noexcept
{
    using namespace DirectX;
    const XMVECTOR q1 = XMLoadFloat4(this);
    const XMVECTOR q2 = XMLoadFloat4(&q);
    return XMVectorGetX(XMQuaternionDot(q1, q2));
}

inline void QuaternionN::RotateTowards(const QuaternionN& target, float maxAngle) noexcept
{
    RotateTowards(target, maxAngle, *this);
}

inline Float3 QuaternionN::ToEuler() const noexcept
{
    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;

    const float m31 = 2.f * x * z + 2.f * y * w;
    const float m32 = 2.f * y * z - 2.f * x * w;
    const float m33 = 1.f - 2.f * xx - 2.f * yy;

    const float cy = sqrtf(m33 * m33 + m31 * m31);
    const float cx = atan2f(-m32, cy);
    if (cy > 16.f * FLT_EPSILON)
    {
        const float m12 = 2.f * x * y + 2.f * z * w;
        const float m22 = 1.f - 2.f * xx - 2.f * zz;

        return Float3(cx, atan2f(m31, m33), atan2f(m12, m22));
    }
    else
    {
        const float m11 = 1.f - 2.f * yy - 2.f * zz;
        const float m21 = 2.f * x * y - 2.f * z * w;

        return Float3(cx, 0.f, atan2f(-m21, m11));
    }
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline QuaternionN QuaternionN::CreateFromAxisAngle(const Float3& axis, float angle) noexcept
{
    using namespace DirectX;
    const XMVECTOR a = XMLoadFloat3(&axis);

    QuaternionN R;
    XMStoreFloat4(&R, XMQuaternionRotationAxis(a, angle));
    return R;
}

inline QuaternionN QuaternionN::CreateFromYawPitchRoll(float yaw, float pitch, float roll) noexcept
{
    using namespace DirectX;
    QuaternionN R;
    XMStoreFloat4(&R, XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
    return R;
}

inline QuaternionN QuaternionN::CreateFromYawPitchRoll(const Float3& angles) noexcept
{
    using namespace DirectX;
    QuaternionN R;
    XMStoreFloat4(&R, XMQuaternionRotationRollPitchYawFromVector(angles));
    return R;
}

inline QuaternionN QuaternionN::CreateFromRotationMatrix(const Float4x4& M) noexcept
{
    using namespace DirectX;
    const XMMATRIX M0 = XMLoadFloat4x4(&M);

    QuaternionN R;
    XMStoreFloat4(&R, XMQuaternionRotationMatrix(M0));
    return R;
}

inline void QuaternionN::Lerp(const QuaternionN& q1, const QuaternionN& q2, float t, QuaternionN& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);

    const XMVECTOR dot = XMVector4Dot(Q0, Q1);

    XMVECTOR R;
    if (XMVector4GreaterOrEqual(dot, XMVectorZero()))
    {
        R = XMVectorLerp(Q0, Q1, t);
    }
    else
    {
        const XMVECTOR tv = XMVectorReplicate(t);
        const XMVECTOR t1v = XMVectorReplicate(1.f - t);
        const XMVECTOR X0 = XMVectorMultiply(Q0, t1v);
        const XMVECTOR X1 = XMVectorMultiply(Q1, tv);
        R = XMVectorSubtract(X0, X1);
    }

    XMStoreFloat4(&result, XMQuaternionNormalize(R));
}

inline QuaternionN QuaternionN::Lerp(const QuaternionN& q1, const QuaternionN& q2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);

    const XMVECTOR dot = XMVector4Dot(Q0, Q1);

    XMVECTOR R;
    if (XMVector4GreaterOrEqual(dot, XMVectorZero()))
    {
        R = XMVectorLerp(Q0, Q1, t);
    }
    else
    {
        const XMVECTOR tv = XMVectorReplicate(t);
        const XMVECTOR t1v = XMVectorReplicate(1.f - t);
        const XMVECTOR X0 = XMVectorMultiply(Q0, t1v);
        const XMVECTOR X1 = XMVectorMultiply(Q1, tv);
        R = XMVectorSubtract(X0, X1);
    }

    QuaternionN result;
    XMStoreFloat4(&result, XMQuaternionNormalize(R));
    return result;
}

inline void QuaternionN::Slerp(const QuaternionN& q1, const QuaternionN& q2, float t, QuaternionN& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);
    XMStoreFloat4(&result, XMQuaternionSlerp(Q0, Q1, t));
}

inline QuaternionN QuaternionN::Slerp(const QuaternionN& q1, const QuaternionN& q2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);

    QuaternionN result;
    XMStoreFloat4(&result, XMQuaternionSlerp(Q0, Q1, t));
    return result;
}

inline void QuaternionN::Concatenate(const QuaternionN& q1, const QuaternionN& q2, QuaternionN& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);
    XMStoreFloat4(&result, XMQuaternionMultiply(Q1, Q0));
}

inline QuaternionN QuaternionN::Concatenate(const QuaternionN& q1, const QuaternionN& q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);

    QuaternionN result;
    XMStoreFloat4(&result, XMQuaternionMultiply(Q1, Q0));
    return result;
}

inline QuaternionN QuaternionN::FromToRotation(const Float3& fromDir, const Float3& toDir) noexcept
{
    QuaternionN result;
    FromToRotation(fromDir, toDir, result);
    return result;
}

inline QuaternionN QuaternionN::LookRotation(const Float3& forward, const Float3& up) noexcept
{
    QuaternionN result;
    LookRotation(forward, up, result);
    return result;
}

inline float QuaternionN::Angle(const QuaternionN& q1, const QuaternionN& q2) noexcept
{
    using namespace DirectX;
    const XMVECTOR Q0 = XMLoadFloat4(&q1);
    const XMVECTOR Q1 = XMLoadFloat4(&q2);

    // We can use the conjugate here instead of inverse assuming q1 & q2 are normalized.
    XMVECTOR R = XMQuaternionMultiply(XMQuaternionConjugate(Q0), Q1);

    const float rs = XMVectorGetW(R);
    R = XMVector3Length(R);
    return 2.f * atan2f(XMVectorGetX(R), rs);
}


/****************************************************************************
 *
 * ColorN
 *
 ****************************************************************************/

inline ColorN::ColorN(const DirectX::PackedVector::XMCOLOR& Packed) noexcept
{
    using namespace DirectX;
    XMStoreFloat4(this, PackedVector::XMLoadColor(&Packed));
}

inline ColorN::ColorN(const DirectX::PackedVector::XMUBYTEN4& Packed) noexcept
{
    using namespace DirectX;
    XMStoreFloat4(this, PackedVector::XMLoadUByteN4(&Packed));
}

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------
inline bool ColorN::operator == (const ColorN& c) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    return XMColorEqual(c1, c2);
}

inline bool ColorN::operator != (const ColorN& c) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    return XMColorNotEqual(c1, c2);
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline ColorN& ColorN::operator= (const DirectX::PackedVector::XMCOLOR& Packed) noexcept
{
    using namespace DirectX;
    XMStoreFloat4(this, PackedVector::XMLoadColor(&Packed));
    return *this;
}

inline ColorN& ColorN::operator= (const DirectX::PackedVector::XMUBYTEN4& Packed) noexcept
{
    using namespace DirectX;
    XMStoreFloat4(this, PackedVector::XMLoadUByteN4(&Packed));
    return *this;
}

inline ColorN& ColorN::operator+= (const ColorN& c) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    XMStoreFloat4(this, XMVectorAdd(c1, c2));
    return *this;
}

inline ColorN& ColorN::operator-= (const ColorN& c) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    XMStoreFloat4(this, XMVectorSubtract(c1, c2));
    return *this;
}

inline ColorN& ColorN::operator*= (const ColorN& c) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    XMStoreFloat4(this, XMVectorMultiply(c1, c2));
    return *this;
}

inline ColorN& ColorN::operator*= (float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(this, XMVectorScale(c, S));
    return *this;
}

inline ColorN& ColorN::operator/= (const ColorN& c) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(this);
    const XMVECTOR c2 = XMLoadFloat4(&c);
    XMStoreFloat4(this, XMVectorDivide(c1, c2));
    return *this;
}

//------------------------------------------------------------------------------
// Urnary operators
//------------------------------------------------------------------------------

inline ColorN ColorN::operator- () const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    ColorN R;
    XMStoreFloat4(&R, XMVectorNegate(c));
    return R;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline ColorN operator+ (const ColorN& C1, const ColorN& C2) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(&C1);
    const XMVECTOR c2 = XMLoadFloat4(&C2);
    ColorN R;
    XMStoreFloat4(&R, XMVectorAdd(c1, c2));
    return R;
}

inline ColorN operator- (const ColorN& C1, const ColorN& C2) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(&C1);
    const XMVECTOR c2 = XMLoadFloat4(&C2);
    ColorN R;
    XMStoreFloat4(&R, XMVectorSubtract(c1, c2));
    return R;
}

inline ColorN operator* (const ColorN& C1, const ColorN& C2) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(&C1);
    const XMVECTOR c2 = XMLoadFloat4(&C2);
    ColorN R;
    XMStoreFloat4(&R, XMVectorMultiply(c1, c2));
    return R;
}

inline ColorN operator* (const ColorN& C, float S) noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(&C);
    ColorN R;
    XMStoreFloat4(&R, XMVectorScale(c, S));
    return R;
}

inline ColorN operator/ (const ColorN& C1, const ColorN& C2) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(&C1);
    const XMVECTOR c2 = XMLoadFloat4(&C2);
    ColorN R;
    XMStoreFloat4(&R, XMVectorDivide(c1, c2));
    return R;
}

inline ColorN operator* (float S, const ColorN& C) noexcept
{
    using namespace DirectX;
    const XMVECTOR c1 = XMLoadFloat4(&C);
    ColorN R;
    XMStoreFloat4(&R, XMVectorScale(c1, S));
    return R;
}

//------------------------------------------------------------------------------
// ColorN operations
//------------------------------------------------------------------------------

inline DirectX::PackedVector::XMCOLOR ColorN::BGRA() const noexcept
{
    using namespace DirectX;
    const XMVECTOR clr = XMLoadFloat4(this);
    PackedVector::XMCOLOR Packed;
    PackedVector::XMStoreColor(&Packed, clr);
    return Packed;
}

inline DirectX::PackedVector::XMUBYTEN4 ColorN::RGBA() const noexcept
{
    using namespace DirectX;
    const XMVECTOR clr = XMLoadFloat4(this);
    PackedVector::XMUBYTEN4 Packed;
    PackedVector::XMStoreUByteN4(&Packed, clr);
    return Packed;
}

inline Float3 ColorN::ToVector3() const noexcept
{
    return Float3(x, y, z);
}

inline Float4 ColorN::ToVector4() const noexcept
{
    return Float4(x, y, z, w);
}

inline void ColorN::Negate() noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(this, XMColorNegative(c));
}

inline void ColorN::Negate(ColorN& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMColorNegative(c));
}

inline void ColorN::Saturate() noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(this, XMVectorSaturate(c));
}

inline void ColorN::Saturate(ColorN& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMVectorSaturate(c));
}

inline void ColorN::Premultiply() noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMVECTOR a = XMVectorSplatW(c);
    a = XMVectorSelect(g_XMIdentityR3, a, g_XMSelect1110);
    XMStoreFloat4(this, XMVectorMultiply(c, a));
}

inline void ColorN::Premultiply(ColorN& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMVECTOR a = XMVectorSplatW(c);
    a = XMVectorSelect(g_XMIdentityR3, a, g_XMSelect1110);
    XMStoreFloat4(&result, XMVectorMultiply(c, a));
}

inline void ColorN::AdjustSaturation(float sat) noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(this, XMColorAdjustSaturation(c, sat));
}

inline void ColorN::AdjustSaturation(float sat, ColorN& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMColorAdjustSaturation(c, sat));
}

inline void ColorN::AdjustContrast(float contrast) noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(this, XMColorAdjustContrast(c, contrast));
}

inline void ColorN::AdjustContrast(float contrast, ColorN& result) const noexcept
{
    using namespace DirectX;
    const XMVECTOR c = XMLoadFloat4(this);
    XMStoreFloat4(&result, XMColorAdjustContrast(c, contrast));
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline void ColorN::Modulate(const ColorN& c1, const ColorN& c2, ColorN& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR C0 = XMLoadFloat4(&c1);
    const XMVECTOR C1 = XMLoadFloat4(&c2);
    XMStoreFloat4(&result, XMColorModulate(C0, C1));
}

inline ColorN ColorN::Modulate(const ColorN& c1, const ColorN& c2) noexcept
{
    using namespace DirectX;
    const XMVECTOR C0 = XMLoadFloat4(&c1);
    const XMVECTOR C1 = XMLoadFloat4(&c2);

    ColorN result;
    XMStoreFloat4(&result, XMColorModulate(C0, C1));
    return result;
}

inline void ColorN::Lerp(const ColorN& c1, const ColorN& c2, float t, ColorN& result) noexcept
{
    using namespace DirectX;
    const XMVECTOR C0 = XMLoadFloat4(&c1);
    const XMVECTOR C1 = XMLoadFloat4(&c2);
    XMStoreFloat4(&result, XMVectorLerp(C0, C1, t));
}

inline ColorN ColorN::Lerp(const ColorN& c1, const ColorN& c2, float t) noexcept
{
    using namespace DirectX;
    const XMVECTOR C0 = XMLoadFloat4(&c1);
    const XMVECTOR C1 = XMLoadFloat4(&c2);

    ColorN result;
    XMStoreFloat4(&result, XMVectorLerp(C0, C1, t));
    return result;
}


/****************************************************************************
 *
 * RayN
 *
 ****************************************************************************/

//-----------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------
inline bool RayN::operator == (const RayN& r) const noexcept
{
    using namespace DirectX;
    const XMVECTOR r1p = XMLoadFloat3(&position);
    const XMVECTOR r2p = XMLoadFloat3(&r.position);
    const XMVECTOR r1d = XMLoadFloat3(&direction);
    const XMVECTOR r2d = XMLoadFloat3(&r.direction);
    return XMVector3Equal(r1p, r2p) && XMVector3Equal(r1d, r2d);
}

inline bool RayN::operator != (const RayN& r) const noexcept
{
    using namespace DirectX;
    const XMVECTOR r1p = XMLoadFloat3(&position);
    const XMVECTOR r2p = XMLoadFloat3(&r.position);
    const XMVECTOR r1d = XMLoadFloat3(&direction);
    const XMVECTOR r2d = XMLoadFloat3(&r.direction);
    return XMVector3NotEqual(r1p, r2p) && XMVector3NotEqual(r1d, r2d);
}

//-----------------------------------------------------------------------------
// RayN operators
//------------------------------------------------------------------------------

inline bool RayN::Intersects(const BoundingSphere& sphere, _Out_ float& Dist) const noexcept
{
    return sphere.Intersects(position, direction, Dist);
}

inline bool RayN::Intersects(const BoundingBox& box, _Out_ float& Dist) const noexcept
{
    return box.Intersects(position, direction, Dist);
}

inline bool RayN::Intersects(const Float3& tri0, const Float3& tri1, const Float3& tri2, _Out_ float& Dist) const noexcept
{
    return DirectX::TriangleTests::Intersects(position, direction, tri0, tri1, tri2, Dist);
}

inline bool RayN::Intersects(const PlaneN& plane, _Out_ float& Dist) const noexcept
{
    using namespace DirectX;

    const XMVECTOR p = XMLoadFloat4(&plane);
    const XMVECTOR dir = XMLoadFloat3(&direction);

    const XMVECTOR nd = XMPlaneDotNormal(p, dir);

    if (XMVector3LessOrEqual(XMVectorAbs(nd), g_RayEpsilon))
    {
        Dist = 0.f;
        return false;
    }
    else
    {
        // t = -(dot(n,origin) + D) / dot(n,dir)
        const XMVECTOR pos = XMLoadFloat3(&position);
        XMVECTOR v = XMPlaneDotNormal(p, pos);
        v = XMVectorAdd(v, XMVectorSplatW(p));
        v = XMVectorDivide(v, nd);
        float dist = -XMVectorGetX(v);
        if (dist < 0)
        {
            Dist = 0.f;
            return false;
        }
        else
        {
            Dist = dist;
            return true;
        }
    }
}


/****************************************************************************
 *
 * ViewportN
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

#if (__cplusplus < 202002L)
inline bool ViewportN::operator == (const ViewportN& vp) const noexcept
{
    return (x == vp.x && y == vp.y
        && width == vp.width && height == vp.height
        && minDepth == vp.minDepth && maxDepth == vp.maxDepth);
}

inline bool ViewportN::operator != (const ViewportN& vp) const noexcept
{
    return (x != vp.x || y != vp.y
        || width != vp.width || height != vp.height
        || minDepth != vp.minDepth || maxDepth != vp.maxDepth);
}
#endif

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// ViewportN operations
//------------------------------------------------------------------------------

inline float ViewportN::AspectRatio() const noexcept
{
    if (width == 0.f || height == 0.f)
        return 0.f;

    return (width / height);
}

inline Float3 ViewportN::Project(const Float3& p, const Float4x4& proj, const Float4x4& view, const Float4x4& world) const noexcept
{
    using namespace DirectX;
    XMVECTOR v = XMLoadFloat3(&p);
    const XMMATRIX projection = XMLoadFloat4x4(&proj);
    v = XMVector3Project(v, x, y, width, height, minDepth, maxDepth, projection, view, world);
    Float3 result;
    XMStoreFloat3(&result, v);
    return result;
}

inline void ViewportN::Project(const Float3& p, const Float4x4& proj, const Float4x4& view, const Float4x4& world, Float3& result) const noexcept
{
    using namespace DirectX;
    XMVECTOR v = XMLoadFloat3(&p);
    const XMMATRIX projection = XMLoadFloat4x4(&proj);
    v = XMVector3Project(v, x, y, width, height, minDepth, maxDepth, projection, view, world);
    XMStoreFloat3(&result, v);
}

inline Float3 ViewportN::Unproject(const Float3& p, const Float4x4& proj, const Float4x4& view, const Float4x4& world) const noexcept
{
    using namespace DirectX;
    XMVECTOR v = XMLoadFloat3(&p);
    const XMMATRIX projection = XMLoadFloat4x4(&proj);
    v = XMVector3Unproject(v, x, y, width, height, minDepth, maxDepth, projection, view, world);
    Float3 result;
    XMStoreFloat3(&result, v);
    return result;
}

inline void ViewportN::Unproject(const Float3& p, const Float4x4& proj, const Float4x4& view, const Float4x4& world, Float3& result) const noexcept
{
    using namespace DirectX;
    XMVECTOR v = XMLoadFloat3(&p);
    const XMMATRIX projection = XMLoadFloat4x4(&proj);
    v = XMVector3Unproject(v, x, y, width, height, minDepth, maxDepth, projection, view, world);
    XMStoreFloat3(&result, v);
}
