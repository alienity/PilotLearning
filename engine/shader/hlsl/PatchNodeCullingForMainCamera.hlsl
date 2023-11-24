#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "InputTypes.hlsli"
#include "d3d12.hlsli"

struct RootIndexBuffer
{
    uint meshPerFrameBufferIndex;
    uint terrainPatchNodeIndex;
    uint terrainPatchNodeCountIndex;
    uint terrainVisNodeIdxIndex;
};

ConstantBuffer<RootIndexBuffer> g_RootIndexBuffer : register(b0, space0);

SamplerState defaultSampler : register(s10);

[numthreads(128, 1, 1)]
void CSMain(CSParams Params) {

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[g_RootIndexBuffer.meshPerFrameBufferIndex];
    StructuredBuffer<TerrainPatchNode> terrainPatchNodeBuffer = ResourceDescriptorHeap[g_RootIndexBuffer.terrainPatchNodeIndex];
    StructuredBuffer<uint> terrainPatchNodeCountBuffer = ResourceDescriptorHeap[g_RootIndexBuffer.terrainPatchNodeCountIndex];
    AppendStructuredBuffer<uint> terrainVisNodeIdxBuffer = ResourceDescriptorHeap[g_RootIndexBuffer.terrainVisNodeIdxIndex];

    uint terrainPatchNodeCount = terrainPatchNodeCountBuffer[0];

    if(Params.DispatchThreadID.x >= terrainPatchNodeCount)
        return;

    uint index = Params.DispatchThreadID.x;

    TerrainPatchNode patchNode = terrainPatchNodeBuffer[index];

    float3 boxMin = float3(patchNode.patchMinPos.x, patchNode.minHeight, patchNode.patchMinPos.y);
    float3 boxMax = float3(patchNode.patchMinPos.x + patchNode.nodeWidth, patchNode.maxHeight, patchNode.patchMinPos.y + patchNode.nodeWidth);

    boxMin.y += 1.5f;
    boxMax.y += 1.5f;

    BoundingBox aabb;
    aabb.Center = (boxMax + boxMin) * 0.5f;
    aabb.Extents = (boxMax - boxMin) * 0.5f;
    aabb._Padding_Center = 0;
    aabb._Padding_Extents = 0;

    Frustum frustum = ExtractPlanesDX(mFrameUniforms.cameraUniform.clipFromWorldMatrix);

    bool visible = FrustumContainsBoundingBox(frustum, aabb) != CONTAINMENT_DISJOINT;
    if (visible)
    {
        terrainVisNodeIdxBuffer.Append(index);
    }
}
