#ifndef LIGHTDEFINITION_CS_HLSL
#define LIGHTDEFINITION_CS_HLSL
//
// UnityEngine.Rendering.HighDefinition.EnvCacheType:  static fields
//
#define ENVCACHETYPE_TEXTURE2D (0)
#define ENVCACHETYPE_CUBEMAP (1)

//
// UnityEngine.Rendering.HighDefinition.EnvLightReflectionDataRT:  static fields
//
#define MAX_PLANAR_REFLECTIONS (16)
#define MAX_CUBE_REFLECTIONS (64)

//
// UnityEngine.Rendering.HighDefinition.EnvShapeType:  static fields
//
#define ENVSHAPETYPE_NONE (0)
#define ENVSHAPETYPE_BOX (1)
#define ENVSHAPETYPE_SPHERE (2)
#define ENVSHAPETYPE_SKY (3)

//
// UnityEngine.Rendering.HighDefinition.EnvConstants:  static fields
//
#define ENVCONSTANTS_CONVOLUTION_MIP_COUNT (7)

//
// UnityEngine.Rendering.HighDefinition.GPUImageBasedLightingType:  static fields
//
#define GPUIMAGEBASEDLIGHTINGTYPE_REFLECTION (0)
#define GPUIMAGEBASEDLIGHTINGTYPE_REFRACTION (1)

//
// UnityEngine.Rendering.HighDefinition.CookieMode:  static fields
//
#define COOKIEMODE_NONE (0)
#define COOKIEMODE_CLAMP (1)
#define COOKIEMODE_REPEAT (2)

//
// UnityEngine.Rendering.HighDefinition.EnvLightReflectionData:  static fields
//
#define MAX_PLANAR_REFLECTIONS (16)
#define MAX_CUBE_REFLECTIONS (64)

//
// UnityEngine.Rendering.HighDefinition.GPULightType:  static fields
//
#define GPULIGHTTYPE_DIRECTIONAL (0)
#define GPULIGHTTYPE_POINT (1)
#define GPULIGHTTYPE_SPOT (2)
#define GPULIGHTTYPE_PROJECTOR_PYRAMID (3)
#define GPULIGHTTYPE_PROJECTOR_BOX (4)
#define GPULIGHTTYPE_TUBE (5)
#define GPULIGHTTYPE_RECTANGLE (6)
#define GPULIGHTTYPE_DISC (7)

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
#endif


// Generated from UnityEngine.Rendering.HighDefinition.EnvLightData
// PackingRules = Exact
struct EnvLightData
{
    uint lightLayers;
    float3 capturePositionRWS;
    int influenceShapeType;
    float3 proxyExtents;
    float minProjectionDistance;
    float3 proxyPositionRWS;
    float3 proxyForward;
    float3 proxyUp;
    float3 proxyRight;
    float3 influencePositionRWS;
    float3 influenceForward;
    float3 influenceUp;
    float3 influenceRight;
    float3 influenceExtents;
    float3 blendDistancePositive;
    float3 blendDistanceNegative;
    float3 blendNormalDistancePositive;
    float3 blendNormalDistanceNegative;
    float3 boxSideFadePositive;
    float3 boxSideFadeNegative;
    float weight;
    float multiplier;
    float rangeCompressionFactorCompensation;
    float roughReflections;
    float distanceBasedRoughness;
    int envIndex;
    float4 L0L1;
    float4 L2_1;
    float L2_2;
    int normalizeWithAPV;
    float2 padding;
};

// Generated from UnityEngine.Rendering.HighDefinition.LightData
// PackingRules = Exact
struct LightData
{
    float3 positionRWS;
    uint lightLayers;
    float lightDimmer;
    float volumetricLightDimmer;
    float angleScale;
    float angleOffset;
    float3 forward;
    float iesCut;
    int lightType;
    float3 right;
    float penumbraTint;
    float range;
    int shadowIndex;
    float3 up;
    float rangeAttenuationScale;
    float3 color;
    float rangeAttenuationBias;
    float3 shadowTint;
    float shadowDimmer;
    float volumetricShadowDimmer;
    int nonLightMappedOnly;
    float minRoughness;
    int screenSpaceShadowIndex;
    float4 shadowMaskSelector;
    float4 size;
    int contactShadowMask;
    float diffuseDimmer;
    float specularDimmer;
    float __unused__;
    float2 padding;
    float isRayTracedContactShadow;
    float boxLightSafeExtent;
};

// Generated from UnityEngine.Rendering.HighDefinition.EnvLightReflectionData
// PackingRules = Exact
struct EnvLightReflectionData
{
    float4x4 _PlanarCaptureVP[16];
    float4 _PlanarScaleOffset[16];
    float4 _CubeScaleOffset[64];
};

// Generated from UnityEngine.Rendering.HighDefinition.DirectionalLightData
// PackingRules = Exact
// Make sure to respect the 16-byte alignment
struct DirectionalLightData
{
    float3 positionRWS;
    uint lightLayers;
    
    float lightDimmer;
    float volumetricLightDimmer; // Replaces 'lightDimmer'
    float3 forward;
    float3 right; // Rescaled by (2 / shapeWidth)
    
    int shadowIndex; // -1 if unused (TODO: 16 bit)
    float3 up; // Rescaled by (2 / shapeHeight)
    
    int contactShadowIndex; // -1 if unused (TODO: 16 bit)
    float3 color;
    
    int contactShadowMask; // 0 if unused (TODO: 16 bit)
    float3 shadowTint; // Use to tint shadow color
    
    float shadowDimmer;
    float volumetricShadowDimmer; // Replaces 'shadowDimmer'
    int nonLightMappedOnly; // Used with ShadowMask (TODO: use a bitfield)
    float minRoughness; // Hack
    
    int screenSpaceShadowIndex; // -1 if unused (TODO: 16 bit)
    float3 __unused__;
    
    float4 shadowMaskSelector; // Used with ShadowMask feature
    
    float diffuseDimmer;
    float specularDimmer;
    float penumbraTint;
    float isRayTracedContactShadow;
    
    float distanceFromCamera; // -1 -> no sky interaction
    float angularDiameter; // Units: radians
    float flareFalloff;
    float flareCosInner;
    
    float flareCosOuter;
    float3 flareTint;
    
    float flareSize; // Units: radians
    float3 surfaceTint;
    
    float4 surfaceTextureScaleOffset; // -1 if unused (TODO: 16 bit)
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
