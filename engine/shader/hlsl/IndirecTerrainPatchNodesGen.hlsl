#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "InputTypes.hlsli"
#include "d3d12.hlsli"

struct RootIndexBuffer
{
    uint meshPerFrameBufferIndex;
    uint terrainPatchNodeIndex;
};

ConstantBuffer<RootIndexBuffer> g_RootIndexBuffer : register(b0, space0);

#define EAST 1
#define SOUTH 2
#define WEST 4
#define NORTH 8

void GetMipLevel(float2 pixelPos, float2 originPos, out int outMipLevel)
{
    // calculate the mip level of the pixel
    float2 dis = pixelPos - originPos;
    int mipLevel = floor(max(abs(dis.x), abs(dis.y)) / 8);

    // fetch from the mipoffset and offset eh current mipLevel
    

    outMipLevel = mipLevel;
}

void GetPivotPosAndMipLevel(float2 pixelPos, float2 originPos, out int2 outNodePivot, out int outMipLevel)
{
    // calculate the mip level of the pixel
    float2 dis = pixelPos - originPos;
    int mipLevel = floor(max(abs(dis.x), abs(dis.y)) / 8);

    // fetch from the mipoffset and offset eh current mipLevel


    // calculate the down-left index of the node
    int mipNodeWidth = pow(2, mipLevel+1);
    int2 nodePivot = floor(dis / mipNodeWidth) * mipNodeWidth;

    outMipLevel = mipLevel;
    outNodePivot = nodePivot;
}

void GetMaxMinHeight(int2 pivotPos, int mipLevel, out float minHeight, out float maxHeight)
{
    // fetch height from pyramid heightmap

    minHeight = -1.0f;
    maxHeight = 1.0f;
}

[numthreads(8, 8, 1)]
void CSMain(CSParams Params) {

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[g_RootIndexBuffer.meshPerFrameBufferIndex];
    AppendStructuredBuffer<TerrainPatchNode> terrainPatchNodeBuffer = ResourceDescriptorHeap[g_RootIndexBuffer.terrainPatchNodeIndex];

    uint2 index = Params.DispatchThreadID.xy;
    float2 pixelPos = index + float2(0.5, 0.5);

    float3 cameraPosition = mFrameUniforms.cameraUniform.cameraPosition;
    float3 cameraDirection = -mFrameUniforms.cameraUniform.worldFromViewMatrix._m02_m12_m22;
    float3 focusPosition = cameraPosition + cameraDirection;
    
    float2 pivotCenter = float2(floor(focusPosition.xz / 2.0f) * 2.0f);

    // calculate the mip level of the pixel
    float2 dis = pixelPos - pivotCenter;
    int mipLevel = floor(max(abs(dis.x), abs(dis.y)) / 8);

    // fetch from the mipoffset and offset eh current mipLevel


    // calculate the down-left index of the node
    int mipNodeWidth = pow(2, mipLevel+1);
    int2 nodePivot = floor(dis / mipNodeWidth) * mipNodeWidth + float2(0.5, 0.5);

    // check whether the nodePivot is the same as the basic index
    float pivotLen = length(nodePivot - dis);
    if(pivotLen == 0)
    {
        uint neighbor = 0;

        int2 northPos = nodePivot + int2(0, mipNodeWidth);
        int2 eastPos = nodePivot + float2(mipNodeWidth, 0);
        int2 southPos = nodePivot + float2(0, -2);
        int2 westPos = nodePivot + float2(-2, 0);

        int neighborMipLevel;
        GetMipLevel(eastPos, nodePivot, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | EAST;
        }
        GetMipLevel(southPos, nodePivot, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | SOUTH;
        }
        GetMipLevel(westPos, nodePivot, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | WEST;
        }
        GetMipLevel(northPos, nodePivot, neighborMipLevel);
        if(mipLevel + 1 == neighborMipLevel)
        {
            neighbor = neighbor | NORTH;
        }

        float minHeight, maxHeight;
        GetMaxMinHeight(nodePivot, mipLevel, minHeight, maxHeight);

        TerrainPatchNode _patchNode;
        _patchNode.patchMinPos = nodePivot;
        _patchNode.maxHeight = maxHeight;
        _patchNode.minHeight = minHeight;
        _patchNode.nodeWidth = mipNodeWidth;
        _patchNode.mipLevel = mipLevel;
        _patchNode.neighbor = neighbor;

        terrainPatchNodeBuffer.Append(_patchNode);
    }
}
