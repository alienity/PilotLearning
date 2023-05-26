#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/core/meta/json.h"

#include <string>
#include <vector>

namespace MoYu
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vector2, x, y)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vector3, x, y, z)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vector4, x, y, z, w)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Quaternion, x, y, z, w)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Color, r, g, b, a)
} // namespace MoYu