#pragma once
#include "runtime/core/meta/reflection/reflection.h"

#include "runtime/core/math/moyu_math.h"

namespace Pilot
{
    REFLECTION_TYPE(Color)
    STRUCT(Color, Fields)
    {
        REFLECTION_BODY(Color);

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
}