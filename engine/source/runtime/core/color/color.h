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

        // Comparison operators
        bool operator==(const Color& rhs) const { return (r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a); }
        bool operator!=(const Color& rhs) const { return (r != rhs.r || g != rhs.g || b != rhs.b || a != rhs.a); }

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