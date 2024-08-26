
// Common includes
#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/CommonLighting.hlsl"
#include "../../ShaderLibrary/Sampling/Sampling.hlsl"
#include "../../ShaderLibrary/Color.hlsl"

#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Material/NormalBuffer.hlsl"
#include "../../Material/Builtin/BuiltinData.hlsl"
#include "../../Tools/Denoising/DenoisingUtils.hlsl"

#define HISTORYREJECTIONFLAGS_DEPTH (1)
#define HISTORYREJECTIONFLAGS_REPROJECTION (2)
#define HISTORYREJECTIONFLAGS_PREVIOUS_DEPTH (4)
#define HISTORYREJECTIONFLAGS_POSITION (8)
#define HISTORYREJECTIONFLAGS_NORMAL (16)
#define HISTORYREJECTIONFLAGS_MOTION (32)
#define HISTORYREJECTIONFLAGS_COMBINED (63)
#define HISTORYREJECTIONFLAGS_COMBINED_NO_MOTION (31)

// Tile size of this compute shaders
#define TEMPORAL_FILTER_TILE_SIZE 8

// The maximal normal difference threshold
#define MAX_NORMAL_DIFFERENCE 0.65
// The minimal motion distance
#define MINIMAL_MOTION_DISTANCE 0.0001

struct TemporalFilterStruct
{
    // This holds the fov angle of a pixel
    float _PixelSpreadAngleTangent;
    // Value that tells us if the current history should be discarded based on scene-level data
    float _HistoryValidity;
    // Controls if the motion of the receiver is a valid rejection codition.
    int _ReceiverMotionRejection;
    // Controls if the motion of the receiver is a valid rejection codition.
    int _OccluderMotionRejection;
    // Contains history buffer size in xy and the uv scale in zw
    float4 _HistorySizeAndScale;
    // Contains resolution multiplier in x, inverse in y, unused on zw
    float4 _DenoiserResolutionMultiplierVals;
    // Value that tells us if the current history should be discarded based on scene-level data
    int _EnableExposureControl;

    int3 _Unused0;
};

SamplerState s_linear_clamp_sampler : register(s10);

// #define VALIDATEHISTORY

#if defined(VALIDATEHISTORY)

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint temporalFilterStructIndex;
    uint cameraMotionVectorIndex; // Velocity buffer for history rejection
    uint minDepthBufferIndex; // Depth buffer of the current frame
    uint historyDepthTextureIndex; // Depth buffer of the previous frame
    uint normalBufferIndex; // // Normal buffer of the current frame
    uint historyNormalTextureIndex; // Normal buffer of the previous frame
    uint validationBufferRWIndex; // Buffer that stores the result of the validation pass of the history
};

[numthreads(TEMPORAL_FILTER_TILE_SIZE, TEMPORAL_FILTER_TILE_SIZE, 1)]
void ValidateHistory(uint3 dispatchThreadId : SV_DispatchThreadID, uint2 groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    ConstantBuffer<FrameUniforms> frameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<TemporalFilterStruct> temporalFilterStruct = ResourceDescriptorHeap[temporalFilterStructIndex];
    
    Texture2D<float> _DepthTexture = ResourceDescriptorHeap[minDepthBufferIndex];
    Texture2D<float> _HistoryDepthTexture = ResourceDescriptorHeap[historyDepthTextureIndex];
    Texture2D<float4> _CameraMotionVectorsTexture = ResourceDescriptorHeap[cameraMotionVectorIndex];
    Texture2D<float4> _NormalBufferTexture = ResourceDescriptorHeap[normalBufferIndex];
    Texture2D<float4> _HistoryNormalTexture = ResourceDescriptorHeap[historyNormalTextureIndex];
    
    RWTexture2D<uint> _ValidationBufferRW = ResourceDescriptorHeap[validationBufferRWIndex];

    float4 _ScreenSize = frameUniforms.baseUniform._ScreenSize;

    float4 _HistorySizeAndScale = temporalFilterStruct._HistorySizeAndScale;
    float _HistoryValidity = temporalFilterStruct._HistoryValidity;
    float _PixelSpreadAngleTangent = temporalFilterStruct._PixelSpreadAngleTangent;
    
    // Fetch the current pixel coordinates
    uint2 centerCoord = groupId * TEMPORAL_FILTER_TILE_SIZE + groupThreadId;

    // Get the posinputs of the current version of the pixel
    float depth = LOAD_TEXTURE2D(_DepthTexture, centerCoord).r;
    PositionInputs posInputs = GetPositionInput(centerCoord, _ScreenSize.zw, depth,
        UNITY_MATRIX_I_VP(frameUniforms), GetWorldToViewMatrix(frameUniforms));

    // Initialize the history validation result
    uint historyRejectionResult = 0;

    // If the current point we are processing is a background point or the whole history should be discarded for an other reason, we invalidate the history
    if (depth == UNITY_RAW_FAR_CLIP_VALUE || _HistoryValidity == 0.0f)
    {
        _ValidationBufferRW[centerCoord] = HISTORYREJECTIONFLAGS_COMBINED;
        return;
    }

    // Decode the velocity of the pixel
    float2 velocity = float2(0.0, 0.0);
    DecodeMotionVector(LOAD_TEXTURE2D(_CameraMotionVectorsTexture, (float2)centerCoord), velocity);

    // Compute the pixel coordinate for the history tapping
    float2 historyUVUnscaled = posInputs.positionNDC - velocity;
    float2 historyTapCoord = historyUVUnscaled * _HistorySizeAndScale.xy;
    float2 historyUV = historyUVUnscaled * _HistorySizeAndScale.zw;

    // If the pixel was outside of the screen during the previous frame, invalidate the history
    if (any(historyTapCoord >= _HistorySizeAndScale.xy) || any(historyTapCoord < 0))
    {
        _ValidationBufferRW[centerCoord] = HISTORYREJECTIONFLAGS_REPROJECTION | HISTORYREJECTIONFLAGS_PREVIOUS_DEPTH | HISTORYREJECTIONFLAGS_POSITION | HISTORYREJECTIONFLAGS_NORMAL | HISTORYREJECTIONFLAGS_MOTION;
        return;
    }

    // Fetch the depth of the history pixel. If the history position was a background point, invalidate the history
    float historyDepth = SAMPLE_TEXTURE2D_LOD(_HistoryDepthTexture, s_linear_clamp_sampler, historyUV, 0).r;
    if (historyDepth == UNITY_RAW_FAR_CLIP_VALUE)
    {
        _ValidationBufferRW[centerCoord] = HISTORYREJECTIONFLAGS_PREVIOUS_DEPTH | HISTORYREJECTIONFLAGS_POSITION | HISTORYREJECTIONFLAGS_NORMAL | HISTORYREJECTIONFLAGS_MOTION;
        return;
    }

    // Real the normal data for this pixel
    NormalData normalData;
    DecodeFromNormalBuffer(_NormalBufferTexture, centerCoord, normalData);

    // Compute the world space position (from previous frame)
    float3 historyPositionWS = ComputeWorldSpacePosition(historyUVUnscaled, historyDepth, UNITY_MATRIX_PREV_I_VP(frameUniforms));

    // Compute the max reprojection distance. This is evaluated as the max between a fixed radius value and an approximation of the footprint of the pixel.
    float maxRadius = ComputeMaxReprojectionWorldRadius(frameUniforms, posInputs.positionWS, normalData.normalWS, _PixelSpreadAngleTangent);

    // Is it too far from the current position?
    if (length(historyPositionWS - posInputs.positionWS) > maxRadius)
        historyRejectionResult = historyRejectionResult | HISTORYREJECTIONFLAGS_POSITION;

    // Compute the world space normal (from previous frame)
    float4 historyNormal = LOAD_TEXTURE2D(_HistoryNormalTexture, historyTapCoord);
    NormalData historyNormalData;
    DecodeFromNormalBuffer(historyNormal, historyNormalData);

    // If the current normal is too different from the previous one, discard the history.
    if (dot(normalData.normalWS, historyNormalData.normalWS) < MAX_NORMAL_DIFFERENCE)
        historyRejectionResult = historyRejectionResult | HISTORYREJECTIONFLAGS_NORMAL;

    // // Was the object of this pixel moving?
    // uint stencilValue = GetStencilValue(LOAD_TEXTURE2D(_StencilTexture, centerCoord));
    // if ((stencilValue & _ObjectMotionStencilBit) != 0)
    //     historyRejectionResult = historyRejectionResult | HISTORYREJECTIONFLAGS_MOTION;

    // If none of the previous conditions have failed, the the history is valid
    _ValidationBufferRW[centerCoord] = historyRejectionResult;
}

#endif

// #define TEMPORALACCUMULATIONSINGLE
// #define SINGLE_CHANNEL 1

#if defined(TEMPORALACCUMULATIONSINGLE)

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint temporalFilterStructIndex;
    uint validationBufferIndex; // Validation buffer that tells us if the history should be ignored for a given pixel.
    uint cameraMotionVectorIndex; // Velocity buffer for history rejection
    uint minDepthBufferIndex; // Depth buffer of the current frame
    uint historyBufferIndex; // This buffer holds the previously accumulated signal
    uint denoiseInputTextureIndex; // Noisy Input Buffer from the current frame
    uint accumulationOutputTextureRWIndex; // Generic output buffer for our kernels
};

[numthreads(TEMPORAL_FILTER_TILE_SIZE, TEMPORAL_FILTER_TILE_SIZE, 1)]
void TemporalAccumulation(uint3 dispatchThreadId : SV_DispatchThreadID, uint2 groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    ConstantBuffer<FrameUniforms> frameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<TemporalFilterStruct> temporalFilterStruct = ResourceDescriptorHeap[temporalFilterStructIndex];
    Texture2D<uint> _ValidationBuffer = ResourceDescriptorHeap[validationBufferIndex];
    Texture2D<float4> _CameraMotionVectorsTexture = ResourceDescriptorHeap[cameraMotionVectorIndex];
    Texture2D<float> _DepthTexture = ResourceDescriptorHeap[minDepthBufferIndex];
    Texture2D<float4> _HistoryBuffer = ResourceDescriptorHeap[historyBufferIndex];
    Texture2D<float4> _DenoiseInputTexture = ResourceDescriptorHeap[denoiseInputTextureIndex];
    RWTexture2D<float4> _AccumulationOutputTextureRW = ResourceDescriptorHeap[accumulationOutputTextureRWIndex];

    float4 _ScreenSize = frameUniforms.baseUniform._ScreenSize;

    int _EnableExposureControl = temporalFilterStruct._EnableExposureControl;
    float4 _HistorySizeAndScale = temporalFilterStruct._HistorySizeAndScale;
    float4 _DenoiserResolutionMultiplierVals = temporalFilterStruct._DenoiserResolutionMultiplierVals;
    int _OccluderMotionRejection = temporalFilterStruct._OccluderMotionRejection;
    int _ReceiverMotionRejection = temporalFilterStruct._ReceiverMotionRejection;
    float _HistoryValidity = temporalFilterStruct._HistoryValidity;
    
    // Fetch the current pixel coordinate
    uint2 currentCoord = groupId * TEMPORAL_FILTER_TILE_SIZE + groupThreadId;
    uint2 sourceCoord = uint2(((float2)currentCoord) * _DenoiserResolutionMultiplierVals.yy);

    // If the depth of this pixel is the depth of the background, we can end the process right away
    float depth = LOAD_TEXTURE2D(_DepthTexture, sourceCoord).r;
    if (depth == UNITY_RAW_FAR_CLIP_VALUE)
    {
        _AccumulationOutputTextureRW[currentCoord] = float4(0.0, 0.0, 0.0, 0);
        return;
    }

    // Fetch the position of the current pixel
    PositionInputs posInputs = GetPositionInput(sourceCoord, _ScreenSize.zw, depth,
        UNITY_MATRIX_I_VP(frameUniforms), GetWorldToViewMatrix(frameUniforms));

    // Compute the velocity information for this pixel
    float2 velocity = float2(0.0, 0.0);
    DecodeMotionVector(LOAD_TEXTURE2D(_CameraMotionVectorsTexture, (float2)sourceCoord), velocity);

    // Compute the pixel coordinate for the history tapping
    float2 historyUVUnscaled = ((currentCoord + 0.5)/(_ScreenSize.xy * _DenoiserResolutionMultiplierVals.xx)) - velocity;
    float2 historyUV = historyUVUnscaled * _DenoiserResolutionMultiplierVals.zz *_HistorySizeAndScale.zw;

    // Fetch the current value, history value and current sample count
    #if SINGLE_CHANNEL
        float color = LOAD_TEXTURE2D(_DenoiseInputTexture, currentCoord).x;
        float2 history = SAMPLE_TEXTURE2D_LOD(_HistoryBuffer, s_linear_clamp_sampler, historyUV, 0).xy;
        float sampleCount = history.y;
    #else
        float3 color = LOAD_TEXTURE2D(_DenoiseInputTexture, currentCoord).xyz;
        float4 history = SAMPLE_TEXTURE2D_LOD(_HistoryBuffer, s_linear_clamp_sampler, historyUV, 0);
        history.xyz = max(history.xyz, 0);
        float sampleCount = history.w;
        if (_EnableExposureControl == 1)
        {
            // Grab the previous frame and current frame exposures
            float prevExposure = GetPreviousExposureMultiplier(frameUniforms);
            float currentExposure = GetCurrentExposureMultiplier(frameUniforms);

            // Compute the exposure ratio (while avoiding zeros)
            float expRatio = (prevExposure * currentExposure) != 0.0 ? currentExposure / prevExposure : 100.0;

            // Evaluate if the exposure multiplier was at least twice bigger or smaller
            bool validExposurechange = max(expRatio, 1.0 / expRatio) < 2.0;

            // If the exposure change was considered valid, we can keep the result and re-exposed it. Otherwise, we cannot use the history buffer
            if (validExposurechange)
                history.xyz = history.xyz * GetInversePreviousExposureMultiplier(frameUniforms) * currentExposure;
            else
                sampleCount = 0.0;
        }
    #endif

    // Get the velocity of the current sample
    float movingIntersection = _OccluderMotionRejection ? LOAD_TEXTURE2D(_CameraMotionVectorsTexture, currentCoord).r > MINIMAL_MOTION_DISTANCE : 0.0f;

    // Accumulation factor that tells us how much we need to keep the history data
    float accumulationFactor = 0.0;

    // Evaluate our validation mask
    float validationMask = (_ReceiverMotionRejection ? (LOAD_TEXTURE2D(_CameraMotionVectorsTexture, sourceCoord).x) : (LOAD_TEXTURE2D(_ValidationBuffer, sourceCoord).x & (~HISTORYREJECTIONFLAGS_MOTION))) != 0 ? 0.0f : 1.0f;

    // Combine the validation mask with the history validity
    bool historyInvalid = ((float)validationMask * _HistoryValidity) < 1.0f;

    // If the history is invalid or the history was flagged as moving (sampleCount == 0.0)
    if (historyInvalid || sampleCount == 0.0 || movingIntersection)
    {
        // We only take the current value
        accumulationFactor = 0.0;
        history = 0.0;
        // And the sample count of history becomes 1 (or 0 if the previous sample was mooving)
        sampleCount = movingIntersection ? 0.0 : 1.0;
    }
    else
    {
        // Otherwise we compute the accumulation factor
        accumulationFactor = sampleCount >= 8.0 ? 0.93 : (sampleCount / (sampleCount + 1.0));
        // Update the sample count
        sampleCount = min(sampleCount + 1.0, 8.0);
    }

    // Store our accumulated value
    #if SINGLE_CHANNEL
    _AccumulationOutputTextureRW[currentCoord] = float4(color * (1.0 - accumulationFactor) + history.x * accumulationFactor, sampleCount, 0.0, 1.0);
    #else
    _AccumulationOutputTextureRW[currentCoord] = float4(color * (1.0 - accumulationFactor) + history.xyz * accumulationFactor, sampleCount);
    #endif
}

#endif

// #define COPYHISTORY

#if defined(COPYHISTORY)

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint temporalFilterStructIndex;
    uint denoiseInputTextureIndex; // Noisy Input Buffer from the current frame
    uint denoiseOutputTextureRWIndex;
};

[numthreads(TEMPORAL_FILTER_TILE_SIZE, TEMPORAL_FILTER_TILE_SIZE, 1)]
void CopyHistory(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    ConstantBuffer<FrameUniforms> frameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<TemporalFilterStruct> temporalFilterStruct = ResourceDescriptorHeap[temporalFilterStructIndex];
    Texture2D<float4> _DenoiseInputTexture = ResourceDescriptorHeap[denoiseInputTextureIndex];
    RWTexture2D<float4> _DenoiseOutputTextureRW = ResourceDescriptorHeap[denoiseOutputTextureRWIndex];

    float4 _ScreenSize = frameUniforms.baseUniform._ScreenSize;

    float4 _DenoiserResolutionMultiplierVals = temporalFilterStruct._DenoiserResolutionMultiplierVals;
    
    if (any(dispatchThreadId.xy > (uint2)round((float2)_ScreenSize.xy * _DenoiserResolutionMultiplierVals.xx)))
        return;  // Out of bounds, discard

    _DenoiseOutputTextureRW[dispatchThreadId.xy] = LOAD_TEXTURE2D(_DenoiseInputTexture, dispatchThreadId.xy);
}

#endif


