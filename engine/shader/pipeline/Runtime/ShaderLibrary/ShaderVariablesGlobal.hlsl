#ifndef UNITY_SHADER_VARIABLES_GLOBAL_INCLUDED
#define UNITY_SHADER_VARIABLES_GLOBAL_INCLUDED

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
#include "../../Lighting/LightDefinition.hlsl"
#include "../../Lighting/Shadow/HDShadowManager.hlsl"
#endif

// ----------------------------------------------------------------------------
// Macros that override the register local for constant buffers (for ray tracing mainly)
// ----------------------------------------------------------------------------
// Global Input Resources - t registers. Unity supports a maximum of 64 global input resources (compute buffers, textures, acceleration structure).

#define RENDERING_LIGHT_LAYERS_MASK (255)
#define RENDERING_LIGHT_LAYERS_MASK_SHIFT (0)
#define RENDERING_DECAL_LAYERS_MASK (65280)
#define RENDERING_DECAL_LAYERS_MASK_SHIFT (8)
#define DEFAULT_RENDERING_LAYER_MASK (257)
#define DEFAULT_DECAL_LAYERS (255)

#define RENDERING_MAX_POINT_LIGHT_COUNT (16)
#define RENDERING_MAX_SPOT_LIGHT_COUNT (16)
#define RENDERING_MAX_LIGHT_COUNT (64)

#define RENDERING_MAX_ENV_LIGHT_COUNT (16)

#define _EnableSSRefraction 0
#define _EnvLightSkyEnabled 0

struct RenderDataPerDraw
{
    float4x4 objectToWorldMatrix;
    float4x4 worldToObjectMatrix;
    float4x4 prevObjectToWorldMatrix;
    float4x4 prevWorldToObjectMatrix;
    float4 vertexBufferView; // D3D12_VERTEX_BUFFER_VIEW 16
    float4 indexBufferView; // D3D12_INDEX_BUFFER_VIEW 16
    float4 drawIndexedArguments0; // D3D12_DRAW_INDEXED_ARGUMENTS 16
    float drawIndexedArguments1; // D3D12_DRAW_INDEXED_ARGUMENTS 4
    uint lightPropertyBufferIndex; // LightPropertyBufferIndex 4
    uint lightPropertyBufferOffset; // lightPropertyBufferOffset 4
    uint Padding0;
    float4 boundingBoxCenter; // BoundingBox 16
    float4 boundingBoxExtents; // BoundingBox 16
};

struct CameraUniform
{
    float4x4 _PrevViewProjMatrix;
    float4x4 _InvPrevViewProjMatrix;
    float4x4 _ViewProjMatrix;
    float4x4 _NonJitteredProjMatrix;
    float4x4 _NonJitteredViewProjMatrix;
    float4x4 _InvNonJitteredProjMatrix;
    float4x4 _InvNonJitteredViewProjMatrix;
    float4x4 _ViewMatrix;
    float4x4 _ProjMatrix;
    float4x4 _InvViewProjMatrix;
    float4x4 _InvViewMatrix;
    float4x4 _InvProjMatrix;
    float4   _ScreenSize;       // {w, h, 1/w, 1/h}
    // float4   _FrustumPlanes[6]; // {(a, b, c) = N, d = -dot(N, P)} [L, R, T, B, N, F]
    
    float3  _WorldSpaceCameraPos;      // camera world position
    uint    jitterIndex;         // jitter index
    
    // x = 1 or -1 (-1 if projection is flipped)
    // y = near plane
    // z = far plane
    // w = 1/far plane
    float4 _ProjectionParams;
    // x = width
    // y = height
    // z = 1 + 1.0/width
    // w = 1 + 1.0/height
    float4 _ScreenParams;
    // Values used to linearize the Z buffer (http://www.humus.name/temp/Linearize%20depth.txt)
    // x = 1-far/near
    // y = far/near
    // z = x/far
    // w = y/far
    // or in case of a reversed depth buffer (UNITY_REVERSED_Z is 1)
    // x = -1+far/near
    // y = 1
    // z = x/far
    // w = 1/far
    float4 _ZBufferParams;
    // x = orthographic camera's width
    // y = orthographic camera's height
    // z = unused
    // w = 1.0 if camera is ortho, 0.0 if perspective
    float4 unity_OrthoParams;
    
    float4 _Resolution; // physical viewport width, height, 1/width, 1/height

    float _CameraNear;
    float _CameraFar; // camera *culling* far-plane distance, always positive (projection far is at +inf)
    float2 _Padding0;
    
    //X : Use last frame positions (right now skinned meshes are the only objects that use this
    //Y : Force No Motion
    //Z : Z bias value
    float4 unity_MotionVectorsParams;
};

struct BaseUniform
{
    float4 _ScreenSize; // {w, h, 1/w, 1/h}
    float4 _PostProcessScreenSize;
    float4 _RTHandleScale;
    float4 _RTHandleScaleHistory;
    float4 _RTHandlePostProcessScale;
    float4 _RTHandlePostProcessScaleHistory;
    float4 _DynamicResolutionFullscreenScale;

    // Time (t = time since current level load) values from Unity
    float4 _Time; // (t/20, t, t*2, t*3)
    float4 _SinTime; // sin(t/8), sin(t/4), sin(t/2), sin(t)
    float4 _CosTime; // cos(t/8), cos(t/4), cos(t/2), cos(t)
    float4 _DeltaTime; // dt, 1/dt, smoothdt, 1/smoothdt
    float4 _TimeParameters; // t, sin(t), cos(t)
    float4 _LastTimeParameters;

    float4 temporalNoise; // noise [0,1] when TAA is used, 0 otherwise

    float needsAlphaChannel;
    float lodBias; // load bias to apply to user materials
    float refractionLodOffset;
    int _FrameCount;
    
    float _IndirectDiffuseLightingMultiplier;
    uint _IndirectDiffuseLightingLayers;
    float _ReflectionLightingMultiplier;
    uint _ReflectionLightingLayers;
    
    uint _CameraDepthTextureIndex;
    uint _ColorPyramidTextureIndex; // Color pyramid (width, height, lodcount, Unused)
    uint _CustomDepthTextureIndex;
    uint _CustomColorTextureIndex;

    uint _SkyTextureIndex;
    uint _AmbientOcclusionTextureIndex;
    uint _CameraMotionVectorsTextureIndex;
    uint _SsrLightingTextureIndex;
};

struct AOUniform
{
    float4 _AmbientOcclusionParam;
    float aoSamplingQualityAndEdgeDistance; // <0: no AO, 0: bilinear, !0: bilateral edge distance
    float aoBentNormals; // 0: no AO bent normal, >0.0 AO bent normals
    float _SpecularOcclusionBlend;
    float aoReserved0;
};

struct TAAUniform
{
    float _HistorySharpening;
    float _AntiFlickerIntensity;
    float _SpeedRejectionIntensity;
    float _ContrastForMaxAntiFlicker;
    
    float _BaseBlendFactor;
    float _CentralWeight;
    uint _ExcludeTAABit;
    float _HistoryContrastBlendLerp;

    float2 _RTHandleScaleForTAAHistory;
    float2 _RTHandleScaleForTAA;
    
    float4 _TaaFrameInfo;
    float4 _TaaJitterStrength;
    
    float4 _InputSize;    
    float4 _TaaHistorySize;

    float _TAAUFilterRcpSigma2;
    float _TAAUScale;
    float _TAAUBoxConfidenceThresh;
    float _TAAURenderScale;
    
    float4 _TaaFilterWeights[2];
    float4 _NeighbourOffsets[4];
};

struct IBLUniform
{
    float4 iblSH[7]; // actually float3 entries (std140 requires float4 alignment)
    float iblLuminance;
    float iblRoughnessOneLevel; // level for roughness == 1
    int   dfg_lut_srv_index;    // specular lut dfg
    int   ld_lut_srv_index;     // specular lut ld
    int   radians_srv_index;    // diffuse
    uint _PreIntegratedFGD_GGXDisneyDiffuseIndex;
    uint _PreIntegratedFGD_CharlieAndFabricIndex;
    uint _Padding0;
};

struct SSRUniform
{
    float _SsrThicknessScale;
    float _SsrThicknessBias;
    int _SsrStencilBit;
    int _SsrIterLimit;
    
    float _SsrRoughnessFadeEnd;
    float _SsrRoughnessFadeRcpLength;
    float _SsrRoughnessFadeEndTimesRcpLength;
    float _SsrEdgeFadeRcpLength;

    int _SsrDepthPyramidMaxMip;
    int _SsrColorPyramidMaxMip;
    int _SsrReflectsSky;
    float _SsrAccumulationAmount;
    
    float _SsrPBRSpeedRejection;
    float _SsrPBRBias;
    float _SsrPRBSpeedRejectionScalerFactor;
    float _SsrPBRPad0;
    
    //float4 ScreenSize;
    //float4 ResolveSize;
    //float4 RayCastSize;
    //float4 JitterSizeAndOffset;
    //float4 NoiseSize;

    //float  SmoothnessRange;
    //float  BRDFBias;
    //float  TResponseMin;
    //float  TResponseMax;

    //float  EdgeFactor;
    //float  Thickness;
    //int    NumSteps;
    //int    MaxMipMap;
};

struct SSGIStruct
{
    uint _IndirectDiffuseTextureIndex;
    uint3 _Padding0;
};

struct VolumeCloudStruct
{
    float4x4 cloud_shadow_view_matrix;
    float4x4 cloud_shadow_proj_matrix;
    float4x4 cloud_shadow_proj_view_matrix;
    uint cloud_shadowmap_srv_index;
    uint cloud_shadowmap_size;
    int2 cloud_shadowmap_bounds;
    float3 sunlight_direction;
    float sun_to_earth_distance;
};

struct LightDataUniform
{
    HDShadowData shadowDatas[RENDERING_MAX_LIGHT_COUNT];
    HDDirectionalShadowData directionalShadowData;

    EnvLightData envData;
    LightData lightData[RENDERING_MAX_LIGHT_COUNT];
    DirectionalLightData directionalLightData;

    uint _PunctualLightCount;
    uint _PointLightCount;
    uint _SpotLightCount;
    uint _unused_;
};

struct MeshUniform
{
    uint totalMeshCount;
    float3 _padding_0;
};

struct TerrainUniform
{
    uint terrainSize;      // 1024*1024
    uint terrainMaxHeight; // 1024
    uint heightMapIndex;
    uint normalMapIndex;
    float4x4 local2WorldMatrix;
    float4x4 world2LocalMatrix;
    float4x4 prevLocal2WorldMatrix;
    float4x4 prevWorld2LocalMatrix;
};

// =======================================
// VolumeLight
// =======================================
struct VolumeLightUniform
{
    float scattering_coef;
    float extinction_coef;
    float volumetrix_range;
    float skybox_extinction_coef;

    float mieG; // x: 1 - g^2, y: 1 + g^2, z: 2*g, w: 1/4pi
    
    float noise_scale;
    float noise_intensity;
    float noise_intensity_offset;

    float2 noise_velocity;
    float ground_level;
    float height_scale;

    float maxRayLength;
    int sampleCount;
    int downscaleMip;
    float minStepSize;
};

// =======================================
// SubsurfaceScattering and Tranlucent
// =======================================
struct SSSUniform
{
    float4 _ShapeParamsAndMaxScatterDists[16]; // RGB = S = 1 / D, A = d = RgbMax(D)
    float4 _TransmissionTintsAndFresnel0[16]; // RGB = 1/4 * color, A = fresnel0
    float4 _WorldScalesAndFilterRadiiAndThicknessRemaps[16]; // X = meters per world unit, Y = filter radius (in mm), Z = remap start, W = end - start
    uint4 _DiffusionProfileHashTable[16];
    uint _EnableSubsurfaceScattering; // Globally toggles subsurface and transmission scattering on/off
    uint _TexturingModeFlags;         // 1 bit/profile; 0 = PreAndPostScatter, 1 = PostScatter
    uint _TransmissionFlags;          // 1 bit/profile; 0 = regular, 1 = thin
    uint _DiffusionProfileCount;
    uint _SssSampleBudget;
    uint3 _padding_0;
};

struct ExposureUniform
{
    float _ProbeExposureScale;
    uint _ExposureTextureIndex;
    uint __PrevExposureTextureIndex;
    uint _padding_0;
};

// =======================================
// FraneUniform
// =======================================

struct FrameUniforms
{
    CameraUniform cameraUniform;
    BaseUniform baseUniform;
    TerrainUniform terrainUniform;
    MeshUniform meshUniform;
    AOUniform aoUniform;
    TAAUniform taaUniform;
    IBLUniform iblUniform;
    SSRUniform ssrUniform;
    SSGIStruct ssgiUniform;
    VolumeCloudStruct volumeCloudUniform;
    LightDataUniform lightDataUniform;
    VolumeLightUniform volumeLightUniform;
    SSSUniform sssUniform;
    ExposureUniform exposureUniform;
};

// =======================================
// Terrain
// =======================================

struct ClipmapTransform
{
    float4x4 transform;
    int mesh_type;
};

struct ClipmapMeshCount
{
    uint tile_count;
    uint filler_count;
    uint trim_count;
    uint cross_cunt;
    uint seam_count;
    uint total_count;
};

struct ClipMeshCommandSigParams
{
    float4 vertexBufferView; // D3D12_VERTEX_BUFFER_VIEW 16
    float4 indexBufferView; // D3D12_INDEX_BUFFER_VIEW 16
    float4 drawIndexedArguments0; // D3D12_DRAW_INDEXED_ARGUMENTS 16
    float4 drawIndexedArguments1; // D3D12_DRAW_INDEXED_ARGUMENTS 4, Empty 12
    float4 clipBoundingBoxCenter; // BoundingBox 16
    float4 clipBoundingBoxExtents; // BoundingBox 16
};

struct ToDrawCommandSignatureParams
{
    uint clipIndex; // ClipIndex 4
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView; // D3D12_VERTEX_BUFFER_VIEW 16
    D3D12_INDEX_BUFFER_VIEW indexBufferView; // D3D12_INDEX_BUFFER_VIEW 16
    D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments; // D3D12_DRAW_INDEXED_ARGUMENTS 20
};

struct TerrainPatchNode
{
    float2 patchMinPos; // node的左下角顶点
    float maxHeight; // 当前node最大高度
    float minHeight; // 当前node最小高度
    float nodeWidth; // patchnode的宽度
    int mipLevel; // 当前node的mip等级
    uint neighbor; // 更高一级mip作为邻居的标识
};

// =======================================
// Samplers
// =======================================

#ifndef _CPP_MACRO_

struct SamplerStruct
{
    SamplerState SPointClampSampler;
    SamplerState SLinearClampSampler;
    SamplerState SLinearRepeatSampler;
    SamplerState STrilinearClampSampler;
    SamplerState STrilinearRepeatSampler;
    
    SamplerComparisonState SLinearClampCompareSampler;
};

//==============================RenderDataPerDraw=================================
D3D12_VERTEX_BUFFER_VIEW ParamsToVertexBufferView(float4 params)
{
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = (D3D12_VERTEX_BUFFER_VIEW) 0;
    vertexBufferView.BufferLocation = asuint(params.xy);
    vertexBufferView.SizeInBytes = asuint(params.z);
    vertexBufferView.StrideInBytes = asuint(params.w);
    return vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW ParamsToIndexBufferView(float4 params)
{
    D3D12_INDEX_BUFFER_VIEW indexBufferView = (D3D12_INDEX_BUFFER_VIEW) 0;
    indexBufferView.BufferLocation = asuint(params.xy);
    indexBufferView.SizeInBytes = asuint(params.z);
    indexBufferView.Format = asuint(params.w);
    return indexBufferView;
}

D3D12_DRAW_INDEXED_ARGUMENTS ParamsToDrawIndexedArgumens(float4 params0, float params2)
{
    D3D12_DRAW_INDEXED_ARGUMENTS drawIndexArguments = (D3D12_DRAW_INDEXED_ARGUMENTS) 0;
    drawIndexArguments.IndexCountPerInstance = asuint(params0[0]);
    drawIndexArguments.InstanceCount = asuint(params0[1]);
    drawIndexArguments.StartIndexLocation = asuint(params0[2]);
    drawIndexArguments.BaseVertexLocation = asint(params0[3]);
    drawIndexArguments.StartInstanceLocation = asuint(params2);
    return drawIndexArguments;
}

float4 VertexBufferViewToParams(D3D12_VERTEX_BUFFER_VIEW vertexBufferView)
{
    float4 outParams = (float4)0;
    outParams.xy = vertexBufferView.BufferLocation;
    outParams.z = vertexBufferView.SizeInBytes;
    outParams.w = vertexBufferView.StrideInBytes;
    return outParams;
}

float4 IndexBufferViewToParams(D3D12_INDEX_BUFFER_VIEW indexBufferView)
{
    float4 outParams = (float4)0;
    outParams.xy = indexBufferView.BufferLocation;
    outParams.z = indexBufferView.SizeInBytes;
    outParams.w = indexBufferView.Format;
    return outParams;
}

void DrawIndexedArgumentsToParams(D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments, out float4 params0, out float params1)
{
    params0.x = drawIndexedArguments.IndexCountPerInstance;
    params0.y = drawIndexedArguments.InstanceCount;
    params0.z = drawIndexedArguments.StartIndexLocation;
    params0.w = drawIndexedArguments.BaseVertexLocation;
    params1 = drawIndexedArguments.StartInstanceLocation;
}

uint GetLightPropertyBufferIndex(RenderDataPerDraw renderDataPerDraw)
{
    uint lightPropertyBufferIndex = renderDataPerDraw.lightPropertyBufferIndex;
    return lightPropertyBufferIndex;
}

uint GetLightPropertyBufferIndexOffset(RenderDataPerDraw renderDataPerDraw)
{
    uint lightPropertyBufferIndexOffset = renderDataPerDraw.lightPropertyBufferOffset;
    return lightPropertyBufferIndexOffset;
}

BoundingBox GetRendererBounds(RenderDataPerDraw renderDataPerDraw)
{
    BoundingBox boundingBox = (BoundingBox) 0;
    boundingBox.Center = renderDataPerDraw.boundingBoxCenter;
    boundingBox.Extents = renderDataPerDraw.boundingBoxExtents;
    return boundingBox;
}

//==============================RenderDataPerDraw=================================

//==============================ClipMeshCommandSigParams=================================

BoundingBox GetRendererBounds(ClipMeshCommandSigParams clipMeshCommandSigParams)
{
    BoundingBox boundingBox = (BoundingBox) 0;
    boundingBox.Center = clipMeshCommandSigParams.clipBoundingBoxCenter;
    boundingBox.Extents = clipMeshCommandSigParams.clipBoundingBoxExtents;
    return boundingBox;
}

//==============================ClipMeshCommandSigParams=================================



#endif


// struct ShaderVariablesGlobal
// {
//     float4x4 _ViewMatrix;
//     float4x4 _CameraViewMatrix;
//     float4x4 _InvViewMatrix;
//     float4x4 _ProjMatrix;
//     float4x4 _InvProjMatrix;
//     float4x4 _ViewProjMatrix;
//     float4x4 _CameraViewProjMatrix;
//     float4x4 _InvViewProjMatrix;
//     float4x4 _NonJitteredViewProjMatrix;
//     float4x4 _PrevViewProjMatrix;
//     float4x4 _PrevInvViewProjMatrix;
//     float4 _WorldSpaceCameraPos_Internal;
//     float4 _PrevCamPosRWS_Internal;
//     float4 _ScreenSize;
//     float4 _PostProcessScreenSize;
//     float4 _RTHandleScale;
//     float4 _RTHandleScaleHistory;
//     float4 _RTHandlePostProcessScale;
//     float4 _RTHandlePostProcessScaleHistory;
//     float4 _DynamicResolutionFullscreenScale;
//     float4 _ZBufferParams;
//     float4 _ProjectionParams;
//     float4 unity_OrthoParams;
//     float4 _ScreenParams;
//     float4 _FrustumPlanes[6];
//     float4 _ShadowFrustumPlanes[6];
//     float4 _TaaFrameInfo;
//     float4 _TaaJitterStrength;
//     float4 _Time;
//     float4 _SinTime;
//     float4 _CosTime;
//     float4 unity_DeltaTime;
//     float4 _TimeParameters;
//     float4 _LastTimeParameters;
//     int _FogEnabled;
//     int _PBRFogEnabled;
//     int _EnableVolumetricFog;
//     float _MaxFogDistance;
//     float4 _FogColor;
//     float _FogColorMode;
//     float _GlobalMipBias;
//     float _GlobalMipBiasPow2;
//     float _Pad0;
//     float4 _MipFogParameters;
//     float4 _HeightFogBaseScattering;
//     float _HeightFogBaseExtinction;
//     float _HeightFogBaseHeight;
//     float _GlobalFogAnisotropy;
//     int _VolumetricFilteringEnabled;
//     float2 _HeightFogExponents;
//     int _FogDirectionalOnly;
//     float _FogGIDimmer;
//     float4 _VBufferViewportSize;
//     float4 _VBufferLightingViewportScale;
//     float4 _VBufferLightingViewportLimit;
//     float4 _VBufferDistanceEncodingParams;
//     float4 _VBufferDistanceDecodingParams;
//     uint _VBufferSliceCount;
//     float _VBufferRcpSliceCount;
//     float _VBufferRcpInstancedViewCount;
//     float _VBufferLastSliceDist;
//     float4 _ShadowAtlasSize;
//     float4 _CascadeShadowAtlasSize;
//     float4 _AreaShadowAtlasSize;
//     float4 _CachedShadowAtlasSize;
//     float4 _CachedAreaShadowAtlasSize;
//     int _ReflectionsMode;
//     int _UnusedPadding0;
//     int _UnusedPadding1;
//     int _UnusedPadding2;
//     uint _DirectionalLightCount;
//     uint _PunctualLightCount;
//     uint _AreaLightCount;
//     uint _EnvLightCount;
//     int _EnvLightSkyEnabled;
//     uint _CascadeShadowCount;
//     int _DirectionalShadowIndex;
//     uint _EnableLightLayers;
//     uint _EnableSkyReflection;
//     uint _EnableSSRefraction;
//     float _SSRefractionInvScreenWeightDistance;
//     float _ColorPyramidLodCount;
//     float _DirectionalTransmissionMultiplier;
//     float _ProbeExposureScale;
//     float _ContactShadowOpacity;
//     float _ReplaceDiffuseForIndirect;
//     float4 _AmbientOcclusionParam;
//     float _IndirectDiffuseLightingMultiplier;
//     uint _IndirectDiffuseLightingLayers;
//     float _ReflectionLightingMultiplier;
//     uint _ReflectionLightingLayers;
//     float _MicroShadowOpacity;
//     uint _EnableProbeVolumes;
//     uint _ProbeVolumeCount;
//     float _SlopeScaleDepthBias;
//     float4 _CookieAtlasSize;
//     float4 _CookieAtlasData;
//     float4 _ReflectionAtlasCubeData;
//     float4 _ReflectionAtlasPlanarData;
//     uint _NumTileFtplX;
//     uint _NumTileFtplY;
//     float g_fClustScale;
//     float g_fClustBase;
//     float g_fNearPlane;
//     float g_fFarPlane;
//     int g_iLog2NumClusters;
//     uint g_isLogBaseBufferEnabled;
//     uint _NumTileClusteredX;
//     uint _NumTileClusteredY;
//     int _EnvSliceSize;
//     uint _EnableDecalLayers;
//     float4 _ShapeParamsAndMaxScatterDists[16];
//     float4 _TransmissionTintsAndFresnel0[16];
//     float4 _WorldScalesAndFilterRadiiAndThicknessRemaps[16];
//     uint4 _DiffusionProfileHashTable[16];
//     uint _EnableSubsurfaceScattering;
//     uint _TexturingModeFlags;
//     uint _TransmissionFlags;
//     uint _DiffusionProfileCount;
//     float2 _DecalAtlasResolution;
//     uint _EnableDecals;
//     uint _DecalCount;
//     float _OffScreenDownsampleFactor;
//     uint _OffScreenRendering;
//     uint _XRViewCount;
//     int _FrameCount;
//     float4 _CoarseStencilBufferSize;
//     int _IndirectDiffuseMode;
//     int _EnableRayTracedReflections;
//     int _RaytracingFrameIndex;
//     uint _EnableRecursiveRayTracing;
//     int _TransparentCameraOnlyMotionVectors;
//     float _GlobalTessellationFactorMultiplier;
//     float _SpecularOcclusionBlend;
//     float _DeExposureMultiplier;
//     float4 _ScreenSizeOverride;
//     float4 _ScreenCoordScaleBias;
//     float4 _ColorPyramidUvScaleAndLimitPrevFrame;
// };
//

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

#endif // UNITY_SHADER_VARIABLES_GLOBAL_INCLUDED
