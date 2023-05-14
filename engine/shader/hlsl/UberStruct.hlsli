#pragma once

/*
 * Here we define all the UBOs known by filament as C structs. It is used by filament to
 * fill the uniform values and get the interface block names. It is also used by filabridge to
 * get the interface block names.
 * 
 * 在c++测也要定义跟这里一样的结构体用于对Shading的统一输入
 */

struct FrameUniforms
{
    // --------------------------------------------------------------------------------------------
    // Values that can be accessed in both surface and post-process materials
    // --------------------------------------------------------------------------------------------

    float4x4 viewFromWorldMatrix;      // clip    view <- world    : view matrix
    float4x4 worldFromViewMatrix;      // clip    view -> world    : model matrix
    float4x4 clipFromViewMatrix;       // clip <- view    world    : projection matrix
    float4x4 viewFromClipMatrix;       // clip -> view    world    : inverse projection matrix
    float4x4 clipFromWorldMatrix;      // clip <- view <- world
    float4x4 worldFromClipMatrix;      // clip -> view -> world
    float4x4 userWorldFromWorldMatrix; // userWorld <- world
    float4   clipTransform;            // [sx, sy, tx, ty] only used by VERTEX_DOMAIN_DEVICE

    float2 clipControl;   // clip control
    float  time;          // time in seconds, with a 1-second period
    float  temporalNoise; // noise [0,1] when TAA is used, 0 otherwise
    float4 userTime;      // time(s), (double)time - (float)time, 0, 0

    // --------------------------------------------------------------------------------------------
    // values below should only be accessed in surface materials
    // (i.e.: not in the post-processing materials)
    // --------------------------------------------------------------------------------------------

    float4 resolution;            // physical viewport width, height, 1/width, 1/height
    float2 logicalViewportScale;  // scale-factor to go from physical to logical viewport
    float2 logicalViewportOffset; // offset to go from physical to logical viewport

    float lodBias; // load bias to apply to user materials
    float refractionLodOffset;

    // camera position in view space (when camera_at_origin is enabled), i.e. it's (0,0,0).
    float oneOverFarMinusNear;  // 1 / (f-n), always positive
    float nearOverFarMinusNear; // n / (f-n), always positive
    float cameraFar;            // camera *culling* far-plane distance, always positive (projection far is at +inf)
    float exposure;
    float ev100;
    float needsAlphaChannel;

    // AO
    float aoSamplingQualityAndEdgeDistance; // <0: no AO, 0: bilinear, !0: bilateral edge distance
    float aoBentNormals;                    // 0: no AO bent normal, >0.0 AO bent normals
    float aoReserved0;
    float aoReserved1;

    // --------------------------------------------------------------------------------------------
    // Dynamic Lighting [variant: DYN]
    // --------------------------------------------------------------------------------------------
    float4 zParams;       // froxel Z parameters
    uint3  fParams;       // stride-x, stride-y, stride-z
    uint   lightChannels; // light channel bits
    float2 froxelCountXY;

    // IBL
    float  iblLuminance;
    float  iblRoughnessOneLevel; // level for roughness == 1
    float4 iblSH[9];             // actually float3 entries (std140 requires float4 alignment)

    // --------------------------------------------------------------------------------------------
    // Directional Lighting [variant: DIR]
    // --------------------------------------------------------------------------------------------
    float3 lightDirection;            // directional light direction
    float  padding0;
    float4 lightColorIntensity;       // directional light
    float4 sun;                       // cos(sunAngle), sin(sunAngle), 1/(sunAngle*HALO_SIZE-sunAngle), HALO_EXP
    float2 lightFarAttenuationParams; // a, a/far (a=1/pct-of-far)

    // --------------------------------------------------------------------------------------------
    // Directional light shadowing [variant: SRE | DIR]
    // --------------------------------------------------------------------------------------------
    // bit 0: directional (sun) shadow enabled
    // bit 1: directional (sun) screen-space contact shadow enabled
    // bit 8-15: screen-space contact shadows ray casting steps
    uint  directionalShadows;
    float ssContactShadowDistance;

    // position of cascade splits, in world space (not including the near plane)
    // -Inf stored in unused components
    float4 cascadeSplits;
    // bit 0-3: cascade count
    // bit 8-11: cascade has visible shadows
    uint  cascades;
    float reserved0;
    float reserved1;                // normal bias
    float shadowPenumbraRatioScale; // For DPCF or PCSS, scale penumbra ratio for artistic use

    // --------------------------------------------------------------------------------------------
    // VSM shadows [variant: VSM]
    // --------------------------------------------------------------------------------------------
    float vsmExponent;
    float vsmDepthScale;
    float vsmLightBleedReduction;
    uint  shadowSamplingType; // 0: vsm, 1: dpcf

    // --------------------------------------------------------------------------------------------
    // Fog [variant: FOG]
    // --------------------------------------------------------------------------------------------
    float3 fogDensity; // { density, -falloff * yc, density * exp(-fallof * yc) }
    float  fogStart;
    float  fogMaxOpacity;
    float  fogHeight;
    float  fogHeightFalloff;
    float  fogCutOffDistance;
    float3 fogColor;
    float  fogColorFromIbl;
    float  fogInscatteringStart;
    float  fogInscatteringSize;
    float  fogReserved1;
    float  fogReserved2;

    // --------------------------------------------------------------------------------------------
    // Screen-space reflections [variant: SSR (i.e.: VSM | SRE)]
    // --------------------------------------------------------------------------------------------
    float4x4 ssrReprojection;
    float4x4 ssrUvFromViewMatrix;
    float    ssrThickness; // ssr thickness, in world units
    float    ssrBias;      // ssr bias, in world units
    float    ssrDistance;  // ssr world raycast distance, 0 when ssr is off
    float    ssrStride;    // ssr texel stride, >= 1.0

    // bring PerViewUib to 2 KiB
    float4 reserved[60];
};

// static_assert(sizeof(PerViewUib) == sizeof(math::float4) * 128, "PerViewUib should be exactly 2KiB");

struct ObjectUniforms
{
    float4x4 worldFromModelMatrix;
    float3x3 worldFromModelNormalMatrix;
    float3   reserved0;
    uint     morphTargetCount;
    uint     flagsChannels; // see packFlags() below (0x00000fll)
    uint     objectId;      // used for picking
    // TODO: We need a better solution, this currently holds the average local scale for the renderable
    float userData;

    float4 reserved[8];
};

// static_assert(sizeof(PerRenderableUib) <= CONFIG_MINSPEC_UBO_SIZE, "PerRenderableUib exceeds max UBO size");

struct LightsUniforms
{
    float4 positionFalloff; // { float3(pos), 1/falloff^2 }
    float3 direction;       // dir
    float  reserved1;       // 0
    half4  colorIES;        // { half3(col),  IES index   }
    float2 spotScaleOffset; // { scale, offset }
    float  reserved3;       // 0
    float  intensity;       // float
    uint   typeShadow;      // 0x00.00.ii.ct (t: 0=point, 1=spot, c:contact, ii: index)
    uint   channels;        // 0x000c00ll (ll: light channels, c: caster)
};

// static_assert(sizeof(LightsUib) == 64, "the actual UBO is an array of 256 mat4");

#define CONFIG_MAX_SHADOWMAPS 64
#define CONFIG_MAX_BONE_COUNT 256
#define CONFIG_MAX_MORPH_TARGET_COUNT 256

struct ShadowData
{
    float4x4 lightFromWorldMatrix; // 64
    float4   lightFromWorldZ;      // 16
    float4   scissorNormalized;    // 16
    float    texelSizeAtOneMeter;  //  4
    float    bulbRadiusLs;         //  4
    float    nearOverFarMinusNear; //  4
    float    normalBias;           //  4
    bool     elvsm;                //  4
    uint     layer;                //  4
    uint     reserved1;            //  4
    uint     reserved2;            //  4
};

struct ShadowUniforms
{
    ShadowData shadows[CONFIG_MAX_SHADOWMAPS];
};

// static_assert(sizeof(ShadowUib) <= CONFIG_MINSPEC_UBO_SIZE, "ShadowUib exceeds max UBO size");

struct FroxelRecordUniforms
{
    uint4 records[1024];
};

// static_assert(sizeof(FroxelRecordUib) == 16384, "FroxelRecordUib should be exactly 16KiB");

struct BoneData
{
    // bone transform, last row assumed [0,0,0,1]
    float4 transform[3];
    // 8 first cofactor matrix of transform's upper left
    uint4 cof;
};

struct BonesUniforms
{
    BoneData bones[CONFIG_MAX_BONE_COUNT];
};

// static_assert(sizeof(PerRenderableBoneUib) <= CONFIG_MINSPEC_UBO_SIZE, "PerRenderableUibBone exceeds max UBO size");

struct MorphingUniforms
{
    // The array stride(the bytes between array elements) is always rounded up to the size of a vec4 in std140.
    float4 weights[CONFIG_MAX_MORPH_TARGET_COUNT];
};

// static_assert(sizeof(PerRenderableMorphingUib) <= CONFIG_MINSPEC_UBO_SIZE, "PerRenderableMorphingUib exceeds max UBO size");





struct MaterialParams
{

};



















