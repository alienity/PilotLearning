//-------------------------------------------------------------------------------------
// Simple Math library for game programing
//
// Right-hand coordinates
//
// Matrix: row-major storage, row-major notation, post-multiplocation
// Vector: column-major notation
//
//-------------------------------------------------------------------------------------

#pragma once

#include <intrin.h>
#include <algorithm>
#include <cassert>
#include <limits>
#include <random>

#define CMP(x, y) (fabsf(x - y) < FLT_EPSILON * fmaxf(1.0f, fmaxf(fabsf(x), fabsf(y))))

#define MOYU_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MOYU_MIN3(x, y, z) MOYU_MIN(MOYU_MIN(x, y), z)
#define MOYU_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MOYU_MAX3(x, y, z) MOYU_MAX(MOYU_MAX(x, y), z)
#define MOYU_CLAMP(a, min_value, max_value) MOYU_MIN(max_value, MOYU_MAX(a, min_value))

#define MOYU_VALID_INDEX(idx, range) (((idx) >= 0) && ((idx) < (range)))
#define MOYU_CLAMP_INDEX(idx, range) MOYU_CLAMP(idx, 0, (range)-1)

#define MOYU_SIGN(x) ((((x) > 0.0f) ? 1.0f : 0.0f) + (((x) < 0.0f) ? -1.0f : 0.0f))

#define INLINE __forceinline

namespace MoYu
{
    template<typename T>
    __forceinline T AlignUpWithMask(T value, size_t mask)
    {
        return (T)(((size_t)value + mask) & ~mask);
    }

    template<typename T>
    __forceinline T AlignDownWithMask(T value, size_t mask)
    {
        return (T)((size_t)value & ~mask);
    }

    template<typename T>
    __forceinline T AlignUp(T value, size_t alignment)
    {
        return AlignUpWithMask(value, alignment - 1);
    }

    template<typename T>
    __forceinline T AlignDown(T value, size_t alignment)
    {
        return AlignDownWithMask(value, alignment - 1);
    }

    template<typename T>
    __forceinline bool IsAligned(T value, size_t alignment)
    {
        return 0 == ((size_t)value & (alignment - 1));
    }

    template<typename T>
    __forceinline T DivideByMultiple(T value, size_t alignment)
    {
        return (T)((value + alignment - 1) / alignment);
    }

    template<typename T>
    __forceinline bool IsPowerOfTwo(T value)
    {
        return 0 == (value & (value - 1));
    }

    template<typename T>
    __forceinline bool IsDivisible(T value, T divisor)
    {
        return (value / divisor) * divisor == value;
    }

    __forceinline uint8_t Log2(uint64_t value)
    {
        unsigned long mssb; // most significant set bit
        unsigned long lssb; // least significant set bit

        // If perfect power of two (only one set bit), return index of bit.  Otherwise round up
        // fractional log by adding 1 to most signicant set bit's index.
        if (_BitScanReverse64(&mssb, value) > 0 && _BitScanForward64(&lssb, value) > 0)
            return uint8_t(mssb + (mssb == lssb ? 0 : 1));
        else
            return 0;
    }

    template<typename T>
    __forceinline T AlignPowerOfTwo(T value)
    {
        return value == 0 ? 0 : 1 << Log2(value);
    }
}

namespace MoYu
{
    static const float Math_POS_INFINITY = std::numeric_limits<float>::infinity();
    static const float Math_NEG_INFINITY = -std::numeric_limits<float>::infinity();
    static const float Math_PI           = 3.14159265358979323846264338327950288f;
    static const float Math_ONE_OVER_PI  = 1.0f / Math_PI;
    static const float Math_TWO_PI       = 2.0f * Math_PI;
    static const float Math_HALF_PI      = 0.5f * Math_PI;
    static const float Math_fDeg2Rad     = Math_PI / 180.0f;
    static const float Math_fRad2Deg     = 180.0f / Math_PI;
    static const float Math_LOG2         = log(2.0f);
    static const float Math_EPSILON      = 1e-6f;

    static const float Float_EPSILON  = FLT_EPSILON;
    static const float Double_EPSILON = DBL_EPSILON;

    struct Vector2;
    struct Vector3;
    struct Vector4;
    struct Matrix3x3;
    struct Matrix4x4;
    struct Quaternion;
    struct Transform;
    struct AxisAlignedBox;

    struct Radian;
    struct Angle;
    struct Degree;

    struct Radian
    {
        float m_rad;

    public:
        explicit Radian(float r = 0) : m_rad(r) {}
        explicit Radian(const Degree& d);
        Radian& operator=(float f)
        {
            m_rad = f;
            return *this;
        }
        Radian& operator=(const Degree& d);

        float valueRadians() const { return m_rad; }
        float valueDegrees() const; // see bottom of this file
        float valueAngleUnits() const;

        void setValue(float f) { m_rad = f; }

        const Radian& operator+() const { return *this; }
        Radian        operator+(const Radian& r) const { return Radian(m_rad + r.m_rad); }
        Radian        operator+(const Degree& d) const;
        Radian&       operator+=(const Radian& r)
        {
            m_rad += r.m_rad;
            return *this;
        }
        Radian& operator+=(const Degree& d);
        Radian  operator-() const { return Radian(-m_rad); }
        Radian  operator-(const Radian& r) const { return Radian(m_rad - r.m_rad); }
        Radian  operator-(const Degree& d) const;
        Radian& operator-=(const Radian& r)
        {
            m_rad -= r.m_rad;
            return *this;
        }
        Radian& operator-=(const Degree& d);
        Radian  operator*(float f) const { return Radian(m_rad * f); }
        Radian  operator*(const Radian& f) const { return Radian(m_rad * f.m_rad); }
        Radian& operator*=(float f)
        {
            m_rad *= f;
            return *this;
        }
        Radian  operator/(float f) const { return Radian(m_rad / f); }
        Radian& operator/=(float f)
        {
            m_rad /= f;
            return *this;
        }

        bool operator<(const Radian& r) const { return m_rad < r.m_rad; }
        bool operator<=(const Radian& r) const { return m_rad <= r.m_rad; }
        bool operator==(const Radian& r) const { return m_rad == r.m_rad; }
        bool operator!=(const Radian& r) const { return m_rad != r.m_rad; }
        bool operator>=(const Radian& r) const { return m_rad >= r.m_rad; }
        bool operator>(const Radian& r) const { return m_rad > r.m_rad; }
    };

    /** Wrapper class which indicates a given angle value is in Degrees.
    @remarks
        Degree values are interchangeable with Radian values, and conversions
        will be done automatically between them.
    */
    struct Degree
    {
        float m_deg; // if you get an error here - make sure to define/typedef 'float' first

    public:
        explicit Degree(float d = 0) : m_deg(d) {}
        explicit Degree(const Radian& r) : m_deg(r.valueDegrees()) {}
        Degree& operator=(float f)
        {
            m_deg = f;
            return *this;
        }
        Degree& operator=(const Degree& d) = default;
        Degree& operator                   =(const Radian& r)
        {
            m_deg = r.valueDegrees();
            return *this;
        }

        float valueDegrees() const { return m_deg; }
        float valueRadians() const; // see bottom of this file
        float valueAngleUnits() const;

        const Degree& operator+() const { return *this; }
        Degree        operator+(const Degree& d) const { return Degree(m_deg + d.m_deg); }
        Degree        operator+(const Radian& r) const { return Degree(m_deg + r.valueDegrees()); }
        Degree&       operator+=(const Degree& d)
        {
            m_deg += d.m_deg;
            return *this;
        }
        Degree& operator+=(const Radian& r)
        {
            m_deg += r.valueDegrees();
            return *this;
        }
        Degree  operator-() const { return Degree(-m_deg); }
        Degree  operator-(const Degree& d) const { return Degree(m_deg - d.m_deg); }
        Degree  operator-(const Radian& r) const { return Degree(m_deg - r.valueDegrees()); }
        Degree& operator-=(const Degree& d)
        {
            m_deg -= d.m_deg;
            return *this;
        }
        Degree& operator-=(const Radian& r)
        {
            m_deg -= r.valueDegrees();
            return *this;
        }
        Degree  operator*(float f) const { return Degree(m_deg * f); }
        Degree  operator*(const Degree& f) const { return Degree(m_deg * f.m_deg); }
        Degree& operator*=(float f)
        {
            m_deg *= f;
            return *this;
        }
        Degree  operator/(float f) const { return Degree(m_deg / f); }
        Degree& operator/=(float f)
        {
            m_deg /= f;
            return *this;
        }

        bool operator<(const Degree& d) const { return m_deg < d.m_deg; }
        bool operator<=(const Degree& d) const { return m_deg <= d.m_deg; }
        bool operator==(const Degree& d) const { return m_deg == d.m_deg; }
        bool operator!=(const Degree& d) const { return m_deg != d.m_deg; }
        bool operator>=(const Degree& d) const { return m_deg >= d.m_deg; }
        bool operator>(const Degree& d) const { return m_deg > d.m_deg; }
    };

    /** Wrapper class which identifies a value as the currently default angle
        type, as defined by Math::setAngleUnit.
    @remarks
        Angle values will be automatically converted between radians and degrees,
        as appropriate.
    */
    struct Angle
    {
        float m_angle;

    public:
        explicit Angle(float angle) : m_angle(angle) {}
        Angle() { m_angle = 0; }

        explicit operator Radian() const;
        explicit operator Degree() const;
    };

    // these functions could not be defined within the class definition of class
    // Radian because they required class Degree to be defined
    inline Radian::Radian(const Degree& d) : m_rad(d.valueRadians()) {}
    inline Radian& Radian::operator=(const Degree& d)
    {
        m_rad = d.valueRadians();
        return *this;
    }
    inline Radian Radian::operator+(const Degree& d) const { return Radian(m_rad + d.valueRadians()); }
    inline Radian& Radian::operator+=(const Degree& d)
    {
        m_rad += d.valueRadians();
        return *this;
    }
    inline Radian Radian::operator-(const Degree& d) const { return Radian(m_rad - d.valueRadians()); }
    inline Radian& Radian::operator-=(const Degree& d)
    {
        m_rad -= d.valueRadians();
        return *this;
    }

    class Math
    {
    private:
        enum class AngleUnit
        {
            AU_DEGREE,
            AU_RADIAN
        };

        // angle units used by the api
        static AngleUnit k_AngleUnit;

    public:
        Math();

        static float abs(float value) { return std::fabs(value); }
        static bool  isNan(float f) { return std::isnan(f); }
        static float sqr(float value) { return value * value; }
        static float sqrt(float fValue) { return std::sqrt(fValue); }
        static float invSqrt(float value) { return 1.f / sqrt(value); }
        static bool  realEqual(float a, float b, float tolerance = std::numeric_limits<float>::epsilon());
        static float clamp(float v, float min, float max) { return MOYU_CLAMP(v, min, max); }
        static float saturate(float f) { return MOYU_MAX(MOYU_MIN(f, 1.0f), 0.0f); }
        static float _max(float a, float b) { return MOYU_MAX(a, b); }
        static float _min(float a, float b) { return MOYU_MIN(a, b); }

        static float degreesToRadians(float degrees);
        static float radiansToDegrees(float radians);
        static float angleUnitsToRadians(float units);
        static float radiansToAngleUnits(float radians);
        static float angleUnitsToDegrees(float units);
        static float degreesToAngleUnits(float degrees);

        static float sin(float value) { return std::sin(value); }
        static float cos(float value) { return std::cos(value); }
        static float tan(float value) { return std::tan(value); }
        static float acos(float value);
        static float asin(float value);
        static float atan(float value) { return std::atan(value); }
        static float atan2(float y_v, float x_v) { return std::atan2(y_v, x_v); }

        static Matrix4x4 makeViewMatrix(const Vector3&    position,
                                        const Quaternion& orientation);

        static Matrix4x4
        makeLookAtMatrix(const Vector3& eye_position, const Vector3& target_position, const Vector3& up_dir);

        static Matrix4x4 makePerspectiveMatrix(Radian fovy, float aspect, float znear, float zfar);

        static Matrix4x4
        makeOrthographicProjectionMatrix(float left, float right, float bottom, float top, float znear, float zfar);
    };


    inline float Radian::valueDegrees() const { return Math::radiansToDegrees(m_rad); }

    inline float Radian::valueAngleUnits() const { return Math::radiansToAngleUnits(m_rad); }

    inline float Degree::valueRadians() const { return Math::degreesToRadians(m_deg); }

    inline float Degree::valueAngleUnits() const { return Math::degreesToAngleUnits(m_deg); }

    inline Angle::operator Radian() const { return Radian(Math::angleUnitsToRadians(m_angle)); }

    inline Angle::operator Degree() const { return Degree(Math::angleUnitsToDegrees(m_angle)); }

    inline Radian operator*(float a, const Radian& b) { return Radian(a * b.valueRadians()); }

    inline Radian operator/(float a, const Radian& b) { return Radian(a / b.valueRadians()); }

    inline Degree operator*(float a, const Degree& b) { return Degree(a * b.valueDegrees()); }

    inline Degree operator/(float a, const Degree& b) { return Degree(a / b.valueDegrees()); }

    //---------------------------------------------------------------------------------------------

    struct Vector2
    {
    public:
        float x {0.f}, y {0.f};

    public:
        Vector2() = default;
        Vector2(float ix) : x(ix), y(ix) {}
        Vector2(float ix, float iy) : x(ix), y(iy) {}
        explicit Vector2(const float v[2]) : x(v[0]), y(v[1]) {}

        Vector2(const Vector2&) = default;
        Vector2& operator=(const Vector2&) = default;

        Vector2(Vector2&&) = default;
        Vector2& operator=(Vector2&&) = default;

        float*       ptr() { return &x; }
        const float* ptr() const { return &x; }

        float operator[](size_t i) const
        {
            assert(i < 2);
            return (i == 0 ? x : y);
        }
        float& operator[](size_t i)
        {
            assert(i < 2);
            return (i == 0 ? x : y);
        }

        // Comparison operators
        bool operator==(const Vector2& rhs) const { return (x == rhs.x && y == rhs.y); }
        bool operator!=(const Vector2& rhs) const { return (x != rhs.x || y != rhs.y); }

        // Assignment operators
        Vector2& operator+=(const Vector2& rhs)
        {
            *this = Vector2(x + rhs.x, y + rhs.y);
            return *this;
        }
        Vector2& operator-=(const Vector2& rhs)
        {
            *this = Vector2(x - rhs.x, y - rhs.y);
            return *this;
        }
        Vector2& operator*=(const Vector2& rhs)
        {
            *this = Vector2(x * rhs.x, y * rhs.y);
            return *this;
        }
        Vector2& operator*=(float s)
        {
            *this = Vector2(x * s, y * s);
            return *this;
        }
        Vector2& operator/=(float s)
        {
            *this = Vector2(x / s, y / s);
            return *this;
        }

        // Binary operators
        friend Vector2 operator+(const Vector2& lhs, const Vector2& rhs)
        {
            return Vector2(lhs.x + rhs.x, lhs.y + rhs.y);
        }
        friend Vector2 operator-(const Vector2& lhs, const Vector2& rhs)
        {
            return Vector2(lhs.x - rhs.x, lhs.y - rhs.y);
        }
        friend Vector2 operator*(const Vector2& lhs, const Vector2& rhs)
        {
            return Vector2(lhs.x * rhs.x, lhs.y * rhs.y);
        }
        friend Vector2 operator*(const Vector2& lhs, float s) { return Vector2(lhs.x * s, lhs.y * s); }
        friend Vector2 operator/(const Vector2& lhs, const Vector2& rhs)
        {
            return Vector2(lhs.x / rhs.x, lhs.y / rhs.y);
        }
        friend Vector2 operator/(const Vector2& lhs, float s) { return Vector2(lhs.x / s, lhs.y / s); }
        friend Vector2 operator*(float s, const Vector2& rhs) { return Vector2(s * rhs.x, s * rhs.y); }

        // Unary operators
        Vector2 operator+() const { return *this; }
        Vector2 operator-() const { return Vector2(-x, -y); }

        // Vector operations
        float length() const { return std::hypot(x, y); }
        float squaredLength() const { return x * x + y * y; }

        float   dot(const Vector2& rhs) { return dot(*this, rhs); }
        Vector2 cross(const Vector2& rhs) const { return cross(*this, rhs); }

        void normalize() { normalize(*this); }

        float getX() const { return x; }
        float getY() const { return y; }

        void setX(float value) { x = value; }
        void setY(float value) { y = value; }

        bool isNaN() const { return Math::isNan(x) || Math::isNan(y); }

        // Static functions
        static Vector2 _min(const Vector2& lhs, const Vector2& rhs)
        {
            return Vector2(std::fminf(lhs.x, rhs.x), std::fminf(lhs.y, rhs.y));
        }
        static Vector2 _max(const Vector2& lhs, const Vector2& rhs)
        {
            return Vector2(std::fmaxf(lhs.x, rhs.x), std::fmaxf(lhs.y, rhs.y));
        }

        static float   dot(const Vector2& lhs, const Vector2& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }
        static Vector2 cross(const Vector2& lhs, const Vector2& rhs)
        {
            float fCross = lhs.x * rhs.y - lhs.y * rhs.x;
            return Vector2(fCross, fCross);
        }

        static float distance(const Vector2& lhs, const Vector2& rhs) { return (lhs - rhs).length(); }
        static float squaredDistance(const Vector2& lhs, const Vector2& rhs) { return (lhs - rhs).squaredLength(); }

        static Vector2 normalize(const Vector2& v)
        {
            Vector2 out   = Vector2(v.x, v.y);
            float   lengh = std::hypot(out.x, out.y);
            if (lengh > 0.0f)
            {
                float inv_length = 1.0f / lengh;
                out.x *= inv_length;
                out.y *= inv_length;
            }
            return out;
        }

        static Vector2 lerp(const Vector2& lhs, const Vector2& rhs, float alpha) { return lhs + alpha * (rhs - lhs); }

        static Vector2 clamp(const Vector2& v, const Vector2& min, const Vector2& max)
        {
            return Vector2(Math::clamp(v.x, min.x, max.x), Math::clamp(v.y, min.y, max.y));
        }

        // Constants
        static const Vector2 Zero;
        static const Vector2 One;
        static const Vector2 UnitX;
        static const Vector2 UnitY;
    };

    struct Vector3
    {
    public:
        float x {0.f}, y {0.f}, z {0.f};

    public:
        Vector3() = default;
        Vector3(float ix) : x(ix), y(ix), z(ix) {}
        Vector3(float ix, float iy, float iz) : x(ix), y(iy), z(iz) {}
        explicit Vector3(const float v[3]) : x(v[0]), y(v[1]), z(v[2]) {}

        Vector3(const Vector2& rhs) { *this = Vector3(rhs[0], rhs[1], 0); }
        Vector3(const Vector2& rhs, float y) { *this = Vector3(rhs[0], rhs[1], y); }

        Vector3(const Vector3&) = default;
        Vector3& operator=(const Vector3&) = default;

        Vector3(Vector3&&) = default;
        Vector3& operator=(Vector3&&) = default;

        float*       ptr() { return &x; }
        const float* ptr() const { return &x; }

        float operator[](size_t i) const
        {
            assert(i < 3);
            return *(&x + i);
        }
        float& operator[](size_t i)
        {
            assert(i < 3);
            return *(&x + i);
        }

        // Comparison operators
        bool operator==(const Vector3& rhs) const { return (x == rhs.x && y == rhs.y && z == rhs.z); }
        bool operator!=(const Vector3& rhs) const { return (x != rhs.x || y != rhs.y || z != rhs.z); }

        // Assignment operators
        Vector3& operator+=(const Vector3& rhs)
        {
            *this = Vector3(x + rhs.x, y + rhs.y, z + rhs.z);
            return *this;
        }
        Vector3& operator-=(const Vector3& rhs)
        {
            *this = Vector3(x - rhs.x, y - rhs.y, z - rhs.z);
            return *this;
        }
        Vector3& operator*=(const Vector3& rhs)
        {
            *this = Vector3(x * rhs.x, y * rhs.y, z * rhs.z);
            return *this;
        }
        Vector3& operator*=(float s)
        {
            *this = Vector3(x * s, y * s, z * s);
            return *this;
        }
        Vector3& operator/=(float s)
        {
            *this = Vector3(x / s, y / s, z / s);
            return *this;
        }

        // Unary operators
        Vector3 operator+() const { return *this; }
        Vector3 operator-() const { return Vector3(-x, -y, -z); }

        // Binary operators
        friend Vector3 operator+(const Vector3& lhs, const Vector3& rhs)
        {
            return Vector3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
        }
        friend Vector3 operator-(const Vector3& lhs, const Vector3& rhs)
        {
            return Vector3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
        }
        friend Vector3 operator*(const Vector3& lhs, const Vector3& rhs)
        {
            return Vector3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
        }
        friend Vector3 operator*(const Vector3& lhs, float s) { return Vector3(lhs.x * s, lhs.y * s, lhs.z * s); }
        friend Vector3 operator/(const Vector3& lhs, const Vector3& rhs)
        {
            return Vector3(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
        }
        friend Vector3 operator/(const Vector3& lhs, float s) { return Vector3(lhs.x / s, lhs.y / s, lhs.z / s); }
        friend Vector3 operator*(float s, const Vector3& rhs) { return Vector3(s * rhs.x, s * rhs.y, s * rhs.z); }

        // Vector operations
        float length() const { return std::sqrt(x * x + y * y + z * z); }
        float squaredLength() const { return x * x + y * y + z * z; }

        float   dot(const Vector3& rhs) const { return dot(*this, rhs); }
        Vector3 cross(const Vector3& rhs) const { return cross(*this, rhs); }

        void normalize() { normalize(*this); }

        bool isNaN() const { return Math::isNan(x) || Math::isNan(y) || Math::isNan(z); }

        // Static functions
        static Vector3 _min(const Vector3& lhs, const Vector3& rhs)
        {
            return Vector3(std::fminf(lhs.x, rhs.x), std::fminf(lhs.y, rhs.y), std::fminf(lhs.z, rhs.z));
        }
        static Vector3 _max(const Vector3& lhs, const Vector3& rhs)
        {
            return Vector3(std::fmaxf(lhs.x, rhs.x), std::fmaxf(lhs.y, rhs.y), std::fmaxf(lhs.z, rhs.z));
        }

        static float dot(const Vector3& lhs, const Vector3& rhs)
        {
            return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
        }
        static Vector3 cross(const Vector3& lhs, const Vector3& rhs)
        {
            return Vector3(lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x);
        }

        static float distance(const Vector3& lhs, const Vector3& rhs) { return (lhs - rhs).length(); }
        static float squaredDistance(const Vector3& lhs, const Vector3& rhs) { return (lhs - rhs).squaredLength(); }

        static Vector3 normalize(const Vector3& v)
        {
            Vector3 out   = Vector3(v.x, v.y, v.z);
            float   lengh = std::sqrt(out.x * out.x + out.y * out.y + out.z * out.z);
            if (lengh > 0.0f)
            {
                float inv_length = 1.0f / lengh;
                out.x *= inv_length;
                out.y *= inv_length;
                out.z *= inv_length;
            }
            return out;
        }

        static Vector3 lerp(const Vector3& lhs, const Vector3& rhs, float alpha) { return lhs + alpha * (rhs - lhs); }

        static Vector3 clamp(const Vector3& v, const Vector3& min, const Vector3& max)
        {
            return Vector3(
                Math::clamp(v.x, min.x, max.x), Math::clamp(v.y, min.y, max.y), Math::clamp(v.z, min.z, max.z));
        }

        // Constants
        static const Vector3 Zero;
        static const Vector3 One;
        static const Vector3 UnitX;
        static const Vector3 UnitY;
        static const Vector3 UnitZ;
        static const Vector3 Up;
        static const Vector3 Down;
        static const Vector3 Right;
        static const Vector3 Left;
        static const Vector3 Forward;
        static const Vector3 Backward;
    };

    struct Vector4
    {
    public:
        float x {0.f}, y {0.f}, z {0.f}, w {0.f};

    public:
        Vector4() = default;
        Vector4(float ix) : x(ix), y(ix), z(ix), w(ix) {}
        Vector4(float ix, float iy, float iz, float iw) : x(ix), y(iy), z(iz), w(iw) {}
        explicit Vector4(const float v[4]) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}

        Vector4(const Vector2& rhs) { *this = Vector4(rhs[0], rhs[1], 0, 0); }
        Vector4(const Vector3& rhs) { *this = Vector4(rhs[0], rhs[1], rhs[2], 0); }
        Vector4(const Vector2& rhs, float z, float w) { *this = Vector4(rhs[0], rhs[1], z, w); }
        Vector4(const Vector3& rhs, float w) { *this = Vector4(rhs[0], rhs[1], rhs[2], w); }

        Vector4(const Vector4&) = default;
        Vector4& operator=(const Vector4&) = default;

        Vector4(Vector4&&) = default;
        Vector4& operator=(Vector4&&) = default;

        float*       ptr() { return &x; }
        const float* ptr() const { return &x; }

        float operator[](size_t i) const
        {
            assert(i < 4);
            return *(&x + i);
        }
        float& operator[](size_t i)
        {
            assert(i < 4);
            return *(&x + i);
        }

        // Comparison operators
        bool operator==(const Vector4& rhs) const { return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w); }
        bool operator!=(const Vector4& rhs) const { return (x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w); }

        // Assignment operators
        Vector4& operator+=(const Vector4& rhs)
        {
            *this = Vector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
            return *this;
        }
        Vector4& operator-=(const Vector4& rhs)
        {
            *this = Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
            return *this;
        }
        Vector4& operator*=(const Vector4& rhs)
        {
            *this = Vector4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
            return *this;
        }
        Vector4& operator*=(float s)
        {
            *this = Vector4(x * s, y * s, z * s, w * s);
            return *this;
        }
        Vector4& operator/=(float s)
        {
            *this = Vector4(x / s, y / s, z / s, w / s);
            return *this;
        }

        // Unary operators
        Vector4 operator+() const { return *this; }
        Vector4 operator-() const { return Vector4(-x, -y, -z, -w); }

        // Binary operators
        friend Vector4 operator+(const Vector4& lhs, const Vector4& rhs)
        {
            return Vector4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
        }
        friend Vector4 operator-(const Vector4& lhs, const Vector4& rhs)
        {
            return Vector4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
        }
        friend Vector4 operator*(const Vector4& lhs, const Vector4& rhs)
        {
            return Vector4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
        }
        friend Vector4 operator*(const Vector4& lhs, float s)
        {
            return Vector4(lhs.x * s, lhs.y * s, lhs.z * s, lhs.w * s);
        }
        friend Vector4 operator/(const Vector4& lhs, const Vector4& rhs)
        {
            return Vector4(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
        }
        friend Vector4 operator/(const Vector4& lhs, float s)
        {
            return Vector4(lhs.x / s, lhs.y / s, lhs.z / s, lhs.w / s);
        }
        friend Vector4 operator*(float s, const Vector4& rhs)
        {
            return Vector4(s / rhs.x, s / rhs.y, s / rhs.z, s / rhs.w);
        }

        // Vector operations
        bool isNaN() { return Math::isNan(x) || Math::isNan(y) || Math::isNan(z) || Math::isNan(w); }

        // Static functions
        static float dot(const Vector4& lhs, const Vector4& rhs)
        {
            return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
        }

        // Constants
        static const Vector4 Zero;
        static const Vector4 One;
        static const Vector4 UnitX;
        static const Vector4 UnitY;
        static const Vector4 UnitZ;
        static const Vector4 UnitW;
    };

    // NOTE.  The (x,y,z) coordinate system is assumed to be right-handed.
    // Coordinate axis rotation matrices are of the form
    //   RX =    1       0       0
    //           0     cos(t) -sin(t)
    //           0     sin(t)  cos(t)
    // where t > 0 indicates a counterclockwise rotation in the yz-plane
    //   RY =  cos(t)    0     sin(t)
    //           0       1       0
    //        -sin(t)    0     cos(t)
    // where t > 0 indicates a counterclockwise rotation in the zx-plane
    //   RZ =  cos(t) -sin(t)    0
    //         sin(t)  cos(t)    0
    //           0       0       1
    // where t > 0 indicates a counterclockwise rotation in the xy-plane.
    /// <strong>this is a rotation matrix which is used to represent a composition of extrinsic rotations about axes z, y, x, (in that order)</strong>
    struct Matrix3x3
    {
    public:
        float m[3][3];

    public:
        Matrix3x3() { operator=(Identity); }

        explicit Matrix3x3(float arr[3][3]);

        Matrix3x3(float (&float_array)[9]);

        Matrix3x3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22);

        Matrix3x3(const Vector3& row0, const Vector3& row1, const Vector3& row2);

        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
        Matrix3x3(const Quaternion& q);

        void fromData(float (&float_array)[9]);

        void toData(float (&float_array)[9]) const;

        // member access, allows use of construct mat[r][c]
        float* operator[](size_t row_index) const;

        Vector3 getColumn(size_t col_index) const;

        void setColumn(size_t col_index, const Vector3& vec);

        void fromAxes(const Vector3& x_axis, const Vector3& y_axis, const Vector3& z_axis);

        // assignment and comparison
        bool operator==(const Matrix3x3& rhs) const;

        bool operator!=(const Matrix3x3& rhs) const;

        // arithmetic operations
        Matrix3x3 operator+(const Matrix3x3& rhs) const;

        Matrix3x3 operator-(const Matrix3x3& rhs) const;

        Matrix3x3 operator*(const Matrix3x3& rhs) const;

        // matrix * vector [3x3 * 3x1 = 3x1]
        Vector3 operator*(const Vector3& rhs) const;

        Matrix3x3 operator-() const;

        // matrix * scalar
        Matrix3x3 operator*(float scalar) const;

        // scalar * matrix
        friend Matrix3x3 operator*(float scalar, const Matrix3x3& rhs);

        // utilities
        Matrix3x3 transpose() const;

        bool      inverse(Matrix3x3& inv_mat, float fTolerance = 1e-06) const;
        Matrix3x3 inverse(float tolerance = 1e-06) const;

        Matrix3x3 adjugate() const;
        float     minor(size_t r0, size_t r1, size_t c0, size_t c1) const;
        float     determinant() const;

        // https://en.wikipedia.org/wiki/Euler_angles
        // https://www.geometrictools.com/Documentation/EulerAngles.pdf
        // Tait–Bryan angles, extrinsic angles, ZYX in order
        Vector3 toTaitBryanAngles() const;
        void    fromTaitBryanAngles(const Vector3& taitBryanAngles);

        // matrix must be orthonormal
        void toAngleAxis(Vector3& axis, float& radian) const;
        void fromAngleAxis(const Vector3& axis, const float& radian);

        static Matrix3x3 fromRotationX(const float& radians);
        static Matrix3x3 fromRotationY(const float& radians);
        static Matrix3x3 fromRotationZ(const float& radians);

        static Matrix3x3 fromQuaternion(const Quaternion& quat);

        static Matrix3x3 scale(const Vector3& scale);

        // Constants
        static const Matrix3x3 Zero;
        static const Matrix3x3 Identity;
    };

    // https://www.geometrictools.com/Documentation/EulerAngles.pdf
    // row-major notation, row-major storage
    struct Matrix4x4
    {
    public:
        float m[4][4];

    public:
        Matrix4x4() { operator=(Identity); }

        explicit Matrix4x4(float arr[4][4]);

        Matrix4x4(const float (&float_array)[16]);

        Matrix4x4(float m00,
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
                  float m33);

        Matrix4x4(const Vector4& r0, const Vector4& r1, const Vector4& r2, const Vector4& r3);

        Matrix4x4(const Matrix3x3& mat);

        Matrix4x4(const Quaternion& rot);

        void fromData(const float (&float_array)[16]);

        void toData(float (&float_array)[16]) const;

        void setMatrix3x3(const Matrix3x3& mat3);

        float*       ptr() { return &m[0][0]; }
        const float* ptr() const { return &m[0][0]; }

        float* operator[](size_t i)
        {
            assert(i < 4);
            return m[i];
        }
        const float* operator[](size_t i) const
        {
            assert(i < 4);
            return m[i];
        }

        Vector4 row(size_t i) const { return Vector4(m[i][0], m[i][1], m[i][2], m[i][3]); }
        Vector4 col(size_t i) const { return Vector4(m[0][i], m[1][i], m[2][i], m[3][i]); }

        // Comparison operators
        bool operator==(const Matrix4x4& rhs) const;
        bool operator!=(const Matrix4x4& rhs) const;

        // Assignment operators
        Matrix4x4& operator=(const Matrix4x4& rhs);
        Matrix4x4& operator+=(const Matrix4x4& rhs);
        Matrix4x4& operator-=(const Matrix4x4& rhs);
        Matrix4x4& operator*=(const Matrix4x4& rhs);
        Matrix4x4& operator*=(float s);
        Matrix4x4& operator/=(float s);

        Matrix4x4& operator/=(const Matrix4x4& rhs);

        // Unary operators
        Matrix4x4 operator+() const { return *this; }
        Matrix4x4 operator-() const;

        // Binary operators
        friend Matrix4x4 operator+(const Matrix4x4& lhs, const Matrix4x4& rhs);
        friend Matrix4x4 operator-(const Matrix4x4& lhs, const Matrix4x4& rhs);
        friend Matrix4x4 operator*(const Matrix4x4& lhs, const Matrix4x4& rhs);
        friend Matrix4x4 operator*(const Matrix4x4& lhs, float s);
        friend Matrix4x4 operator/(const Matrix4x4& lhs, float s);
        friend Matrix4x4 operator/(const Matrix4x4& lhs, const Matrix4x4& rhs);
        // Element-wise divide
        friend Matrix4x4 operator*(float s, const Matrix4x4& rhs);

        // Vector3 = Matrix4x4 * Vector3
        friend Vector3 operator*(const Matrix4x4& lhs, Vector3 v);
        // Vector4 = Matrix4x4 * Vector4
        friend Vector4 operator*(const Matrix4x4& lhs, Vector4 v);

        // Properties
        Vector3 Up() const { return Vector3(m[0][1], m[1][1], m[2][1]); }
        void    Up(const Vector3& v);

        Vector3 Down() const { return Vector3(-m[0][1], -m[1][1], -m[2][1]); }
        void    Down(const Vector3& v);

        Vector3 Right() const { return Vector3(m[0][0], m[1][0], m[2][0]); }
        void    Right(const Vector3& v);

        Vector3 Left() const { return Vector3(-m[0][0], -m[1][0], -m[2][0]); }
        void    Left(const Vector3& v);

        Vector3 Forward() const { return Vector3(-m[0][2], -m[1][2], -m[2][2]); }
        void    Forward(const Vector3& v);

        Vector3 Backward() const { return Vector3(m[0][2], m[1][2], m[2][2]); }
        void    Backward(const Vector3& v);

        Vector3 Translation() const { return Vector3(m[0][3], m[1][3], m[2][3]); }
        void    Translation(const Vector3& v);

        // https://opensource.apple.com/source/WebCore/WebCore-514/platform/graphics/transforms/TransformationMatrix.cpp
        // Matrix operations
        bool decompose(Vector3& scale, Quaternion& rotation, Vector3& translation);

        Matrix4x4 transpose() const;

        bool      inverse(Matrix4x4& inv_mat, float fTolerance = 1e-06) const;
        Matrix4x4 inverse(float tolerance = 1e-06) const;

        Matrix4x4 adjugate() const;
        float     minor(size_t r0, size_t r1, size_t r2, size_t c0, size_t c1, size_t c2) const;
        float     determinant() const;

        Vector3 toTaitBryanAngles() const;

        // Static functions
        static Matrix4x4 translation(const Vector3& position);
        static Matrix4x4 translation(float x, float y, float z);

        static Matrix4x4 scale(const Vector3& scales);
        static Matrix4x4 scale(float xs, float ys, float zs);
        static Matrix4x4 scale(float scale);

        static Matrix4x4 rotationX(float radians);
        static Matrix4x4 rotationY(float radians);
        static Matrix4x4 rotationZ(float radians);

        static Matrix4x4 fromAxisAngle(const Vector3& axis, float angle);

        // 注意区分：在OpenGL中计算投影矩阵的时候，一般都用zNear和zFar绝对值，但是我们这里还是使用位于右手坐标中的值

        // 使用右手坐标系，相机朝向-z方向，参考 Fundamentals of Computer Graphics
        // 其中 zNearPlane > zFarPlane，且都是负值
        // 输出 canonical view volume 是xy区间是[-1,1]，z的区间是[0,1]
        // 参考 http://www.songho.ca/opengl/gl_projectionmatrix.html
        static Matrix4x4 createPerspectiveFieldOfView(float fovY, float aspectRatio, float zNearPlane, float zFarPlane);
        static Matrix4x4 createPerspective(float width, float height, float zNearPlane, float zFarPlane);
        static Matrix4x4 createPerspectiveOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane);
        static Matrix4x4 createOrthographic(float width, float height, float zNearPlane, float zFarPlane);
        static Matrix4x4 createOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane);

        // eye是相机位置，gaze是相机向前朝向，up是相机向上朝向
        static Matrix4x4 lookAt(const Vector3& eye, const Vector3& gaze, const Vector3& up);
        static Matrix4x4 createView(const Vector3& position, const Quaternion& orientation);
        static Matrix4x4 createWorld(const Vector3& position, const Vector3& forward, const Vector3& up);

        static Matrix4x4 fromQuaternion(const Quaternion& quat);

        static Matrix4x4 lerp(const Matrix4x4& lhs, const Matrix4x4& rhs, float t);

        /** Building a Matrix4 from orientation / scale / position.
        @remarks
        Transform is performed in the order scale, rotate, translation, i.e. translation is independent
        of orientation axes, scale does not affect size of translation, rotation and scaling are always
        centered on the origin.
        */
        static Matrix4x4 makeTransform(const Vector3& position, const Vector3& scale, const Quaternion& orientation);

        /** Building an inverse Matrix4 from orientation / scale / position.
        @remarks
        As makeTransform except it build the inverse given the same data as makeTransform, so
        performing -translation, -rotate, 1/scale in that order.
        */
        static Matrix4x4 makeInverseTransform(const Vector3& position, const Vector3& scale, const Quaternion& orientation);

        // Constants
        static const Matrix4x4 Zero;
        static const Matrix4x4 Identity;
    };

    // https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
    // {\displaystyle \mathbf {p'} =\mathbf {q} \mathbf {p} \mathbf {q} ^{-1}}
    struct Quaternion
    {
    public:
        float x {0.f}, y {0.f}, z {0.f}, w {1.f};

    public:
        Quaternion() = default;
        Quaternion(float iw, float ix, float iy, float iz) : x(ix), y(iy), z(iz), w(iw) {}
        Quaternion(const Vector3& v, float scalar) : x(v.x), y(v.y), z(v.z), w(scalar) {}
        explicit Quaternion(const Vector4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
        explicit Quaternion(const float* pArray) : x(pArray[0]), y(pArray[1]), z(pArray[2]), w(pArray[3]) {}

        Quaternion(const Quaternion&) = default;
        Quaternion& operator=(const Quaternion&) = default;

        Quaternion(Quaternion&&) = default;
        Quaternion& operator=(Quaternion&&) = default;

        // Comparison operators
        bool operator==(const Quaternion& q) const { return (x == q.x && y == q.y && z == q.z && w == q.w); }
        bool operator!=(const Quaternion& q) const { return (x != q.x || y != q.y || z != q.z || w != q.w); }

        // Assignment operators
        Quaternion& operator+=(const Quaternion& q)
        {
            *this = Quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
            return *this;
        }
        Quaternion& operator-=(const Quaternion& q)
        {
            *this = Quaternion(x - q.x, y - q.y, z - q.z, w - q.w);
            return *this;
        }
        Quaternion& operator*=(const Quaternion& q);
        Quaternion& operator*=(float s)
        {
            *this = Quaternion(x * s, y * s, z * s, w * s);
            return *this;
        }
        Quaternion& operator/=(const Quaternion& q);

        // Unary operators
        Quaternion operator+() const { return *this; }
        Quaternion operator-() const;

        // Binary operators
        friend Quaternion operator+(const Quaternion& q1, const Quaternion& q2);
        friend Quaternion operator-(const Quaternion& q1, const Quaternion& q2);
        friend Quaternion operator*(const Quaternion& q1, const Quaternion& q2);
        friend Quaternion operator*(const Quaternion& q, float s);
        friend Quaternion operator/(const Quaternion& q1, const Quaternion& q2);
        friend Quaternion operator*(float S, const Quaternion& q);

        // Vector3 = Matrix4x4 * Vector3
        friend Vector3 operator*(const Quaternion& lhs, Vector3 rhs);

        // Quaternion operations
        float length() const;
        float squaredLength() const;

        Vector3 toTaitBryanAngles() const;

        // Static functions
        static Quaternion fromTaitBryanAngles(const Vector3& angles);

        static Quaternion normalize(const Quaternion& q);
        static Quaternion conjugate(const Quaternion& q);
        static Quaternion inverse(const Quaternion& q);

        static Quaternion fromAxes(const Vector3& x_axis, const Vector3& y_axis, const Vector3& z_axis);
        static Quaternion fromAxisAngle(const Vector3& axis, float angle);

        static Quaternion fromRotationMatrix(const Matrix4x4& rotation);
        static Quaternion fromRotationMatrix(const Matrix3x3& rotation);

        static Quaternion lerp(const Quaternion& q1, const Quaternion& q2, float t);

        static Quaternion slerp(const Quaternion& q1, const Quaternion& q2, float t);

        static Quaternion concatenate(const Quaternion& q1, const Quaternion& q2);

        static Quaternion fromToRotation(const Vector3& fromDir, const Vector3& toDir);

        static Quaternion lookRotation(const Vector3& forward, const Vector3& up);

        static float angle(const Quaternion& q1, const Quaternion& q2);

        // Constants
        static const Quaternion Identity;
    };

    struct Transform
    {
    public:
        Vector3    m_position {Vector3::Zero};
        Vector3    m_scale {Vector3::One};
        Quaternion m_rotation {Quaternion::Identity};

        // Comparison operators
        bool operator==(const Transform& t) const { return (m_position == t.m_position && m_scale == t.m_scale && m_rotation == t.m_rotation); }
        bool operator!=(const Transform& t) const { return (m_position != t.m_position || m_scale != t.m_scale || m_rotation != t.m_rotation); }

        Transform() = default;
        Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale) :
            m_position {position}, m_scale {scale}, m_rotation {rotation}
        {}

        Matrix4x4 getMatrix() const { return Matrix4x4::makeTransform(m_position, m_scale, m_rotation); }
    };

    //---------------------------------------------------------------------------------------------

    struct AxisAlignedBox
    {
    public:
        AxisAlignedBox() {}
        AxisAlignedBox(const Vector3& center, const Vector3& half_extent);

        void merge(const AxisAlignedBox& axis_aligned_box);
        void merge(const Vector3& new_point);
        void update(const Vector3& center, const Vector3& half_extent);

        const Vector3& getCenter() const { return m_center; }
        const Vector3& getHalfExtent() const { return m_half_extent; }
        const Vector3& getMinCorner() const { return m_min_corner; }
        const Vector3& getMaxCorner() const { return m_max_corner; }

    private:
        Vector3 m_center {Vector3::Zero};
        Vector3 m_half_extent {Vector3::Zero};

        Vector3 m_min_corner {FLT_MAX, FLT_MAX, FLT_MAX};
        Vector3 m_max_corner {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    };

    struct Color
    {
        float r, g, b, a;

    public:
        Color() = default;
        Color(float r, float g, float b) : r(r), g(g), b(b), a(1) {}
        Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
        Color(Vector3 color) : r(color[0]), g(color[1]), b(color[2]), a(1) {}
        Color(Vector4 color) : r(color[0]), g(color[1]), b(color[2]), a(color[2]) {}

        float operator[](size_t i) const
        {
            assert(i < 4);
            return *(&r + i);
        }
        float& operator[](size_t i)
        {
            assert(i < 4);
            return *(&r + i);
        }

        float R() const { return (&r)[0]; }
        float G() const { return (&r)[1]; }
        float B() const { return (&r)[2]; }
        float A() const { return (&r)[3]; }

        // Comparison operators
        bool operator==(const Color& rhs) const { return (r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a); }
        bool operator!=(const Color& rhs) const { return (r != rhs.r || g != rhs.g || b != rhs.b || a != rhs.a); }

        void SetR(float r) { (&r)[0] = r; }
        void SetG(float g) { (&r)[1] = g; }
        void SetB(float b) { (&r)[2] = b; }
        void SetA(float a) { (&r)[3] = a; }

        void SetRGB(float r, float g, float b)
        {
            (&r)[0] = r;
            (&r)[1] = g;
            (&r)[2] = b;
        }

        Color ToSRGB() const;
        Color FromSRGB() const;
        Color ToREC709() const;
        Color FromREC709() const;

        // Probably want to convert to sRGB or Rec709 first
        uint32_t R10G10B10A2() const;
        uint32_t R8G8B8A8() const;

        // Pack an HDR color into 32-bits
        uint32_t R11G11B10F(bool RoundToEven = false) const;
        uint32_t R9G9B9E5() const;

        Vector4 toVector4() const { return Vector4(r, g, b, a); }
        Vector3 toVector3() const { return Vector3(r, g, b); }

    public:
        static const Color White;
        static const Color Black;
        static const Color Red;
        static const Color Green;
        static const Color Blue;
    };

    //---------------------------------------------------------------------------------------------

    template<typename NumericType>
    using uniform_distribution = typename std::conditional<std::is_integral<NumericType>::value,
                                                           std::uniform_int_distribution<NumericType>,
                                                           std::uniform_real_distribution<NumericType>>::type;

    template<typename RandomEngine = std::default_random_engine>
    class RandomNumberGenerator
    {

    private:
        RandomEngine m_engine;

    public:
        template<typename... Params>
        explicit RandomNumberGenerator(Params&&... params) : m_engine(std::forward<Params>(params)...)
        {}

        template<typename... Params>
        void seed(Params&&... seeding)
        {
            m_engine.seed(std::forward<Params>(seeding)...);
        }

        template<typename DistributionFunc, typename... Params>
        typename DistributionFunc::result_type distribution(Params&&... params)
        {
            DistributionFunc dist(std::forward<Params>(params)...);
            return dist(m_engine);
        }

        template<typename NumericType>
        NumericType uniformDistribution(NumericType lower, NumericType upper)
        {
            if (lower == upper)
            {
                return lower;
            }
            return distribution<uniform_distribution<NumericType>>(lower, upper);
        }

        float uniformUnit() { return uniformDistribution(0.f, std::nextafter(1.f, FLT_MAX)); }

        float uniformSymmetry() { return uniformDistribution(-1.f, std::nextafter(1.f, FLT_MAX)); }

        bool bernoulliDistribution(float probability) { return distribution<std::bernoulli_distribution>(probability); }

        float normalDistribution(float mean, float stddev)
        {
            return distribution<std::normal_distribution<float>>(mean, stddev);
        }

        template<typename DistributionFunc, typename Range, typename... Params>
        void generator(Range&& range, Params&&... params)
        {
            // using ResultType = typename DistributionFunc::result_type;

            DistributionFunc dist(std::forward<Params>(params)...);
            return std::generate(std::begin(range), std::end(range), [&] { return dist(m_engine); });
        }
    };

    template<typename DistributionFunc,
             typename RandomEngine = std::default_random_engine,
             typename SeedType     = std::seed_seq>
    class DistRandomNumberGenerator
    {
        using ResultType = typename DistributionFunc::result_type;

    private:
        RandomEngine      m_engine;
        DistributionFunc* m_dist = nullptr;

    public:
        template<typename... Params>
        explicit DistRandomNumberGenerator(SeedType&& seeding, Params&&... /*params*/) : m_engine(seeding)
        {}

        ~DistRandomNumberGenerator() {}

        template<typename... Params>
        void seed(Params&&... params)
        {
            m_engine.seed(std::forward<Params>(params)...);
        }

        ResultType next() { return (*m_dist)(m_engine); }
    };

    using DefaultRNG = RandomNumberGenerator<std::mt19937>;

} // namespace MoYu