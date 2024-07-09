//
// This file was automatically generated. Please don't edit by hand. Execute Editor command [ Edit > Rendering > Generate Shader Includes ] instead
//

#ifndef HDSHADOWMANAGER_CS_HLSL
#define HDSHADOWMANAGER_CS_HLSL

#ifdef _CPP_MACRO_
#define uint glm::uint
#define uint2 glm::uvec2
#define uint3 glm::uvec3
#define uint4 glm::uvec4
#define int2 glm::int2
#define int3 glm::int3
#define int4 glm::int4
#define float2 glm::fvec2
#define float3 glm::fvec3
#define float4 glm::fvec4
#define float4x4 glm::mat4x4
#endif

// Generated from UnityEngine.Rendering.HighDefinition.DShadowData
// PackingRules = Exact
struct HDShadowData
{
    int shadowmapIndex; // -1 means no shadow
    float worldTexelSize; // 2.0f / shadowRequest.deviceProjectionYFlip.m00 / viewportSize.x * Mathf.Sqrt(2.0f);
    float normalBias; // Controls the bias this Light applies along the normal of surfaces it illuminates.
    uint cascadeNumber; // cascade shadow count
    float4x4 viewMatrix; // viewMatrix for light
    float4x4 projMatrix; // projectionMatrix
    float4x4 viewProjMatrix;
    float4 zBufferParam; // float4((f - n) / n, 1.0f, (f - n) / (n * f), 1.0f / f);
    float4 shadowBounds; // float4(width, height, 0, 0)
    float4 shadowMapSize; // float4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
    float4 cacheTranslationDelta; // cameraPos - m_CachedViewPositions[index];
    float4x4 shadowToWorld; // transpose(invViewProjection)
};

// Generated from UnityEngine.Rendering.HighDefinition.HDDirectionalShadowData
// PackingRules = Exact
struct HDDirectionalShadowData
{
    float4 shdowCenterOffset;
    float4 shadowSizePowScale;
    float4x4 viewMatrix[4];
    float4x4 projMatrix[4];
    float4x4 viewprojMatrix[4];
};

#ifdef _CPP_MACRO_
#undef uint
#undef uint2
#undef uint3
#undef uint4
#undef int2
#undef int3
#undef int4
#undef float2
#undef float3
#undef float4
#undef float4x4
#endif

#endif
