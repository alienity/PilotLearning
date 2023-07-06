#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/core/meta/json.h"

#include <string>
#include <vector>

namespace MoYu
{
#define TransformComponentName "TransformComponent"
#define MeshComponentName "MeshComponent"
#define LightComponentName "LightComponent"
#define CameraComponentName "CameraComponent"

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Vector2, x, y)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Vector3, x, y, z)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Vector4, x, y, z, w)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Quaternion, x, y, z, w)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Color, r, g, b, a)
} // namespace MoYu