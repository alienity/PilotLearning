#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Tools/VolumeLighting/VolumetricLightingCommon.hlsl"
#include "../../Core/Utilities/GeometryUtils.c.hlsl"


ConstantBuffer<FrameUniforms> _FrameUniform : register(b1, space0);
ConstantBuffer<ShaderVariablesVolumetric> _ShaderVariablesVolumetric : register(b2, space0);
StructuredBuffer<LocalVolumetricFogDatas> _LocalVolumetricDatas : register(t0, space0);
ByteAddressBuffer g_CounterBuffer : register(t1, space0);
ByteAddressBuffer g_SortIndexDisBuffer : register(t2, space0);

RWBuffer<uint> _VolumetricGlobalIndirectArgsBuffer : register(u0, space0);
RWByteAddressBuffer _VolumetricGlobalIndirectionBuffer : register(u1, space0);
RWStructuredBuffer<VolumetricMaterialRenderingData> _VolumetricMaterialData : register(u2, space0);

int DistanceToSlice(float distance, float _VBufferRcpSliceCount, float4 _VBufferDistanceEncodingParams)
{
    float de = _VBufferRcpSliceCount; // Log-encoded distance between slices

    float e1 = EncodeLogarithmicDepthGeneralized(distance, _VBufferDistanceEncodingParams);
    e1 -= de;
    e1 /= de;

    return int(e1 - 0.5);
}

float DepthDistance(float3 position)
{
    float cameraDistance = length(position);
    float3 viewDirWS = normalize(position);

    float4x4 _ViewMatrix = UNITY_MATRIX_V(_FrameUniform.cameraUniform);
    float3 cameraForward = -_ViewMatrix[2].xyz;
    float depth = cameraDistance * dot(viewDirWS, cameraForward);

    return depth;
}

// https://iquilezles.org/www/articles/distfunctions/distfunctions.htm
float DistanceToAABB(float3 p, float3 b)
{
    float3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

// Optimized version of https://www.sciencedirect.com/topics/computer-science/oriented-bounding-box
float DistanceDistanceToOBB(float3 p, OrientedBBox obb)
{
    float3 offset = p - obb.center;
    float3 boxForward = normalize(cross(obb.right, obb.up));
    float3 axisAlignedPoint = float3(dot(offset, normalize(obb.right)), dot(offset, normalize(obb.up)), dot(offset, boxForward));

    return DistanceToAABB(axisAlignedPoint, float3(obb.extentX, obb.extentY, obb.extentZ));
}

float3 ComputeCubeVertexPositionRWS(OrientedBBox obb, float3 minPositionRWS, float3 vertexMask)
{
    float3x3 obbFrame   = float3x3(obb.right, obb.up, cross(obb.right, obb.up));
    float3   obbExtents = float3(obb.extentX, obb.extentY, obb.extentZ);
    return mul((vertexMask * 2 - 1) * obbExtents, (obbFrame)) + obb.center;
}

// vertexMask contains the vertices of the cube in this order
//    6 .+------+ 7
//    .' |    .'|
// 2 +---+--+'3 |
//   |   |  |   |
//   | 4.+--+---+ 5
//   |.'    | .'
// 0 +------+' 1

static const float3 vertexMask[8] = {
    float3(0, 0, 0),
    float3(1, 0, 0),
    float3(0, 1, 0),
    float3(1, 1, 0),
    float3(0, 0, 1),
    float3(1, 0, 1),
    float3(0, 1, 1),
    float3(1, 1, 1),
};

// Sort cube indices to respect the winding order described in 'A COMPARISON OF GPU BOX-PLANE INTERSECTION ALGORITHMSFOR DIRECT VOLUME RENDERING' section 3
// We do this in a compute so we don't need to sample 3 index tables in the vertex shader
static const uint vertexIndicesMap[8 * 8] = {
    0, 1, 4, 2, 3, 5, 6, 7, // front vertex = 1
    1, 5, 3, 0, 4, 7, 2, 6, // front vertex = 1
    2, 3, 0, 6, 7, 1, 4, 5, // front vertex = 2
    3, 7, 1, 2, 6, 5, 0, 4, // front vertex = 3
    4, 0, 5, 6, 2, 1, 7, 3, // front vertex = 4
    5, 1, 7, 4, 0, 3, 6, 2, // front vertex = 5
    6, 2, 4, 7, 3, 0, 5, 1, // front vertex = 6
    7, 6, 5, 3, 2, 4, 1, 0, // front vertex = 7
};

void ComputeCubeVerticesOrder(uint volumeIndex)
{
    LocalVolumetricFogDatas localVolumetricData = _LocalVolumetricDatas[volumeIndex];
    VolumetricMaterialDataCBuffer volumetricMatDataBuffer = localVolumetricData.volumeMaterialDataCBuffer;

    OrientedBBox obb;
    obb.center = volumetricMatDataBuffer._VolumetricMaterialObbCenter;
    obb.right = volumetricMatDataBuffer._VolumetricMaterialObbRight;
    obb.up = volumetricMatDataBuffer._VolumetricMaterialObbUp;
    obb.extentX = volumetricMatDataBuffer._VolumetricMaterialObbExtents.x;
    obb.extentY = volumetricMatDataBuffer._VolumetricMaterialObbExtents.y;
    obb.extentZ = volumetricMatDataBuffer._VolumetricMaterialObbExtents.z;
    
    // OrientedBBox obb = _VolumeBounds[volumeIndex]; // the OBB is in world space
    float3 obbExtents = float3(obb.extentX, obb.extentY, obb.extentZ);

    float4x4 _ViewMatrix = UNITY_MATRIX_V(_FrameUniform.cameraUniform);
    
    // Find the min and max distance value from vertices
    float3 minPositionRWS = obb.center - obbExtents;
    float minVertexDistance = -1;
    int frontVertexIndex = 0; // Compute the index of the vertex in front of the camera, needed for the slicing algorithm
    int i;
    for (i = 0; i < 8; i++)
    {
        // Select the correct starting vertex to sort the cube vertices
        float3 vertexNormal = normalize(vertexMask[i] * 2 - 1);
        float3 cameraForward = -_ViewMatrix[2].xyz;
        float d = dot(vertexNormal, cameraForward);
        if (d > minVertexDistance)
        {
            minVertexDistance = d;
            frontVertexIndex = i;
        }
    }

    // Write the cube vertices in a certain order respecting the winding described in vertexIndicesMap.
    uint vertexIndexMapOffset = frontVertexIndex * 8;
    for (i = 0; i < 8; i++)
    {
        float3 vertex = vertexMask[vertexIndicesMap[vertexIndexMapOffset + i]];
        _VolumetricMaterialData[volumeIndex].obbVertexPositionWS[i].xyz = ComputeCubeVertexPositionRWS(obb, minPositionRWS, vertex);
    }
}

// Generate the compute buffer to dispatch the indirect draw
[numthreads(32, 1, 1)]
void ComputeVolumetricMaterialRenderingParameters(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    int _VolumeCount = g_CounterBuffer.Load(0);
    
    if (dispatchThreadID.x >= _VolumeCount)
        return;
    
    uint volumeIndex = dispatchThreadID.x % _VolumeCount;
    uint globalBufferWriteIndex = g_SortIndexDisBuffer.Load(volumeIndex * 8);
    
    // uint globalBufferWriteIndex = _VolumetricVisibleGlobalIndicesBuffer.Load(volumeIndex << 2);
    uint materialDataIndex = volumeIndex;

    LocalVolumetricFogDatas localVolumetricData = _LocalVolumetricDatas[volumeIndex];
    VolumetricMaterialDataCBuffer localVolumeMatData = localVolumetricData.volumeMaterialDataCBuffer;
    
    float4 _VBufferDistanceDecodingParams = _ShaderVariablesVolumetric._VBufferPrevDistanceDecodingParams;
    float4 _VBufferDistanceEncodingParams = _ShaderVariablesVolumetric._VBufferPrevDistanceEncodingParams;
    float _MaxSliceCount = _ShaderVariablesVolumetric._MaxSliceCount;
    float _VBufferRcpSliceCount = _ShaderVariablesVolumetric._VBufferRcpSliceCount;
    
    OrientedBBox obb;
    obb.center = localVolumeMatData._VolumetricMaterialObbCenter;
    obb.right = localVolumeMatData._VolumetricMaterialObbRight;
    obb.up = localVolumeMatData._VolumetricMaterialObbUp;
    obb.extentX = localVolumeMatData._VolumetricMaterialObbExtents[0];
    obb.extentY = localVolumeMatData._VolumetricMaterialObbExtents[1];
    obb.extentZ = localVolumeMatData._VolumetricMaterialObbExtents[2];

    // OrientedBBox obb = _VolumeBounds[volumeIndex]; // the OBB is in world space
    float3 obbExtents = float3(obb.extentX, obb.extentY, obb.extentZ);

    float2 minPositionVS = 1;
    float2 maxPositionVS = -1;

    float cameraDistanceToOBB = DistanceDistanceToOBB(GetCameraPositionWS(_FrameUniform), obb);

    if (cameraDistanceToOBB <= 0)
    {
        minPositionVS = -1;
        maxPositionVS = 1;
    }

    // Find the min and max distance value from vertices
    float3 minPositionRWS = obb.center - obbExtents;
    float maxVertexDepth = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        float3 position = ComputeCubeVertexPositionRWS(obb, minPositionRWS, vertexMask[i]);
        float distance = length(position);
        maxVertexDepth = max(maxVertexDepth, distance);

        if (cameraDistanceToOBB > 0)
        {
            float4 positionCS = TransformWorldToHClip(_FrameUniform, position);

            // clamp positionCS inside the view in case the point is behind the camera
            if (positionCS.w < 0)
            {
                minPositionVS = -1;
                maxPositionVS = 1;
            }
            else
            {
                positionCS.xy /= positionCS.w;
                minPositionVS = min(positionCS.xy, minPositionVS);
                maxPositionVS = max(positionCS.xy, maxPositionVS);
            }
        }
    }

    minPositionVS = clamp(minPositionVS, -1, 1);
    maxPositionVS = clamp(maxPositionVS, -1, 1);

    float vBufferNearPlane = DecodeLogarithmicDepthGeneralized(0, _VBufferDistanceDecodingParams);
    float minBoxDistance = max(vBufferNearPlane, cameraDistanceToOBB);
    int startSliceIndex = clamp(DistanceToSlice(minBoxDistance, _VBufferRcpSliceCount, _VBufferDistanceEncodingParams), 0, int(_MaxSliceCount));
    int stopSliceIndex = DistanceToSlice(maxVertexDepth, _VBufferRcpSliceCount, _VBufferDistanceEncodingParams);
    uint sliceCount = clamp(stopSliceIndex - startSliceIndex, 0, int(_MaxSliceCount) - startSliceIndex);

    _VolumetricGlobalIndirectArgsBuffer[globalBufferWriteIndex * 5 + 0] = 6; // IndexCountPerInstance
    _VolumetricGlobalIndirectArgsBuffer[globalBufferWriteIndex * 5 + 1] = sliceCount; // InstanceCount
    _VolumetricGlobalIndirectArgsBuffer[globalBufferWriteIndex * 5 + 2] = 0; // StartIndexLocation
    _VolumetricGlobalIndirectArgsBuffer[globalBufferWriteIndex * 5 + 3] = 0; // BaseVertexLocation
    _VolumetricGlobalIndirectArgsBuffer[globalBufferWriteIndex * 5 + 4] = 0; // StartInstanceLocation

    // Provide smaller buffer index in the global indirection buffer to sample those buffers in the vertex during the voxelization.
    _VolumetricGlobalIndirectionBuffer.Store(globalBufferWriteIndex << 2, volumeIndex);

    _VolumetricMaterialData[materialDataIndex].sliceCount = sliceCount;
    _VolumetricMaterialData[materialDataIndex].startSliceIndex = startSliceIndex;
    _VolumetricMaterialData[materialDataIndex].viewSpaceBounds = float4(minPositionVS, maxPositionVS - minPositionVS);
}