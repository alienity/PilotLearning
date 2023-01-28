﻿#include "Math.hlsli"
#include "Shader.hlsli"
#include "SharedTypes.hlsli"
#include "d3d12.hlsli"

/*
struct CommandSignatureParams
{
    uint  MeshIndex;
    float ToCameraDistance;
};
*/

ConstantBuffer<MeshPerframeStorageBufferObject> g_ConstantBufferParams : register(b0, space0);

StructuredBuffer<MeshInstance> g_MeshesInstance : register(t0, space0);
StructuredBuffer<MaterialInstance> g_MaterialsInstance : register(t1, space0);

AppendStructuredBuffer<uint2> g_OpaqueSortIndexDisBuffer : register(u0, space0);
AppendStructuredBuffer<uint2> g_TransSortIndexDisBuffer : register(u1, space0);

[numthreads(128, 1, 1)]
void CSMain(CSParams Params) {
    // Each thread processes one mesh instance
    // Compute index and ensure is within bounds
    uint index = (Params.GroupID.x * 128) + Params.GroupIndex;
    if (index < g_ConstantBufferParams.total_mesh_num)
    {
        MeshInstance mesh = g_MeshesInstance[index];
        MaterialInstance material = g_MaterialsInstance[mesh.materialIndex];

        StructuredBuffer<PerMaterialUniformBufferObject> matBuffer =
            ResourceDescriptorHeap[material.uniformBufferIndex];

        BoundingBox aabb;
        mesh.boundingBox.Transform(mesh.localToWorldMatrix, aabb);

        Frustum frustum = ExtractPlanesDX(g_ConstantBufferParams.cameraInstance.projViewMatrix);

        bool visible = FrustumContainsBoundingBox(frustum, aabb) != CONTAINMENT_DISJOINT;
        if (visible)
        {
            float3 cameraPos = g_ConstantBufferParams.cameraInstance.cameraPosition;
            float3 aabbCenter = aabb.Center;

            float meshDistance = distance(cameraPos, aabbCenter);

            uint2 sortIndexDistance = uint2(index, asuint(meshDistance));

            if (matBuffer[0].is_blend)
            {
                g_TransSortIndexDisBuffer.Append(sortIndexDistance);
            }
            else
            {
                g_OpaqueSortIndexDisBuffer.Append(sortIndexDistance);
            }
        }
    }
}
