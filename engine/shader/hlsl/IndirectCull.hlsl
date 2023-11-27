#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "InputTypes.hlsli"
#include "d3d12.hlsli"

ConstantBuffer<FrameUniforms> g_FrameUniforms : register(b0, space0);

StructuredBuffer<PerRenderableMeshData> g_MeshesInstance : register(t0, space0);
StructuredBuffer<PerMaterialViewIndexBuffer> g_MaterialsInstance : register(t1, space0);

AppendStructuredBuffer<uint2> g_OpaqueSortIndexDisBuffer : register(u0, space0);
AppendStructuredBuffer<uint2> g_TransSortIndexDisBuffer : register(u1, space0);

[numthreads(128, 1, 1)]
void CSMain(CSParams Params) {

    ConstantBuffer<FrameUniforms>                mMeshPerframeBuffer      = g_FrameUniforms;
    StructuredBuffer<PerRenderableMeshData>      meshesInstanceBuffer     = g_MeshesInstance;
    StructuredBuffer<PerMaterialViewIndexBuffer> materialsInstance        = g_MaterialsInstance;
    AppendStructuredBuffer<uint2>                opaqueSortIndexDisBuffer = g_OpaqueSortIndexDisBuffer;
    AppendStructuredBuffer<uint2>                transSortIndexDisBuffer  = g_TransSortIndexDisBuffer;

    uint index = (Params.GroupID.x * 128) + Params.GroupIndex;
    if (index < mMeshPerframeBuffer.meshUniform.totalMeshCount)
    {
        PerRenderableMeshData mesh     = meshesInstanceBuffer[index];
        PerMaterialViewIndexBuffer material = materialsInstance[mesh.perMaterialViewIndexBufferIndex];

        StructuredBuffer<PerMaterialParametersBuffer> matBuffer = ResourceDescriptorHeap[material.parametersBufferIndex];

        BoundingBox aabb;
        mesh.boundingBox.Transform(mesh.worldFromModelMatrix, aabb);

        Frustum frustum = ExtractPlanesDX(mMeshPerframeBuffer.cameraUniform.curFrameUniform.clipFromWorldMatrix);

        bool visible = FrustumContainsBoundingBox(frustum, aabb) != CONTAINMENT_DISJOINT;
        if (visible)
        {
            float3 cameraPos  = mMeshPerframeBuffer.cameraUniform.curFrameUniform.cameraPosition;
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
