#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/Color.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Lighting/Lighting.hlsl"
#include "../../Material/Material.hlsl"
#include "../../Material/NormalBuffer.hlsl"
#include "../../Lighting/LightLoop/LightLoopDef.hlsl"
#include "../../Material/BuiltinGIUtilities.hlsl"
#include "../../Material/MaterialEvaluation.hlsl"
#include "../../Lighting/LightEvaluation.hlsl"

// Raytracing includes (should probably be in generic files)
#include "../../RenderPipeline/Raytracing/Shaders/RaytracingSampling.hlsl"
#include "../../RenderPipeline/Raytracing/Shaders/RayTracingCommon.hlsl"

// The dispatch tile resolution
#define INDIRECT_DIFFUSE_TILE_SIZE 8

// Defines the mip offset for the color buffer
#define SSGI_MIP_OFFSET 1

#define SSGI_CLAMP_VALUE 7.0f

struct ScreenSpaceReprojectGIStruct
{
    // Ray marching constants
    int _RayMarchingSteps;
    float _RayMarchingThicknessScale;
    float _RayMarchingThicknessBias;
    int _RayMarchingReflectsSky;

    int _RayMarchingFallbackHierarchy;
    int _IndirectDiffuseProbeFallbackFlag;
    int _IndirectDiffuseProbeFallbackBias;
    int _SsrStencilBit;

    int _IndirectDiffuseFrameIndex;
    int _ObjectMotionStencilBit;
    float _RayMarchingLowResPercentageInv;
    int _SSGIUnused0;

    float4 _ColorPyramidUvScaleAndLimitPrevFrame;
};

cbuffer RootConstants : register(b0, space0)
{
    uint frameUniformIndex;
    uint colorPyramidTextureIndex;
    uint depthPyramidTextureIndex;
    uint normalBufferIndex;
    uint screenSpaceReprojectGIStructIndex;
    uint _IndirectDiffuseHitPointTextureIndex;
    uint _CameraMotionVectorsTextureIndex;
    uint _HistoryDepthTextureIndex;
    uint _IndirectDiffuseTextureRWIndex;

    uint _OwenScrambledTextureIndex;
    uint _ScramblingTileXSPPIndex;
    uint _RankingTileXSPPIndex;
};

SamplerState s_point_clamp_sampler      : register(s10);
SamplerState s_linear_clamp_sampler     : register(s11);

// The maximal difference in depth that is considered acceptable to read from the color pyramid
#define DEPTH_DIFFERENCE_THRESHOLD 0.1

[numthreads(INDIRECT_DIFFUSE_TILE_SIZE, INDIRECT_DIFFUSE_TILE_SIZE, 1)]
void REPROJECT_GLOBAL_ILLUMINATION(uint3 dispatchThreadId : SV_DispatchThreadID, uint2 groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    Texture2D<float4> _ColorPyramidTexture = ResourceDescriptorHeap[colorPyramidTextureIndex];
    Texture2D<float> _DepthPyramidTexture = ResourceDescriptorHeap[depthPyramidTextureIndex];
    Texture2D<float4> _NormalBufferTexture = ResourceDescriptorHeap[normalBufferIndex];
    Texture2D<float4> _IndirectDiffuseHitPointTexture = ResourceDescriptorHeap[_IndirectDiffuseHitPointTextureIndex];
    Texture2D<float4> _CameraMotionVectorsTexture = ResourceDescriptorHeap[_CameraMotionVectorsTextureIndex];
    Texture2D<float> _HistoryDepthTexture = ResourceDescriptorHeap[_HistoryDepthTextureIndex];
    RWTexture2D<float4> _IndirectDiffuseTextureRW = ResourceDescriptorHeap[_IndirectDiffuseTextureRWIndex];

    Texture2D<float> _OwenScrambledTexture = ResourceDescriptorHeap[_OwenScrambledTextureIndex];
    Texture2D<float> _ScramblingTileXSPP = ResourceDescriptorHeap[_ScramblingTileXSPPIndex];
    Texture2D<float> _RankingTileXSPP = ResourceDescriptorHeap[_RankingTileXSPPIndex];

    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[frameUniformIndex];
    float4 _ScreenSize = frameUniform.baseUniform._ScreenSize;
    
    ConstantBuffer<ScreenSpaceReprojectGIStruct> _ScreenSpaceReprojectGIStruct = ResourceDescriptorHeap[screenSpaceReprojectGIStructIndex];
    int _IndirectDiffuseFrameIndex = _ScreenSpaceReprojectGIStruct._IndirectDiffuseFrameIndex;
    // float _RayMarchingLowResPercentageInv = _ScreenSpaceReprojectGIStruct._RayMarchingLowResPercentageInv;
    float4 _ColorPyramidUvScaleAndLimitPrevFrame = _ScreenSpaceReprojectGIStruct._ColorPyramidUvScaleAndLimitPrevFrame;
    
    // Compute the pixel position to process
    uint2 inputCoord = dispatchThreadId.xy;
    uint2 currentCoord = dispatchThreadId.xy;

    // Read the depth and compute the position
    float deviceDepth = LOAD_TEXTURE2D(_DepthPyramidTexture, inputCoord).x;
    PositionInputs posInput = GetPositionInput(inputCoord, _ScreenSize.zw, deviceDepth,
        UNITY_MATRIX_I_VP(frameUniform), GetWorldToViewMatrix(frameUniform));

    // Read the pixel normal
    NormalData normalData;
    DecodeFromNormalBuffer(_NormalBufferTexture, inputCoord.xy, normalData);

    // Generete a new direction to follow
    float2 newSample;
    newSample.x = GetBNDSequenceSample(_OwenScrambledTexture, _ScramblingTileXSPP, _RankingTileXSPP,
        currentCoord.xy, _IndirectDiffuseFrameIndex, 0);
    newSample.y = GetBNDSequenceSample(_OwenScrambledTexture, _ScramblingTileXSPP, _RankingTileXSPP,
        currentCoord.xy, _IndirectDiffuseFrameIndex, 1);

    // Importance sample with a cosine lobe (direction that will be used for ray casting)
    float3 sampleDir = SampleHemisphereCosine(newSample.x, newSample.y, normalData.normalWS);

    // Read the hit point ndc position to fetch
    float2 hitPositionNDC = LOAD_TEXTURE2D(_IndirectDiffuseHitPointTexture, dispatchThreadId.xy).xy;

    // Grab the depth of the hit point
    float hitPointDepth = LOAD_TEXTURE2D(_DepthPyramidTexture, hitPositionNDC * _ScreenSize.xy, 0).x;

    // Flag that tracks if this ray lead to a valid result
    bool invalid = false;

    // If this missed, we need to find something else to fallback on
    if (hitPositionNDC.x > 1.0)
        invalid = true;

    // Fetch the motion vector of the current target pixel
    float2 motionVectorNDC;
    DecodeMotionVector(SAMPLE_TEXTURE2D_LOD(_CameraMotionVectorsTexture, s_linear_clamp_sampler, hitPositionNDC, 0), motionVectorNDC);

    // // Was the object of this pixel moving?
    // uint stencilValue = GetStencilValue(LOAD_TEXTURE2D(_StencilTexture, hitPositionNDC * _ScreenSize.xy));
    // bool movingHitPoint = (stencilValue & _ObjectMotionStencilBit) != 0;

    float2 prevFrameNDC = hitPositionNDC - motionVectorNDC;
    float2 prevFrameUV  = prevFrameNDC * _ColorPyramidUvScaleAndLimitPrevFrame.xy;

    // If the previous value to read was out of screen, this is invalid, needs a fallback
    if ((prevFrameUV.x < 0)
        || (prevFrameUV.x > _ColorPyramidUvScaleAndLimitPrevFrame.z)
        || (prevFrameUV.y < 0)
        || (prevFrameUV.y > _ColorPyramidUvScaleAndLimitPrevFrame.w))
        invalid = true;

    // Grab the depth of the hit point and reject the history buffer if the depth is too different
    // TODO: Find a better metric
    float hitPointHistoryDepth = LOAD_TEXTURE2D(_HistoryDepthTexture, prevFrameNDC * _ScreenSize.xy).x;
    if (abs(hitPointHistoryDepth - hitPointDepth) > DEPTH_DIFFERENCE_THRESHOLD)
        invalid = true;

    // Based on if the intersection was valid (or not, pick a source for the lighting)
    float3 color = 0.0;
    if (!invalid)
    {
        // The intersection was considered valid, we can read from the color pyramid
        color = SAMPLE_TEXTURE2D_LOD(_ColorPyramidTexture, s_linear_clamp_sampler, prevFrameUV, SSGI_MIP_OFFSET).rgb;
        color *= GetInversePreviousExposureMultiplier(frameUniform);
    }
    
    // TODO: Remove me when you can find where the nans come from
    if (AnyIsNaN(color))
        color = 0.0f;

    // Convert to HSV space
    color = RgbToHsv(color * GetCurrentExposureMultiplier(frameUniform));
    // Expose and clamp the final color
    color.z = clamp(color.z, 0.0, SSGI_CLAMP_VALUE);
    // Convert back to HSV space
    color = HsvToRgb(color);

    // Write the output to the target pixel
    _IndirectDiffuseTextureRW[currentCoord] = color;
}
