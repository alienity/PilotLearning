// =============== Convolves transmitted radiance with the Disney diffusion profile ================

//--------------------------------------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------------------------------------

// #pragma enable_d3d11_debug_symbols
#pragma only_renderers d3d11 playstation xboxone xboxseries vulkan metal switch

#pragma kernel SubsurfaceScattering

#pragma multi_compile _ ENABLE_MSAA

// TODO: use sharp load hoisting on PS4.

// Tweak parameters.
#define SSS_BILATERAL_FILTER  1
#define SSS_USE_LDS_CACHE     1 // Use LDS as an L0 texture cache.
#define SSS_RANDOM_ROTATION   1 // Hides undersampling artifacts with high-frequency noise. TAA blurs the noise.
#define SSS_USE_TANGENT_PLANE 0 // Improves the accuracy of the approximation(0 -> 1st order). High cost. Does not work with back-facing normals.
#define SSS_CLAMP_ARTIFACT    0 // Reduces bleeding. Use with SSS_USE_TANGENT_PLANE.
#define SSS_DEBUG_LOD         0 // Displays the sampling rate: green = no filtering, blue = 1 sample, red = _SssSampleBudget samples.
#define SSS_DEBUG_NORMAL_VS   0 // Allows detection of back-facing normals.

// Do not modify these.
#include "Packages/com.unity.render-pipelines.high-definition/Runtime/RenderPipeline/ShaderPass/ShaderPass.cs.hlsl"
#define SHADERPASS            SHADERPASS_SUBSURFACE_SCATTERING
#define GROUP_SIZE_1D         16
#define GROUP_SIZE_2D         (GROUP_SIZE_1D * GROUP_SIZE_1D)
#define TEXTURE_CACHE_BORDER  2
#define TEXTURE_CACHE_SIZE_1D (GROUP_SIZE_1D + 2 * TEXTURE_CACHE_BORDER)
#define TEXTURE_CACHE_SIZE_2D (TEXTURE_CACHE_SIZE_1D * TEXTURE_CACHE_SIZE_1D)

// Check for support of typed UAV loads from FORMAT_R16G16B16A16_FLOAT.
// TODO: query the format support more precisely.
#if !(defined(SHADER_API_PSSL) || defined(SHADER_API_XBOXONE) || defined(SHADER_API_GAMECORE)) || defined(ENABLE_MSAA)
#define USE_INTERMEDIATE_BUFFER
#endif

//--------------------------------------------------------------------------------------------------
// Included headers
//--------------------------------------------------------------------------------------------------

#include "Packages/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl"
#include "Packages/com.unity.render-pipelines.core/ShaderLibrary/Packing.hlsl"
#include "Packages/com.unity.render-pipelines.core/ShaderLibrary/Sampling/Sampling.hlsl"
#include "Packages/com.unity.render-pipelines.core/ShaderLibrary/SpaceFillingCurves.hlsl"
#include "Packages/com.unity.render-pipelines.high-definition/Runtime/ShaderLibrary/ShaderVariables.hlsl"
#include "Packages/com.unity.render-pipelines.high-definition/Runtime/Lighting/LightDefinition.cs.hlsl"
#include "Packages/com.unity.render-pipelines.high-definition/Runtime/Material/SubsurfaceScattering/SubsurfaceScattering.hlsl"
#include "Packages/com.unity.render-pipelines.high-definition/Runtime/RenderPipeline/HDStencilUsage.cs.hlsl"

//--------------------------------------------------------------------------------------------------
// Inputs & outputs
//--------------------------------------------------------------------------------------------------

int _SssSampleBudget;

TEXTURE2D_X(_DepthTexture);                           // Z-buffer
TEXTURE2D_X(_IrradianceSource);                       // Includes transmitted light

StructuredBuffer<uint>  _CoarseStencilBuffer;

#ifdef USE_INTERMEDIATE_BUFFER
    RW_TEXTURE2D_X(float4, _CameraFilteringTexture);  // Target texture
#else
    RW_TEXTURE2D_X(float4, _CameraColorTexture);      // Target texture
#endif

//--------------------------------------------------------------------------------------------------
// Implementation
//--------------------------------------------------------------------------------------------------

// 6656 bytes used. It appears that the reserved LDS space must be a multiple of 512 bytes.
#if SSS_USE_LDS_CACHE
groupshared float2 textureCache0[TEXTURE_CACHE_SIZE_2D]; // {irradiance.rg}
groupshared float2 textureCache1[TEXTURE_CACHE_SIZE_2D]; // {irradiance.b, deviceDepth}
#endif
groupshared bool   processGroup;

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

float4 LoadSampleFromVideoMemory(int2 pixelCoord)
{
    float3 irradiance = LOAD_TEXTURE2D_X(_IrradianceSource, pixelCoord).rgb;
    float  depth      = LOAD_TEXTURE2D_X(_DepthTexture,     pixelCoord).r;

    return float4(irradiance, depth);
}

// Returns {irradiance, linearDepth}.
float4 LoadSample(int2 pixelCoord, int2 cacheOffset)
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
        value = LoadSampleFromVideoMemory(pixelCoord);
    }

    value.a = LinearEyeDepth(value.a, _ZBufferParams);

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
                    inout float3 totalIrradiance, inout float3 totalWeight, float linearDepth)
{
    // The sample count is loop-invariant.
    const float scale  = rcp(n);
    const float offset = rcp(n) * 0.5;

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

    float4 textureSample = LoadSample(position, cacheOffset);
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

void StoreResult(uint2 pixelCoord, float3 irradiance)
{
#ifdef USE_INTERMEDIATE_BUFFER
    _CameraFilteringTexture[COORD_TEXTURE2D_X(pixelCoord)] = float4(irradiance, 1);
#else
    _CameraColorTexture[COORD_TEXTURE2D_X(pixelCoord)]    += float4(irradiance, 0);
#endif
}

[numthreads(GROUP_SIZE_2D, 1, 1)]
void SubsurfaceScattering(uint3 groupId          : SV_GroupID,
                          uint  groupThreadId    : SV_GroupThreadID,
                          uint3 dispatchThreadId : SV_DispatchThreadID)
{
    groupThreadId &= GROUP_SIZE_2D - 1; // Help the compiler
    UNITY_XR_ASSIGN_VIEW_INDEX(dispatchThreadId.z);

    // Note: any factor of 64 is a suitable wave size for our algorithm.
    uint waveIndex = WaveReadLaneFirst(groupThreadId / 64);
    uint laneIndex = groupThreadId % 64;
    uint quadIndex = laneIndex / 4;

    // Arrange threads in the Morton order to optimally match the memory layout of GCN tiles.
    uint2 groupCoord  = DecodeMorton2D(groupThreadId);
    uint2 groupOffset = groupId.xy * GROUP_SIZE_1D;
    uint2 pixelCoord  = groupOffset + groupCoord;
    int2  cacheOffset = (int2)groupOffset - TEXTURE_CACHE_BORDER;

    if (groupThreadId == 0)
    {
        uint stencilRef = STENCILUSAGE_SUBSURFACE_SCATTERING;

        // Check whether the thread group needs to perform any work.
        uint s00Address = Get1DAddressFromPixelCoord(2 * groupId.xy + uint2(0, 0), _CoarseStencilBufferSize.xy, groupId.z);
        uint s10Address = Get1DAddressFromPixelCoord(2 * groupId.xy + uint2(1, 0), _CoarseStencilBufferSize.xy, groupId.z);
        uint s01Address = Get1DAddressFromPixelCoord(2 * groupId.xy + uint2(0, 1), _CoarseStencilBufferSize.xy, groupId.z);
        uint s11Address = Get1DAddressFromPixelCoord(2 * groupId.xy + uint2(1, 1), _CoarseStencilBufferSize.xy, groupId.z);

        uint s00 = _CoarseStencilBuffer[s00Address];
        uint s10 = _CoarseStencilBuffer[s10Address];
        uint s01 = _CoarseStencilBuffer[s01Address];
        uint s11 = _CoarseStencilBuffer[s11Address];

        uint HTileValue = s00 | s10 | s01 | s11;
        // Perform the stencil test (reject at the tile rate).
        processGroup = ((HTileValue & stencilRef) != 0);
    }

    // Wait for the LDS.
    GroupMemoryBarrierWithGroupSync();

    if (!processGroup) { return; }

    float3 centerIrradiance  = LOAD_TEXTURE2D_X(_IrradianceSource, pixelCoord).rgb;
    float  centerDepth       = 0;
    bool   passedStencilTest = TestLightingForSSS(centerIrradiance);

    // Save some bandwidth by only loading depth values for SSS pixels.
    if (passedStencilTest)
    {
        centerDepth = LOAD_TEXTURE2D_X(_DepthTexture, pixelCoord).r;
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
        float3 irradiance2 = LOAD_TEXTURE2D_X(_IrradianceSource, pixelCoord2).rgb;
        float  depth2      = 0;

        // Save some bandwidth by only loading depth values for SSS pixels.
        if (TestLightingForSSS(irradiance2))
        {
            depth2 = LOAD_TEXTURE2D_X(_DepthTexture, pixelCoord2).r;
        }

        // Populate the border region of the LDS cache.
        StoreSampleToCacheMemory(float4(irradiance2, depth2), cacheCoord2);
    }

    // Wait for the LDS.
    GroupMemoryBarrierWithGroupSync();
#endif

    if (!passedStencilTest) { return; }

    PositionInputs posInput = GetPositionInput(pixelCoord, _ScreenSize.zw);

    // The result of the stencil test allows us to statically determine the material type (SSS).
    SSSData sssData;
    DECODE_FROM_SSSBUFFER(posInput.positionSS, sssData);

    int    profileIndex  = sssData.diffusionProfileIndex;
    float  distScale     = sssData.subsurfaceMask;
    float3 S             = _ShapeParamsAndMaxScatterDists[profileIndex].rgb;
    float  d             = _ShapeParamsAndMaxScatterDists[profileIndex].a;
    float  metersPerUnit = _WorldScalesAndFilterRadiiAndThicknessRemaps[profileIndex].x;
    float  filterRadius  = _WorldScalesAndFilterRadiiAndThicknessRemaps[profileIndex].y; // In millimeters

    // Reconstruct the view-space position corresponding to the central sample.
    float2 centerPosNDC = posInput.positionNDC;
    float2 cornerPosNDC = centerPosNDC + 0.5 * _ScreenSize.zw;
    float3 centerPosVS  = ComputeViewSpacePosition(centerPosNDC, centerDepth, UNITY_MATRIX_I_P);
    float3 cornerPosVS  = ComputeViewSpacePosition(cornerPosNDC, centerDepth, UNITY_MATRIX_I_P);

    // Rescaling the filter is equivalent to inversely scaling the world.
    float mmPerUnit  = MILLIMETERS_PER_METER * (metersPerUnit * rcp(distScale));
    float unitsPerMm = rcp(mmPerUnit);

    // Compute the view-space dimensions of the pixel as a quad projected onto geometry.
    // Assuming square pixels, both X and Y are have the same dimensions.
    float unitsPerPixel = max(0.0001f, 2 * abs(cornerPosVS.x - centerPosVS.x));
    float pixelsPerMm   = rcp(unitsPerPixel) * unitsPerMm;

    // Area of a disk.
    float filterArea   = PI * Sq(filterRadius * pixelsPerMm);
    uint  sampleCount  = (uint)(filterArea * rcp(SSS_PIXELS_PER_SAMPLE));
    uint  sampleBudget = (uint)_SssSampleBudget;

    uint   texturingMode = GetSubsurfaceScatteringTexturingMode(profileIndex);
    float3 albedo        = ApplySubsurfaceScatteringTexturingMode(texturingMode, sssData.diffuseColor);

    if (distScale == 0 || sampleCount < 1)
    {
    #if SSS_DEBUG_LOD
        float3 green = float3(0, 1, 0);
        StoreResult(pixelCoord, green);
    #else
        StoreResult(pixelCoord, albedo * centerIrradiance);
    #endif
        return;
    }

#if SSS_DEBUG_LOD
    float3 red  = float3(1, 0, 0);
    float3 blue = float3(0, 0, 1);
    StoreResult(pixelCoord, lerp(blue, red, saturate(sampleCount * rcp(sampleBudget))));
    return;
#endif

    float4x4 viewMatrix, projMatrix;
    GetLeftHandedViewSpaceMatrices(viewMatrix, projMatrix);

    // TODO: Since we have moved to forward SSS, we don't support anymore a bsdfData.normalWS.
    // Once we include normal+roughness rendering during the prepass, we will have a buffer to bind here and we will be able to reuse this part of the algorithm on demand.
#if SSS_USE_TANGENT_PLANE
    #error ThisWillNotCompile_SeeComment
    // Compute the tangent frame in view space.
    float3 normalVS = mul((float3x3)viewMatrix, bsdfData.normalWS);
    float3 tangentX = GetLocalFrame(normalVS)[0] * unitsPerMm;
    float3 tangentY = GetLocalFrame(normalVS)[1] * unitsPerMm;
#else
    float3 normalVS = float3(0, 0, 0);
    float3 tangentX = float3(0, 0, 0);
    float3 tangentY = float3(0, 0, 0);
#endif

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

    float linearDepth = LinearEyeDepth(centerDepth, _ZBufferParams);
    for (uint i = 0; i < n; i++)
    {
        // Integrate over the image or tangent plane in the view space.
        EvaluateSample(i, n, pixelCoord, cacheOffset,
                       S, d, centerPosVS, mmPerUnit, pixelsPerMm,
                       phase, tangentX, tangentY, projMatrix,
                       totalIrradiance, totalWeight, linearDepth);
    }

    // Total weight is 0 for color channels without scattering.
    totalWeight = max(totalWeight, FLT_MIN);

    StoreResult(pixelCoord, albedo * (totalIrradiance / totalWeight));
}
