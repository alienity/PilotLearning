#ifndef UNITY_BUILTIN_DATA_INCLUDED
#define UNITY_BUILTIN_DATA_INCLUDED

//-----------------------------------------------------------------------------
// BuiltinData
// This structure include common data that should be present in all material
// and are independent from the BSDF parametrization.
// Note: These parameters can be store in GBuffer if the writer wants
//-----------------------------------------------------------------------------

//
// UnityEngine.Rendering.HighDefinition.Builtin+BuiltinData:  static fields
//
#define DEBUGVIEW_BUILTIN_BUILTINDATA_OPACITY (100)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_ALPHA_CLIP_TRESHOLD (101)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_BAKED_DIFFUSE_LIGHTING (102)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_BACK_BAKED_DIFFUSE_LIGHTING (103)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_SHADOWMASK_0 (104)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_SHADOWMASK_1 (105)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_SHADOWMASK_2 (106)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_SHADOWMASK_3 (107)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_EMISSIVE_COLOR (108)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_MOTION_VECTOR (109)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_DISTORTION (110)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_DISTORTION_BLUR (111)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_IS_LIGHTMAP (112)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_RENDERING_LAYERS (113)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_DEPTH_OFFSET (114)
#define DEBUGVIEW_BUILTIN_BUILTINDATA_VT_PACKED_FEEDBACK (115)

// Generated from UnityEngine.Rendering.HighDefinition.Builtin+BuiltinData
// PackingRules = Exact
struct BuiltinData
{
    float opacity;
    float alphaClipTreshold;
    float3 bakeDiffuseLighting;
    float3 backBakeDiffuseLighting;
    float shadowMask0;
    float shadowMask1;
    float shadowMask2;
    float shadowMask3;
    float3 emissiveColor;
    float2 motionVector;
    float2 distortion;
    float distortionBlur;
    uint renderingLayers;
    float depthOffset;
};

// Generated from UnityEngine.Rendering.HighDefinition.Builtin+LightTransportData
// PackingRules = Exact
struct LightTransportData
{
    float3 diffuseColor;
    float3 emissiveColor;
};

//-----------------------------------------------------------------------------
// Modification Options
//-----------------------------------------------------------------------------
// Due to various transform and conversions that happen, some precision is lost along the way.
// as a result, motion vectors that are close to 0 due to cancellation of components (camera and object) end up not doing so.
// To workaround the issue, if the computed motion vector is less than MICRO_MOVEMENT_THRESHOLD (now 1% of a pixel)
// if  KILL_MICRO_MOVEMENT is == 1, we set the motion vector to 0 instead.
// An alternative could be rounding the motion vectors (e.g. round(motionVec.xy * 1eX) / 1eX) with X varying on how many digits)
// but that might lead to artifacts with mismatch between actual motion and written motion vectors on non trivial motion vector lengths.
#define KILL_MICRO_MOVEMENT
#define MICRO_MOVEMENT_THRESHOLD(_ScreenSize) (0.01f * _ScreenSize.zw)

//-----------------------------------------------------------------------------
// helper macro
//-----------------------------------------------------------------------------

#define BUILTIN_DATA_SHADOW_MASK                float4(builtinData.shadowMask0, builtinData.shadowMask1, builtinData.shadowMask2, builtinData.shadowMask3)
#define ZERO_BUILTIN_INITIALIZE(builtinData)    ZERO_INITIALIZE(BuiltinData, builtinData)

//-----------------------------------------------------------------------------
// common Encode/Decode functions
//-----------------------------------------------------------------------------

// Guideline for motion vectors buffer.
// The object motion vectors buffer is potentially fill in several pass.
// - In gbuffer pass with extra RT (Not supported currently)
// - In forward prepass pass
// - In dedicated motion vectors pass
// So same motion vectors buffer is use for all scenario, so if deferred define a motion vectors buffer, the same is reuse for forward case.
// THis is similar to NormalBuffer

// EncodeMotionVector / DecodeMotionVector code for now, i.e it must do nothing like it is doing currently.
// Design note: We assume that motion vector/distortion fit into a single buffer (i.e not spread on several buffer)
void EncodeMotionVector(float2 motionVector, out float4 outBuffer)
{
    // RT - 16:16 float
    outBuffer = float4(motionVector.xy, 0.0, 0.0);
}

bool PixelSetAsNoMotionVectors(float4 inBuffer)
{
    return inBuffer.x > 1.0f;
}

void DecodeMotionVector(float4 inBuffer, out float2 motionVector)
{
    motionVector = PixelSetAsNoMotionVectors(inBuffer) ? 0.0f : inBuffer.xy;
}

void EncodeDistortion(float2 distortion, float distortionBlur, bool isValidSource, out float4 outBuffer)
{
    // RT - 16:16:16:16 float
    // distortionBlur in alpha for a different blend mode
    outBuffer = float4(distortion, isValidSource, distortionBlur); // Caution: Blend mode depends on order of attribut here, can't change without updating blend mode.
}

void DecodeDistortion(float4 inBuffer, out float2 distortion, out float distortionBlur, out bool isValidSource)
{
    distortion = inBuffer.xy;
    distortionBlur = inBuffer.a;
    isValidSource = (inBuffer.z != 0.0);
}

#endif // UNITY_BUILTIN_DATA_INCLUDED
