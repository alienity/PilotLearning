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

#define MAXMIPLEVEL 4

#define BASENODEWIDTH 2

#define MIP0WIDTH 16
#define MIP1WIDTH 32
#define MIP2WIDTH 64
#define MIP3WIDTH 128

/*
[0, 8] 区间是mip0
(8, 16] 区间是mip1
(16, 32] 区间是mip2
(32, 64] 区间是mip3
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
    float pixelOffset = pow(2, mipLevel);
    float2 pivotUV = (pivotPos.xy+float2(pixelOffset,pixelOffset)) / float2(heightSize.x, heightSize.y);
    // fetch height from pyramid heightmap
    minHeight = minHeightmap.SampleLevel(defaultSampler, pivotUV, mipLevel+1).b;
    maxHeight = maxHeightmap.SampleLevel(defaultSampler, pivotUV, mipLevel+1).b;

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
    // 因为只算左下角
    float2 vertexPos = index;

    float4x4 terrainWorld2LocalMat = mFrameUniforms.terrainUniform.world2LocalMatrix;

    float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;

    float3 cameraPosition = mFrameUniforms.cameraUniform.curFrameUniform.cameraPosition;
    float3 cameraDirection = -mFrameUniforms.cameraUniform.curFrameUniform.worldFromViewMatrix._m02_m12_m22;
    float3 focusPosition = cameraPosition + cameraDirection;

    focusPosition = mul(terrainWorld2LocalMat, float4(focusPosition, 1.0f)).xyz;
    
    // float2 vertexCenter = float2(floor(focusPosition.xz / MIP3WIDTH) * MIP3WIDTH);
    float2 vertexCenter = float2(floor(focusPosition.xz / BASENODEWIDTH) * BASENODEWIDTH);

    // calculate the mip level of the pixel
    float2 vertexToCenterDis = vertexPos - vertexCenter;
    int mipLevel = mipCalc(max(abs(vertexToCenterDis.x), abs(vertexToCenterDis.y))-0.5f);

    // fetch from the mipoffset and offset eh current mipLevel


    // calculate the down-left index of the node
    int mipNodeWidth = pow(2, mipLevel) * BASENODEWIDTH;
    float2 alignedToCenterDis = floor(vertexToCenterDis / mipNodeWidth) * mipNodeWidth;

    // 这里考虑patch的(1,1)点作为计算锚点，所有做后续确认计算的点都是锚点
    float2 alignedPivot = alignedToCenterDis + float2(1,1);

    // check whether the alignedToCenterDis is the same as the basic index
    float2 vertexDiff = alignedPivot - vertexToCenterDis;
    if(vertexDiff.x == 0 && vertexDiff.y == 0)
    {
        float2 curIdxPivot = vertexCenter + alignedPivot;

        uint neighbor = 0;

        float2 northPos = curIdxPivot + float2(0, mipNodeWidth);
        float2 eastPos = curIdxPivot + float2(mipNodeWidth, 0);
        float2 southPos = curIdxPivot + float2(0, -2);
        float2 westPos = curIdxPivot + float2(-2, 0);

        int neighborMipLevel;
        GetMipLevel(eastPos, vertexCenter, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | EAST;
        }
        GetMipLevel(southPos, vertexCenter, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | SOUTH;
        }
        GetMipLevel(westPos, vertexCenter, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | WEST;
        }
        GetMipLevel(northPos, vertexCenter, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | NORTH;
        }

        float2 patchMinPos = curIdxPivot - float2(1, 1);

        float minHeight, maxHeight;
        GetMaxMinHeight(terrainMinHeightmap, terrainMaxHeightmap, patchMinPos, int2(terrainWidth-1, terrainHeight-1), mipLevel + 1, terrainMaxHeight, minHeight, maxHeight);

        // float patchScale = mipNodeWidth / BASENODEWIDTH;

        TerrainPatchNode _patchNode;
        _patchNode.patchMinPos = patchMinPos;
        _patchNode.maxHeight = maxHeight;
        _patchNode.minHeight = minHeight;
        _patchNode.nodeWidth = mipNodeWidth;
        _patchNode.mipLevel = mipLevel;
        _patchNode.neighbor = neighbor;

        terrainPatchNodeBuffer.Append(_patchNode);
    }
}
