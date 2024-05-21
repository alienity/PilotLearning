// =============== Convolves transmitted radiance with the Disney diffusion profile ================

//--------------------------------------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------------------------------------

// Tweak parameters.
#define SSS_BILATERAL_FILTER  1
#define SSS_USE_LDS_CACHE     1 // Use LDS as an L0 texture cache.
#define SSS_RANDOM_ROTATION   1 // Hides undersampling artifacts with high-frequency noise. TAA blurs the noise.
#define SSS_USE_TANGENT_PLANE 0 // Improves the accuracy of the approximation(0 -> 1st order). High cost. Does not work with back-facing normals.
#define SSS_CLAMP_ARTIFACT    0 // Reduces bleeding. Use with SSS_USE_TANGENT_PLANE.
#define SSS_DEBUG_LOD         0 // Displays the sampling rate: green = no filtering, blue = 1 sample, red = _SssSampleBudget samples.
#define SSS_DEBUG_NORMAL_VS   0 // Allows detection of back-facing normals.

// Do not modify these.
#define SHADERPASS            SHADERPASS_SUBSURFACE_SCATTERING
#define GROUP_SIZE_1D         16
#define GROUP_SIZE_2D         (GROUP_SIZE_1D * GROUP_SIZE_1D)
#define TEXTURE_CACHE_BORDER  2
#define TEXTURE_CACHE_SIZE_1D (GROUP_SIZE_1D + 2 * TEXTURE_CACHE_BORDER)
#define TEXTURE_CACHE_SIZE_2D (TEXTURE_CACHE_SIZE_1D * TEXTURE_CACHE_SIZE_1D)

//--------------------------------------------------------------------------------------------------
// Included headers
//--------------------------------------------------------------------------------------------------

#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ShadingParameters.hlsli"
#include "SubsurfaceScattering.hlsli"
#include "Fibonacci.hlsl"
#include "SpaceFillingCurves.hlsl"

//--------------------------------------------------------------------------------------------------
// Inputs & outputs
//--------------------------------------------------------------------------------------------------

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint depthTextureIndex; // Z-buffer
    uint irradianceSourceIndex; // Includes transmitted light
    uint sssTextureIndex; // SSSBuffer
    uint outCameraFilteringTextureIndex; // Target texture
};

SamplerState defaultSampler : register(s10);

struct SubsurfaceTexStruct
{
    Texture2D<float4> sssBufferTexture;
    Texture2D<float4> irradianceSource;
    Texture2D<float> depthTexture;
    RWTexture2D<float4> cameraFilteringTexture;
};

//--------------------------------------------------------------------------------------------------
// Implementation
//--------------------------------------------------------------------------------------------------

// 6656 bytes used. It appears that the reserved LDS space must be a multiple of 512 bytes.
#if SSS_USE_LDS_CACHE
groupshared float2 textureCache0[TEXTURE_CACHE_SIZE_2D]; // {irradiance.rg}
groupshared float2 textureCache1[TEXTURE_CACHE_SIZE_2D]; // {irradiance.b, deviceDepth}
#endif
//groupshared bool   processGroup;

#if SSS_USE_LDS_CACHE
void StoreSampleToCacheMemory(float4 value, int2 cacheCoord)
{
    int linearCoord = Mad24(TEXTURE_CACHE_SIZE_1D, cacheCoord.y, cacheCoord.x);

    textureCache0[linearCoord] = value.rg;
    textureCache1[linearCoord] = value.ba;
}

float4 LoadSampleFromCacheMemory(int2 cacheCoord)
{
    int linearCoord = Mad24(TEXTURE_CACHE_SIZE_1D, cacheCoord.y, cacheCoord.x);

    return float4(textureCache0[linearCoord],
                  textureCache1[linearCoord]);
}
#endif

float4 LoadSampleFromVideoMemory(int2 pixelCoord, SubsurfaceTexStruct subsurfaceStruct)
{
    float3 irradiance = subsurfaceStruct.irradianceSource.Load(int3(pixelCoord, 0)).rgb;
    float depth = subsurfaceStruct.depthTexture.Load(int3(pixelCoord, 0)).r;
    
    return float4(irradiance, depth);
}

// Returns {irradiance, linearDepth}.
float4 LoadSample(int2 pixelCoord, int2 cacheOffset, FrameUniforms framUniform, SubsurfaceTexStruct subsurfaceStruct)
{
    float4 value;

#if SSS_USE_LDS_CACHE
    int2 cacheCoord = pixelCoord - cacheOffset;
    bool isInCache  = max((uint)cacheCoord.x, (uint)cacheCoord.y) < TEXTURE_CACHE_SIZE_1D;

    if (isInCache)
    {
        value = LoadSampleFromCacheMemory(cacheCoord);
    }
    else
#endif
    {
        // Always load both irradiance and depth.
        // Avoid dependent texture reads at the cost of extra bandwidth.
        value = LoadSampleFromVideoMemory(pixelCoord, subsurfaceStruct);
    }

    float4 zBufferParams = framUniform.cameraUniform.curFrameUniform.zBufferParams;
    value.a = LinearEyeDepth(value.a, zBufferParams);
    
    return value;
}

// Computes f(r, s)/p(r, s), s.t. r = sqrt(xy^2 + z^2).
// Rescaling of the PDF is handled by 'totalWeight'.
float3 ComputeBilateralWeight(float xy2, float z, float mmPerUnit, float3 S, float rcpPdf)
{
#if (SSS_BILATERAL_FILTER == 0)
    z = 0;
#endif

    // Note: we perform all computation in millimeters.
    // So we must convert from world units (using 'mmPerUnit') to millimeters.
#if SSS_USE_TANGENT_PLANE
    // Both 'xy2' and 'z' require conversion to millimeters.
    float r = sqrt(xy2 + z * z) * mmPerUnit;
    float p = sqrt(xy2) * mmPerUnit;
#else
    // Only 'z' requires conversion to millimeters.
    float r = sqrt(xy2 + (z * mmPerUnit) * (z * mmPerUnit));
    float p = sqrt(xy2);
#endif

    float area = rcpPdf;

#if 0
    // Boost the area associated with the sample by the ratio between the sample-center distance
    // and its orthogonal projection onto the integration plane (disk).
    area *= r / p;
#endif

#if SSS_CLAMP_ARTIFACT
    return saturate(EvalBurleyDiffusionProfile(r, S) * area);
#else
    return EvalBurleyDiffusionProfile(r, S) * area;
#endif
}

void EvaluateSample(uint i, uint n, int2 pixelCoord, int2 cacheOffset,
                    float3 S, float d, float3 centerPosVS, float mmPerUnit, float pixelsPerMm,
                    float phase, float3 tangentX, float3 tangentY, float4x4 projMatrix,
                    inout float3 totalIrradiance, inout float3 totalWeight, float linearDepth,
                    FrameUniforms framUniform, SubsurfaceTexStruct subsurfaceStruct)
{
    // The sample count is loop-invariant.
    const float scale = rcp(float(n));
    const float offset = rcp(float(n)) * 0.5;

    // The phase angle is loop-invariant.
    float sinPhase, cosPhase;
    sincos(phase, sinPhase, cosPhase);

    float r, rcpPdf;
    SampleBurleyDiffusionProfile(i * scale + offset, d, r, rcpPdf);

    float phi = SampleDiskGolden(i, n).y;
    float sinPhi, cosPhi;
    sincos(phi, sinPhi, cosPhi);

    float sinPsi = cosPhase * sinPhi + sinPhase * cosPhi; // sin(phase + phi)
    float cosPsi = cosPhase * cosPhi - sinPhase * sinPhi; // cos(phase + phi)

    float2 vec = r * float2(cosPsi, sinPsi);

    // Compute the screen-space position and the squared distance (in mm) in the image plane.
    int2 position; float xy2;

    #if SSS_USE_TANGENT_PLANE
        float3 relPosVS   = vec.x * tangentX + vec.y * tangentY;
        float3 positionVS = centerPosVS + relPosVS;
        float2 positionNDC = ComputeNormalizedDeviceCoordinates(positionVS, projMatrix);

        position = (int2)(positionNDC * _ScreenSize.xy);
        xy2      = dot(relPosVS.xy, relPosVS.xy);
    #else
        // floor((pixelCoord + 0.5) + vec * pixelsPerMm)
        // position = pixelCoord + floor(0.5 + vec * pixelsPerMm);
        // position = pixelCoord + round(vec * pixelsPerMm);
        // Note that (int) truncates towards 0, while floor() truncates towards -Inf!
        position = pixelCoord + (int2)round((pixelsPerMm * r) * float2(cosPsi, sinPsi));
        xy2      = r * r;
    #endif

    float4 textureSample = LoadSample(position, cacheOffset, framUniform, subsurfaceStruct);
    float3 irradiance    = textureSample.rgb;

    // Check the results of the stencil test.
    if (TestLightingForSSS(irradiance))
    {
        // Apply bilateral weighting.
        float  viewZ  = textureSample.a;
        float  relZ   = viewZ - linearDepth;
        float3 weight = ComputeBilateralWeight(xy2, relZ, mmPerUnit, S, rcpPdf);

        // Note: if the texture sample if off-screen, (z = 0) -> (viewZ = far) -> (weight ≈ 0).
        totalIrradiance += weight * irradiance;
        totalWeight     += weight;
    }
    else
    {
        // The irradiance is 0. This could happen for 2 reasons.
        // Most likely, the surface fragment does not have an SSS material.
        // Alternatively, our sample comes from a region without any geometry.
        // Our blur is energy-preserving, so 'centerWeight' should be set to 0.
        // We do not terminate the loop since we want to gather the contribution
        // of the remaining samples (e.g. in case of hair covering skin).
    }
}

void StoreResult(uint2 pixelCoord, float3 irradiance, SubsurfaceTexStruct subsurfaceStruct)
{
    subsurfaceStruct.cameraFilteringTexture[pixelCoord] = float4(irradiance, 1);
}

[numthreads(GROUP_SIZE_2D, 1, 1)]
void SubsurfaceScattering(uint3 groupId          : SV_GroupID,
                          uint  groupThreadId    : SV_GroupThreadID,
                          uint3 dispatchThreadId : SV_DispatchThreadID)
{
    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[perFrameBufferIndex];
    SubsurfaceTexStruct subsurfaceStruct;
    subsurfaceStruct.sssBufferTexture = ResourceDescriptorHeap[sssTextureIndex];
    subsurfaceStruct.depthTexture = ResourceDescriptorHeap[depthTextureIndex];
    subsurfaceStruct.irradianceSource = ResourceDescriptorHeap[irradianceSourceIndex];
    subsurfaceStruct.cameraFilteringTexture = ResourceDescriptorHeap[outCameraFilteringTextureIndex];
    
    groupThreadId &= GROUP_SIZE_2D - 1; // Help the compiler

    // Note: any factor of 64 is a suitable wave size for our algorithm.
    uint waveIndex = WaveReadLaneFirst(groupThreadId / 64);
    uint laneIndex = groupThreadId % 64;
    uint quadIndex = laneIndex / 4;

    // Arrange threads in the Morton order to optimally match the memory layout of GCN tiles.
    uint2 groupCoord  = DecodeMorton2D(groupThreadId);
    uint2 groupOffset = groupId.xy * GROUP_SIZE_1D;
    uint2 pixelCoord  = groupOffset + groupCoord;
    int2  cacheOffset = (int2)groupOffset - TEXTURE_CACHE_BORDER;

    float3 centerIrradiance  = subsurfaceStruct.irradianceSource[pixelCoord].rgb;
    float  centerDepth       = 0;
    bool   passedStencilTest = TestLightingForSSS(centerIrradiance);

    // Save some bandwidth by only loading depth values for SSS pixels.
    if (passedStencilTest)
    {
        centerDepth = subsurfaceStruct.depthTexture[pixelCoord].r;
    }

#if SSS_USE_LDS_CACHE
    uint2 cacheCoord = groupCoord + TEXTURE_CACHE_BORDER;
    // Populate the central region of the LDS cache.
    StoreSampleToCacheMemory(float4(centerIrradiance, centerDepth), cacheCoord);

    uint numBorderQuadsPerWave = TEXTURE_CACHE_SIZE_1D / 2 - 1;
    uint halfCacheWidthInQuads = TEXTURE_CACHE_SIZE_1D / 4;

    if (quadIndex < numBorderQuadsPerWave)
    {
        // Fetch another texel into the LDS.
        uint2 startQuad = halfCacheWidthInQuads * DeinterleaveQuad(waveIndex);

        uint2 quadCoord;

        // The traversal order is such that the quad's X coordinate is monotonically increasing.
        // The corner is always the near the block of the corresponding wavefront.
        // Note: the compiler can heavily optimize the code below, as the switch is scalar,
        // and there are very few unique values due to the symmetry.
        switch (waveIndex)
        {
            case 0:  // Bottom left
                quadCoord.x = max(0, (int)(quadIndex - (halfCacheWidthInQuads - 1)));
                quadCoord.y = max(0, (int)((halfCacheWidthInQuads - 1) - quadIndex));
                break;
            case 1:  // Bottom right
                quadCoord.x = min(quadIndex, halfCacheWidthInQuads - 1);
                quadCoord.y = max(0, (int)(quadIndex - (halfCacheWidthInQuads - 1)));
                break;
            case 2:  // Top left
                quadCoord.x = max(0, (int)(quadIndex - (halfCacheWidthInQuads - 1)));
                quadCoord.y = min(quadIndex, halfCacheWidthInQuads - 1);
                break;
            default: // Top right
                quadCoord.x = min(quadIndex, halfCacheWidthInQuads - 1);
                quadCoord.y = min(halfCacheWidthInQuads - 1, 2 * (halfCacheWidthInQuads - 1) - quadIndex);
                break;
        }

        uint2  cacheCoord2 = 2 * (startQuad + quadCoord) + DeinterleaveQuad(laneIndex);
        int2   pixelCoord2 = (int2)(groupOffset + cacheCoord2) - TEXTURE_CACHE_BORDER;
        float3 irradiance2 = subsurfaceStruct.irradianceSource[pixelCoord2].rgb;
        float  depth2      = 0;

        // Save some bandwidth by only loading depth values for SSS pixels.
        if (TestLightingForSSS(irradiance2))
        {
            depth2 = subsurfaceStruct.depthTexture[pixelCoord2].r;
        }

        // Populate the border region of the LDS cache.
        StoreSampleToCacheMemory(float4(irradiance2, depth2), cacheCoord2);
    }

    // Wait for the LDS.
    GroupMemoryBarrierWithGroupSync();
#endif

    if (!passedStencilTest)
    {
        StoreResult(pixelCoord, centerIrradiance, subsurfaceStruct);
        return;
    }

    uint screenWidth, screenHeight;
    subsurfaceStruct.depthTexture.GetDimensions(screenWidth, screenHeight);
    
    float4 _ScreenSize = float4(float2(screenWidth, screenHeight), rcp(float2(screenWidth, screenHeight)));
    
    int2 positionSS = int2(pixelCoord);
    float2 positionNDC = positionSS * _ScreenSize.zw;
    
    float4 sssBuffer = subsurfaceStruct.sssBufferTexture.Load(uint3(positionSS, 0));
    
    // The result of the stencil test allows us to statically determine the material type (SSS).
    SSSData sssData;
    DecodeFromSSSBuffer(sssBuffer, positionSS, sssData);

    int    profileIndex  = sssData.diffusionProfileIndex;
    float  distScale     = sssData.subsurfaceMask;
    float3 S             = frameUniform.sssUniform._ShapeParamsAndMaxScatterDists[profileIndex].rgb;
    float  d             = frameUniform.sssUniform._ShapeParamsAndMaxScatterDists[profileIndex].a;
    float  metersPerUnit = frameUniform.sssUniform._WorldScalesAndFilterRadiiAndThicknessRemaps[profileIndex].x;
    float  filterRadius  = frameUniform.sssUniform._WorldScalesAndFilterRadiiAndThicknessRemaps[profileIndex].y; // In millimeters

    // Reconstruct the view-space position corresponding to the central sample.
    float4x4 projMatrixInv = frameUniform.cameraUniform.curFrameUniform.unJitterProjectionMatrixInv;
    
    float2 centerPosNDC = positionNDC;
    float2 cornerPosNDC = centerPosNDC + 0.5 * _ScreenSize.zw;
    float3 centerPosVS = ComputeViewSpacePosition(centerPosNDC, centerDepth, projMatrixInv);
    float3 cornerPosVS = ComputeViewSpacePosition(cornerPosNDC, centerDepth, projMatrixInv);

    // Rescaling the filter is equivalent to inversely scaling the world.
    float mmPerUnit  = MILLIMETERS_PER_METER * (metersPerUnit * rcp(distScale));
    float unitsPerMm = rcp(mmPerUnit);

    // Compute the view-space dimensions of the pixel as a quad projected onto geometry.
    // Assuming square pixels, both X and Y are have the same dimensions.
    float unitsPerPixel = max(0.0001f, 2 * abs(cornerPosVS.x - centerPosVS.x));
    float pixelsPerMm   = rcp(unitsPerPixel) * unitsPerMm;

    // Area of a disk.
    float filterArea   = PI * Sq(filterRadius * pixelsPerMm);
    uint sampleCount = (uint) (filterArea * rcp(float(SSS_PIXELS_PER_SAMPLE)));
    uint sampleBudget = (uint) frameUniform.sssUniform._SssSampleBudget;

    uint   texturingMode = GetSubsurfaceScatteringTexturingMode(profileIndex);
    float3 albedo        = ApplySubsurfaceScatteringTexturingMode(texturingMode, sssData.diffuseColor);

    if (distScale == 0 || sampleCount < 1)
    {
    #if SSS_DEBUG_LOD
        float3 green = float3(0, 1, 0);
        StoreResult(pixelCoord, green);
    #else
        StoreResult(pixelCoord, albedo * centerIrradiance, subsurfaceStruct);
    #endif
        return;
    }

#if SSS_DEBUG_LOD
    float3 red  = float3(1, 0, 0);
    float3 blue = float3(0, 0, 1);
    StoreResult(pixelCoord, lerp(blue, red, saturate(sampleCount * rcp(sampleBudget))));
    return;
#endif

    float4x4 projMatrix = frameUniform.cameraUniform.curFrameUniform.unJitterProjectionMatrix;
    
    // TODO: Since we have moved to forward SSS, we don't support anymore a bsdfData.normalWS.
    // Once we include normal+roughness rendering during the prepass, we will have a buffer to bind here and we will be able to reuse this part of the algorithm on demand.
    float3 normalVS = float3(0, 0, 0);
    float3 tangentX = float3(0, 0, 0);
    float3 tangentY = float3(0, 0, 0);

#if SSS_DEBUG_NORMAL_VS
    // We expect the normal to be front-facing.
    float3 viewDirVS = normalize(centerPosVS);
    if (dot(normalVS, viewDirVS) >= 0)
    {
        StoreResult(pixelCoord, float3(1, 1, 1));
        return;
    }
#endif

#if SSS_RANDOM_ROTATION
    // Note that GenerateHashedRandomFloat() only uses the 23 low bits, hence the 2^24 factor.
    float phase = TWO_PI * GenerateHashedRandomFloat(uint3(pixelCoord, (uint)(centerDepth * 16777216)));
#else
    float phase = 0;
#endif

    uint n = min(sampleCount, sampleBudget);

    // Accumulate filtered irradiance and bilateral weights (for renormalization).
    float3 centerWeight    = 0; // Defer (* albedo)
    float3 totalIrradiance = 0;
    float3 totalWeight     = 0;

    float4 zBufferParams = frameUniform.cameraUniform.curFrameUniform.zBufferParams;
    float linearDepth = LinearEyeDepth(centerDepth, zBufferParams);
    for (uint i = 0; i < n; i++)
    {
        // Integrate over the image or tangent plane in the view space.
        EvaluateSample(i, n, pixelCoord, cacheOffset,
                       S, d, centerPosVS, mmPerUnit, pixelsPerMm,
                       phase, tangentX, tangentY, projMatrix,
                       totalIrradiance, totalWeight, linearDepth,
                       frameUniform, subsurfaceStruct);
    }

    // Total weight is 0 for color channels without scattering.
    totalWeight = max(totalWeight, FLT_MIN);

    StoreResult(pixelCoord, albedo * (totalIrradiance / totalWeight), subsurfaceStruct);
}
