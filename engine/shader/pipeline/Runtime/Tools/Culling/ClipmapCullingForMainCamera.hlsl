#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Material/Lit/LitProperties.hlsl"

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
void CSMain(uint3 DispatchThreadID : SV_DispatchThreadID) {

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<ClipmapMeshCount> clipmapMeshCount = ResourceDescriptorHeap[clipmapMeshCountIndex];
    StructuredBuffer<ClipmapTransform> clipTransformBuffer = ResourceDescriptorHeap[transformBufferIndex];
    StructuredBuffer<ClipMeshCommandSigParams> clipBaseMeshBuffer = ResourceDescriptorHeap[clipmapBaseCommandSigIndex];
    Texture2D<float4> terrainMinHeightmap = ResourceDescriptorHeap[terrainMinHeightmapIndex];
    Texture2D<float4> terrainMaxHeightmap = ResourceDescriptorHeap[terrainMaxHeightmapIndex];
    AppendStructuredBuffer<ToDrawCommandSignatureParams> toDrawCommandSigBuffer = ResourceDescriptorHeap[clipmapVisableBufferIndex];

    int totalCount = clipmapMeshCount.total_count;

    if(DispatchThreadID.x >= totalCount)
        return;

    uint terrainWidth;
    uint terrainHeight;
    terrainMaxHeightmap.GetDimensions(terrainWidth, terrainHeight);

    float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;

    uint index = DispatchThreadID.x;

    ClipmapTransform clipTrans = clipTransformBuffer[index];
    float4x4 tLocalTransform = clipTrans.transform;
    uint mesh_type = clipTrans.mesh_type;

    ClipMeshCommandSigParams clipSig = clipBaseMeshBuffer[mesh_type];
    BoundingBox clipBoundingBox = GetRendererBounds(clipSig);

    clipBoundingBox.Transform(tLocalTransform);

    float3 clipLocalPos = clipBoundingBox.Center.xyz;
    float3 clipLocalExtents = clipBoundingBox.Extents.xyz; 

    float maxClipWidth = max(abs(clipLocalExtents.x), abs(clipLocalExtents.z)) * 2;
    int clipMipLevel = clamp(ceil(log2(maxClipWidth)), 0, 8);

    float minHeight, maxHeight;
    GetMaxMinHeight(terrainMinHeightmap, terrainMaxHeightmap, clipLocalPos, 
        int2(terrainWidth, terrainHeight), clipMipLevel, terrainMaxHeight, minHeight, maxHeight);

    float halfExtentY = (maxHeight - minHeight) * 0.5f + 0.1f;
    float midHeight = (maxHeight + minHeight) * 0.5f + clipLocalPos.y;

    clipBoundingBox.Center = float4(clipLocalPos.x, midHeight, clipLocalPos.z, 0);
    clipBoundingBox.Extents = float4(clipLocalExtents.x, halfExtentY, clipLocalExtents.z, 0);

    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    float4x4 clipFromWorldMatrix = UNITY_MATRIX_VP(mFrameUniforms.cameraUniform);
    float4x4 clipFromLocalMat = mul(clipFromWorldMatrix, localToWorldMatrix);

    Frustum frustum = ExtractPlanesDX(clipFromLocalMat);

    bool visible = FrustumContainsBoundingBox(frustum, clipBoundingBox) != CONTAINMENT_DISJOINT;
    if (visible)
    {
        ToDrawCommandSignatureParams toDrawCommandSig = (ToDrawCommandSignatureParams)0;
        toDrawCommandSig.clipIndex = index;
        toDrawCommandSig.vertexBufferView = ParamsToVertexBufferView(clipSig.vertexBufferView);
        toDrawCommandSig.indexBufferView = ParamsToIndexBufferView(clipSig.indexBufferView);
        toDrawCommandSig.drawIndexedArguments = ParamsToDrawIndexedArgumens(clipSig.drawIndexedArguments0, clipSig.drawIndexedArguments1.x);
        
        toDrawCommandSigBuffer.Append(toDrawCommandSig);
    }
}
