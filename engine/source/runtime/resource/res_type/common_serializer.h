#pragma once

#include "runtime/core/math/moyu_math2.h"
#include "runtime/core/meta/json.h"

#include <string>
#include <vector>

using json = nlohmann::json;

namespace glm
{
    //**********************************************************************************
    inline void to_json(json& j, const glm::float2& P) { j = { {"x", P.x}, {"y", P.y} }; }
    inline void from_json(const json& j, glm::float2& P)
    {
        P.x = j.at("x").get<float>();
        P.y = j.at("y").get<float>();
    }

    inline void to_json(json& j, const glm::float3& P) { j = { {"x", P.x}, {"y", P.y}, {"z", P.z} }; }
    inline void from_json(const json& j, glm::float3& P)
    {
        P.x = j.at("x").get<float>();
        P.y = j.at("y").get<float>();
        P.z = j.at("z").get<float>();
    }

    inline void to_json(json& j, const glm::float4& P) { j = { {"x", P.x}, {"y", P.y}, {"z", P.z}, {"w", P.w} }; }
    inline void from_json(const json& j, glm::float4& P)
    {
        P.x = j.at("x").get<float>();
        P.y = j.at("y").get<float>();
        P.z = j.at("z").get<float>();
        P.w = j.at("w").get<float>();
    }
    //**********************************************************************************

    //**********************************************************************************
    inline void to_json(json& j, const glm::uvec1& P) { j = {{"x", P.x}}; }
    inline void from_json(const json& j, glm::uvec1& P) { P.x = j.at("x").get<glm::uint>(); }

    inline void to_json(json& j, const glm::uvec2& P) { j = {{"x", P.x}, {"y", P.y}}; }
    inline void from_json(const json& j, glm::uvec2& P)
    {
        P.x = j.at("x").get<glm::uint>();
        P.y = j.at("y").get<glm::uint>();
    }

    inline void to_json(json& j, const glm::uvec3& P) { j = {{"x", P.x}, {"y", P.y}, {"z", P.z}}; }
    inline void from_json(const json& j, glm::uvec3& P)
    {
        P.x = j.at("x").get<glm::uint>();
        P.y = j.at("y").get<glm::uint>();
        P.z = j.at("z").get<glm::uint>();
    }

    inline void to_json(json& j, const glm::uvec4& P) { j = {{"x", P.x}, {"y", P.y}, {"z", P.z}, {"w", P.w}}; }
    inline void from_json(const json& j, glm::uvec4& P)
    {
        P.x = j.at("x").get<glm::uint>();
        P.y = j.at("y").get<glm::uint>();
        P.z = j.at("z").get<glm::uint>();
        P.w = j.at("w").get<glm::uint>();
    }
    //**********************************************************************************

    //**********************************************************************************
    inline void to_json(json& j, const glm::ivec1& P) { j = {{"x", P.x}}; }
    inline void from_json(const json& j, glm::ivec1& P) { P.x = j.at("x").get<int>(); }

    inline void to_json(json& j, const glm::ivec2& P) { j = {{"x", P.x}, {"y", P.y}}; }
    inline void from_json(const json& j, glm::ivec2& P)
    {
        P.x = j.at("x").get<int>();
        P.y = j.at("y").get<int>();
    }

    inline void to_json(json& j, const glm::ivec3& P) { j = {{"x", P.x}, {"y", P.y}, {"z", P.z}}; }
    inline void from_json(const json& j, glm::ivec3& P)
    {
        P.x = j.at("x").get<int>();
        P.y = j.at("y").get<int>();
        P.z = j.at("z").get<int>();
    }

    inline void to_json(json& j, const glm::ivec4& P) { j = {{"x", P.x}, {"y", P.y}, {"z", P.z}, {"w", P.w}}; }
    inline void from_json(const json& j, glm::ivec4& P)
    {
        P.x = j.at("x").get<int>();
        P.y = j.at("y").get<int>();
        P.z = j.at("z").get<int>();
        P.w = j.at("w").get<int>();
    }
    //**********************************************************************************

    //**********************************************************************************
    inline void to_json(json& j, const glm::fvec1& P) { j = {{"x", P.x}}; }
    inline void from_json(const json& j, glm::fvec1& P)
    {
        P.x = j.at("x").get<double>();
    }

    inline void to_json(json& j, const glm::fvec2& P) { j = {{"x", P.x}, {"y", P.y}}; }
    inline void from_json(const json& j, glm::fvec2& P)
    {
        P.x = j.at("x").get<double>();
        P.y = j.at("y").get<double>();
    }

    inline void to_json(json& j, const glm::fvec3& P) { j = {{"x", P.x}, {"y", P.y}, {"z", P.z}}; }
    inline void from_json(const json& j, glm::fvec3& P)
    {
        P.x = j.at("x").get<double>();
        P.y = j.at("y").get<double>();
        P.z = j.at("z").get<double>();
    }

    inline void to_json(json& j, const glm::fvec4& P) { j = {{"x", P.x}, {"y", P.y}, {"z", P.z}, {"w", P.w}}; }
    inline void from_json(const json& j, glm::fvec4& P)
    {
        P.x = j.at("x").get<double>();
        P.y = j.at("y").get<double>();
        P.z = j.at("z").get<double>();
        P.w = j.at("w").get<double>();
    }
    //**********************************************************************************

    //**********************************************************************************
    inline void to_json(json& j, const glm::mat3x3& P)
    {
        j = {
            {"_m00", P[0][0]}, {"_m01", P[1][0]}, {"_m02", P[2][0]},
            {"_m10", P[0][1]}, {"_m11", P[1][1]}, {"_m12", P[2][1]},
            {"_m20", P[0][2]}, {"_m21", P[1][2]}, {"_m22", P[2][2]}
        };
    }
    inline void from_json(const json& j, glm::mat3x3& P)
    {
        P[0][0] = j.at("_m00").get<double>();
        P[0][1] = j.at("_m10").get<double>();
        P[0][2] = j.at("_m20").get<double>();
        P[1][0] = j.at("_m01").get<double>();
        P[1][1] = j.at("_m11").get<double>();
        P[1][2] = j.at("_m21").get<double>();
        P[2][0] = j.at("_m02").get<double>();
        P[2][1] = j.at("_m12").get<double>();
        P[2][2] = j.at("_m22").get<double>();
    }
    //**********************************************************************************

    //**********************************************************************************
    inline void to_json(json& j, const glm::mat4x4& P)
    {
        j = {
            {"_m00", P[0][0]}, {"_m01", P[1][0]}, {"_m02", P[2][0]}, {"_m03", P[3][0]},
            {"_m10", P[0][1]}, {"_m11", P[1][1]}, {"_m12", P[2][1]}, {"_m13", P[3][1]},
            {"_m20", P[0][2]}, {"_m21", P[1][2]}, {"_m22", P[2][2]}, {"_m23", P[3][2]},
            {"_m30", P[0][3]}, {"_m31", P[1][3]}, {"_m32", P[2][3]}, {"_m33", P[3][3]}
        };
    }
    inline void from_json(const json& j, glm::mat4x4& P)
    {
        P[0][0] = j.at("_m00").get<double>();
        P[0][1] = j.at("_m10").get<double>();
        P[0][2] = j.at("_m20").get<double>();
        P[0][3] = j.at("_m30").get<double>();
        P[1][0] = j.at("_m01").get<double>();
        P[1][1] = j.at("_m11").get<double>();
        P[1][2] = j.at("_m21").get<double>();
        P[1][3] = j.at("_m31").get<double>();
        P[2][0] = j.at("_m02").get<double>();
        P[2][1] = j.at("_m12").get<double>();
        P[2][2] = j.at("_m22").get<double>();
        P[2][3] = j.at("_m32").get<double>();
        P[3][0] = j.at("_m03").get<double>();
        P[3][1] = j.at("_m13").get<double>();
        P[3][2] = j.at("_m23").get<double>();
        P[3][3] = j.at("_m33").get<double>();
    }
    //**********************************************************************************

    //**********************************************************************************
    inline void to_json(json& j, const glm::quat& P) { j = {{"w", P.w}, {"x", P.x}, {"y", P.y}, {"z", P.z}}; }
    inline void from_json(const json& j, glm::quat& P)
    {
        P.w = j.at("w").get<double>();
        P.x = j.at("x").get<double>();
        P.y = j.at("y").get<double>();
        P.z = j.at("z").get<double>();
    }
    //**********************************************************************************

} // namespace glm

namespace MoYu
{
#define TransformComponentName "TransformComponent"
#define MeshComponentName "MeshComponent"
#define LightComponentName "LightComponent"
#define CameraComponentName "CameraComponent"

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Color, r, g, b, a)
} // namespace MoYu