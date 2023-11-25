#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "InputTypes.hlsli"
#include "d3d12.hlsli"

struct RootIndexBuffer
{
    uint meshPerFrameBufferIndex;
    uint terrainPatchNodeIndex;
    uint terrainMinHeightmap;
    uint terrainMaxHeightmap;
};

ConstantBuffer<RootIndexBuffer> g_RootIndexBuffer : register(b0, space0);

SamplerState defaultSampler : register(s10);

#define EAST 1
#define SOUTH 2
#define WEST 4
#define NORTH 8

#define MAXMIPLEVEL 3

#define BASENODEWIDTH 2

#define MIP0WIDTH 16
#define MIP1WIDTH 32
#define MIP2WIDTH 64
#define MIP3WIDTH 128

/*
[0, 16] 区间是mip0
(16, 32] 区间是mip1
(32, 64] 区间是mip2
(64, 128] 区间是mip3
// (64, 128] 区间是mip4
// (128, 256] 区间是mip5
// (256, 512] 区间是mip6
// (512, 1024] 区间是mip7
*/
int mipCalc(float x)
{
    int mipLevel = floor(max(0,-2+log2(x)));
    mipLevel = min(mipLevel, MAXMIPLEVEL);
    return mipLevel;
}

void GetMipLevel(float2 pixelPos, float2 originPos, out int outMipLevel)
{
    // calculate the mip level of the pixel
    float2 dis = pixelPos - originPos;
    int mipLevel = mipCalc(max(abs(dis.x), abs(dis.y)));

    // fetch from the mipoffset and offset eh current mipLevel
    

    outMipLevel = mipLevel;
}

void GetPivotPosAndMipLevel(float2 pixelPos, float2 originPos, out int2 outNodePivot, out int outMipLevel)
{
    // calculate the mip level of the pixel
    float2 dis = pixelPos - originPos;
    int mipLevel = mipCalc(max(abs(dis.x), abs(dis.y)));

    // fetch from the mipoffset and offset eh current mipLevel


    // calculate the down-left index of the node
    int mipNodeWidth = pow(2, mipLevel+1);
    int2 nodePivot = floor(dis / mipNodeWidth) * mipNodeWidth;

    outMipLevel = mipLevel;
    outNodePivot = nodePivot;
}

void GetMaxMinHeight(Texture2D<float4> minHeightmap, Texture2D<float4> maxHeightmap, 
    float2 pivotPos, int2 heightSize, int mipLevel, float terrainMaxHeight, out float minHeight, out float maxHeight)
{
    float2 pivotUV = pivotPos.xy / float2(heightSize.x, heightSize.y);
    // fetch height from pyramid heightmap
    minHeight = minHeightmap.SampleLevel(defaultSampler, pivotUV, mipLevel).b;
    maxHeight = maxHeightmap.SampleLevel(defaultSampler, pivotUV, mipLevel).b;

    minHeight = minHeight * terrainMaxHeight;
    maxHeight = maxHeight * terrainMaxHeight;
}

[numthreads(8, 8, 1)]
void CSMain(CSParams Params) {

    // const int baseNodeWidth = 2;

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[g_RootIndexBuffer.meshPerFrameBufferIndex];
    AppendStructuredBuffer<TerrainPatchNode> terrainPatchNodeBuffer = ResourceDescriptorHeap[g_RootIndexBuffer.terrainPatchNodeIndex];
    Texture2D<float4> terrainMinHeightmap = ResourceDescriptorHeap[g_RootIndexBuffer.terrainMinHeightmap];
    Texture2D<float4> terrainMaxHeightmap = ResourceDescriptorHeap[g_RootIndexBuffer.terrainMaxHeightmap];

    uint terrainWidth;
    uint terrainHeight;
    terrainMaxHeightmap.GetDimensions(terrainWidth, terrainHeight);

    if(Params.DispatchThreadID.x > terrainWidth || Params.DispatchThreadID.y > terrainHeight)
        return;

    uint2 index = Params.DispatchThreadID.xy;
    float2 pixelPos = index + float2(0, 0);

    float4x4 terrainWorld2LocalMat = mFrameUniforms.terrainUniform.world2LocalMatrix;

    float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;

    float3 cameraPosition = mFrameUniforms.cameraUniform.cameraPosition;
    float3 cameraDirection = -mFrameUniforms.cameraUniform.worldFromViewMatrix._m02_m12_m22;
    float3 focusPosition = cameraPosition + cameraDirection;

    focusPosition = mul(terrainWorld2LocalMat, float4(focusPosition, 1.0f)).xyz;
    
    // float2 pivotCenter = float2(floor(focusPosition.xz / MIP3WIDTH) * MIP3WIDTH);
    float2 pivotCenter = float2(floor(focusPosition.xz / BASENODEWIDTH) * BASENODEWIDTH);

    // calculate the mip level of the pixel
    float2 pixelDis = pixelPos - pivotCenter;
    int mipLevel = mipCalc(max(abs(pixelDis.x), abs(pixelDis.y)));

    // fetch from the mipoffset and offset eh current mipLevel


    // calculate the down-left index of the node
    int mipNodeWidth = pow(2, mipLevel+1);
    float2 alignPivotDis = floor(pixelDis / mipNodeWidth) * mipNodeWidth;

    // check whether the alignPivotDis is the same as the basic index
    float2 pivotDiff = alignPivotDis - pixelDis;
    if(pivotDiff.x == 0 && pivotDiff.y == 0)
    {
        float2 curIdxPivot = pivotCenter + alignPivotDis;

        uint neighbor = 0;

        float2 northPos = curIdxPivot + float2(0, mipNodeWidth);
        float2 eastPos = curIdxPivot + float2(mipNodeWidth, 0);
        float2 southPos = curIdxPivot + float2(0, -2);
        float2 westPos = curIdxPivot + float2(-2, 0);

        int neighborMipLevel;
        GetMipLevel(eastPos, pivotCenter, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | EAST;
        }
        GetMipLevel(southPos, pivotCenter, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | SOUTH;
        }
        GetMipLevel(westPos, pivotCenter, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | WEST;
        }
        GetMipLevel(northPos, pivotCenter, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | NORTH;
        }

        float minHeight, maxHeight;
        GetMaxMinHeight(terrainMinHeightmap, terrainMaxHeightmap, curIdxPivot, int2(terrainWidth-1, terrainHeight-1), mipLevel + 1, terrainMaxHeight, minHeight, maxHeight);

        TerrainPatchNode _patchNode;
        _patchNode.patchMinPos = curIdxPivot;
        _patchNode.maxHeight = maxHeight;
        _patchNode.minHeight = minHeight;
        _patchNode.nodeWidth = mipNodeWidth;
        _patchNode.mipLevel = mipLevel;
        _patchNode.neighbor = neighbor;

        terrainPatchNodeBuffer.Append(_patchNode);
    }
}
