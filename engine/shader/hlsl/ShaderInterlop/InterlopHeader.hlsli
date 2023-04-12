// clang-format on
#pragma once

// To be able to share Constant Buffer Types (Structs) between HLSL and C++, a few defines are set here.
// Define the Structs in HLSLS syntax, and with this defines the C++ application can also use the structs.
#ifdef __cplusplus

#define float4 glm::vec4
#define float3 glm::vec3
#define float2 glm::vec2

#define int int32_t

#define uint uint32_t
#define uint2 glm::uvec2
#define uint3 glm::uvec3
#define uint4 glm::uvec4

// Note : using the typedef of matrix (float4x4) and struct (ConstantBufferStruct) to prevent name collision on the cpp
// code side.
#define float4x4 glm::mat4x4
#define float3x4 glm::mat3x4

#define ConstantBufferStruct struct alignas(256)

#else
// if HLSL

#define ConstantBufferStruct struct

#endif
// clang-format on