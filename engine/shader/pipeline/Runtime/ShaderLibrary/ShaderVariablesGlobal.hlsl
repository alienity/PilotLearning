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
    float4x4 objectToWorldMatrix;
    float4x4 worldToObjectMatrix;
    float4x4 prevObjectToWorldMatrix;
    float4x4 prevWorldToObjectMatrix;
    float4   vertexBufferView; // D3D12_VERTEX_BUFFER_VIEW 16
    float4   indexBufferView; // D3D12_INDEX_BUFFER_VIEW 16
    float2x4 drawIndexedArguments; // D3D12_DRAW_INDEXED_ARGUMENTS 20, LightPropertyBufferIndex 4, LightPropertyBufferIndexOffset 4, Empty 4
    float2x4 rendererBounds; // BoundingBox 32
};

struct CameraDataBuffer
{
    float4x4 viewFromWorldMatrix; // clip    view <- world    : view matrix
    float4x4 worldFromViewMatrix; // clip    view -> world    : model matrix
    float4x4 clipFromViewMatrix;  // clip <- view    world    : projection matrix
    float4x4 viewFromClipMatrix;  // clip -> view    world    : inverse projection matrix
    float4x4 unJitterProjectionMatrix; // Unjitter projection matrix
    float4x4 unJitterProjectionMatrixInv; // Unjitter projection matrix inverse
    float4x4 clipFromWorldMatrix; // clip <- view <- world
    float4x4 worldFromClipMatrix; // clip -> view -> world
    float4  clipTransform;       // [sx, sy, tx, ty] only used by VERTEX_DOMAIN_DEVICE
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
};

struct CameraUniform
{
    CameraDataBuffer _CurFrameUniform;
    CameraDataBuffer _PrevFrameUniform;
    float4 _Resolution; // physical viewport width, height, 1/width, 1/height
    float2 _LogicalViewportScale; // scale-factor to go from physical to logical viewport
    float2 _LogicalViewportOffset; // offset to go from physical to logical viewport
    // camera position in view space (when camera_at_origin is enabled), i.e. it's (0,0,0).
    float4 _ZBufferParams;
    float _CameraNear;
    float _CameraFar; // camera *culling* far-plane distance, always positive (projection far is at +inf)
    float2 _Padding0;
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
    float baseReserved0;
    
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
    float aoSamplingQualityAndEdgeDistance; // <0: no AO, 0: bilinear, !0: bilateral edge distance
    float aoBentNormals; // 0: no AO bent normal, >0.0 AO bent normals
    float aoReserved0;
    float aoReserved1;
};

struct TAAUniform
{
    float4 projectionExtents;  // xy = frustum extents at distance 1, zw = jitter at distance 1
    float4 jitterUV;
    float feedbackMin;
    float feedbackMax;
    float motionScale;
    float __Reserved0;
};

struct IBLUniform
{
    float4 iblSH[7]; // actually float3 entries (std140 requires float4 alignment)
    float iblLuminance;
    float iblRoughnessOneLevel; // level for roughness == 1
    int   dfg_lut_srv_index;    // specular lut dfg
    int   ld_lut_srv_index;     // specular lut ld
    int   radians_srv_index;    // diffuse
    float3 __Reserved0;
};

struct SSRUniform
{
    float4 ScreenSize;
    float4 ResolveSize;
    float4 RayCastSize;
    float4 JitterSizeAndOffset;
    float4 NoiseSize;
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
    float4x4 cloud_shadow_view_matrix;
    float4x4 cloud_shadow_proj_matrix;
    float4x4 cloud_shadow_proj_view_matrix;
    uint cloud_shadowmap_srv_index;
    uint cloud_shadowmap_size;
    int2 cloud_shadowmap_bounds;
    float3 sunlight_direction;
    float sun_to_earth_distance;
};

struct DirectionalLightShadowmap
{
    uint shadowmap_srv_index; // shadowmap srv in descriptorheap index    
    uint cascadeCount; // how many cascade level, default 4
    uint2 shadowmap_size; // shadowmap size
    uint4 shadow_bounds; // shadow bounds for each cascade level
    float4x4 light_view_matrix; // direction light view matrix
    float4x4 light_proj_matrix[4];
    float4x4 light_proj_view[4];
};

struct DirectionalLightStruct
{
    float4 lightColorIntensity; // directional light rgb - color, a - intensity
    float3 lightPosition; // directional light position
    float lightRadius; // sun radius
    float3 lightDirection; // directional light direction
    uint shadowType; // 1 use shadowmap
    float2 lightFarAttenuationParams; // a, a/far (a=1/pct-of-far)
    float2 _padding_0;
    
    DirectionalLightShadowmap directionalLightShadowmap;
};

struct PointLightShadowmap
{
    uint shadowmap_srv_index; // shadowmap srv in descriptorheap index
    uint2 shadowmap_size;
    float _padding_shadowmap;
    float4x4 light_proj_view[6];
};

struct PointLightStruct
{
    float3 lightPosition;
    float lightRadius;
    float4 lightIntensity; // point light rgb - color, a - intensity
    uint shadowType;
    float falloff; // default 1.0f
    uint2 _padding_0;

    PointLightShadowmap pointLightShadowmap;
};

struct PointLightUniform
{
    uint pointLightCounts;
    float3 _padding_0;
    PointLightStruct pointLightStructs[RENDERING_MAX_POINT_LIGHT_COUNT];
};

struct SpotLightShadowmap
{
    uint shadowmap_srv_index; // shadowmap srv in descriptorheap index
    uint2 shadowmap_size;
    float _padding_0;
    float4x4 light_proj_view;
};

struct SpotLightStruct
{
    float3 lightPosition;
    float lightRadius;
    float4 lightIntensity; // spot light rgb - color, a - intensity
    float3 lightDirection;
    float inner_radians;
    float outer_radians;
    uint shadowType;
    float falloff; // default 1.0f
    float _padding_0;

    SpotLightShadowmap spotLightShadowmap;
};

struct SpotLightUniform
{
    uint spotLightCounts;
    float3 _padding_0;
    SpotLightStruct spotLightStructs[RENDERING_MAX_SPOT_LIGHT_COUNT];
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
    float4   VertexBuffer; // D3D12_VERTEX_BUFFER_VIEW 16
    float4   IndexBuffer; // D3D12_INDEX_BUFFER_VIEW 16
    float2x4 DrawIndexedArguments; // D3D12_DRAW_INDEXED_ARGUMENTS 20, Empty 12
    float2x4 ClipBoundingBox; // BoundingBox 32
};

struct ToDrawCommandSignatureParams
{
    float4   VertexBuffer; // D3D12_VERTEX_BUFFER_VIEW 16
    float4   IndexBuffer; // D3D12_INDEX_BUFFER_VIEW 16
    float2x4 DrawIndexedArguments; // D3D12_DRAW_INDEXED_ARGUMENTS 20, ClipIndex 4, Empty 8
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


D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(RenderDataPerDraw renderDataPerDraw)
{
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = (D3D12_VERTEX_BUFFER_VIEW) 0;
    vertexBufferView.BufferLocation = asuint(renderDataPerDraw.vertexBufferView.xy);
    vertexBufferView.SizeInBytes = asuint(renderDataPerDraw.vertexBufferView.z);
    vertexBufferView.StrideInBytes = asuint(renderDataPerDraw.vertexBufferView.w);
    return vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(RenderDataPerDraw renderDataPerDraw)
{
    D3D12_INDEX_BUFFER_VIEW indexBufferView = (D3D12_INDEX_BUFFER_VIEW) 0;
    indexBufferView.BufferLocation = asuint(renderDataPerDraw.indexBufferView.xy);
    indexBufferView.SizeInBytes = asuint(renderDataPerDraw.indexBufferView.z);
    indexBufferView.Format = asuint(renderDataPerDraw.indexBufferView.w);
    return indexBufferView;
}

D3D12_DRAW_INDEXED_ARGUMENTS GetDrawIndexedArguments(RenderDataPerDraw renderDataPerDraw)
{
    D3D12_DRAW_INDEXED_ARGUMENTS drawIndexArguments = (D3D12_DRAW_INDEXED_ARGUMENTS) 0;
    drawIndexArguments.IndexCountPerInstance = asuint(renderDataPerDraw.drawIndexedArguments[0][0]);
    drawIndexArguments.InstanceCount = asuint(renderDataPerDraw.drawIndexedArguments[0][1]);
    drawIndexArguments.StartIndexLocation = asuint(renderDataPerDraw.drawIndexedArguments[0][2]);
    drawIndexArguments.BaseVertexLocation = asint(renderDataPerDraw.drawIndexedArguments[0][3]);
    drawIndexArguments.StartInstanceLocation = asuint(renderDataPerDraw.drawIndexedArguments[1][0]);
    return drawIndexArguments;
}

uint GetLightPropertyBufferIndex(RenderDataPerDraw renderDataPerDraw)
{
    uint lightPropertyBufferIndex = asuint(renderDataPerDraw.drawIndexedArguments[1][1]);
    return lightPropertyBufferIndex;
}

uint GetLightPropertyBufferIndexOffset(RenderDataPerDraw renderDataPerDraw)
{
    uint lightPropertyBufferIndexOffset = asuint(renderDataPerDraw.drawIndexedArguments[1][2]);
    return lightPropertyBufferIndexOffset;
}

BoundingBox GetRendererBounds(RenderDataPerDraw renderDataPerDraw)
{
    BoundingBox boundingBox = (BoundingBox) 0;
    boundingBox.Center = renderDataPerDraw.rendererBounds[0];
    boundingBox.Extents = renderDataPerDraw.rendererBounds[1];
    return boundingBox;
}

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
