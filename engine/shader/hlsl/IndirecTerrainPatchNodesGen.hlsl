#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "InputTypes.hlsli"
#include "d3d12.hlsli"

struct RootIndexBuffer
{
    uint meshPerFrameBufferIndex;
    uint terrainPatchNodeIndex;

    int2 pivotCenter;
};

ConstantBuffer<RootIndexBuffer> g_RootIndexBuffer : register(b0, space0);

[numthreads(8, 8, 1)]
void CSMain(CSParams Params) {

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[g_RootIndexBuffer.meshPerFrameBufferIndex];
    AppendStructuredBuffer<TerrainPatchNode> terrainPatchNodeBuffer = ResourceDescriptorHeap[g_RootIndexBuffer.terrainPatchNodeIndex];

    uint2 index = Params.DispatchThreadID.xy;

    // calculate the mip level of the pixel
    float2 dis = index - pivotCenter;
    int mipLevel = floor(max(abs(dis.x), abs(dis.y)) * 0.25f);

    // fetch from the mipoffset and offset eh current mipLevel


    // calculate the down-left index of the node
    int mipNodeWidth = pow(2, mipLevel+1);
    int2 nodePivot = floor(dis / mipNodeWidth)*mipNodeWidth;

    // check whether the nodePivot is the same as the basic index
    float pivotLen = length(nodePivot - dis);
    if(pivotLen == 0)
    {
        TerrainPatchNode _patchNode;
        _patchNode.patchMinPos = nodePivot;
        _pathNode.maxHeight = 1.0f;
        _pathNode.minHeight = -1.0f;
        _pathNode.nodeWidth = mipNodeWidth;
        _pathNode.mipLevel = mipLevel;
        _pathNode.neighbor = 0;

        terrainPatchNodeBuffer.Append(_pathNode);
    }
}
