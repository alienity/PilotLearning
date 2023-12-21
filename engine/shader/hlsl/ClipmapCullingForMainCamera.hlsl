#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "InputTypes.hlsli"
#include "d3d12.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint clipmapMeshCountIndex;
    uint transformBufferIndex;
    uint clipmapBaseCommandSigIndex;
    uint clipmapVisableBufferIndex;
};

SamplerState defaultSampler : register(s10);

[numthreads(8, 1, 1)]
void CSMain(CSParams Params) {

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<ClipmapMeshCount> clipmapMeshCount = ResourceDescriptorHeap[clipmapMeshCountIndex];
    StructuredBuffer<ClipmapTransform> clipTransformBuffer = ResourceDescriptorHeap[transformBufferIndex];
    StructuredBuffer<ClipMeshCommandSigParams> clipBaseMeshBuffer = ResourceDescriptorHeap[clipmapBaseCommandSigIndex];
    AppendStructuredBuffer<ToDrawCommandSignatureParams> toDrawCommandSigBuffer = ResourceDescriptorHeap[clipmapVisableBufferIndex];

    int totalCount = clipmapMeshCount.total_count;

    if(Params.DispatchThreadID.x >= totalCount)
        return;

    uint index = Params.DispatchThreadID.x;

    ClipmapTransform clipTrans = clipTransformBuffer[index];
    uint mesh_type = clipTrans.mesh_type;

    ClipMeshCommandSigParams clipSig = clipBaseMeshBuffer[mesh_type];

    ToDrawCommandSignatureParams toDrawCommandSig;
    toDrawCommandSig.ClipIndex = index;
    toDrawCommandSig.VertexBuffer = clipSig.VertexBuffer;
    toDrawCommandSig.IndexBuffer = clipSig.IndexBuffer;
    toDrawCommandSig.DrawIndexedArguments = clipSig.DrawIndexedArguments;

    toDrawCommandSigBuffer.Append(toDrawCommandSig);

    /*
    float heightBias = 0.1f;
    float3 boxMin = float3(patchNode.patchMinPos.x, patchNode.minHeight - heightBias, patchNode.patchMinPos.y);
    float3 boxMax = float3(patchNode.patchMinPos.x + patchNode.nodeWidth, patchNode.maxHeight + heightBias, patchNode.patchMinPos.y + patchNode.nodeWidth);

    // boxMin.y += 1.5f;
    // boxMax.y += 1.5f;

    BoundingBox patchBox;
    patchBox.Center = (boxMax + boxMin) * 0.5f;
    patchBox.Extents = (boxMax - boxMin) * 0.5f;

    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    BoundingBox aabb;
    patchBox.Transform(localToWorldMatrix, aabb);

    Frustum frustum = ExtractPlanesDX(mFrameUniforms.cameraUniform.curFrameUniform.clipFromWorldMatrix);

    bool visible = FrustumContainsBoundingBox(frustum, aabb) != CONTAINMENT_DISJOINT;
    if (visible)
    {
        terrainVisNodeIdxBuffer.Append(index);
    }
    */
}
