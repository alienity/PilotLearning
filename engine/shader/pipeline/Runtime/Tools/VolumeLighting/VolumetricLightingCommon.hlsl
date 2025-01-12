#ifndef RENDERPIPELINE_VOLUMETRICLIGHTING_HLSL
#define RENDERPIPELINE_VOLUMETRICLIGHTING_HLSL

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
#define float4x4 glm::float4x4
#define float2x4 glm::float2x4
#else
#include "../../ShaderLibrary/Common.hlsl"
#endif

#define LOCALVOLUMETRICFOGBLENDINGMODE_OVERWRITE (0)
#define LOCALVOLUMETRICFOGBLENDINGMODE_ADDITIVE (1)
#define LOCALVOLUMETRICFOGBLENDINGMODE_MULTIPLY (2)
#define LOCALVOLUMETRICFOGBLENDINGMODE_MIN (3)
#define LOCALVOLUMETRICFOGBLENDINGMODE_MAX (4)

#define LOCALVOLUMETRICFOGFALLOFFMODE_LINEAR (0)
#define LOCALVOLUMETRICFOGFALLOFFMODE_EXPONENTIAL (1)

struct ShaderVariablesVolumetric
{
    float4x4 _VBufferCoordToViewDirWS;
    float _VBufferUnitDepthTexelSpacing;
    uint _NumVisibleLocalVolumetricFog;
    float _CornetteShanksConstant;
    uint _VBufferHistoryIsValid;
    float4 _VBufferSampleOffset;
    float _VBufferVoxelSize;
    float _HaveToPad;
    float _OtherwiseTheBuffer;
    float _IsFilledWithGarbage;
    float4 _VBufferPrevViewportSize;
    float4 _VBufferHistoryViewportScale;
    float4 _VBufferHistoryViewportLimit;
    float4 _VBufferPrevDistanceEncodingParams;
    float4 _VBufferPrevDistanceDecodingParams;
    uint _NumTileBigTileX;
    uint _NumTileBigTileY;
    uint _MaxSliceCount;
    float _MaxVolumetricFogDistance;
    float4 _CameraRight;
    float4x4 _CameraInverseViewProjection_NO;
    uint _VolumeCount;
    uint _IsObliqueProjectionMatrix;
    float _VBufferSliceCount;
    float _VBufferRcpSliceCount;
};

struct VolumetricMaterialDataCBuffer
{
    float4 _VolumetricMaterialObbRight;
    float4 _VolumetricMaterialObbUp;
    float4 _VolumetricMaterialObbExtents;
    float4 _VolumetricMaterialObbCenter;
    float4 _VolumetricMaterialRcpPosFaceFade;
    float4 _VolumetricMaterialRcpNegFaceFade;
    float _VolumetricMaterialInvertFade;
    float _VolumetricMaterialRcpDistFadeLen;
    float _VolumetricMaterialEndTimesRcpDistFadeLen;
    float _VolumetricMaterialFalloffMode;
};

struct LocalVolumetricFogEngineData
{
    float3 scattering;
    int falloffMode;
    float3 textureTiling;
    int invertFade;
    float3 textureScroll;
    float rcpDistFadeLen;
    float3 rcpPosFaceFade;
    float endTimesRcpDistFadeLen;
    float3 rcpNegFaceFade;
    int blendingMode;
};

struct VolumetricMaterialRenderingData
{
    float4 viewSpaceBounds;
    uint startSliceIndex;
    uint sliceCount;
    uint padding0;
    uint padding1;
    float4 obbVertexPositionWS[8];
};

//********************************************

struct LocalVolumetricTransform
{
    float4x4 objectToWorldMatrix;
    float4x4 worldToObjectMatrix;
    float4x4 prevObjectToWorldMatrix;
    float4x4 prevWorldToObjectMatrix;
    float4 volumeSize;
};

struct LocalVolumetricFogTextures
{
    uint noise3DIndex;
    uint3 _padding0;
};

struct LocalFogCustomData
{
    float4 _FogVolumeSingleScatteringAlbedo;
    int _FogVolumeBlendMode;
    float _FogVolumeFogDistanceProperty;
    float2 _padding1;
};

struct LocalVolumetricFogDatas
{
    LocalVolumetricTransform localTransformData;
    LocalVolumetricFogEngineData localFogEngineData;
    LocalVolumetricFogTextures localFogTextures;
    VolumetricMaterialDataCBuffer volumeMaterialDataCBuffer;
    LocalFogCustomData localFogCustomData;
};

struct LocalVolumetricFogRenderDatas
{
    VolumetricMaterialRenderingData volumetricRenderData;
};

#define MAX_VOLUMETRIC_FOG_COUNT 32

//********************************************

#ifndef _CPP_MACRO_
float3 GetScattering(LocalVolumetricFogEngineData value)
{
    return value.scattering;
}
float3 GetTextureTiling(LocalVolumetricFogEngineData value)
{
    return value.textureTiling;
}
int GetInvertFade(LocalVolumetricFogEngineData value)
{
    return value.invertFade;
}
float3 GetTextureScroll(LocalVolumetricFogEngineData value)
{
    return value.textureScroll;
}
float GetRcpDistFadeLen(LocalVolumetricFogEngineData value)
{
    return value.rcpDistFadeLen;
}
float3 GetRcpPosFaceFade(LocalVolumetricFogEngineData value)
{
    return value.rcpPosFaceFade;
}
float GetEndTimesRcpDistFadeLen(LocalVolumetricFogEngineData value)
{
    return value.endTimesRcpDistFadeLen;
}
float3 GetRcpNegFaceFade(LocalVolumetricFogEngineData value)
{
    return value.rcpNegFaceFade;
}
int GetBlendingMode(LocalVolumetricFogEngineData value)
{
    return value.blendingMode;
}
int GetFalloffMode(LocalVolumetricFogEngineData value)
{
    return value.falloffMode;
}
#endif

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
#undef float2x4
#endif

#endif
