#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/Color.hlsl"
#include "../../Material/Builtin/BuiltinData.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"

#ifndef UNITY_PP_DEFINES_INCLUDED
#define UNITY_PP_DEFINES_INCLUDED

#define ENABLE_ALPHA 1

#if !defined(ENABLE_ALPHA)
    #define CTYPE float3
    #define CTYPE_SWIZZLE xyz
#else
    #define CTYPE float4
    #define CTYPE_SWIZZLE xyzw
#endif //ENABLE_ALPHA

#endif //UNITY_PP_DEFINES_INCLUDED

#include "TemporalAntialiasingOptionDef.hlsl"

#define HIGH_QUALITY

#ifdef LOW_QUALITY
    #define YCOCG 0
    #define HISTORY_SAMPLING_METHOD BILINEAR
    #define WIDE_NEIGHBOURHOOD 0
    #define NEIGHBOUROOD_CORNER_METHOD MINMAX
    #define CENTRAL_FILTERING NO_FILTERING
    #define HISTORY_CLIP SIMPLE_CLAMP
    #define ANTI_FLICKER 0
    #define VELOCITY_REJECTION (defined(ENABLE_MV_REJECTION) && 0)
    #define PERCEPTUAL_SPACE 0
    #define PERCEPTUAL_SPACE_ONLY_END 1 && (PERCEPTUAL_SPACE == 0)
    #define BLEND_FACTOR_MV_TUNE 0
    #define MV_DILATION DEPTH_DILATION

#elif defined(MEDIUM_QUALITY)
    #define YCOCG 1
    #define HISTORY_SAMPLING_METHOD BICUBIC_5TAP
    #define WIDE_NEIGHBOURHOOD 0
    #define NEIGHBOUROOD_CORNER_METHOD VARIANCE
    #define CENTRAL_FILTERING NO_FILTERING
    #define HISTORY_CLIP DIRECT_CLIP
    #define ANTI_FLICKER 1
    #define ANTI_FLICKER_MV_DEPENDENT 0
    #define VELOCITY_REJECTION (defined(ENABLE_MV_REJECTION) && 0)
    #define PERCEPTUAL_SPACE 1
    #define PERCEPTUAL_SPACE_ONLY_END 0 && (PERCEPTUAL_SPACE == 0)
    #define BLEND_FACTOR_MV_TUNE 1
    #define MV_DILATION DEPTH_DILATION

#elif defined(HIGH_QUALITY) // TODO: We can do better in term of quality here (e.g. subpixel changes etc) and can be optimized a bit more
    #define YCOCG 1
    #define HISTORY_SAMPLING_METHOD BICUBIC_5TAP
    #define WIDE_NEIGHBOURHOOD 1
    #define NEIGHBOUROOD_CORNER_METHOD VARIANCE
    #define CENTRAL_FILTERING BLACKMAN_HARRIS
    #define HISTORY_CLIP DIRECT_CLIP
    #define ANTI_FLICKER 1
    #define ANTI_FLICKER_MV_DEPENDENT 1
    #define VELOCITY_REJECTION defined(ENABLE_MV_REJECTION)
    #define PERCEPTUAL_SPACE 1
    #define PERCEPTUAL_SPACE_ONLY_END 0 && (PERCEPTUAL_SPACE == 0)
    #define BLEND_FACTOR_MV_TUNE 1
    #define MV_DILATION DEPTH_DILATION

#elif defined(TAA_UPSAMPLE)
    #define YCOCG 1
    #define HISTORY_SAMPLING_METHOD BICUBIC_5TAP
    #define WIDE_NEIGHBOURHOOD 1
    #define NEIGHBOUROOD_CORNER_METHOD VARIANCE
    #define CENTRAL_FILTERING UPSAMPLE
    #define HISTORY_CLIP DIRECT_CLIP
    #define ANTI_FLICKER 1
    #define ANTI_FLICKER_MV_DEPENDENT 1
    #define VELOCITY_REJECTION defined(ENABLE_MV_REJECTION)
    #define PERCEPTUAL_SPACE 1
    #define PERCEPTUAL_SPACE_ONLY_END 0 && (PERCEPTUAL_SPACE == 0)
    #define BLEND_FACTOR_MV_TUNE 1
    #define MV_DILATION DEPTH_DILATION

#elif defined(POST_DOF)
    #define YCOCG 1
    #define HISTORY_SAMPLING_METHOD BILINEAR
    #define WIDE_NEIGHBOURHOOD 0
    #define NEIGHBOUROOD_CORNER_METHOD VARIANCE
    #define CENTRAL_FILTERING NO_FILTERING
    #define HISTORY_CLIP DIRECT_CLIP
    #define ANTI_FLICKER 1
    #define ANTI_FLICKER_MV_DEPENDENT 1
    #define VELOCITY_REJECTION defined(ENABLE_MV_REJECTION)
    #define PERCEPTUAL_SPACE 1
    #define PERCEPTUAL_SPACE_ONLY_END 0 && (PERCEPTUAL_SPACE == 0)
    #define BLEND_FACTOR_MV_TUNE 1
    #define MV_DILATION DEPTH_DILATION

#endif

#include "TemporalAntialiasing.hlsl"

#if DIRECT_STENCIL_SAMPLE
TEXTURE2D_X_UINT2(_StencilTexture);
#endif

#if VELOCITY_REJECTION
TEXTURE2D_X(_InputVelocityMagnitudeHistory);
#ifdef SHADER_API_PSSL
RW_TEXTURE2D_X(float, _OutputVelocityMagnitudeHistory) : register(u1);
#else
RW_TEXTURE2D_X(float, _OutputVelocityMagnitudeHistory) : register(u2);
#endif
#endif

cbuffer RootConstants : register(b0, space0)
{
    uint frameUniformIndex;
    uint cameraMotionVectorsTextureIndex;
    uint inputTextureIndex;
    uint depthTextureIndex;
    uint inputHistoryTextureIndex;
    uint outputHistoryTextureIndex;
    uint aaOutIndex;
};

SamplerState s_point_clamp_sampler      : register(s10);
SamplerState s_linear_clamp_sampler     : register(s11);
SamplerState s_linear_repeat_sampler    : register(s12);
SamplerState s_trilinear_clamp_sampler  : register(s13);
SamplerState s_trilinear_repeat_sampler : register(s14);
SamplerComparisonState s_linear_clamp_compare_sampler : register(s15);


// struct Attributes
// {
//     uint vertexID : SV_VertexID;
// };
//
// struct Varyings
// {
//     float4 positionCS : SV_POSITION;
//     float2 texcoord   : TEXCOORD0;
// };
//
// Varyings Vert(Attributes input)
// {
//     Varyings output;
//     output.positionCS = GetFullScreenTriangleVertexPosition(input.vertexID);
//     output.texcoord = GetFullScreenTriangleTexCoord(input.vertexID);
//     return output;
// }

// void FragTAA(Varyings input, out CTYPE outColor : SV_Target0)

[numthreads( 8, 8, 1 )]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[frameUniformIndex];
    Texture2D<float4> _CameraMotionVectorsTexture = ResourceDescriptorHeap[cameraMotionVectorsTextureIndex];
    Texture2D<float4> _InputTexture = ResourceDescriptorHeap[inputTextureIndex];
    Texture2D<float4> _InputHistoryTexture = ResourceDescriptorHeap[inputHistoryTextureIndex];
    Texture2D<float> _DepthTexture = ResourceDescriptorHeap[depthTextureIndex];
    RWTexture2D<float4> _OutputHistoryTexture = ResourceDescriptorHeap[outputHistoryTextureIndex];
    RWTexture2D<float4> _OutputAATexture = ResourceDescriptorHeap[aaOutIndex];

    float4 _InputSize = frameUniform.taaUniform._InputSize;

    if(DTid.x >= _InputSize.x || DTid.y >= _InputSize.y)
        return;

    float2 inputTexcoord = (DTid.xy + float2(0.5f, 0.5f)) * _InputSize.zw;
    int2 inputPositionCS = DTid.xy;
    
    float4 _RTHandleScale = frameUniform.baseUniform._RTHandleScale;
    
    float _HistorySharpening = frameUniform.taaUniform._HistorySharpening;
    float _AntiFlickerIntensity = frameUniform.taaUniform._AntiFlickerIntensity;
    float _SpeedRejectionIntensity = frameUniform.taaUniform._SpeedRejectionIntensity;
    float _ContrastForMaxAntiFlicker = frameUniform.taaUniform._ContrastForMaxAntiFlicker;

    float _BaseBlendFactor = frameUniform.taaUniform._BaseBlendFactor;
    float _CentralWeight = frameUniform.taaUniform._CentralWeight;
    uint _ExcludeTAABit = frameUniform.taaUniform._ExcludeTAABit;
    float _HistoryContrastBlendLerp = frameUniform.taaUniform._HistoryContrastBlendLerp;

    float2 _RTHandleScaleForTAAHistory = frameUniform.taaUniform._RTHandleScaleForTAAHistory;
    float2 _RTHandleScaleForTAA = frameUniform.taaUniform._RTHandleScaleForTAA;
    
    float4 _TaaFrameInfo = frameUniform.taaUniform._TaaFrameInfo;
    float4 _TaaJitterStrength = frameUniform.taaUniform._TaaJitterStrength;
    
    float4 _TaaHistorySize = frameUniform.taaUniform._TaaHistorySize;

    float4 _TaaFilterWeights[2] = frameUniform.taaUniform._TaaFilterWeights;

    float _TAAUFilterRcpSigma2 = frameUniform.taaUniform._TAAUFilterRcpSigma2;
    float _TAAUScale = frameUniform.taaUniform._TAAUScale;
    float _TAAUBoxConfidenceThresh = frameUniform.taaUniform._TAAUBoxConfidenceThresh;
    float _TAAURenderScale = frameUniform.taaUniform._TAAURenderScale;
    
    SetNeighbourOffsets(frameUniform.taaUniform._NeighbourOffsets);

    float sharpenStrength = _TaaFrameInfo.x;
    float2 jitter = _TaaJitterStrength.zw;

    float2 uv = inputTexcoord;

#ifdef TAA_UPSAMPLE
    float2 outputPixInInput = input.texcoord * _InputSize.xy - _TaaJitterStrength.xy;

    uv = _InputSize.zw * (0.5f + floor(outputPixInInput));
#endif

    // --------------- Get closest motion vector ---------------

    int2 samplePos = inputPositionCS.xy;

#ifdef TAA_UPSAMPLE
    samplePos = outputPixInInput;
#endif

    bool excludeTAABit = false;
#if DIRECT_STENCIL_SAMPLE
    uint stencil = GetStencilValue(LOAD_TEXTURE2D_X(_StencilTexture, inputPositionCS.xy));
    excludeTAABit = (stencil == _ExcludeTAABit);
#endif

    float lengthMV = 0;

    float2 motionVector = GetMotionVector(_CameraMotionVectorsTexture, _DepthTexture, s_point_clamp_sampler, uv, inputPositionCS.xy, _InputSize);
    // --------------------------------------------------------

    // --------------- Get resampled history ---------------
    float2 prevUV = inputTexcoord - motionVector;

    CTYPE history = GetFilteredHistory(_InputHistoryTexture, s_point_clamp_sampler, prevUV, _HistorySharpening, _TaaHistorySize, _RTHandleScaleForTAAHistory, _InputSize);
    bool offScreen = any(abs(prevUV * 2 - 1) >= (1.0f - (1.0 * _TaaHistorySize.zw)));
    history.xyz *= PerceptualWeight(history);
    // -----------------------------------------------------

    // --------------- Gather neigbourhood data ---------------
    CTYPE color = Fetch4(_InputTexture, s_linear_clamp_sampler, uv, float2(0.0f, 0.0f), _RTHandleScaleForTAA, _InputSize).CTYPE_SWIZZLE;

#if defined(ENABLE_ALPHA)
    // Removes history rejection when the current alpha value is 0. Instead it does blend with the history color when alpha value is 0 on the current plane.
    // The reasoning for blending again with the history when alpha is 0 is because we want the color to blend a bit with opacity, which is the main reason for the alpha values. sort of like a precomputed color
    // As a safety, we set the color to black if alpha is 0. This results in better image quality when alpha is enabled.
    color =  color.w > 0.0 ? color : (CTYPE)0;    
#endif

    if (!excludeTAABit)
    {
        color = clamp(color, 0, CLAMP_MAX);
        color = ConvertToWorkingSpace(color);

        NeighbourhoodSamples samples;
        GatherNeighbourhood(_InputTexture, s_linear_clamp_sampler, uv, floor(inputPositionCS.xy), color, _RTHandleScaleForTAA, _InputSize, samples);
        // --------------------------------------------------------

        // --------------- Filter central sample ---------------
        float4 filterParams = 0;
#ifdef TAA_UPSAMPLE
        filterParams.x = _TAAUFilterRcpSigma2;
        filterParams.y = _TAAUScale;
        filterParams.zw = outputPixInInput - (floor(outputPixInInput) + 0.5f);
#endif

#if CENTRAL_FILTERING == BLACKMAN_HARRIS
        CTYPE filteredColor = FilterCentralColor(samples, _CentralWeight, _TaaFilterWeights);
#else
        CTYPE filteredColor = FilterCentralColor(samples, filterParams);
#endif
        // ------------------------------------------------------

        if (offScreen)
            history = filteredColor;

        // --------------- Get neighbourhood information and clamp history ---------------
        float colorLuma = GetLuma(filteredColor);
        float historyLuma = GetLuma(history);

        float motionVectorLength = 0.0f;
        float motionVectorLenInPixels = 0.0f;

#if ANTI_FLICKER_MV_DEPENDENT || VELOCITY_REJECTION || BLEND_FACTOR_MV_TUNE
        motionVectorLength = length(motionVector);
        motionVectorLenInPixels = motionVectorLength * length(_InputSize.xy);
#endif

        float aggressivelyClampedHistoryLuma = 0;
        GetNeighbourhoodCorners(samples, historyLuma, colorLuma, float2(_AntiFlickerIntensity, _ContrastForMaxAntiFlicker),
            motionVectorLenInPixels, _TAAURenderScale, aggressivelyClampedHistoryLuma);

        history = GetClippedHistory(filteredColor, history, samples.minNeighbour, samples.maxNeighbour);
        filteredColor = SharpenColor(samples, filteredColor, sharpenStrength);
        // ------------------------------------------------------------------------------

        // --------------- Compute blend factor for history ---------------
        float blendFactor = GetBlendFactor(colorLuma, aggressivelyClampedHistoryLuma,
            GetLuma(samples.minNeighbour), GetLuma(samples.maxNeighbour), _BaseBlendFactor, _HistoryContrastBlendLerp);
#if BLEND_FACTOR_MV_TUNE
        blendFactor = lerp(blendFactor, saturate(2.0f * blendFactor), saturate(motionVectorLenInPixels  / 50.0f));
#endif
        // --------------------------------------------------------

        // ------------------- Alpha handling ---------------------------
#if defined(ENABLE_ALPHA)
        // Compute the antialiased alpha value
        filteredColor.w = lerp(history.w, filteredColor.w, blendFactor);
        // TAA should not overwrite pixels with zero alpha. This allows camera stacking with mixed TAA settings (bottom camera with TAA OFF and top camera with TAA ON).
        CTYPE unjitteredColor = Fetch4(_InputTexture, s_linear_clamp_sampler,
            inputTexcoord - color.w * jitter, float2(0.0f, 0.0f), _RTHandleScale.xy, _InputSize).CTYPE_SWIZZLE;
        unjitteredColor = ConvertToWorkingSpace(unjitteredColor);
        unjitteredColor.xyz *= PerceptualWeight(unjitteredColor);
        filteredColor.xyz = lerp(unjitteredColor.xyz, filteredColor.xyz, filteredColor.w);
#endif
        // ---------------------------------------------------------------

        // --------------- Blend to final value and output ---------------

#if VELOCITY_REJECTION
        // The 10 multiplier serves a double purpose, it is an empirical scale value used to perform the rejection and it also helps with storing the value itself.
        lengthMV = motionVectorLength * 10;
        blendFactor = ModifyBlendWithMotionVectorRejection(_InputVelocityMagnitudeHistory, lengthMV, prevUV, blendFactor, _SpeedRejectionIntensity, _RTHandleScaleForTAAHistory);
#endif

#ifdef TAA_UPSAMPLE
        blendFactor *= GetUpsampleConfidence(filterParams.zw, _TAAUBoxConfidenceThresh, _TAAUFilterRcpSigma2, _TAAUScale);
#endif
        blendFactor = clamp(blendFactor, 0.03f, 0.98f);

        CTYPE finalColor;
#if PERCEPTUAL_SPACE_ONLY_END
        finalColor.xyz = lerp(ReinhardToneMap(history).xyz, ReinhardToneMap(filteredColor).xyz, blendFactor);
        finalColor.xyz = InverseReinhardToneMap(finalColor).xyz;
#else
        finalColor.xyz = lerp(history.xyz, filteredColor.xyz, blendFactor);
        finalColor.xyz *= PerceptualInvWeight(finalColor);
#endif

        color.xyz = ConvertToOutputSpace(finalColor.xyz);
        color.xyz = clamp(color.xyz, 0, CLAMP_MAX);
#if defined(ENABLE_ALPHA)
        // Set output alpha to the antialiased alpha.
        color.w = filteredColor.w;
#endif
    }

    _OutputHistoryTexture[inputPositionCS.xy] = color.CTYPE_SWIZZLE;
    _OutputAATexture[inputPositionCS.xy] = color.CTYPE_SWIZZLE;
#if VELOCITY_REJECTION && !defined(POST_DOF)
    _OutputVelocityMagnitudeHistory[COORD_TEXTURE2D_X(input.positionCS.xy)] = lengthMV;
#endif
    // -------------------------------------------------------------
}

// void FragCopyHistory(Varyings input, out CTYPE outColor : SV_Target0)
// {
//     ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[frameUniformIndex];
//     Texture2D<float4> _InputTexture = ResourceDescriptorHeap[inputTextureIndex];
//     
//     float4 _TaaJitterStrength = frameUniform.taaUniform._TaaJitterStrength;
//     float2 _RTHandleScaleForTAA = frameUniform.taaUniform._RTHandleScaleForTAA;
//
//     float4 _InputSize = frameUniform.taaUniform._InputSize;
//     
//     float2 jitter = _TaaJitterStrength.zw;
//     float2 uv = input.texcoord;
//
// #ifdef TAA_UPSAMPLE
//     float2 outputPixInInput = input.texcoord * _InputSize.xy - _TaaJitterStrength.xy;
//
//     uv = _InputSize.zw * (0.5f + floor(outputPixInInput));
// #endif
//     CTYPE color = Fetch4(_InputTexture, s_linear_clamp_sampler, uv, 0.0, _RTHandleScaleForTAA, _InputSize).CTYPE_SWIZZLE;
//
//     outColor = color;
// }


