

#include "./TerrainCommonInput.hlsl"
#include "../../ShaderLibrary/Common.hlsl"

// https://github.com/wlgys8/GPUDrivenTerrainLearn

float GetNodeSize(TerrainConsData inConsBuffer, uint lod)
{
    return inConsBuffer.WorldLodParams[lod].x;
}

float GetNodeCount(TerrainConsData inConsBuffer, uint lod)
{
    return inConsBuffer.WorldLodParams[lod].z;
}

float GetPatchExtent(TerrainConsData inConsBuffer, uint lod)
{
    return inConsBuffer.WorldLodParams[lod].y;
}

uint GetSectorCountPerNode(TerrainConsData inConsBuffer, uint lod)
{
    return (uint)inConsBuffer.WorldLodParams[lod].w;
}

float2 GetNodePositionWS2(TerrainConsData inConsBuffer, uint2 nodeLoc, uint mip)
{
    float nodeMeterSize = GetNodeSize(inConsBuffer, mip);
    float nodeCount = GetNodeCount(inConsBuffer, mip);
    float2 nodePositionWS = ((float2)nodeLoc - (nodeCount-1)*0.5) * nodeMeterSize;
    return nodePositionWS;
}

float3 GetNodePositionWS(TerrainConsData inConsBuffer, Texture2D<float> minHeightTexture, Texture2D<float> maxHeightTexture, uint2 nodeLoc, uint lod)
{
    float2 nodePositionWS = GetNodePositionWS2(inConsBuffer, nodeLoc, lod);
    float minHeight = minHeightTexture.mips[lod + 3][nodeLoc].x;
    float maxHeight = maxHeightTexture.mips[lod + 3][nodeLoc].x;
    float y = (minHeight + maxHeight) * 0.5 * inConsBuffer.WorldSize.y;
    return float3(nodePositionWS.x, y, nodePositionWS.y);
}

bool EvaluateNode(TerrainConsData inConsBuffer, Texture2D<float> minHeightTexture, Texture2D<float> maxHeightTexture, uint2 nodeLoc, uint lod)
{
    float3 positionWS = GetNodePositionWS(inConsBuffer, minHeightTexture, maxHeightTexture, nodeLoc, lod);
    positionWS = mul(inConsBuffer.TerrainModelMatrix, float4(positionWS, 1.0f)).xyz;
    float dis = distance(inConsBuffer.CameraPositionWS, positionWS);
    float nodeSize = GetNodeSize(inConsBuffer, lod);
    float f = dis / (nodeSize * inConsBuffer.NodeEvaluationC.x);
    if( f <= 1.0f)
    {
        return true;
    }
    return false;
}

uint GetNodeId(TerrainConsData inConsBuffer, uint3 nodeLoc)
{
    return inConsBuffer.NodeIDOffsetOfLOD[nodeLoc.z] + nodeLoc.y * GetNodeCount(inConsBuffer, nodeLoc.z) + nodeLoc.x;
}

uint GetNodeId(TerrainConsData inConsBuffer, uint2 nodeLoc, uint mip)
{
    return GetNodeId(inConsBuffer, uint3(nodeLoc, mip));
}

#ifdef INIT_QUAD_TREE

cbuffer RootConstants : register(b0, space0)
{
    uint appendNodeListBufferIndex;
};

[numthreads(1,1,1)]
void InitQuadTree (uint3 id : SV_DispatchThreadID)
{
    AppendStructuredBuffer<uint2> AppendNodeList = ResourceDescriptorHeap[appendNodeListBufferIndex];

    const int PassLOD = 0;
    for(int i = 0; i < MAX_LOD_NODE_COUNT; i++)
    {
        for(int j = 0; j < MAX_LOD_NODE_COUNT; j++)
        {
            int2 nodeLoc = int2(i, j);
            AppendNodeList.Append(nodeLoc);
        }
    }
}

#elif TRAVERSE_QUAD_TREE

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
    ConstantBuffer<TerrainConsData> InConsBuffer = ResourceDescriptorHeap[consBufferIndex];

    Texture2D<float> MinHeightTexture = ResourceDescriptorHeap[minHeightmapIndex];
    Texture2D<float> MaxHeightTexture = ResourceDescriptorHeap[maxHeightmapIndex];
    
    ConsumeStructuredBuffer<uint2> ConsumeNodeList = ResourceDescriptorHeap[consumeNodeListBufferIndex];
    AppendStructuredBuffer<uint2> AppendNodeList = ResourceDescriptorHeap[appendNodeListBufferIndex];
    AppendStructuredBuffer<uint3> AppendFinalNodeList = ResourceDescriptorHeap[appendFinalNodeListBufferIndex];
    RWStructuredBuffer<uint> NodeDescriptors = ResourceDescriptorHeap[nodeDescriptorsBufferIndex];

    uint2 nodeLoc = ConsumeNodeList.Consume();
    uint nodeId = GetNodeId(InConsBuffer, nodeLoc, PassLOD);
    uint branch;
    if(PassLOD > 0 && EvaluateNode(InConsBuffer, MinHeightTexture, MaxHeightTexture, nodeLoc, PassLOD))
    {
        //divide
        AppendNodeList.Append(nodeLoc * 2);
        AppendNodeList.Append(nodeLoc * 2 + uint2(1,0));
        AppendNodeList.Append(nodeLoc * 2 + uint2(0,1));
        AppendNodeList.Append(nodeLoc * 2 + uint2(1,1));
        branch = 1;
    }
    else
    {
        AppendFinalNodeList.Append(uint3(nodeLoc, PassLOD));
        branch = 0;
    }
    NodeDescriptors[nodeId] = branch;
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
    ConstantBuffer<TerrainConsData> InConsBuffer = ResourceDescriptorHeap[consBufferIndex];
    StructuredBuffer<uint> NodeDescriptors = ResourceDescriptorHeap[nodeDescriptorsBufferIndex];
    RWTexture2D<float4> LodMap = ResourceDescriptorHeap[lodMapIndex];
    
    uint2 sectorLoc = id.xy;
    [unroll(MAX_TERRAIN_LOD)]
    for(uint lod = MAX_TERRAIN_LOD; lod >= 0; lod --)
    {
        uint sectorCount = GetSectorCountPerNode(InConsBuffer, lod);
        uint2 nodeLoc = sectorLoc / sectorCount;
        uint nodeId = GetNodeId(InConsBuffer, nodeLoc, lod);
        uint branch = NodeDescriptors[nodeId];
        if(branch == 0)
        {
            LodMap[sectorLoc] = lod * 1.0 / MAX_TERRAIN_LOD;
            return;
        }
    }
    LodMap[sectorLoc] = 0;
}

#elif defined(BUILD_PATCHES)

TerrainPatchBounds GetPatchBounds(TerrainConsData inConsBuffer, TerrainRenderPatch patch)
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

TerrainRenderPatch CreatePatch(
    TerrainConsData inConsBuffer, Texture2D<float> minHeightTexture, Texture2D<float> maxHeightTexture, uint3 nodeLoc, uint2 patchOffset)
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
uint4 GetSectorBounds(TerrainConsData inConsBuffer, uint3 nodeLoc)
{
    uint sectorCountPerNode = GetSectorCountPerNode(inConsBuffer, nodeLoc.z);
    uint2 sectorMin = nodeLoc.xy * sectorCountPerNode;
    return uint4(sectorMin, sectorMin + sectorCountPerNode - 1);
}

uint GetLod(TerrainConsData inConsBuffer, Texture2D<float4> LodMap, uint2 sectorLoc)
{
    if(sectorLoc.x < 0 || sectorLoc.y < 0 || sectorLoc.x >= SECTOR_COUNT_WORLD || sectorLoc.y >= SECTOR_COUNT_WORLD)
    {
        return 0;
    }
    return round(LodMap[sectorLoc].r * MAX_TERRAIN_LOD);
}

void SetLodTrans(inout TerrainRenderPatch patch, TerrainConsData inConsBuffer, Texture2D<float4> LodMap, uint3 nodeLoc, uint2 patchOffset)
{
    uint lod = nodeLoc.z;
    uint4 sectorBounds = GetSectorBounds(inConsBuffer, nodeLoc);
    int4 lodTrans = int4(0,0,0,0);
    if(patchOffset.x == 0)
    {
        //左边缘
        lodTrans.x = GetLod(inConsBuffer, LodMap, sectorBounds.xy + int2(-1, 0)) - lod;
    }

    if(patchOffset.y == 0)
    {
        //下边缘
        lodTrans.y = GetLod(inConsBuffer, LodMap, sectorBounds.xy + int2(0, -1)) - lod;
    }

    if(patchOffset.x == 7)
    {
        //右边缘
        lodTrans.z = GetLod(inConsBuffer, LodMap, sectorBounds.zw + int2(1, 0)) - lod;
    }

    if(patchOffset.y == 7)
    {
        //上边缘
        lodTrans.w = GetLod(inConsBuffer, LodMap, sectorBounds.zw + int2(0, 1)) - lod;
    }
    patch.lodTrans = (uint4)max(0, lodTrans);
}

bool Cull(TerrainConsData inConsBuffer, TerrainPatchBounds bounds)
{
#if ENABLE_FRUS_CULL
    Frustum frustum = ExtractPlanesDX(inConsBuffer.CameraViewProj);
    float3 bCenter = (bounds.maxPosition + bounds.minPosition) * 0.5f;
    float3 bExtents = (bounds.maxPosition - bounds.minPosition) * 0.5f;
    bCenter = mul(inConsBuffer.TerrainModelMatrix, float4(bCenter, 1.0f)).xyz;
    BoundingBox clipBoundingBox;
    clipBoundingBox.Center = float4(bCenter, 0);
    clipBoundingBox.Extents = float4(bExtents, 0);
    if(FrustumContainsBoundingBox(frustum, clipBoundingBox) == CONTAINMENT_DISJOINT)
    {
        return true;
    }
#endif
#if ENABLE_HIZ_CULL
    if(HizOcclusionCull(bounds))
    {
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
    uint lodMapIndex;
    uint finalNodeListBufferIndex;
    uint culledPatchListBufferIndex;
};

[numthreads(8,8,1)]
void BuildPatches(uint3 id : SV_DispatchThreadID, uint3 groupId: SV_GroupID, uint3 groupThreadId: SV_GroupThreadID)
{
    ConstantBuffer<TerrainConsData> InConsBuffer = ResourceDescriptorHeap[consBufferIndex];

    Texture2D<float> MinHeightTexture = ResourceDescriptorHeap[minHeightmapIndex];
    Texture2D<float> MaxHeightTexture = ResourceDescriptorHeap[maxHeightmapIndex];
    
    StructuredBuffer<uint3> FinalNodeList = ResourceDescriptorHeap[finalNodeListBufferIndex];
    AppendStructuredBuffer<TerrainRenderPatch> CulledPatchList = ResourceDescriptorHeap[culledPatchListBufferIndex];

    Texture2D<float4> LodMap = ResourceDescriptorHeap[lodMapIndex];
    
    uint3 nodeLoc = FinalNodeList[groupId.x];
    uint2 patchOffset = groupThreadId.xy;
    //生成Patch
    TerrainRenderPatch patch = CreatePatch(InConsBuffer, MinHeightTexture, MaxHeightTexture, nodeLoc, patchOffset);

    //裁剪
    TerrainPatchBounds bounds = GetPatchBounds(InConsBuffer, patch);
    if(Cull(InConsBuffer, bounds))
    {
        return;
    }
    SetLodTrans(patch, InConsBuffer, LodMap, nodeLoc, patchOffset);
    CulledPatchList.Append(patch);
}

#endif