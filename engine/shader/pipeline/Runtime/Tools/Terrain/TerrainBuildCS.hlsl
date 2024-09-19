

#include "./TerrainCommonInput.hlsl"

struct ConsBuffer
{
    uint PassLOD; //表示TraverseQuadTree kernel执行的LOD级别
    float3 CameraPositionWS;
    int BoundsHeightRedundance; //包围盒在高度方向留出冗余空间，应对MinMaxHeightTexture的精度不足
    float3 WorldSize; //世界大小
    float4 NodeEvaluationC; //节点评价系数。x为距离系数
    /*
    - nodeSize为Node的边长(米)
    - patchExtent等于nodeSize/16
    - nodeCount等于WorldSize/nodeSize
    - sectorCountPerNode等于2^lod
     */
    float4 WorldLodParams[6]; // (nodeSize,patchExtent,nodeCount,sectorCountPerNode)
    uint NodeIDOffsetOfLOD[6];
};

SamplerState s_point_clamp_sampler      : register(s10);
SamplerState s_linear_clamp_sampler     : register(s11);
SamplerState s_linear_repeat_sampler    : register(s12);
SamplerState s_trilinear_clamp_sampler  : register(s13);
SamplerState s_trilinear_repeat_sampler : register(s14);

float GetNodeSize(ConsBuffer inConsBuffer, uint lod)
{
    return inConsBuffer.WorldLodParams[lod].x;
}

float GetNodeCount(ConsBuffer inConsBuffer, uint lod)
{
    return inConsBuffer.WorldLodParams[lod].z;
}

float GetPatchExtent(ConsBuffer inConsBuffer, uint lod)
{
    return inConsBuffer.WorldLodParams[lod].y;
}

uint GetSectorCountPerNode(ConsBuffer inConsBuffer, uint lod)
{
    return (uint)inConsBuffer.WorldLodParams[lod].w;
}

float2 GetNodePositionWS2(ConsBuffer inConsBuffer, uint2 nodeLoc, uint mip)
{
    float nodeMeterSize = GetNodeSize(inConsBuffer, mip);
    float nodeCount = GetNodeCount(inConsBuffer, mip);
    float2 nodePositionWS = ((float2)nodeLoc - (nodeCount-1)*0.5) * nodeMeterSize;
    return nodePositionWS;
}

float3 GetNodePositionWS(ConsBuffer inConsBuffer, Texture2D<float4> minMaxHeightTexture, uint2 nodeLoc, uint lod)
{
    float2 nodePositionWS = GetNodePositionWS2(inConsBuffer, nodeLoc,lod);
    float2 minMaxHeight = minMaxHeightTexture.mips[lod + 3][nodeLoc].xy;
    float y = (minMaxHeight.x + minMaxHeight.y) * 0.5 * inConsBuffer.WorldSize.y;
    return float3(nodePositionWS.x, y, nodePositionWS.y);
}

bool EvaluateNode(ConsBuffer inConsBuffer, Texture2D<float4> minMaxHeightTexture, uint2 nodeLoc, uint lod)
{
    float3 positionWS = GetNodePositionWS(inConsBuffer, minMaxHeightTexture, nodeLoc,lod);
    float dis = distance(inConsBuffer.CameraPositionWS, positionWS);
    float nodeSize = GetNodeSize(inConsBuffer, lod);
    float f = dis / (nodeSize * inConsBuffer.NodeEvaluationC.x);
    if( f < 1)
    {
        return true;
    }
    return false;
}

uint GetNodeId(ConsBuffer inConsBuffer, uint3 nodeLoc)
{
    return inConsBuffer.NodeIDOffsetOfLOD[nodeLoc.z] + nodeLoc.y * GetNodeCount(inConsBuffer, nodeLoc.z) + nodeLoc.x;
}

uint GetNodeId(ConsBuffer inConsBuffer, uint2 nodeLoc, uint mip)
{
    return GetNodeId(inConsBuffer, uint3(nodeLoc,mip));
}


#ifdef TRAVERSE_QUAD_TREE

cbuffer RootConstants : register(b0, space0)
{
    uint consBufferIndex;
    uint minmaxHeightmapIndex;
    uint consumeNodeListBufferIndex;
    uint appendNodeListBufferIndex;
    uint appendFinalNodeListBufferIndex;
    uint nodeDescriptorsBufferIndex;
};

[numthreads(1,1,1)]
void TraverseQuadTree (uint3 id : SV_DispatchThreadID)
{
    ConstantBuffer<ConsBuffer> InConsBuffer = ResourceDescriptorHeap[consBufferIndex];

    Texture2D<float4> MinMaxHeightTexture = ResourceDescriptorHeap[minmaxHeightmapIndex];
    
    ConsumeStructuredBuffer<uint2> ConsumeNodeList = ResourceDescriptorHeap[consumeNodeListBufferIndex];
    AppendStructuredBuffer<uint2> AppendNodeList = ResourceDescriptorHeap[appendNodeListBufferIndex];
    AppendStructuredBuffer<uint3> AppendFinalNodeList = ResourceDescriptorHeap[appendFinalNodeListBufferIndex];
    RWStructuredBuffer<NodeDescriptor> NodeDescriptors = ResourceDescriptorHeap[nodeDescriptorsBufferIndex];
    
    uint2 nodeLoc = ConsumeNodeList.Consume();
    uint nodeId = GetNodeId(InConsBuffer, nodeLoc, InConsBuffer.PassLOD);
    NodeDescriptor desc = NodeDescriptors[nodeId];
    if(InConsBuffer.PassLOD > 0 && EvaluateNode(InConsBuffer, MinMaxHeightTexture, nodeLoc, InConsBuffer.PassLOD))
    {
        //divide
        AppendNodeList.Append(nodeLoc * 2);
        AppendNodeList.Append(nodeLoc * 2 + uint2(1,0));
        AppendNodeList.Append(nodeLoc * 2 + uint2(0,1));
        AppendNodeList.Append(nodeLoc * 2 + uint2(1,1));
        desc.branch = 1;
    }
    else
    {
        AppendFinalNodeList.Append(uint3(nodeLoc, InConsBuffer.PassLOD));
        desc.branch = 0;
    }
    NodeDescriptors[nodeId] = desc;
}

#elif defined(BUILD_LODMAP)

cbuffer RootConstants : register(b0, space0)
{
    uint consBufferIndex;
    uint nodeDescriptorsBufferIndex;
    uint lodMapIndex;
};

[numthreads(8,8,1)]
void BuildLodMap(uint3 id : SV_DispatchThreadID)
{
    ConstantBuffer<ConsBuffer> InConsBuffer = ResourceDescriptorHeap[consBufferIndex];
    StructuredBuffer<NodeDescriptor> NodeDescriptors = ResourceDescriptorHeap[nodeDescriptorsBufferIndex];
    RWTexture2D<float4> LodMap = ResourceDescriptorHeap[lodMapIndex];
    
    uint2 sectorLoc = id.xy;
    [unroll(MAX_TERRAIN_LOD)]
    for(uint lod = MAX_TERRAIN_LOD; lod >= 0; lod --)
    {
        uint sectorCount = GetSectorCountPerNode(InConsBuffer, lod);
        uint2 nodeLoc = sectorLoc / sectorCount;
        uint nodeId = GetNodeId(InConsBuffer, nodeLoc, lod);
        NodeDescriptor desc = NodeDescriptors[nodeId];
        if(desc.branch == 0)
        {
            LodMap[sectorLoc] = lod * 1.0 / MAX_TERRAIN_LOD;
            return;
        }
    }
    LodMap[sectorLoc] = 0;
}

#elif defined(BUILD_PATCHES)

cbuffer RootConstants : register(b0, space0)
{
    uint consBufferIndex;
    uint finalNodeListBufferIndex;
    uint nodeDescriptorsBufferIndex;
    uint lodMapIndex;
};

[numthreads(8,8,1)]
void BuildPatches(uint3 id : SV_DispatchThreadID,uint3 groupId:SV_GroupID,uint3 groupThreadId:SV_GroupThreadID)
{
    ConstantBuffer<ConsBuffer> InConsBuffer = ResourceDescriptorHeap[consBufferIndex];
    StructuredBuffer<uint3> FinalNodeList = ResourceDescriptorHeap[finalNodeListBufferIndex];
    
    uint3 nodeLoc = FinalNodeList[groupId.x];
    uint2 patchOffset = groupThreadId.xy;
    //生成Patch
    RenderPatch patch = CreatePatch(nodeLoc,patchOffset);

    //裁剪
    Bounds bounds = GetPatchBounds(patch);
    if(Cull(bounds)){
        return;
    }
    SetLodTrans(patch,nodeLoc,patchOffset);
    CulledPatchList.Append(patch);
#if BOUNDS_DEBUG
    BoundsDebug boundsDebug;
    boundsDebug.bounds = bounds;
    boundsDebug.color = float4((bounds.minPosition + _WorldSize * 0.5) / _WorldSize,1);
    PatchBoundsList.Append(boundsDebug);
#endif
}

#endif