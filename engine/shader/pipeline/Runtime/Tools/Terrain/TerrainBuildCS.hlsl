

#include "./TerrainCommonInput.hlsl"

/*
* 对于WorldLodParams
- nodeSize为Node的边长(米)
- patchExtent等于nodeSize/16
- nodeCount等于WorldSize/nodeSize
- sectorCountPerNode等于2^lod
 */
struct ConsBuffer
{
    float3 CameraPositionWS; // 相机世界空间坐标
    int BoundsHeightRedundance; //包围盒在高度方向留出冗余空间，应对MinMaxHeightTexture的精度不足
    float3 WorldSize; //世界大小
    float Padding0;
    float4 NodeEvaluationC; //节点评价系数。x为距离系数
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

float3 GetNodePositionWS(ConsBuffer inConsBuffer, Texture2D<float> minHeightTexture, Texture2D<float> maxHeightTexture, uint2 nodeLoc, uint lod)
{
    float2 nodePositionWS = GetNodePositionWS2(inConsBuffer, nodeLoc, lod);
    float minHeight = minHeightTexture.mips[lod + 3][nodeLoc].x;
    float maxHeight = maxHeightTexture.mips[lod + 3][nodeLoc].x;
    float y = (minHeight + maxHeight) * 0.5 * inConsBuffer.WorldSize.y;
    return float3(nodePositionWS.x, y, nodePositionWS.y);
}

bool EvaluateNode(ConsBuffer inConsBuffer, Texture2D<float> minHeightTexture, Texture2D<float> maxHeightTexture, uint2 nodeLoc, uint lod)
{
    float3 positionWS = GetNodePositionWS(inConsBuffer, minHeightTexture, maxHeightTexture, nodeLoc, lod);
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
    return GetNodeId(inConsBuffer, uint3(nodeLoc, mip));
}

#ifdef TRAVERSE_QUAD_TREE

cbuffer RootConstants : register(b0, space0)
{
    uint consBufferIndex;
    uint minHeightmapIndex;
    uint maxHeightmapIndex;
    uint consumeNodeListBufferIndex;
    uint appendNodeListBufferIndex;
    uint appendFinalNodeListBufferIndex;
    uint nodeDescriptorsBufferIndex;

    uint PassLOD; //表示TraverseQuadTree kernel执行的LOD级别
};

[numthreads(1,1,1)]
void TraverseQuadTree (uint3 id : SV_DispatchThreadID)
{
    ConstantBuffer<ConsBuffer> InConsBuffer = ResourceDescriptorHeap[consBufferIndex];

    Texture2D<float> MinHeightTexture = ResourceDescriptorHeap[minHeightmapIndex];
    Texture2D<float> MaxHeightTexture = ResourceDescriptorHeap[maxHeightmapIndex];
    
    ConsumeStructuredBuffer<uint2> ConsumeNodeList = ResourceDescriptorHeap[consumeNodeListBufferIndex];
    AppendStructuredBuffer<uint2> AppendNodeList = ResourceDescriptorHeap[appendNodeListBufferIndex];
    AppendStructuredBuffer<uint3> AppendFinalNodeList = ResourceDescriptorHeap[appendFinalNodeListBufferIndex];
    RWStructuredBuffer<NodeDescriptor> NodeDescriptors = ResourceDescriptorHeap[nodeDescriptorsBufferIndex];

    uint2 nodeLoc = ConsumeNodeList.Consume();
    uint nodeId = GetNodeId(InConsBuffer, nodeLoc, PassLOD);
    NodeDescriptor desc = NodeDescriptors[nodeId];
    if(PassLOD > 0 && EvaluateNode(InConsBuffer, MinHeightTexture, MaxHeightTexture, nodeLoc, PassLOD))
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
        AppendFinalNodeList.Append(uint3(nodeLoc, PassLOD));
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

TerrainPatchBounds GetPatchBounds(ConsBuffer inConsBuffer, TerrainRenderPatch patch)
{
    float halfSize = GetPatchExtent(inConsBuffer, patch.lod);
#if ENABLE_SEAM
    halfSize *= 0.9;
#endif
    TerrainPatchBounds bounds;
    float3 boundsMin, boundsMax;
    boundsMin.xz = patch.position - halfSize;
    boundsMax.xz = patch.position + halfSize;
    boundsMin.y = patch.minHeight;
    boundsMax.y = patch.maxHeight;

    bounds.minPosition = boundsMin;
    bounds.maxPosition = boundsMax;
    return bounds;
}

TerrainRenderPatch CreatePatch(ConsBuffer inConsBuffer,
    Texture2D<float> minHeightTexture, Texture2D<float> maxHeightTexture, uint3 nodeLoc, uint2 patchOffset)
{
    uint lod = nodeLoc.z;
    float nodeMeterSize = GetNodeSize(inConsBuffer, lod);
    float patchMeterSize = nodeMeterSize / PATCH_COUNT_PER_NODE;
    float2 nodePositionWS = GetNodePositionWS2(inConsBuffer, nodeLoc.xy, lod);

    uint2 patchLoc = nodeLoc.xy * PATCH_COUNT_PER_NODE + patchOffset;
    //经测试，当min和max相差较小时，RG32似乎还是存在精度问题
    float minHeight = minHeightTexture.mips[lod][patchLoc].r * inConsBuffer.WorldSize.y - inConsBuffer.BoundsHeightRedundance;
    float maxHeight = maxHeightTexture.mips[lod][patchLoc].r * inConsBuffer.WorldSize.y + inConsBuffer.BoundsHeightRedundance;
    TerrainRenderPatch patch;
    patch.lod = lod;
    patch.position = nodePositionWS + (patchOffset - (PATCH_COUNT_PER_NODE - 1) * 0.5) * patchMeterSize;
    patch.minHeight = minHeight;
    patch.maxHeight = maxHeight;
    patch.lodTrans = 0;
    return patch;
}

//返回一个node节点覆盖的Sector范围
uint4 GetSectorBounds(ConsBuffer inConsBuffer, uint3 nodeLoc)
{
    uint sectorCountPerNode = GetSectorCountPerNode(inConsBuffer, nodeLoc.z);
    uint2 sectorMin = nodeLoc.xy * sectorCountPerNode;
    return uint4(sectorMin, sectorMin + sectorCountPerNode - 1);
}

uint GetLod(ConsBuffer inConsBuffer, Texture2D<float4> LodMap, uint2 sectorLoc)
{
    if(sectorLoc.x < 0 || sectorLoc.y < 0 || sectorLoc.x >= SECTOR_COUNT_WORLD || sectorLoc.y >= SECTOR_COUNT_WORLD)
    {
        return 0;
    }
    return round(LodMap[sectorLoc].r * MAX_TERRAIN_LOD);
}

void SetLodTrans(inout TerrainRenderPatch patch, ConsBuffer inConsBuffer, Texture2D<float4> LodMap, uint3 nodeLoc, uint2 patchOffset)
{
    uint lod = nodeLoc.z;
    uint4 sectorBounds = GetSectorBounds(inConsBuffer, nodeLoc);
    int4 lodTrans = int4(0,0,0,0);
    if(patchOffset.x == 0)
    {
        //左边缘
        lodTrans.x = GetLod(inConsBuffer, LodMap, sectorBounds.xy + int2(-1,0)) - lod;
    }

    if(patchOffset.y == 0)
    {
        //下边缘
        lodTrans.y = GetLod(inConsBuffer, LodMap, sectorBounds.xy + int2(0,-1)) - lod;
    }

    if(patchOffset.x == 7)
    {
        //右边缘
        lodTrans.z = GetLod(inConsBuffer, LodMap, sectorBounds.zw + int2(1,0)) - lod;
    }

    if(patchOffset.y == 7)
    {
        //上边缘
        lodTrans.w = GetLod(inConsBuffer, LodMap, sectorBounds.zw + int2(0,1)) - lod;
    }
    patch.lodTrans = (uint4)max(0,lodTrans);
}

bool Cull(TerrainPatchBounds bounds)
{
#if ENABLE_FRUS_CULL
    if(FrustumCull(_CameraFrustumPlanes,bounds)){
        return true;
    }
#endif
#if ENABLE_HIZ_CULL
    if(HizOcclusionCull(bounds)){
        return true;
    }
#endif
    return false;
}

cbuffer RootConstants : register(b0, space0)
{
    uint consBufferIndex;
    uint minHeightmapIndex;
    uint maxHeightmapIndex;
    uint finalNodeListBufferIndex;
    uint patchConsumeListBufferIndex;
    uint culledPatchListBufferIndex;
    uint lodMapIndex;
};

[numthreads(8,8,1)]
void BuildPatches(uint3 id : SV_DispatchThreadID, uint3 groupId:SV_GroupID, uint3 groupThreadId:SV_GroupThreadID)
{
    ConstantBuffer<ConsBuffer> InConsBuffer = ResourceDescriptorHeap[consBufferIndex];

    Texture2D<float> MinHeightTexture = ResourceDescriptorHeap[minHeightmapIndex];
    Texture2D<float> MaxHeightTexture = ResourceDescriptorHeap[maxHeightmapIndex];
    
    StructuredBuffer<uint3> FinalNodeList = ResourceDescriptorHeap[finalNodeListBufferIndex];
    ConsumeStructuredBuffer<TerrainRenderPatch> PatchConsumeList = ResourceDescriptorHeap[patchConsumeListBufferIndex];
    AppendStructuredBuffer<TerrainRenderPatch> CulledPatchList = ResourceDescriptorHeap[culledPatchListBufferIndex];

    Texture2D<float4> LodMap = ResourceDescriptorHeap[lodMapIndex];
    
    uint3 nodeLoc = FinalNodeList[groupId.x];
    uint2 patchOffset = groupThreadId.xy;
    //生成Patch
    TerrainRenderPatch patch = CreatePatch(InConsBuffer, MinHeightTexture, MaxHeightTexture, nodeLoc, patchOffset);

    //裁剪
    TerrainPatchBounds bounds = GetPatchBounds(InConsBuffer, patch);
    if(Cull(bounds))
    {
        return;
    }
    SetLodTrans(patch, InConsBuffer, LodMap, nodeLoc, patchOffset);
    CulledPatchList.Append(patch);
}

#endif