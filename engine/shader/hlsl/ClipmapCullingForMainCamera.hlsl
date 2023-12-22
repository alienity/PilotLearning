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
    uint terrainMinHeightmapIndex;
    uint terrainMaxHeightmapIndex;
    uint clipmapVisableBufferIndex;
};

SamplerState defaultSampler : register(s10);

void GetMaxMinHeight(Texture2D<float4> minHeightmap, Texture2D<float4> maxHeightmap, 
    float3 terrainPos, int2 heightSize, int mipLevel, float terrainMaxHeight, out float minHeight, out float maxHeight)
{
    float2 pivotUV = (terrainPos.xz + 1024 * heightSize.xy) / float2(heightSize.xy);
    pivotUV = frac(pivotUV);
    // fetch height from pyramid heightmap
    minHeight = minHeightmap.SampleLevel(defaultSampler, pivotUV, mipLevel).b;
    maxHeight = maxHeightmap.SampleLevel(defaultSampler, pivotUV, mipLevel).b;

    minHeight = minHeight * terrainMaxHeight;
    maxHeight = maxHeight * terrainMaxHeight;
}

[numthreads(8, 1, 1)]
void CSMain(CSParams Params) {

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<ClipmapMeshCount> clipmapMeshCount = ResourceDescriptorHeap[clipmapMeshCountIndex];
    StructuredBuffer<ClipmapTransform> clipTransformBuffer = ResourceDescriptorHeap[transformBufferIndex];
    StructuredBuffer<ClipMeshCommandSigParams> clipBaseMeshBuffer = ResourceDescriptorHeap[clipmapBaseCommandSigIndex];
    Texture2D<float4> terrainMinHeightmap = ResourceDescriptorHeap[terrainMinHeightmapIndex];
    Texture2D<float4> terrainMaxHeightmap = ResourceDescriptorHeap[terrainMaxHeightmapIndex];
    AppendStructuredBuffer<ToDrawCommandSignatureParams> toDrawCommandSigBuffer = ResourceDescriptorHeap[clipmapVisableBufferIndex];

    int totalCount = clipmapMeshCount.total_count;

    if(Params.DispatchThreadID.x >= totalCount)
        return;

    uint terrainWidth;
    uint terrainHeight;
    terrainMaxHeightmap.GetDimensions(terrainWidth, terrainHeight);

    float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;

    uint index = Params.DispatchThreadID.x;

    ClipmapTransform clipTrans = clipTransformBuffer[index];
    float4x4 tLocalTransform = clipTrans.transform;
    uint mesh_type = clipTrans.mesh_type;

    ClipMeshCommandSigParams clipSig = clipBaseMeshBuffer[mesh_type];
    BoundingBox clipBoundingBox = clipSig.ClipBoundingBox;

    BoundingBox aabbView;
    clipBoundingBox.Transform(tLocalTransform, aabbView);

    float3 clipLocalPos = aabbView.Center;
    float3 clipLocalExtents = aabbView.Extents; 

    float maxClipWidth = max(abs(clipLocalExtents.x), abs(clipLocalExtents.z)) * 2;
    int clipMipLevel = clamp(ceil(log2(maxClipWidth)), 0, 8);

    float minHeight, maxHeight;
    GetMaxMinHeight(terrainMinHeightmap, terrainMaxHeightmap, clipLocalPos, 
        int2(terrainWidth, terrainHeight), clipMipLevel, terrainMaxHeight, minHeight, maxHeight);

    float halfExtentY = (maxHeight - minHeight) * 0.5f + 0.1f;
    float midHeight = (maxHeight + minHeight) * 0.5f + clipLocalPos.y;

    aabbView.Center = float3(clipLocalPos.x, midHeight, clipLocalPos.z);
    aabbView.Extents = float3(clipLocalExtents.x, halfExtentY, clipLocalExtents.z);

    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    float4x4 clipFromWorldMatrix = mFrameUniforms.cameraUniform.curFrameUniform.clipFromWorldMatrix;
    float4x4 clipFromLocalMat = mul(clipFromWorldMatrix, localToWorldMatrix);

    Frustum frustum = ExtractPlanesDX(clipFromLocalMat);

    bool visible = FrustumContainsBoundingBox(frustum, aabbView) != CONTAINMENT_DISJOINT;
    if (visible)
    {
        ToDrawCommandSignatureParams toDrawCommandSig;
        toDrawCommandSig.ClipIndex = index;
        toDrawCommandSig.VertexBuffer = clipSig.VertexBuffer;
        toDrawCommandSig.IndexBuffer = clipSig.IndexBuffer;
        toDrawCommandSig.DrawIndexedArguments = clipSig.DrawIndexedArguments;

        toDrawCommandSigBuffer.Append(toDrawCommandSig);
    }
}
