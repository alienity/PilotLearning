#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "SharedTypes.hlsli"
#include "d3d12.hlsli"

ConstantBuffer<MeshPerframeBuffer> g_ConstantBufferParams : register(b0, space0);

StructuredBuffer<MeshInstance>     g_MeshesInstance : register(t0, space0);
StructuredBuffer<MaterialInstance> g_MaterialsInstance : register(t1, space0);

AppendStructuredBuffer<uint2> g_OpaqueSortIndexDisBuffer : register(u0, space0);
AppendStructuredBuffer<uint2> g_TransSortIndexDisBuffer : register(u1, space0);

[numthreads(128, 1, 1)]
void CSMain(CSParams Params) {

    ConstantBuffer<MeshPerframeBuffer> mMeshPerframeBuffer      = g_ConstantBufferParams;
    StructuredBuffer<MeshInstance>     meshesInstanceBuffer     = g_MeshesInstance;
    StructuredBuffer<MaterialInstance> materialsInstance        = g_MaterialsInstance;
    AppendStructuredBuffer<uint2>      opaqueSortIndexDisBuffer = g_OpaqueSortIndexDisBuffer;
    AppendStructuredBuffer<uint2>      transSortIndexDisBuffer  = g_TransSortIndexDisBuffer;

    uint index = (Params.GroupID.x * 128) + Params.GroupIndex;
    if (index < mMeshPerframeBuffer.total_mesh_num)
    {
        MeshInstance     mesh     = meshesInstanceBuffer[index];
        MaterialInstance material = materialsInstance[mesh.materialIndex];

        StructuredBuffer<PerMaterialUniformBuffer> matBuffer = ResourceDescriptorHeap[material.uniformBufferIndex];

        BoundingBox aabb;
        mesh.boundingBox.Transform(mesh.localToWorldMatrix, aabb);

        Frustum frustum = ExtractPlanesDX(mMeshPerframeBuffer.cameraInstance.projViewMatrix);

        //bool visible = FrustumContainsBoundingBox(frustum, aabb) != CONTAINMENT_DISJOINT;
        //if (visible)
        {
            float3 cameraPos  = mMeshPerframeBuffer.cameraInstance.cameraPosition;
            float3 aabbCenter = aabb.Center;

            float meshDistance = distance(cameraPos, aabbCenter);

            uint2 sortIndexDistance = uint2(index, asuint(meshDistance));

            if (matBuffer[0].is_blend)
            {
                transSortIndexDisBuffer.Append(sortIndexDistance);
            }
            else
            {
                opaqueSortIndexDisBuffer.Append(sortIndexDistance);
            }
        }
    }
}
