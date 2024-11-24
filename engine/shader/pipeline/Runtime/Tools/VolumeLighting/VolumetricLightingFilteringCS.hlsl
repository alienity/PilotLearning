//--------------------------------------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Included headers
//--------------------------------------------------------------------------------------------------

#include "../../ShaderLibrary/Common.hlsl"
#include "../../Material/Builtin/BuiltinData.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Tools/VolumeLighting/VolumetricLightingCommon.hlsl"

//--------------------------------------------------------------------------------------------------
// Inputs & outputs
//--------------------------------------------------------------------------------------------------

// RW_TEXTURE3D(float4, _VBufferLighting);

ConstantBuffer<FrameUniforms> _PerFrameBuffer : register(b0, space0);
RWTexture3D<float4> _VBufferLighting : register(u0, space0);

#define GAUSSIAN_SIGMA 1.0
#define GROUP_SIZE_1D_XY 8
#define GROUP_SIZE_1D_Z 1

#define FILTER_SIZE_1D (GROUP_SIZE_1D_XY + 2) // With a 8x8 group, we have a 10x10 working area
#define LDS_SIZE FILTER_SIZE_1D * FILTER_SIZE_1D

// TODO: May use 1 uint for 2 pixels
groupshared float gs_cacheR[LDS_SIZE];
groupshared float gs_cacheG[LDS_SIZE];
groupshared float gs_cacheB[LDS_SIZE];
groupshared float gs_cacheA[LDS_SIZE];

float4 GetSample(uint index)
{
    float4 outVal;
    outVal.r = gs_cacheR[index];
    outVal.g = gs_cacheG[index];
    outVal.b = gs_cacheB[index];
    outVal.a = gs_cacheA[index];

    return outVal;
}

void PrefetchData(inout RWTexture3D<float4> _VBufferLighting, float4 _VBufferViewportSize, uint groupIndex, uint2 groupOrigin, uint sliceIndex)
{
    int2 originXY = groupOrigin - int2(1, 1);

    for (int i = 0; i < 2; ++i)
    {
        uint sampleID = i + (groupIndex * 2);
        int offsetX = sampleID % FILTER_SIZE_1D;
        int offsetY = sampleID / FILTER_SIZE_1D;

        int3 sampleCoord = int3(clamp(originXY.x + offsetX, 0, _VBufferViewportSize.x - 1),
                                clamp(originXY.y + offsetY, 0, _VBufferViewportSize.y - 1),
                                sliceIndex);

        float4 sampleVal = _VBufferLighting[sampleCoord];

        int LDSIndex = offsetX + offsetY * FILTER_SIZE_1D;
        gs_cacheR[LDSIndex] = sampleVal.r;
        gs_cacheG[LDSIndex] = sampleVal.g;
        gs_cacheB[LDSIndex] = sampleVal.b;
        gs_cacheA[LDSIndex] = sampleVal.a;
    }
}

void WriteOutput(inout RWTexture3D<float4> _VBufferLighting, uint3 voxelCoord, float4 value)
{
    _VBufferLighting[voxelCoord] = value;
}

float Gaussian(float radius, float sigma)
{
    float v = radius / sigma;
    return exp(-(v*v));
}

[numthreads(GROUP_SIZE_1D_XY, GROUP_SIZE_1D_XY, GROUP_SIZE_1D_Z)]
void FilterVolumetricLighting(uint3 dispatchThreadId : SV_DispatchThreadID,
                                int   groupIndex       : SV_GroupIndex,
                                uint2 groupId          : SV_GroupID,
                                uint2 groupThreadId    : SV_GroupThreadID)
{
    VBufferUniform volumeLightUniform = _PerFrameBuffer.vBufferUniform;
    float4 _VBufferViewportSize = volumeLightUniform._VBufferViewportSize;
    
    // Compute the coordinate that this thread needs to process
    uint2 currentCoord  = groupId * GROUP_SIZE_1D_XY + groupThreadId;
    uint currentSlice = dispatchThreadId.z;

    // Compute the output voxel coordinate
    uint3 voxelCoord = uint3(currentCoord, currentSlice);

    if (groupIndex < 50)
    {
        // Load 2 values per thread.
        PrefetchData(_VBufferLighting, _VBufferViewportSize, groupIndex, groupId * GROUP_SIZE_1D_XY, voxelCoord.z);
    }

    // Make sure all values are loaded in LDS by now.
    GroupMemoryBarrierWithGroupSync();

    // Values used for accumulation
    float sumW = 0.0;
    float4 value = float4(0.0, 0.0, 0.0, 0.0);

    const int radius = 1;

    for (int idx = -radius; idx <= radius; ++idx)
    {
        for (int idx2 = -radius; idx2 <= radius; ++idx2)
        {
            // Compute the next tapping coordinate
            const int3 tapCoord = int3(voxelCoord.x, voxelCoord.y, voxelCoord.z) + int3(idx2, idx, 0);

            // Tap from LDS
            int2 tapAddress = (groupThreadId + 1) + int2(idx2, idx);
            uint ldsTapAddress = uint(tapAddress.x) % FILTER_SIZE_1D + tapAddress.y * FILTER_SIZE_1D;
            float4 currentValue = GetSample(ldsTapAddress);

            // Compute the weight for this tap
            float weight = Gaussian(length(int2(idx, idx2)), GAUSSIAN_SIGMA);

            // Accumulate the value and weight
            value += currentValue * weight;
            sumW += weight;
        }

    }

    WriteOutput(_VBufferLighting, voxelCoord, value / sumW);
}
