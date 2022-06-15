#pragma once
#include "runtime/core/meta/reflection/reflection.h"

#include "runtime/core/math/moyu_math.h"

namespace Pilot
{
    REFLECTION_TYPE(Color)
    STRUCT(Color, Fields)
    {
        REFLECTION_BODY(Color);

    public:
        float r;
        float g;
        float b;

        Vector3 toVector3() const { return Vector3(r, g, b); }
    };
}