#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"

// #define MAX_Z_DOWNSAMPLE 1
// #define FINAL_MASK 1
// #define DILATE_MASK 1

#define GROUP_SIZE 8

#ifdef MAX_Z_DOWNSAMPLE

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint _DepthBufferIndex;
    uint _OutputTextureIndex;
};

groupshared float gs_maxDepth[GROUP_SIZE * GROUP_SIZE];

float GetDepthToDownsample(Texture2D<float> depthBuffer, float4 _ZBufferParams, uint2 pixCoord)
{
    float deviceDepth = LoadCameraDepth(depthBuffer, pixCoord);
    float outputDepth = 0;

    if (deviceDepth == UNITY_RAW_FAR_CLIP_VALUE)
        outputDepth = 1e10f;
    else
        outputDepth = LinearEyeDepth(LoadCameraDepth(depthBuffer, pixCoord), _ZBufferParams);

    return outputDepth;
}

// Important! This function assumes that a max operation is carried so if using reversed Z the depth must be passed as 1-depth or must be linear.
// GetDepthToDownsample should be used to enforce the right depth is used.
float ParallelReduction(uint gid, uint threadIdx, float depth)
{
    gs_maxDepth[threadIdx] = depth;

    GroupMemoryBarrierWithGroupSync();

    [unroll]
    for (uint s = (GROUP_SIZE * GROUP_SIZE) / 2u; s > 0u; s >>= 1u)
    {
        if (threadIdx < s)
        {
            gs_maxDepth[threadIdx] = max(gs_maxDepth[threadIdx], gs_maxDepth[threadIdx + s]);
        }

        GroupMemoryBarrierWithGroupSync();
    }
    return gs_maxDepth[0];
}

[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void ComputeMaxZ(uint3 dispatchThreadId : SV_DispatchThreadID, uint gid : SV_GroupIndex, uint2 groupThreadId : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    uint threadIdx = groupThreadId.y * GROUP_SIZE + groupThreadId.x;

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float> depthBuffer = ResourceDescriptorHeap[_DepthBufferIndex];
    RWTexture2D<float> _OutputTexture = ResourceDescriptorHeap[_OutputTextureIndex];

    float4 _ScreenSize = mFrameUniforms.baseUniform._ScreenSize;
    float4 _ZBufferParams = mFrameUniforms.cameraUniform._ZBufferParams;
    
    float currDepth = -1;
    if (all((float2)(dispatchThreadId.xy) < _ScreenSize.xy))
        currDepth = GetDepthToDownsample(depthBuffer, _ZBufferParams, dispatchThreadId.xy);

    float maxDepth = ParallelReduction(gid, threadIdx, currDepth);

    // Race condition which is fine, but errors on some platforms.
    if (threadIdx == 0)
    {
        _OutputTexture[groupID.xy] = maxDepth;
    }
}

#elif FINAL_MASK

cbuffer ConstantBufferStruct : register(b0, space0)
{
    float4 _SrcOffsetAndLimit;
    float  _DilationWidth;
};

#define _SrcLimit _SrcOffsetAndLimit.xy
#define _DepthMipOffset _SrcOffsetAndLimit.zw

ConstantBuffer<FrameUniforms> perFrameBuffer : register(b1, space0);
Texture2D<float> _InputTexture : register(t0, space0);
RWTexture2D<float> _OutputTexture : register(u0, space0);

void DownsampleDepth(float s0, float s1, float s2, float s3, out float maxDepth)
{
    maxDepth = max(Max3(s0, s1, s2), s3);
}

float GetMinDepth(Texture2D<float> depthBuffer, float4 _ZBufferParams, uint2 pixCoord)
{
    uint2 minDepthLocation = _DepthMipOffset + pixCoord;

    float minDepth = LoadCameraDepth(depthBuffer, minDepthLocation);
    return LinearEyeDepth(minDepth, _ZBufferParams);
}

float GetDepthGradient(float minDepth, float maxDepth)
{
    //return minDepth;
    return abs(maxDepth - minDepth) / maxDepth;
}

[numthreads(8, 8, 1)]
void ComputeFinalMask(uint3 dispatchThreadId : SV_DispatchThreadID, uint gid : SV_GroupIndex, uint2 groupThreadId : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    // Upper-left pixel coordinate of quad that this thread will read
    uint2 srcPixelUL = (dispatchThreadId.xy << 1);

    float s0 = _InputTexture[min(srcPixelUL + uint2(0u, 0u), _SrcLimit)].x;
    float s1 = _InputTexture[min(srcPixelUL + uint2(1u, 0u), _SrcLimit)].x;
    float s2 = _InputTexture[min(srcPixelUL + uint2(0u, 1u), _SrcLimit)].x;
    float s3 = _InputTexture[min(srcPixelUL + uint2(1u, 1u), _SrcLimit)].x;

    float maxDepth;
    DownsampleDepth(s0, s1, s2, s3, maxDepth);

    _OutputTexture[dispatchThreadId.xy] = maxDepth;
}

#elif DILATE_MASK

cbuffer ConstantBufferStruct : register(b0, space0)
{
    float4 _SrcOffsetAndLimit;
    float  _DilationWidth;
};

#define _SrcLimit _SrcOffsetAndLimit.xy
#define _DepthMipOffset _SrcOffsetAndLimit.zw

ConstantBuffer<FrameUniforms> perFrameBuffer : register(b1, space0);
Texture2D<float> _InputTexture : register(t0, space0);
RWTexture2D<float> _OutputTexture : register(u0, space0);

float DilateValue(float currMax, float currSample)
{
    return max(currMax, currSample);
}

[numthreads(8, 8, 1)]
void DilateMask(uint3 dispatchThreadId : SV_DispatchThreadID, uint gid : SV_GroupIndex, uint2 groupThreadId : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
    int dilationWidth = _DilationWidth;

    // Make sure to convert the coordinate to signed for the kernel taps
    int2 currentCoord = (int2)dispatchThreadId.xy;

    float dilatedMaxVals = -1;
    for (int i = -dilationWidth; i <= dilationWidth; ++i)
    {
        for (int j = -dilationWidth; j <= dilationWidth; ++j)
        {
            // Evaluate the tap coordinate and clamp it to the texture
            int2 tapCoordinate = clamp(currentCoord + int2(i, j), 0, _SrcLimit - 1);
            float s = _InputTexture[tapCoordinate].x;
            dilatedMaxVals = DilateValue(dilatedMaxVals, s);
        }
    }

    _OutputTexture[dispatchThreadId.xy] = dilatedMaxVals;
}

#endif










