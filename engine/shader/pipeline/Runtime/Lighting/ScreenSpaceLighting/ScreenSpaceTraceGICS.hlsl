
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

struct ScreenSpaceTraceGIStruct
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
};

cbuffer RootConstants : register(b0, space0)
{
    uint frameUniformIndex;
    uint depthPyramidTextureIndex;
    uint normalBufferIndex;
    uint screenSpaceTraceGIStructIndex;
    uint indirectDiffuseHitPointTextureRWIndex;

    uint _OwenScrambledTextureIndex;
    uint _ScramblingTileXSPPIndex;
    uint _RankingTileXSPPIndex;
};

#include "../../Lighting/ScreenSpaceLighting/RayMarching.hlsl"

[numthreads(INDIRECT_DIFFUSE_TILE_SIZE, INDIRECT_DIFFUSE_TILE_SIZE, 1)]
void TRACE_GLOBAL_ILLUMINATION(uint3 dispatchThreadId : SV_DispatchThreadID, uint2 groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    Texture2D<float> _DepthPyramidTexture = ResourceDescriptorHeap[depthPyramidTextureIndex];
    Texture2D<float4> _NormalBufferTexture = ResourceDescriptorHeap[normalBufferIndex];
    RWTexture2D<float2> _IndirectDiffuseHitPointTextureRW = ResourceDescriptorHeap[indirectDiffuseHitPointTextureRWIndex];

    Texture2D<float> _OwenScrambledTexture = ResourceDescriptorHeap[_OwenScrambledTextureIndex];
    Texture2D<float> _ScramblingTileXSPP = ResourceDescriptorHeap[_ScramblingTileXSPPIndex];
    Texture2D<float> _RankingTileXSPP = ResourceDescriptorHeap[_RankingTileXSPPIndex];

    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[frameUniformIndex];
    float4 _ScreenSize = frameUniform.baseUniform._ScreenSize;
    
    ConstantBuffer<ScreenSpaceTraceGIStruct> _ScreenSpaceTraceGIStruct = ResourceDescriptorHeap[screenSpaceTraceGIStructIndex];
    int _IndirectDiffuseFrameIndex = _ScreenSpaceTraceGIStruct._IndirectDiffuseFrameIndex;
    bool _RayMarchingReflectsSky = _ScreenSpaceTraceGIStruct._RayMarchingReflectsSky;
    int _RayMarchingSteps = _ScreenSpaceTraceGIStruct._RayMarchingSteps;
    float _RayMarchingThicknessScale = _ScreenSpaceTraceGIStruct._RayMarchingThicknessScale;
    float _RayMarchingThicknessBias = _ScreenSpaceTraceGIStruct._RayMarchingThicknessBias;
    
    // Compute the pixel position to process
    uint2 currentCoord = dispatchThreadId.xy;
    uint2 inputCoord = dispatchThreadId.xy;
    
    // Read the depth value as early as possible
    float deviceDepth = LOAD_TEXTURE2D(_DepthPyramidTexture, inputCoord, 0).x;

    // Initialize the hitpoint texture to a miss
    _IndirectDiffuseHitPointTextureRW[currentCoord.xy] = float2(99.0, 0.0);

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

    // Compute the camera position
    float3 camPosWS = GetCurrentViewPosition(frameUniform);

    // If this is a background pixel, we flag the ray as a dead ray (we are also trying to keep the usage of the depth buffer the latest possible)
    bool killRay = deviceDepth == UNITY_RAW_FAR_CLIP_VALUE;
    // Convert this to a world space position (camera relative)
    PositionInputs posInput = GetPositionInput(inputCoord, _ScreenSize.zw, deviceDepth,
        UNITY_MATRIX_I_VP(frameUniform), GetWorldToViewMatrix(frameUniform), 0);

    // Compute the view direction (world space)
    float3 viewWS = GetWorldSpaceNormalizeViewDir(frameUniform, posInput.positionWS);

    // Apply normal bias with the magnitude dependent on the distance from the camera.
    // Unfortunately, we only have access to the shading normal, which is less than ideal...
    posInput.positionWS  = camPosWS + (posInput.positionWS - camPosWS) * (1 - 0.001 * rcp(max(dot(normalData.normalWS, viewWS), FLT_EPS)));
    deviceDepth = ComputeNormalizedDeviceCoordinatesWithZ(posInput.positionWS, UNITY_MATRIX_VP(frameUniform)).z;

    // Ray March along our ray
    float3 rayPos;
    bool hit = RayMarch(frameUniform, _DepthPyramidTexture,
        _RayMarchingReflectsSky, _RayMarchingSteps, _RayMarchingThicknessScale, _RayMarchingThicknessBias,
        posInput.positionWS, sampleDir, normalData.normalWS, posInput.positionSS, deviceDepth, killRay, rayPos);

    // If we had a hit, store the NDC position of the intersection point
    if (hit)
    {
        // Note that we are using 'rayPos' from the penultimate iteration, rather than
        // recompute it using the last value of 't', which would result in an overshoot.
        // It also needs to be precisely at the center of the pixel to avoid artifacts.
        float2 hitPositionNDC = floor(rayPos.xy) * _ScreenSize.zw + (0.5 * _ScreenSize.zw); // Should we precompute the half-texel bias? We seem to use it a lot.
        _IndirectDiffuseHitPointTextureRW[currentCoord.xy] = hitPositionNDC;
    }
}











