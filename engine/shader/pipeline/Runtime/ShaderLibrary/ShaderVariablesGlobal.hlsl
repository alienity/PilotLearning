#ifndef UNITY_SHADER_VARIABLES_GLOBAL_INCLUDED
#define UNITY_SHADER_VARIABLES_GLOBAL_INCLUDED

#ifdef _CPP_MACRO_
#define PREMACRO(T) glm::T
#else
#define PREMACRO(T) T
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

struct RenderDataPerDraw
{
    PREMACRO(float4x4) objectToWorldMatrix;
    PREMACRO(float4x4) worldToObjectMatrix;
    PREMACRO(float4x4) prevObjectToWorldMatrix;
    PREMACRO(float4x4) prevWorldToObjectMatrix;
    PREMACRO(float4)   vertexBufferView; // D3D12_VERTEX_BUFFER_VIEW 16
    PREMACRO(float4)   indexBufferView; // D3D12_INDEX_BUFFER_VIEW 16
    PREMACRO(float4x2) drawIndexedArguments; // D3D12_DRAW_INDEXED_ARGUMENTS 20, LightPropertyBufferIndex 4, LightPropertyBufferIndexOffset 4, Empty 4
    PREMACRO(float4x2) rendererBounds; // BoundingBox 32
};

struct CameraDataBuffer
{
    PREMACRO(float4x4) viewFromWorldMatrix; // clip    view <- world    : view matrix
    PREMACRO(float4x4) worldFromViewMatrix; // clip    view -> world    : model matrix
    PREMACRO(float4x4) clipFromViewMatrix;  // clip <- view    world    : projection matrix
    PREMACRO(float4x4) viewFromClipMatrix;  // clip -> view    world    : inverse projection matrix
    PREMACRO(float4x4) unJitterProjectionMatrix; // Unjitter projection matrix
    PREMACRO(float4x4) unJitterProjectionMatrixInv; // Unjitter projection matrix inverse
    PREMACRO(float4x4) clipFromWorldMatrix; // clip <- view <- world
    PREMACRO(float4x4) worldFromClipMatrix; // clip -> view -> world
    PREMACRO(float4)   clipTransform;       // [sx, sy, tx, ty] only used by VERTEX_DOMAIN_DEVICE
    PREMACRO(float3)   _WorldSpaceCameraPos;      // camera world position
    PREMACRO(uint)     jitterIndex;         // jitter index
    // x = 1 or -1 (-1 if projection is flipped)
    // y = near plane
    // z = far plane
    // w = 1/far plane
    PREMACRO(float4) _ProjectionParams;
    // x = width
    // y = height
    // z = 1 + 1.0/width
    // w = 1 + 1.0/height
    PREMACRO(float4) _ScreenParams;
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
    PREMACRO(float4) _ZBufferParams;
    // x = orthographic camera's width
    // y = orthographic camera's height
    // z = unused
    // w = 1.0 if camera is ortho, 0.0 if perspective
    PREMACRO(float4) unity_OrthoParams;
};

struct CameraUniform
{
    CameraDataBuffer _CurFrameUniform;
    CameraDataBuffer _PrevFrameUniform;
    PREMACRO(float4) _Resolution; // physical viewport width, height, 1/width, 1/height
    PREMACRO(float2) _LogicalViewportScale; // scale-factor to go from physical to logical viewport
    PREMACRO(float2) _LogicalViewportOffset; // offset to go from physical to logical viewport
    // camera position in view space (when camera_at_origin is enabled), i.e. it's (0,0,0).
    PREMACRO(float4) _ZBufferParams;
    float _CameraNear;
    float _CameraFar; // camera *culling* far-plane distance, always positive (projection far is at +inf)
    PREMACRO(float2) _Padding0;
};

struct BaseUniform
{
    PREMACRO(float4) _ScreenSize; // {w, h, 1/w, 1/h}
    PREMACRO(float4) _PostProcessScreenSize;
    PREMACRO(float4) _RTHandleScale;
    PREMACRO(float4) _RTHandleScaleHistory;
    PREMACRO(float4) _RTHandlePostProcessScale;
    PREMACRO(float4) _RTHandlePostProcessScaleHistory;
    PREMACRO(float4) _DynamicResolutionFullscreenScale;

    // Time (t = time since current level load) values from Unity
    PREMACRO(float4) _Time; // (t/20, t, t*2, t*3)
    PREMACRO(float4) _SinTime; // sin(t/8), sin(t/4), sin(t/2), sin(t)
    PREMACRO(float4) _CosTime; // cos(t/8), cos(t/4), cos(t/2), cos(t)
    PREMACRO(float4) _DeltaTime; // dt, 1/dt, smoothdt, 1/smoothdt
    PREMACRO(float4) _TimeParameters; // t, sin(t), cos(t)
    PREMACRO(float4) _LastTimeParameters;
    
    PREMACRO(float4) temporalNoise; // noise [0,1] when TAA is used, 0 otherwise
    float needsAlphaChannel;
    float lodBias; // load bias to apply to user materials
    float refractionLodOffset;
    float baseReserved0;
    
    float _IndirectDiffuseLightingMultiplier;
    PREMACRO(uint) _IndirectDiffuseLightingLayers;
    float _ReflectionLightingMultiplier;
    PREMACRO(uint) _ReflectionLightingLayers;
    
    PREMACRO(uint) _CameraDepthTextureIndex;
    PREMACRO(uint) _ColorPyramidTextureIndex; // Color pyramid (width, height, lodcount, Unused)
    PREMACRO(uint) _CustomDepthTextureIndex;
    PREMACRO(uint) _CustomColorTextureIndex;

    PREMACRO(uint) _SkyTextureIndex;
    PREMACRO(uint) _AmbientOcclusionTextureIndex;
    PREMACRO(uint) _CameraMotionVectorsTextureIndex;
    PREMACRO(uint) _SsrLightingTextureIndex;
};

struct AOUniform
{
    float aoSamplingQualityAndEdgeDistance; // <0: no AO, 0: bilinear, !0: bilateral edge distance
    float aoBentNormals; // 0: no AO bent normal, >0.0 AO bent normals
    float aoReserved0;
    float aoReserved1;
};

struct TAAUniform
{
    PREMACRO(float4) projectionExtents;  // xy = frustum extents at distance 1, zw = jitter at distance 1
    PREMACRO(float4) jitterUV;
    float feedbackMin;
    float feedbackMax;
    float motionScale;
    float __Reserved0;
};

struct IBLUniform
{
    PREMACRO(float4) iblSH[7]; // actually PREMACRO(float3) entries (std140 requires PREMACRO(float4) alignment)
    float iblLuminance;
    float iblRoughnessOneLevel; // level for roughness == 1
    int   dfg_lut_srv_index;    // specular lut dfg
    int   ld_lut_srv_index;     // specular lut ld
    int   radians_srv_index;    // diffuse
    PREMACRO(float3) __Reserved0;
};

struct SSRUniform
{
    PREMACRO(float4) ScreenSize;
    PREMACRO(float4) ResolveSize;
    PREMACRO(float4) RayCastSize;
    PREMACRO(float4) JitterSizeAndOffset;
    PREMACRO(float4) NoiseSize;
    float  SmoothnessRange;
    float  BRDFBias;
    float  TResponseMin;
    float  TResponseMax;
    float  EdgeFactor;
    float  Thickness;
    int    NumSteps;
    int    MaxMipMap;
};

struct VolumeCloudStruct
{
    PREMACRO(float4x4) cloud_shadow_view_matrix;
    PREMACRO(float4x4) cloud_shadow_proj_matrix;
    PREMACRO(float4x4) cloud_shadow_proj_view_matrix;
    PREMACRO(uint) cloud_shadowmap_srv_index;
    PREMACRO(uint) cloud_shadowmap_size;
    int2 cloud_shadowmap_bounds;
    PREMACRO(float3) sunlight_direction;
    float sun_to_earth_distance;
};

struct DirectionalLightShadowmap
{
    PREMACRO(uint) shadowmap_srv_index; // shadowmap srv in descriptorheap index    
    PREMACRO(uint) cascadeCount; // how many cascade level, default 4
    PREMACRO(uint2) shadowmap_size; // shadowmap size
    PREMACRO(uint4) shadow_bounds; // shadow bounds for each cascade level
    PREMACRO(float4x4) light_view_matrix; // direction light view matrix
    PREMACRO(float4x4) light_proj_matrix[4];
    PREMACRO(float4x4) light_proj_view[4];
};

struct DirectionalLightStruct
{
    PREMACRO(float4) lightColorIntensity; // directional light rgb - color, a - intensity
    PREMACRO(float3) lightPosition; // directional light position
    float lightRadius; // sun radius
    PREMACRO(float3) lightDirection; // directional light direction
    PREMACRO(uint) shadowType; // 1 use shadowmap
    PREMACRO(float2) lightFarAttenuationParams; // a, a/far (a=1/pct-of-far)
    PREMACRO(float2) _padding_0;
    
    DirectionalLightShadowmap directionalLightShadowmap;
};

struct PointLightShadowmap
{
    PREMACRO(uint) shadowmap_srv_index; // shadowmap srv in descriptorheap index
    PREMACRO(uint2) shadowmap_size;
    float _padding_shadowmap;
    PREMACRO(float4x4) light_proj_view[6];
};

struct PointLightStruct
{
    PREMACRO(float3) lightPosition;
    float lightRadius;
    PREMACRO(float4) lightIntensity; // point light rgb - color, a - intensity
    PREMACRO(uint) shadowType;
    float falloff; // default 1.0f
    PREMACRO(uint2) _padding_0;

    PointLightShadowmap pointLightShadowmap;
};

struct PointLightUniform
{
    PREMACRO(uint) pointLightCounts;
    PREMACRO(float3) _padding_0;
    PointLightStruct pointLightStructs[RENDERING_MAX_POINT_LIGHT_COUNT];
};

struct SpotLightShadowmap
{
    PREMACRO(uint) shadowmap_srv_index; // shadowmap srv in descriptorheap index
    PREMACRO(uint2) shadowmap_size;
    float _padding_0;
    PREMACRO(float4x4) light_proj_view;
};

struct SpotLightStruct
{
    PREMACRO(float3) lightPosition;
    float lightRadius;
    PREMACRO(float4) lightIntensity; // spot light rgb - color, a - intensity
    PREMACRO(float3) lightDirection;
    float inner_radians;
    float outer_radians;
    PREMACRO(uint) shadowType;
    float falloff; // default 1.0f
    float _padding_0;

    SpotLightShadowmap spotLightShadowmap;
};

struct SpotLightUniform
{
    PREMACRO(uint) spotLightCounts;
    PREMACRO(float3) _padding_0;
    SpotLightStruct spotLightStructs[RENDERING_MAX_SPOT_LIGHT_COUNT];
};

struct MeshUniform
{
    PREMACRO(uint) totalMeshCount;
    PREMACRO(float3) _padding_0;
};

struct TerrainUniform
{
    PREMACRO(uint) terrainSize;      // 1024*1024
    PREMACRO(uint) terrainMaxHeight; // 1024
    PREMACRO(uint) heightMapIndex;
    PREMACRO(uint) normalMapIndex;
    PREMACRO(float4x4) local2WorldMatrix;
    PREMACRO(float4x4) world2LocalMatrix;
    PREMACRO(float4x4) prevLocal2WorldMatrix;
    PREMACRO(float4x4) prevWorld2LocalMatrix;
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

    PREMACRO(float2) noise_velocity;
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
    PREMACRO(float4) _ShapeParamsAndMaxScatterDists[16]; // RGB = S = 1 / D, A = d = RgbMax(D)
    PREMACRO(float4) _TransmissionTintsAndFresnel0[16]; // RGB = 1/4 * color, A = fresnel0
    PREMACRO(float4) _WorldScalesAndFilterRadiiAndThicknessRemaps[16]; // X = meters per world unit, Y = filter radius (in mm), Z = remap start, W = end - start
    PREMACRO(uint4) _DiffusionProfileHashTable[16];
    PREMACRO(uint) _EnableSubsurfaceScattering; // Globally toggles subsurface and transmission scattering on/off
    PREMACRO(uint) _TexturingModeFlags;         // 1 bit/profile; 0 = PreAndPostScatter, 1 = PostScatter
    PREMACRO(uint) _TransmissionFlags;          // 1 bit/profile; 0 = regular, 1 = thin
    PREMACRO(uint) _DiffusionProfileCount;
    PREMACRO(uint) _SssSampleBudget;
    PREMACRO(uint3) _padding_0;
};

struct ExposureUniform
{
    float _ProbeExposureScale;
    PREMACRO(uint) _ExposureTextureIndex;
    PREMACRO(uint) __PrevExposureTextureIndex;
    PREMACRO(uint) _padding_0;
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
    VolumeCloudStruct volumeCloudUniform;
    DirectionalLightStruct directionalLight;
    PointLightUniform pointLightUniform;
    SpotLightUniform spotLightUniform;
    VolumeLightUniform volumeLightUniform;
    SSSUniform sssUniform;
    ExposureUniform exposureUniform;
};

// =======================================
// Terrain
// =======================================

struct ClipmapTransform
{
    PREMACRO(float4x4) transform;
    int mesh_type;
};

struct ClipmapMeshCount
{
    PREMACRO(uint) tile_count;
    PREMACRO(uint) filler_count;
    PREMACRO(uint) trim_count;
    PREMACRO(uint) cross_cunt;
    PREMACRO(uint) seam_count;
    PREMACRO(uint) total_count;
};

struct ClipMeshCommandSigParams
{
    PREMACRO(float4)   VertexBuffer; // D3D12_VERTEX_BUFFER_VIEW 16
    PREMACRO(float4)   IndexBuffer; // D3D12_INDEX_BUFFER_VIEW 16
    PREMACRO(float4x2) DrawIndexedArguments; // D3D12_DRAW_INDEXED_ARGUMENTS 20, Emoty 12
    PREMACRO(float4x2) ClipBoundingBox; // BoundingBox 32
};

struct ToDrawCommandSignatureParams
{
    PREMACRO(float4)   VertexBuffer; // D3D12_VERTEX_BUFFER_VIEW 16
    PREMACRO(float4)   IndexBuffer; // D3D12_INDEX_BUFFER_VIEW 16
    PREMACRO(float4x2) DrawIndexedArguments; // D3D12_DRAW_INDEXED_ARGUMENTS 20, ClipIndex 4, Emoty 8
};

struct TerrainPatchNode
{
    PREMACRO(float2) patchMinPos; // node的左下角顶点
    float maxHeight; // 当前node最大高度
    float minHeight; // 当前node最小高度
    float nodeWidth; // patchnode的宽度
    int mipLevel; // 当前node的mip等级
    PREMACRO(uint) neighbor; // 更高一级mip作为邻居的标识
};

// =======================================
// Samplers
// =======================================

struct SamplerStruct
{
    SamplerState SPointClampSampler;
    SamplerState SLinearClampSampler;
    SamplerState SLinearRepeatSampler;
    SamplerState STrilinearClampSampler;
    SamplerState STrilinearRepeatSampler;
    
    SamplerComparisonState SLinearClampCompareSampler;
};



// struct ShaderVariablesGlobal
// {
//     PREMACRO(float4x4) _ViewMatrix;
//     PREMACRO(float4x4) _CameraViewMatrix;
//     PREMACRO(float4x4) _InvViewMatrix;
//     PREMACRO(float4x4) _ProjMatrix;
//     PREMACRO(float4x4) _InvProjMatrix;
//     PREMACRO(float4x4) _ViewProjMatrix;
//     PREMACRO(float4x4) _CameraViewProjMatrix;
//     PREMACRO(float4x4) _InvViewProjMatrix;
//     PREMACRO(float4x4) _NonJitteredViewProjMatrix;
//     PREMACRO(float4x4) _PrevViewProjMatrix;
//     PREMACRO(float4x4) _PrevInvViewProjMatrix;
//     PREMACRO(float4) _WorldSpaceCameraPos_Internal;
//     PREMACRO(float4) _PrevCamPosRWS_Internal;
//     PREMACRO(float4) _ScreenSize;
//     PREMACRO(float4) _PostProcessScreenSize;
//     PREMACRO(float4) _RTHandleScale;
//     PREMACRO(float4) _RTHandleScaleHistory;
//     PREMACRO(float4) _RTHandlePostProcessScale;
//     PREMACRO(float4) _RTHandlePostProcessScaleHistory;
//     PREMACRO(float4) _DynamicResolutionFullscreenScale;
//     PREMACRO(float4) _ZBufferParams;
//     PREMACRO(float4) _ProjectionParams;
//     PREMACRO(float4) unity_OrthoParams;
//     PREMACRO(float4) _ScreenParams;
//     PREMACRO(float4) _FrustumPlanes[6];
//     PREMACRO(float4) _ShadowFrustumPlanes[6];
//     PREMACRO(float4) _TaaFrameInfo;
//     PREMACRO(float4) _TaaJitterStrength;
//     PREMACRO(float4) _Time;
//     PREMACRO(float4) _SinTime;
//     PREMACRO(float4) _CosTime;
//     PREMACRO(float4) unity_DeltaTime;
//     PREMACRO(float4) _TimeParameters;
//     PREMACRO(float4) _LastTimeParameters;
//     int _FogEnabled;
//     int _PBRFogEnabled;
//     int _EnableVolumetricFog;
//     float _MaxFogDistance;
//     PREMACRO(float4) _FogColor;
//     float _FogColorMode;
//     float _GlobalMipBias;
//     float _GlobalMipBiasPow2;
//     float _Pad0;
//     PREMACRO(float4) _MipFogParameters;
//     PREMACRO(float4) _HeightFogBaseScattering;
//     float _HeightFogBaseExtinction;
//     float _HeightFogBaseHeight;
//     float _GlobalFogAnisotropy;
//     int _VolumetricFilteringEnabled;
//     PREMACRO(float2) _HeightFogExponents;
//     int _FogDirectionalOnly;
//     float _FogGIDimmer;
//     PREMACRO(float4) _VBufferViewportSize;
//     PREMACRO(float4) _VBufferLightingViewportScale;
//     PREMACRO(float4) _VBufferLightingViewportLimit;
//     PREMACRO(float4) _VBufferDistanceEncodingParams;
//     PREMACRO(float4) _VBufferDistanceDecodingParams;
//     PREMACRO(uint) _VBufferSliceCount;
//     float _VBufferRcpSliceCount;
//     float _VBufferRcpInstancedViewCount;
//     float _VBufferLastSliceDist;
//     PREMACRO(float4) _ShadowAtlasSize;
//     PREMACRO(float4) _CascadeShadowAtlasSize;
//     PREMACRO(float4) _AreaShadowAtlasSize;
//     PREMACRO(float4) _CachedShadowAtlasSize;
//     PREMACRO(float4) _CachedAreaShadowAtlasSize;
//     int _ReflectionsMode;
//     int _UnusedPadding0;
//     int _UnusedPadding1;
//     int _UnusedPadding2;
//     PREMACRO(uint) _DirectionalLightCount;
//     PREMACRO(uint) _PunctualLightCount;
//     PREMACRO(uint) _AreaLightCount;
//     PREMACRO(uint) _EnvLightCount;
//     int _EnvLightSkyEnabled;
//     PREMACRO(uint) _CascadeShadowCount;
//     int _DirectionalShadowIndex;
//     PREMACRO(uint) _EnableLightLayers;
//     PREMACRO(uint) _EnableSkyReflection;
//     PREMACRO(uint) _EnableSSRefraction;
//     float _SSRefractionInvScreenWeightDistance;
//     float _ColorPyramidLodCount;
//     float _DirectionalTransmissionMultiplier;
//     float _ProbeExposureScale;
//     float _ContactShadowOpacity;
//     float _ReplaceDiffuseForIndirect;
//     PREMACRO(float4) _AmbientOcclusionParam;
//     float _IndirectDiffuseLightingMultiplier;
//     PREMACRO(uint) _IndirectDiffuseLightingLayers;
//     float _ReflectionLightingMultiplier;
//     PREMACRO(uint) _ReflectionLightingLayers;
//     float _MicroShadowOpacity;
//     PREMACRO(uint) _EnableProbeVolumes;
//     PREMACRO(uint) _ProbeVolumeCount;
//     float _SlopeScaleDepthBias;
//     PREMACRO(float4) _CookieAtlasSize;
//     PREMACRO(float4) _CookieAtlasData;
//     PREMACRO(float4) _ReflectionAtlasCubeData;
//     PREMACRO(float4) _ReflectionAtlasPlanarData;
//     PREMACRO(uint) _NumTileFtplX;
//     PREMACRO(uint) _NumTileFtplY;
//     float g_fClustScale;
//     float g_fClustBase;
//     float g_fNearPlane;
//     float g_fFarPlane;
//     int g_iLog2NumClusters;
//     PREMACRO(uint) g_isLogBaseBufferEnabled;
//     PREMACRO(uint) _NumTileClusteredX;
//     PREMACRO(uint) _NumTileClusteredY;
//     int _EnvSliceSize;
//     PREMACRO(uint) _EnableDecalLayers;
//     PREMACRO(float4) _ShapeParamsAndMaxScatterDists[16];
//     PREMACRO(float4) _TransmissionTintsAndFresnel0[16];
//     PREMACRO(float4) _WorldScalesAndFilterRadiiAndThicknessRemaps[16];
//     PREMACRO(uint4) _DiffusionProfileHashTable[16];
//     PREMACRO(uint) _EnableSubsurfaceScattering;
//     PREMACRO(uint) _TexturingModeFlags;
//     PREMACRO(uint) _TransmissionFlags;
//     PREMACRO(uint) _DiffusionProfileCount;
//     PREMACRO(float2) _DecalAtlasResolution;
//     PREMACRO(uint) _EnableDecals;
//     PREMACRO(uint) _DecalCount;
//     float _OffScreenDownsampleFactor;
//     PREMACRO(uint) _OffScreenRendering;
//     PREMACRO(uint) _XRViewCount;
//     int _FrameCount;
//     PREMACRO(float4) _CoarseStencilBufferSize;
//     int _IndirectDiffuseMode;
//     int _EnableRayTracedReflections;
//     int _RaytracingFrameIndex;
//     PREMACRO(uint) _EnableRecursiveRayTracing;
//     int _TransparentCameraOnlyMotionVectors;
//     float _GlobalTessellationFactorMultiplier;
//     float _SpecularOcclusionBlend;
//     float _DeExposureMultiplier;
//     PREMACRO(float4) _ScreenSizeOverride;
//     PREMACRO(float4) _ScreenCoordScaleBias;
//     PREMACRO(float4) _ColorPyramidUvScaleAndLimitPrevFrame;
// };
//

#endif // UNITY_SHADER_VARIABLES_GLOBAL_INCLUDED
