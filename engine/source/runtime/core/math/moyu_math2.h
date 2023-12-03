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

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/matrix_interpolation.hpp>
#include <glm/ext/matrix_clip_space.hpp>

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
    INLINE T AlignUpWithMask(T value, size_t mask)
    {
        return (T)(((size_t)value + mask) & ~mask);
    }

    template<typename T>
    INLINE T AlignDownWithMask(T value, size_t mask)
    {
        return (T)((size_t)value & ~mask);
    }

    template<typename T>
    INLINE T AlignUp(T value, size_t alignment)
    {
        return AlignUpWithMask(value, alignment - 1);
    }

    template<typename T>
    INLINE T AlignDown(T value, size_t alignment)
    {
        return AlignDownWithMask(value, alignment - 1);
    }

    template<typename T>
    INLINE bool IsAligned(T value, size_t alignment)
    {
        return 0 == ((size_t)value & (alignment - 1));
    }

    template<typename T>
    INLINE T DivideByMultiple(T value, size_t alignment)
    {
        return (T)((value + alignment - 1) / alignment);
    }

    template<typename T>
    INLINE bool IsPowerOfTwo(T value)
    {
        return 0 == (value & (value - 1));
    }

    template<typename T>
    INLINE bool IsDivisible(T value, T divisor)
    {
        return (value / divisor) * divisor == value;
    }

    INLINE uint8_t Log2(uint64_t value)
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
    INLINE T AlignPowerOfTwo(T value)
    {
        return value == 0 ? 0 : 1 << Log2(value);
    }
}

namespace MoYu
{
    constexpr const double F_E        = 2.71828182845904523536028747135266250;
    constexpr const double F_LOG2E    = 1.44269504088896340735992468100189214;
    constexpr const double F_LOG10E   = 0.434294481903251827651128918916605082;
    constexpr const double F_LN2      = 0.693147180559945309417232121458176568;
    constexpr const double F_LN10     = 2.30258509299404568401799145468436421;
    constexpr const double F_PI       = 3.14159265358979323846264338327950288;
    constexpr const double F_PI_2     = 1.57079632679489661923132169163975144;
    constexpr const double F_PI_4     = 0.785398163397448309615660845819875721;
    constexpr const double F_1_PI     = 0.318309886183790671537767526745028724;
    constexpr const double F_2_PI     = 0.636619772367581343075535053490057448;
    constexpr const double F_2_SQRTPI = 1.12837916709551257389615890312154517;
    constexpr const double F_SQRT2    = 1.41421356237309504880168872420969808;
    constexpr const double F_SQRT1_2  = 0.707106781186547524400844362104849039;
    constexpr const double F_TAU      = 2.0 * F_PI;

    namespace d {
    constexpr const double E                = F_E;
    constexpr const double LOG2E            = F_LOG2E;
    constexpr const double LOG10E           = F_LOG10E;
    constexpr const double LN2              = F_LN2;
    constexpr const double LN10             = F_LN10;
    constexpr const double PI               = F_PI;
    constexpr const double PI_2             = F_PI_2;
    constexpr const double PI_4             = F_PI_4;
    constexpr const double ONE_OVER_PI      = F_1_PI;
    constexpr const double TWO_OVER_PI      = F_2_PI;
    constexpr const double TWO_OVER_SQRTPI  = F_2_SQRTPI;
    constexpr const double SQRT2            = F_SQRT2;
    constexpr const double SQRT1_2          = F_SQRT1_2;
    constexpr const double TAU              = F_TAU;
    constexpr const double DEG_TO_RAD       = F_PI / 180.0;
    constexpr const double RAD_TO_DEG       = 180.0 / F_PI;
    } // namespace d

    namespace f {
    constexpr const float E                = (float)d::E;
    constexpr const float LOG2E            = (float)d::LOG2E;
    constexpr const float LOG10E           = (float)d::LOG10E;
    constexpr const float LN2              = (float)d::LN2;
    constexpr const float LN10             = (float)d::LN10;
    constexpr const float PI               = (float)d::PI;
    constexpr const float PI_2             = (float)d::PI_2;
    constexpr const float PI_4             = (float)d::PI_4;
    constexpr const float ONE_OVER_PI      = (float)d::ONE_OVER_PI;
    constexpr const float TWO_OVER_PI      = (float)d::TWO_OVER_PI;
    constexpr const float TWO_OVER_SQRTPI  = (float)d::TWO_OVER_SQRTPI;
    constexpr const float SQRT2            = (float)d::SQRT2;
    constexpr const float SQRT1_2          = (float)d::SQRT1_2;
    constexpr const float TAU              = (float)d::TAU;
    constexpr const float DEG_TO_RAD       = (float)d::DEG_TO_RAD;
    constexpr const float RAD_TO_DEG       = (float)d::RAD_TO_DEG;
    } // namespace f

    constexpr float factorial(size_t n, size_t d = 1);

    struct Transform;
    struct AxisAlignedBox;

    namespace MYFloat2
    {
        extern glm::float2 Zero;
        extern glm::float2 One;
        extern glm::float2 UnitX;
        extern glm::float2 UnitY;

        // positive if v2 is on the left side of v1
        float signedAngle(glm::float2 v1, glm::float2 v2);
        glm::float2 rotate(glm::float2 v, float theta);
    }

    namespace MYFloat3
    {
        extern glm::float3 Zero;
        extern glm::float3 One;
        extern glm::float3 UnitX;
        extern glm::float3 UnitY;
        extern glm::float3 UnitZ;
        extern glm::float3 Up;
        extern glm::float3 Down;
        extern glm::float3 Right;
        extern glm::float3 Left;
        extern glm::float3 Forward;
        extern glm::float3 Backward;
    }

    namespace MYFloat4
    {
        extern glm::float4 Zero;
        extern glm::float4 One;
        extern glm::float4 UnitX;
        extern glm::float4 UnitY;
        extern glm::float4 UnitZ;
        extern glm::float4 UnitW;
    }

    namespace MYMatrix3x3
    {
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
        /// <strong>this is a rotation matrix which is used to represent a composition of extrinsic rotations about axes
        /// z, y, x, (in that order)</strong>


        //// https://en.wikipedia.org/wiki/Euler_angles
        //// https://www.geometrictools.com/Documentation/EulerAngles.pdf
        //// Tait–Bryan angles, extrinsic angles, ZYX in order
        //glm::float3 toTaitBryanAngles() const;
        //void    fromTaitBryanAngles(const glm::float3& taitBryanAngles);

        extern glm::float3x3 Zero;
        extern glm::float3x3 Identity;
    }

    namespace MYMatrix4x4
    {
        // https://www.geometrictools.com/Documentation/EulerAngles.pdf

        // 注意区分：在OpenGL中计算投影矩阵的时候，一般都用zNear和zFar绝对值，但是我们这里还是使用位于右手坐标中的值

        // 使用右手坐标系，相机朝向-z方向，参考 Fundamentals of Computer Graphics
        // 其中 zNearPlane < zFarPlane，且都是正值
        // 输出 canonical view volume 是xy区间是[-1,1]，z的区间是[0,1]
        // 参考 http://www.songho.ca/opengl/gl_projectionmatrix.html
        glm::float4x4 createPerspectiveFieldOfView(float fovY, float aspectRatio, float zNearPlane, float zFarPlane);
        glm::float4x4 createPerspective(float width, float height, float zNearPlane, float zFarPlane);
        glm::float4x4 createPerspectiveOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane);
        glm::float4x4 createOrthographic(float width, float height, float zNearPlane, float zFarPlane);
        glm::float4x4 createOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane);

        // eye是相机位置，gaze是相机向前朝向，up是相机向上朝向
        glm::float4x4 createLookAtMatrix(const glm::float3& eye, const glm::float3& center, const glm::float3& up);
        glm::float4x4 createViewMatrix(const glm::float3& position, const glm::quat& orientation);
        glm::float4x4 createWorldMatrix(const glm::float3& position, const glm::quat& orientation, const glm::float3& scale);

        /** Building a Matrix4 from orientation / scale / position.
        @remarks
        Transform is performed in the order scale, rotate, translation, i.e. translation is independent
        of orientation axes, scale does not affect size of translation, rotation and scaling are always
        centered on the origin.
        */
        glm::float4x4 makeTransform(const glm::float3& position, const glm::quat& orientation, const glm::float3& scale);

        /** Building an inverse Matrix4 from orientation / scale / position.
        @remarks
        As makeTransform except it build the inverse given the same data as makeTransform, so
        performing -translation, -rotate, 1/scale in that order.
        */
        glm::float4x4 makeInverseTransform(const glm::float3& position, const glm::quat& orientation, const glm::float3& scale);

        // Constants
        extern glm::float4x4 Zero;
        extern glm::float4x4 Identity;
    }

    namespace MYQuaternion
    {
        // https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation

        extern glm::quat Identity;
    }

    INLINE float degreesToRadians(float degrees) { return degrees * f::DEG_TO_RAD; }
    INLINE float radiansToDegrees(float radians) { return radians * f::RAD_TO_DEG; }

    //---------------------------------------------------------------------------------------------


    // https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
    struct Transform
    {
    public:
        glm::float3     m_position {MYFloat3::Zero};
        glm::float3     m_scale {MYFloat3::One};
        glm::quat       m_rotation {MYQuaternion::Identity};

        // Comparison operators
        bool operator==(const Transform& t) const
        {
            return (m_position == t.m_position && m_scale == t.m_scale && m_rotation == t.m_rotation);
        }
        bool operator!=(const Transform& t) const
        {
            return (m_position != t.m_position || m_scale != t.m_scale || m_rotation != t.m_rotation);
        }

        Transform() = default;
        Transform(const glm::float3& position, const glm::quat& rotation, const glm::float3& scale) :
            m_position {position}, m_rotation {rotation}, m_scale {scale}
        {}

        glm::float4x4 getMatrix() const { return MYMatrix4x4::makeTransform(m_position, m_rotation, m_scale); }
    };

    //---------------------------------------------------------------------------------------------

    struct AxisAlignedBox
    {
    public:
        AxisAlignedBox() {}
        AxisAlignedBox(const glm::float3& center, const glm::float3& half_extent);

        void merge(const AxisAlignedBox& axis_aligned_box);
        void merge(const glm::float3& new_point);
        void update(const glm::float3& center, const glm::float3& half_extent);

        const glm::float3& getCenter() const { return m_center; }
        const glm::float3& getHalfExtent() const { return m_half_extent; }
        const glm::float3& getMinCorner() const { return m_min_corner; }
        const glm::float3& getMaxCorner() const { return m_max_corner; }

    private:
        glm::float3 m_center {MYFloat3::Zero};
        glm::float3 m_half_extent {MYFloat3::Zero};

        glm::float3 m_min_corner {FLT_MAX, FLT_MAX, FLT_MAX};
        glm::float3 m_max_corner {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    };

    struct Color
    {
        float r, g, b, a;

    public:
        Color() = default;
        Color(float r, float g, float b) : r(r), g(g), b(b), a(1) {}
        Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
        Color(glm::float3 color) : r(color[0]), g(color[1]), b(color[2]), a(1) {}
        Color(glm::float4 color) : r(color[0]), g(color[1]), b(color[2]), a(color[2]) {}

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

        glm::float4 toVector4() const { return glm::float4(r, g, b, a); }
        glm::float3 toVector3() const { return glm::float3(r, g, b); }

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
