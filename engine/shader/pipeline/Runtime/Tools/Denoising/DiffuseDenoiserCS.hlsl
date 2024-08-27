// We need the stencil flag of this.
#define BILATERLAL_UNLIT

// #pragma enable_d3d11_debug_symbols

// Common includes
#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/CommonLighting.hlsl"
#include "../../ShaderLibrary/Sampling/Sampling.hlsl"
#include "../../ShaderLibrary/Color.hlsl"

// HDRP includes
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Material/NormalBuffer.hlsl"
#include "../../Material/Builtin/BuiltinData.hlsl"
#include "../../Tools/Denoising/DenoisingUtils.hlsl"

// Ray Tracing includes
#include "../../RenderPipeline/Raytracing/Shaders/RaytracingSampling.hlsl"
#include "../../Tools/Denoising/BilateralFilter.hlsl"
#include "../../Lighting/ScreenSpaceLighting/BilateralUpsample.hlsl"

// Tile size of this compute shaders
#define DIFFUSE_DENOISER_TILE_SIZE 8

#define PIXEL_RADIUS_TOLERANCE_THRESHOLD 2

// #define GENERATEPOINTDISTRIBUTION

#ifdef GENERATE_POINT_DISTRIBUTION

cbuffer RootConstants : register(b0, space0)
{
    uint OwenScrambledTextureIndex;
    uint PointDistributionRWIndex;
}

[numthreads(64, 1, 1)]
void GeneratePointDistribution(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    Texture2D<float> _OwenScrambledTexture = ResourceDescriptorHeap[OwenScrambledTextureIndex];
    RWStructuredBuffer<float2> _PointDistributionRW = ResourceDescriptorHeap[PointDistributionRWIndex];
    
    _PointDistributionRW[dispatchThreadId.x] = SampleDiskCubic(
        GetLDSequenceSampleFloat(_OwenScrambledTexture, dispatchThreadId.x, 0),
        GetLDSequenceSampleFloat(_OwenScrambledTexture, dispatchThreadId.x, 1));
}

#endif

// #define BILATERAL_FILTER


struct BilateralFilterStruct
{
    // Radius of the filter (world space)
    float4 _DenoiserResolutionMultiplierVals;
    float _DenoiserFilterRadius;
    float _PixelSpreadAngleTangent;
    int _JitterFramePeriod;
    // Flag used to do a half resolution filter
    int _HalfResolutionFilter;
};

#ifdef BILATERAL_FILTER

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint bilateralFilterStructIndex;
    uint depthTextureIndex;
    uint normalTextureIndex;
    uint OwenScrambledTextureIndex;
    uint pointDistributionIndex;
    uint denoiseInputTextureIndex;
    uint denoiseOutputTextureRWIndex;
}

float ComputeMaxDenoisingRadius(float denoiserFilterRadius, float3 positionRWS)
{
    // Compute the distance to the pixel
    float distanceToPoint = length(positionRWS);
    // This is purely empirical, values were obtained  while experimenting with various scenes and these valuesgive good visual results.
    // The world space radius for sample picking goes from distance/10.0 to distance/50.0 linearly until reaching 500.0 meters away from the camera
    // and it is always 20.0f (or two pixels if subpixel.
    // TODO: @Anis, I have a bunch of idea how to make this better and less empirical but it's for any other day
    return distanceToPoint * denoiserFilterRadius / lerp(5.0, 50.0, saturate(distanceToPoint / 500.0));
}

[numthreads(DIFFUSE_DENOISER_TILE_SIZE, DIFFUSE_DENOISER_TILE_SIZE, 1)]
void BilateralFilter(uint3 dispatchThreadId : SV_DispatchThreadID, uint2 groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    ConstantBuffer<FrameUniforms> frameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<BilateralFilterStruct> temporalFilterStruct = ResourceDescriptorHeap[bilateralFilterStructIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[depthTextureIndex];
    Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalTextureIndex];
    StructuredBuffer<float2> _PointDistribution = ResourceDescriptorHeap[pointDistributionIndex];
    Texture2D<float4> _DenoiseInputTexture = ResourceDescriptorHeap[denoiseInputTextureIndex];
    RWTexture2D<float4> _DenoiseOutputTextureRW = ResourceDescriptorHeap[denoiseOutputTextureRWIndex];

    float4 _DenoiserResolutionMultiplierVals = temporalFilterStruct._DenoiserResolutionMultiplierVals;
    float _DenoiserFilterRadius = temporalFilterStruct._DenoiserFilterRadius;
    float _PixelSpreadAngleTangent = temporalFilterStruct._PixelSpreadAngleTangent;
    int _JitterFramePeriod = temporalFilterStruct._JitterFramePeriod;
    
    int _HalfResolutionFilter = temporalFilterStruct._HalfResolutionFilter;
    
    // Fetch the current pixel coordinate
    uint2 currentCoord = groupId * DIFFUSE_DENOISER_TILE_SIZE + groupThreadId;
    uint2 sourceCoord = (uint2)(currentCoord * _DenoiserResolutionMultiplierVals.y);

    // Read the central position
    const BilateralData center = TapBilateralData(frameUniforms, depthTexture, normalTexture, sourceCoord);

    // If this is a background pixel, we are done
    if (center.z01 == 1.0 || center.isUnlit)
    {
        #if SINGLE_CHANNEL
        _DenoiseOutputTextureRW[currentCoord] = 0.0;
        #else
        _DenoiseOutputTextureRW[currentCoord] = float4(0.0, 0.0, 0.0, 1.0);
        #endif
        return;
    }

    // Create the local ortho basis for our sampling
    float3x3 localToWorld = GetLocalFrame(center.normal);

    // Intialize the accumulation values
    #if SINGLE_CHANNEL
    float colorSum = 0.0;
    float wSum = 0.0;
    #else
    float3 colorSum = 0.0;
    float wSum = 0.0;
    #endif

    // Compute the radius of the filter. This is evaluated as the max between a fixed radius value and an approximation of the footprint of the pixel
    const float denoisingRadius = ComputeMaxReprojectionWorldRadius(frameUniforms, center.position, center.normal,
        _PixelSpreadAngleTangent, ComputeMaxDenoisingRadius(_DenoiserFilterRadius, center.position), PIXEL_RADIUS_TOLERANCE_THRESHOLD);

    // Compute the sigma value
    const float sigma = 0.9 * denoisingRadius;

    // Index of the pixel in the 2x2 group that are used for the half res filter
    int localIndex = (currentCoord.x & 1) + (currentCoord.y & 1) * 2;

    // Define the sample count for this pixel. 16 samples per pixels if it is a full res or 4 if half resolution
    const int numSamples = _HalfResolutionFilter ? 4 : 16;

    int sampleOffset = (_HalfResolutionFilter != 0 ? localIndex * numSamples : 0);
    if (_JitterFramePeriod != -1)
        sampleOffset += _JitterFramePeriod * 16;

    // Loop through the samples that we need to aggrgate
    for (uint sampleIndex = 0; sampleIndex < (uint)numSamples; ++sampleIndex)
    {
        // Fetch the noise value for the current sample
        float2 newSample = _PointDistribution[sampleIndex + sampleOffset] * denoisingRadius;

        // Convert the point to hemogenous clip space
        float3 wsPos = center.position + localToWorld[0] * newSample.x + localToWorld[1] * newSample.y;
        float4 hClip = TransformWorldToHClip(frameUniforms, wsPos);
        hClip.xyz /= hClip.w;

        // Is the target pixel in the screen?
        if (hClip.x > 1.0 || hClip.x < -1.0 || hClip.y > 1.0 || hClip.y < -1.0)
            continue;

        // Convert it to screen sample space
        float2 nDC = hClip.xy * 0.5 + 0.5;
    #if UNITY_UV_STARTS_AT_TOP
        nDC.y = 1.0 - nDC.y;
    #endif

        float4 _ScreenSize = frameUniforms.baseUniform._ScreenSize;
        
        // Tap the data for this pixel
        // Not all pixels can be fetched (only the 2x2 representative)
        uint2 tapCoord = (nDC * _ScreenSize.xy);
        uint2 lowResTapCoord = (tapCoord) * _DenoiserResolutionMultiplierVals.x;

        // Fetch the corresponding data
        const BilateralData tapData = TapBilateralData(frameUniforms, depthTexture, normalTexture, tapCoord);

        // If the tapped pixel is a background pixel or too far from the center pixel
        if (tapData.z01 == UNITY_RAW_FAR_CLIP_VALUE || tapData.isUnlit || abs(tapData.zNF - hClip.w) > 0.1)
            continue;

        // Compute the radius of the sample
        float r = length(newSample);

        // Compute the weight (skip computation for the center)
        const float w = r > 0.001f ? gaussian(r, sigma) * ComputeBilateralWeight(center, tapData) : 1.0;

        // Accumulate the new sample
    #if SINGLE_CHANNEL
        colorSum += LOAD_TEXTURE2D(_DenoiseInputTexture, lowResTapCoord).x * w;
    #else
        colorSum += LOAD_TEXTURE2D(_DenoiseInputTexture, lowResTapCoord).xyz * w;
    #endif
        wSum += w;
    }

    // If no samples were found, we take the center pixel only
    if (wSum == 0.0)
    {
        #if SINGLE_CHANNEL
        colorSum += LOAD_TEXTURE2D(_DenoiseInputTexture, currentCoord).x;
        #else
        colorSum += LOAD_TEXTURE2D(_DenoiseInputTexture, currentCoord).xyz;
        #endif
        wSum += 1.0;
    }

    // Normalize the result
    #if SINGLE_CHANNEL
    _DenoiseOutputTextureRW[currentCoord] = colorSum / wSum;
    #else
    _DenoiseOutputTextureRW[currentCoord] = float4(colorSum / wSum, 1.0);
    #endif
}

#endif

// #define GATHER_FILTER

#ifdef GATHER_FILTER

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint gatherFilterStructIndex;
    uint depthTextureIndex;
    uint denoiseInputTextureIndex;
    uint denoiseOutputTextureRWIndex;
}

#define GATHER_REGION_SIZE DIFFUSE_DENOISER_TILE_SIZE
#define GATHER_REGION_SIZE_2 (GATHER_REGION_SIZE * GATHER_REGION_SIZE)
groupshared uint gs_cacheLighting[GATHER_REGION_SIZE_2];
groupshared float gs_cacheLuminance[GATHER_REGION_SIZE_2];
groupshared float gs_cacheDepth[GATHER_REGION_SIZE_2];

void FillGatherDataLDS(
    Texture2D<float4> denoiseInputTexture, Texture2D<float> depthTexture,
    float4 denoiserResolutionMultiplierVals, float4 screenSize, uint groupIndex, uint2 pixelCoord)
{
    int2 sampleCoord = int2(clamp(pixelCoord.x, 0, screenSize.x - 1), clamp(pixelCoord.y, 0, screenSize.y - 1));
    #ifdef SINGLE_CHANNEL
    gs_cacheLuminance[groupIndex] = LOAD_TEXTURE2D_X(_DenoiseInputTexture, sampleCoord).x;
    #else
    float3 lighting = LOAD_TEXTURE2D(denoiseInputTexture, sampleCoord).xyz;
    gs_cacheLighting[groupIndex] = PackToR11G11B10f(lighting);
    #endif

    float depthValue = LOAD_TEXTURE2D(depthTexture, sampleCoord * denoiserResolutionMultiplierVals.y).x;
    gs_cacheDepth[groupIndex] = depthValue;
}

uint OffsetToLDSAdress(uint2 groupThreadId, int2 offset)
{
    // Compute the tap coordinate in the 8x8 grid
    uint2 tapAddress = (uint2)((int2)(groupThreadId) + offset);
    return clamp(tapAddress.x + tapAddress.y * GATHER_REGION_SIZE, 0, GATHER_REGION_SIZE_2 - 1);
}

[numthreads(DIFFUSE_DENOISER_TILE_SIZE, DIFFUSE_DENOISER_TILE_SIZE, 1)]
void GatherFilter(uint3 centerCoord : SV_DispatchThreadID, int groupIndex : SV_GroupIndex, uint2 groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    ConstantBuffer<FrameUniforms> frameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<BilateralFilterStruct> gatherFilterStruct = ResourceDescriptorHeap[gatherFilterStructIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[depthTextureIndex];
    Texture2D<float4> denoiseInputTexture = ResourceDescriptorHeap[denoiseInputTextureIndex];
    
    RWTexture2D<float4> _DenoiseOutputTextureRW = ResourceDescriptorHeap[denoiseOutputTextureRWIndex];

    float4 _DenoiserResolutionMultiplierVals = gatherFilterStruct._DenoiserResolutionMultiplierVals;
    float _DenoiserFilterRadius = gatherFilterStruct._DenoiserFilterRadius;
    float _PixelSpreadAngleTangent = gatherFilterStruct._PixelSpreadAngleTangent;
    int _JitterFramePeriod = gatherFilterStruct._JitterFramePeriod;
    
    // Fill color and lighting to the LDS
    FillGatherDataLDS(denoiseInputTexture, depthTexture, _DenoiserResolutionMultiplierVals, groupIndex, centerCoord.xy);

    // Make sure all values are loaded in LDS by now.
    GroupMemoryBarrierWithGroupSync();

    // Read the high res depth
    int outputIdx = OffsetToLDSAdress(groupThreadId, int2(0, 0));
    float targetDepth = gs_cacheDepth[outputIdx];

    // Compute the 2x2 pixelregioncorner
    uint2 corner = centerCoord.xy - uint2(centerCoord.x & 1, centerCoord.y & 1);
    uint2 cornerGroupThread = corner - groupId * DIFFUSE_DENOISER_TILE_SIZE;

    // Grab the indices of the sub-region to use
    int ldsIdx0 = OffsetToLDSAdress(cornerGroupThread, int2(0, 0));
    int ldsIdx1 = OffsetToLDSAdress(cornerGroupThread, int2(1, 0));
    int ldsIdx2 = OffsetToLDSAdress(cornerGroupThread, int2(0, 1));
    int ldsIdx3 = OffsetToLDSAdress(cornerGroupThread, int2(1, 1));
    float4 lowDepths = float4(gs_cacheDepth[ldsIdx0], gs_cacheDepth[ldsIdx1], gs_cacheDepth[ldsIdx2], gs_cacheDepth[ldsIdx3]);

    #if SINGLE_CHANNEL
    float value = BilUpSingle_Uniform(targetDepth, lowDepths, float4(gs_cacheLuminance[ldsIdx0], gs_cacheLuminance[ldsIdx1], gs_cacheLuminance[ldsIdx2], gs_cacheLuminance[ldsIdx3]));
    _DenoiseOutputTextureRW[COORD_TEXTURE2D_X(centerCoord.xy)] = value;
    #else
    _DenoiseOutputTextureRW[centerCoord.xy] = float4(
        BilUpColor3_Uniform(targetDepth, lowDepths,
        UnpackFromR11G11B10f(gs_cacheLighting[ldsIdx0]),
        UnpackFromR11G11B10f(gs_cacheLighting[ldsIdx1]),
        UnpackFromR11G11B10f(gs_cacheLighting[ldsIdx2]),
        UnpackFromR11G11B10f(gs_cacheLighting[ldsIdx3])), 1.0);
    #endif
}

#endif