// UNITY_SHADER_NO_UPGRADE

#ifndef UNITY_SHADER_VARIABLES_INCLUDED
#define UNITY_SHADER_VARIABLES_INCLUDED

// This must be included first before we declare any global constant buffer and will onyl affect ray tracing shaders
#include "../ShaderLibrary/ShaderVariablesGlobal.hlsl"

// CAUTION:
// Currently the shaders compiler always include regualr Unity shaderVariables, so I get a conflict here were UNITY_SHADER_VARIABLES_INCLUDED is already define, this need to be fixed.
// As I haven't change the variables name yet, I simply don't define anything, and I put the transform function at the end of the file outside the guard header.
// This need to be fixed.

// Define the type for shadow (either colored shadow or monochrome shadow)
#if SHADEROPTIONS_COLORED_SHADOW
#define SHADOW_TYPE float3
#define SHADOW_TYPE_SWIZZLE xyz
#define SHADOW_TYPE_REPLICATE xxx
#else
#define SHADOW_TYPE float
#define SHADOW_TYPE_SWIZZLE x
#define SHADOW_TYPE_REPLICATE x
#endif

// ----------------------------------------------------------------------------

struct UnityPerDraw
{
    float4x4 unity_ObjectToWorld;
    float4x4 unity_WorldToObject;
    float4 unity_LODFade; // x is the fade value ranging within [0,1]. y is x quantized into 16 levels
    float4 unity_WorldTransformParams; // w is usually 1.0, or -1.0 for odd-negative scale transforms
    float4 unity_RenderingLayer;

    float4 unity_LightmapST;
    float4 unity_DynamicLightmapST;

    // SH lighting environment
    float4 unity_SHAr;
    float4 unity_SHAg;
    float4 unity_SHAb;
    float4 unity_SHBr;
    float4 unity_SHBg;
    float4 unity_SHBb;
    float4 unity_SHC;

    // Renderer bounding box.
    float4 unity_RendererBounds_Min;
    float4 unity_RendererBounds_Max;

    // x = Disabled(0)/Enabled(1)
    // y = Computation are done in global space(0) or local space(1)
    // z = Texel size on U texture coordinate
    float4 unity_ProbeVolumeParams;
    float4x4 unity_ProbeVolumeWorldToObject;
    float4 unity_ProbeVolumeSizeInv; // Note: This variable is float4 and not float3 (compare to builtin unity) to be compatible with SRP batcher
    float4 unity_ProbeVolumeMin; // Note: This variable is float4 and not float3 (compare to builtin unity) to be compatible with SRP batcher

    // This contain occlusion factor from 0 to 1 for dynamic objects (no SH here)
    float4 unity_ProbesOcclusion;

    // Velocity
    float4x4 unity_MatrixPreviousM;
    float4x4 unity_MatrixPreviousMI;
    //X : Use last frame positions (right now skinned meshes are the only objects that use this
    //Y : Force No Motion
    //Z : Z bias value
    //W : Camera only
    float4 unity_MotionVectorsParams;

    int _CameraDepthTextureIndex;
    // Color pyramid (width, height, lodcount, Unused)
    int _ColorPyramidTextureIndex;
    // Custom pass buffer
    int _CustomDepthTextureIndex;
    int _CustomColorTextureIndex;
    // Main lightmap
    int unity_LightmapIndex;
    int unity_LightmapsIndex;
    
    // Dual or directional lightmap (always used with unity_Lightmap, so can share sampler)
    int unity_LightmapIndIndex;
    int unity_LightmapsIndIndex;
    
    // Dynamic GI lightmap
    int unity_DynamicLightmapIndex;
    int unity_DynamicDirectionalityIndex;
    
    int unity_ShadowMaskIndex;
    int unity_ShadowMasksIndex;
    
    // TODO: Change code here so probe volume use only one transform instead of all this parameters!
    int unity_ProbeVolumeSHIndex;
    
    // Exposure texture - 1x1 RG16F (r: exposure mult, g: exposure EV100)
    int _ExposureTextureIndex;
    int _PrevExposureTextureIndex;
};

// ----------------------------------------------------------------------------

//// These are the samplers available in the HDRenderPipeline.
//// Avoid declaring extra samplers as they are 4x SGPR each on GCN.
//SAMPLER(s_point_clamp_sampler);
//SAMPLER(s_linear_clamp_sampler);
//SAMPLER(s_linear_repeat_sampler);
//SAMPLER(s_trilinear_clamp_sampler);
//SAMPLER(s_trilinear_repeat_sampler);
//SAMPLER_CMP(s_linear_clamp_compare_sampler);

// ----------------------------------------------------------------------------
/*
TEXTURE2D_X(_CameraDepthTexture);
SAMPLER(sampler_CameraDepthTexture);

// Color pyramid (width, height, lodcount, Unused)
TEXTURE2D_X(_ColorPyramidTexture);

// Custom pass buffer
TEXTURE2D_X(_CustomDepthTexture);
TEXTURE2D_X(_CustomColorTexture);

// Main lightmap
TEXTURE2D(unity_Lightmap);
SAMPLER(samplerunity_Lightmap);
TEXTURE2D_ARRAY(unity_Lightmaps);
SAMPLER(samplerunity_Lightmaps);

// Dual or directional lightmap (always used with unity_Lightmap, so can share sampler)
TEXTURE2D(unity_LightmapInd);
TEXTURE2D_ARRAY(unity_LightmapsInd);

// Dynamic GI lightmap
TEXTURE2D(unity_DynamicLightmap);
SAMPLER(samplerunity_DynamicLightmap);

TEXTURE2D(unity_DynamicDirectionality);

TEXTURE2D(unity_ShadowMask);
SAMPLER(samplerunity_ShadowMask);
TEXTURE2D_ARRAY(unity_ShadowMasks);
SAMPLER(samplerunity_ShadowMasks);

// TODO: Change code here so probe volume use only one transform instead of all this parameters!
TEXTURE3D(unity_ProbeVolumeSH);
SAMPLER(samplerunity_ProbeVolumeSH);

// Exposure texture - 1x1 RG16F (r: exposure mult, g: exposure EV100)
TEXTURE2D(_ExposureTexture);
TEXTURE2D(_PrevExposureTexture);
*/
// Note: To sample camera depth in HDRP we provide these utils functions because the way we store the depth mips can change
// Currently it's an atlas and it's layout can be found at ComputePackedMipChainInfo in HDUtils.cs
float LoadCameraDepth(uint2 pixelCoords)
{
    return LOAD_TEXTURE2D_X_LOD(_CameraDepthTexture, pixelCoords, 0).r;
}

float SampleCameraDepth(float2 uv)
{
    return LoadCameraDepth(uint2(uv * _ScreenSize.xy));
}

float3 LoadCameraColor(uint2 pixelCoords, uint lod)
{
    return LOAD_TEXTURE2D_X_LOD(_ColorPyramidTexture, pixelCoords, lod).rgb;
}

float3 SampleCameraColor(float2 uv, float lod)
{
    return SAMPLE_TEXTURE2D_X_LOD(_ColorPyramidTexture, s_trilinear_clamp_sampler, uv * _RTHandleScaleHistory.xy, lod).rgb;
}

float3 LoadCameraColor(uint2 pixelCoords)
{
    return LoadCameraColor(pixelCoords, 0);
}

float3 SampleCameraColor(float2 uv)
{
    return SampleCameraColor(uv, 0);
}

float4 SampleCustomColor(float2 uv)
{
    return SAMPLE_TEXTURE2D_X_LOD(_CustomColorTexture, s_trilinear_clamp_sampler, uv * _RTHandleScale.xy, 0);
}

float4 LoadCustomColor(uint2 pixelCoords)
{
    return LOAD_TEXTURE2D_X_LOD(_CustomColorTexture, pixelCoords, 0);
}

float LoadCustomDepth(uint2 pixelCoords)
{
    return LOAD_TEXTURE2D_X_LOD(_CustomDepthTexture, pixelCoords, 0).r;
}

float SampleCustomDepth(float2 uv)
{
    return LoadCustomDepth(uint2(uv * _ScreenSize.xy));
}

bool IsSky(float deviceDepth)
{
    return deviceDepth == UNITY_RAW_FAR_CLIP_VALUE; // We assume the sky is the part of the depth buffer that haven't been written.
}

bool IsSky(uint2 pixelCoord)
{
    float deviceDepth = LoadCameraDepth(pixelCoord);
    return IsSky(deviceDepth);
}

bool IsSky(float2 uv)
{
    return IsSky(uint2(uv * _ScreenSize.xy));
}

float4x4 OptimizeProjectionMatrix(float4x4 M)
{
    // Matrix format (x = non-constant value).
    // Orthographic Perspective  Combined(OR)
    // | x 0 0 x |  | x 0 x 0 |  | x 0 x x |
    // | 0 x 0 x |  | 0 x x 0 |  | 0 x x x |
    // | x x x x |  | x x x x |  | x x x x | <- oblique projection row
    // | 0 0 0 1 |  | 0 0 x 0 |  | 0 0 x x |
    // Notice that some values are always 0.
    // We can avoid loading and doing math with constants.
    M._21_41 = 0;
    M._12_42 = 0;
    return M;
}

// Helper to handle camera relative space

float3 GetCameraPositionWS()
{
#if (SHADEROPTIONS_CAMERA_RELATIVE_RENDERING != 0)
    return 0;
#endif
    return _WorldSpaceCameraPos;
}

float4x4 ApplyCameraTranslationToMatrix(float4x4 modelMatrix)
{
    // To handle camera relative rendering we substract the camera position in the model matrix
#if (SHADEROPTIONS_CAMERA_RELATIVE_RENDERING != 0)
    modelMatrix._m03_m13_m23 -= _WorldSpaceCameraPos.xyz;
#endif
    return modelMatrix;
}

float4x4 ApplyCameraTranslationToInverseMatrix(float4x4 inverseModelMatrix)
{
#if (SHADEROPTIONS_CAMERA_RELATIVE_RENDERING != 0)
    // To handle camera relative rendering we need to apply translation before converting to object space
    float4x4 translationMatrix = { { 1.0, 0.0, 0.0, _WorldSpaceCameraPos.x },{ 0.0, 1.0, 0.0, _WorldSpaceCameraPos.y },{ 0.0, 0.0, 1.0, _WorldSpaceCameraPos.z },{ 0.0, 0.0, 0.0, 1.0 } };
    return mul(inverseModelMatrix, translationMatrix);
#else
    return inverseModelMatrix;
#endif
}

float4x4 RevertCameraTranslationFromInverseMatrix(float4x4 inverseModelMatrix)
{
#if (SHADEROPTIONS_CAMERA_RELATIVE_RENDERING != 0)
    // To handle camera relative rendering we need to apply translation before converting to object space
    float4x4 translationMatrix = { { 1.0, 0.0, 0.0, -_WorldSpaceCameraPos.x },{ 0.0, 1.0, 0.0, -_WorldSpaceCameraPos.y },{ 0.0, 0.0, 1.0, -_WorldSpaceCameraPos.z },{ 0.0, 0.0, 0.0, 1.0 } };
    return mul(inverseModelMatrix, translationMatrix);
#else
    return inverseModelMatrix;
#endif
}

float4x4 RevertCameraTranslationFromMatrix(float4x4 modelMatrix)
{
#if (SHADEROPTIONS_CAMERA_RELATIVE_RENDERING != 0)
    modelMatrix._m03_m13_m23 += _WorldSpaceCameraPos.xyz;
#endif
    return modelMatrix;
}
void ApplyCameraRelativeXR(inout float3 positionWS)
{
#if (SHADEROPTIONS_CAMERA_RELATIVE_RENDERING != 0) && defined(USING_STEREO_MATRICES)
    positionWS += _WorldSpaceCameraPosViewOffset;
#endif
}

float GetCurrentExposureMultiplier()
{
#if SHADEROPTIONS_PRE_EXPOSITION
    // _ProbeExposureScale is a scale used to perform range compression to avoid saturation of the content of the probes. It is 1.0 if we are not rendering probes.
    return LOAD_TEXTURE2D(_ExposureTexture, int2(0, 0)).x * _ProbeExposureScale;
#else
    return _ProbeExposureScale;
#endif
}

float GetPreviousExposureMultiplier()
{
#if SHADEROPTIONS_PRE_EXPOSITION
    // _ProbeExposureScale is a scale used to perform range compression to avoid saturation of the content of the probes. It is 1.0 if we are not rendering probes.
    return LOAD_TEXTURE2D(_PrevExposureTexture, int2(0, 0)).x * _ProbeExposureScale;
#else
    return _ProbeExposureScale;
#endif
}

float GetInverseCurrentExposureMultiplier()
{
    float exposure = GetCurrentExposureMultiplier();
    return rcp(exposure + (exposure == 0.0)); // zero-div guard
}

float GetInversePreviousExposureMultiplier()
{
    float exposure = GetPreviousExposureMultiplier();
    return rcp(exposure + (exposure == 0.0)); // zero-div guard
}

// Helper function for indirect control volume
float GetIndirectDiffuseMultiplier(uint renderingLayers)
{
    return (_IndirectDiffuseLightingLayers & renderingLayers) ? _IndirectDiffuseLightingMultiplier : 1.0f;
}

float GetIndirectSpecularMultiplier(uint renderingLayers)
{
    return (_ReflectionLightingLayers & renderingLayers) ? _ReflectionLightingMultiplier : 1.0f;
}

// Functions to clamp UVs to use when RTHandle system is used.

float2 ClampAndScaleUV(float2 UV, float2 texelSize, float numberOfTexels, float2 scale)
{
    float2 maxCoord = 1.0f - numberOfTexels * texelSize;
    return min(UV, maxCoord) * scale;
}

float2 ClampAndScaleUV(float2 UV, float2 texelSize, float numberOfTexels)
{
    return ClampAndScaleUV(UV, texelSize, numberOfTexels, _RTHandleScale.xy);
}

// This is assuming half a texel offset in the clamp.
float2 ClampAndScaleUVForBilinear(float2 UV, float2 texelSize)
{
    return ClampAndScaleUV(UV, texelSize, 0.5f);
}

// This is assuming full screen buffer and half a texel offset for the clamping.
float2 ClampAndScaleUVForBilinear(float2 UV)
{
    return ClampAndScaleUV(UV, _ScreenSize.zw, 0.5f);
}

// This is assuming an upsampled texture used in post processing, with original screen size and a half a texel offset for the clamping.
float2 ClampAndScaleUVForBilinearPostProcessTexture(float2 UV)
{
    return ClampAndScaleUV(UV, _PostProcessScreenSize.zw, 0.5f, _RTHandlePostProcessScale.xy);
}

// This is assuming an upsampled texture used in post processing, with original screen size and a half a texel offset for the clamping.
float2 ClampAndScaleUVForBilinearPostProcessTexture(float2 UV, float2 texelSize)
{
    return ClampAndScaleUV(UV, texelSize, 0.5f, _RTHandlePostProcessScale.xy);
}

// This is assuming an upsampled texture used in post processing, with original screen size and numberOfTexels offset for the clamping.
float2 ClampAndScaleUVPostProcessTexture(float2 UV, float2 texelSize, float numberOfTexels)
{
    return ClampAndScaleUV(UV, texelSize, numberOfTexels, _RTHandlePostProcessScale.xy);
}

float2 ClampAndScaleUVForPoint(float2 UV)
{
    return min(UV, 1.0f) * _RTHandleScale.xy;
}

float2 ClampAndScaleUVPostProcessTextureForPoint(float2 UV)
{
    return min(UV, 1.0f) * _RTHandlePostProcessScale.xy;
}

// IMPORTANT: This is expecting the corner not the center.
float2 FromOutputPosSSToPreupsampleUV(int2 posSS)
{
    return (posSS + 0.5f) * _PostProcessScreenSize.zw;
}

// IMPORTANT: This is expecting the corner not the center.
float2 FromOutputPosSSToPreupsamplePosSS(float2 posSS)
{
    float2 uv = FromOutputPosSSToPreupsampleUV(posSS);
    return floor(uv * _ScreenSize.xy);
}



uint Get1DAddressFromPixelCoord(uint2 pixCoord, uint2 screenSize, uint eye)
{
    // We need to shift the index to look up the right eye info.
    return (pixCoord.y * screenSize.x + pixCoord.x) + eye * (screenSize.x * screenSize.y);
}

uint Get1DAddressFromPixelCoord(uint2 pixCoord, uint2 screenSize)
{
    return Get1DAddressFromPixelCoord(pixCoord, screenSize, 0);
}

// There is no UnityPerDraw cbuffer with BatchRendererGroup. Those matrices don't exist, so don't try to access them
#ifndef DOTS_INSTANCING_ON

void GetAbsoluteWorldRendererBounds(out float3 minBounds, out float3 maxBounds)
{
    minBounds = unity_RendererBounds_Min.xyz;
    maxBounds = unity_RendererBounds_Max.xyz;
}

// Define Model Matrix Macro
// Note: In order to be able to define our macro to forbid usage of unity_ObjectToWorld/unity_WorldToObject/unity_MatrixPreviousM/unity_MatrixPreviousMI
// We need to declare inline function. Using uniform directly mean they are expand with the macro
float4x4 GetRawUnityObjectToWorld()     { return unity_ObjectToWorld; }
float4x4 GetRawUnityWorldToObject()     { return unity_WorldToObject; }
float4x4 GetRawUnityPrevObjectToWorld() { return unity_MatrixPreviousM; }
float4x4 GetRawUnityPrevWorldToObject() { return unity_MatrixPreviousMI; }

#define UNITY_MATRIX_M         ApplyCameraTranslationToMatrix(GetRawUnityObjectToWorld())
#define UNITY_MATRIX_I_M       ApplyCameraTranslationToInverseMatrix(GetRawUnityWorldToObject())
#define UNITY_PREV_MATRIX_M    ApplyCameraTranslationToMatrix(GetRawUnityPrevObjectToWorld())
#define UNITY_PREV_MATRIX_I_M  ApplyCameraTranslationToInverseMatrix(GetRawUnityPrevWorldToObject())

#else

// Not yet supported by BRG
void GetAbsoluteWorldRendererBounds(out float3 minBounds, out float3 maxBounds)
{
    minBounds = 0;
    maxBounds = 0;
}

#endif

void GetRendererBounds(out float3 minBounds, out float3 maxBounds)
{
    GetAbsoluteWorldRendererBounds(minBounds, maxBounds);

#if (SHADEROPTIONS_CAMERA_RELATIVE_RENDERING != 0)
    minBounds -= _WorldSpaceCameraPos.xyz;
    maxBounds -= _WorldSpaceCameraPos.xyz;
#endif
}

float3 GetRendererExtents()
{
    float3 minBounds, maxBounds;
    GetRendererBounds(minBounds, maxBounds);
    return (maxBounds - minBounds) * 0.5;
}

// This utility function is currently used to support an alpha blending friendly format
// for virtual texturing + transparency support.
float2 ConvertRGBA8UnormToRG16Unorm(float4 inputRGBA8Unorm)
{
    uint4 uintValues = (uint4)(inputRGBA8Unorm * 255.0f) & 0xFF;
    float scale = 1.0 / (float)(0xFFFF);
    float2 packedRG16UnormVal = float2(
        (float)(uintValues.x | (uintValues.y << 8)),
        (float)(uintValues.z | (uintValues.w << 8))) * scale;
    return packedRG16UnormVal;
}

// This utility function is currently used to support an alpha blending friendly format
// for virtual texturing + transparency support.
float4 ConvertRG16UnormToRGBA8Unorm(float2 compressedFeedback)
{
    float scale = (float)(0xFFFF);
    uint2 packedUintValues = (uint2)(compressedFeedback * scale);
    uint4 uintValues = uint4(packedUintValues.x, packedUintValues.x >> 8, packedUintValues.y, packedUintValues.y >> 8) & 0xFF;
    return (float4)uintValues / 255.0f;
}

float4 PackVTFeedbackWithAlpha(float4 feedback, float2 pixelCoord, float alpha)
{
    float2 vtFeedbackCompressed = ConvertRGBA8UnormToRG16Unorm(feedback);
    if (alpha == 1.0 || alpha == 0.0)
        return float4(vtFeedbackCompressed.x, vtFeedbackCompressed.y, 0.0, alpha);

    float2 pixelLocationAlpha = frac(pixelCoord * 0.25f); // We don't scale after the frac so this will give coords 0, 0.25, 0.5, 0.75
    int pixelId = (int)(pixelLocationAlpha.y * 16 + pixelLocationAlpha.x * 4) & 0xF;

    // Modern hardware supports array indexing with per pixel varying indexes
    // on old hardware this will be expanded to a conditional tree by the compiler
    const float thresholdMaxtrix[16] = {1.0f / 17.0f, 9.0f / 17.0f, 3.0f / 17.0f, 11.0f / 17.0f,
                                        13.0f / 17.0f,  5.0f / 17.0f, 15.0f / 17.0f, 7.0f / 17.0f,
                                        4.0f / 17.0f, 12.0f / 17.0f, 2.0f / 17.0f, 10.0f / 17.0f,
                                        16.0f / 17.0f, 8.0f / 17.0f, 14.0f / 17.0f, 6.0f / 17.0f};

    float threshold = thresholdMaxtrix[pixelId];
    return float4(vtFeedbackCompressed.x, vtFeedbackCompressed.y, 0.0, alpha < threshold ? 0.0f : 1.0);
}

float4 UnpackVTFeedbackWithAlpha(float4 feedbackWithAlpha)
{
    return ConvertRG16UnormToRGBA8Unorm(feedbackWithAlpha.xy);
}

// To get instancing working, we must use UNITY_MATRIX_M/UNITY_MATRIX_I_M/UNITY_PREV_MATRIX_M/UNITY_PREV_MATRIX_I_M as UnityInstancing.hlsl redefine them
#define unity_ObjectToWorld Use_Macro_UNITY_MATRIX_M_instead_of_unity_ObjectToWorld
#define unity_WorldToObject Use_Macro_UNITY_MATRIX_I_M_instead_of_unity_WorldToObject

// Define View/Projection matrix macro
#define UNITY_MATRIX_V     _ViewMatrix
#define UNITY_MATRIX_I_V   _InvViewMatrix
#define UNITY_MATRIX_P     OptimizeProjectionMatrix(_ProjMatrix)
#define UNITY_MATRIX_I_P   _InvProjMatrix
#define UNITY_MATRIX_VP    _ViewProjMatrix
#define UNITY_MATRIX_I_VP  _InvViewProjMatrix
#define UNITY_MATRIX_UNJITTERED_VP _NonJitteredViewProjMatrix
#define UNITY_MATRIX_PREV_VP _PrevViewProjMatrix
#define UNITY_MATRIX_PREV_I_VP _PrevInvViewProjMatrix

// ShaderVariablesLightLoop.hlsl
StructuredBuffer<DirectionalLightData> _DirectionalLightDatas;
StructuredBuffer<LightData> _LightDatas;
StructuredBuffer<EnvLightData> _EnvLightDatas;

// ShaderVariablesAtmosphericScattering.hlsl
TEXTURECUBE_ARRAY(_SkyTexture);
StructuredBuffer<float4> _AmbientProbeData;

#define _MipFogNear         _MipFogParameters.x
#define _MipFogFar          _MipFogParameters.y
#define _MipFogMaxMip       _MipFogParameters.z

#define _FogColor           _FogColor

// ShaderVariablesScreenSpaceLighting.hlsl
TEXTURE2D_X(_AmbientOcclusionTexture);
TEXTURE2D_X(_CameraMotionVectorsTexture);
TEXTURE2D_X(_SsrLightingTexture);

// ShaderVariablesDecal.hlsl
StructuredBuffer<DecalData> _DecalDatas;

TEXTURE2D(_DecalAtlas2D);
SAMPLER(_trilinear_clamp_sampler_DecalAtlas2D);

UNITY_INSTANCING_BUFFER_START(Decal)
UNITY_DEFINE_INSTANCED_PROP(float4x4, _NormalToWorld)
UNITY_DEFINE_INSTANCED_PROP(uint, _DecalLayerMaskFromDecal)
UNITY_INSTANCING_BUFFER_END(Decal)


#include "../ShaderLibrary/ShaderVariablesFunctions.hlsl"

#endif // UNITY_SHADER_VARIABLES_INCLUDED
