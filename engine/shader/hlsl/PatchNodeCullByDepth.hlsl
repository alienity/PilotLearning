#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "InputTypes.hlsli"
#include "d3d12.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint perframeBufferIndex;
    uint maxDepthPyramidIndex;
    uint terrainPatchNodeIndex;
    uint inputPatchNodeIdxBufferIndex;
    uint inputPatchNodeIdxCounterIndex;
    uint nonVisiableIndexBufferIndex;
    uint visiableIndexBufferIndex;
};

SamplerState defaultSampler : register(s10);

[numthreads(128, 1, 1)]
void CSMain(CSParams Params) {

    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perframeBufferIndex];
    Texture2D<float> minDepthPyramid = ResourceDescriptorHeap[maxDepthPyramidIndex];
    StructuredBuffer<TerrainPatchNode> terrainPatchNodeBuffer = ResourceDescriptorHeap[terrainPatchNodeIndex];
    StructuredBuffer<uint> terrainPatchNodeIdxBuffer = ResourceDescriptorHeap[inputPatchNodeIdxBufferIndex];
    StructuredBuffer<uint> terrainPatchNodeIdxCountBuffer = ResourceDescriptorHeap[inputPatchNodeIdxCounterIndex];
    AppendStructuredBuffer<uint> nonVisiableIndexBuffer = ResourceDescriptorHeap[nonVisiableIndexBufferIndex];
    AppendStructuredBuffer<uint> visiableIndexBuffer = ResourceDescriptorHeap[visiableIndexBufferIndex];

    uint terrainPatchNodeCount = terrainPatchNodeIdxCountBuffer[0];

    if(Params.DispatchThreadID.x >= terrainPatchNodeCount)
        return;

    uint index = Params.DispatchThreadID.x;

    uint patchIndex = terrainPatchNodeIdxBuffer[index];
    TerrainPatchNode patchNode = terrainPatchNodeBuffer[patchIndex];

    float heightBias = 0.1f;
    float3 boxMin = float3(patchNode.patchMinPos.x, patchNode.minHeight - heightBias, patchNode.patchMinPos.y);
    float3 boxMax = float3(patchNode.patchMinPos.x + patchNode.nodeWidth, patchNode.maxHeight + heightBias, patchNode.patchMinPos.y + patchNode.nodeWidth);

    BoundingBox patchBox;
    patchBox.Center = (boxMax + boxMin) * 0.5f;
    patchBox.Extents = (boxMax - boxMin) * 0.5f;

    float4x4 projectionMatrix = mFrameUniforms.cameraUniform.lastFrameUniform.clipFromViewMatrix;
    float4x4 worldtoViewMatrix = mFrameUniforms.cameraUniform.lastFrameUniform.viewFromWorldMatrix;
    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    float4x4 localToViewMatrix = mul(worldtoViewMatrix, localToWorldMatrix);
    BoundingBox aabbView;
    patchBox.Transform(localToViewMatrix, aabbView);

    float3 centerView = aabbView.Center;
    float3 extentsView = aabbView.Extents;
    float maxExtentView = max(max(abs(extentsView.x), abs(extentsView.y)), abs(extentsView.z));
    
    int baseMip = ceil(log2(abs(centerView.z)));
    int mipOffset = ceil(log2(max(abs(maxExtentView / pow(2,baseMip)), 1)));
    baseMip += mipOffset;

    float4 clipPos = mul(projectionMatrix, float4(centerView, 1.0f));
    float3 ndc = clipPos.xyz / clipPos.w;

    int mdWidth, mdHeight;
    minDepthPyramid.GetDimensions(mdWidth, mdHeight);

    int2 uvCoord = (ndc.xy*0.5+0.5)*float2(mdWidth,mdHeight);

    float minDepth = minDepthPyramid.Load(int3(uvCoord, baseMip));

    if(ndc.z >= minDepth)
    {
        visiableIndexBuffer.Append(index);
    }
    else
    {
        nonVisiableIndexBuffer.Append(index);
    }
}
