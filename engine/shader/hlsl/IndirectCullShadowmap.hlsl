#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "InputTypes.hlsli"
#include "d3d12.hlsli"

struct CommandSignatureParams
{
    uint                         MeshIndex;
    D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
    D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
    D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
};

#if defined(SPOTSHADOW)
cbuffer RootConstants : register(b0, space0) { uint spotIndex; };
#elif defined(DIRECTIONSHADOW)
cbuffer RootConstants : register(b0, space0) { uint cascadeLevel; };
#endif

ConstantBuffer<FrameUniforms> g_FrameUniforms : register(b1, space0);

StructuredBuffer<PerRenderableMeshData> g_MeshesInstance : register(t0, space0);
StructuredBuffer<PerMaterialViewIndexBuffer> g_MaterialsInstance : register(t1, space0);

AppendStructuredBuffer<CommandSignatureParams> g_DrawSceneCommandBuffer : register(u0, space0);

[numthreads(128, 1, 1)]
void CSMain(CSParams Params) {
    // Each thread processes one mesh instance
    // Compute index and ensure is within bounds
    uint index = (Params.GroupID.x * 128) + Params.GroupIndex;
    if (index < g_FrameUniforms.meshUniform.totalMeshCount)
    {
        PerRenderableMeshData mesh = g_MeshesInstance[index];
        PerMaterialViewIndexBuffer material = g_MaterialsInstance[mesh.perMaterialViewIndexBufferIndex];

        StructuredBuffer<PerMaterialParametersBuffer> matBuffer =
            ResourceDescriptorHeap[material.parametersBufferIndex];

        BoundingBox aabb;
        mesh.boundingBox.Transform(mesh.worldFromModelMatrix, aabb);

#if defined(DIRECTIONSHADOW)
        Frustum frustum = ExtractPlanesDX(g_FrameUniforms.directionalLight.directionalLightShadowmap.light_proj_view[cascadeLevel]);
#elif defined(SPOTSHADOW)
        Frustum frustum = ExtractPlanesDX(g_FrameUniforms.spotLightUniform.spotLightStructs[spotIndex].spotLightShadowmap.light_proj_view);
#else
        Frustum frustum = ExtractPlanesDX(g_FrameUniforms.cameraUniform.curFrameUniform.clipFromWorldMatrix);
#endif

        bool visible = FrustumContainsBoundingBox(frustum, aabb) != CONTAINMENT_DISJOINT;
        if (visible)
        {
            if (!matBuffer[0].is_blend)
            {
                CommandSignatureParams command;
                command.MeshIndex            = index;
                command.VertexBuffer         = mesh.vertexBuffer;
                command.IndexBuffer          = mesh.indexBuffer;
                command.DrawIndexedArguments = mesh.drawIndexedArguments;
                g_DrawSceneCommandBuffer.Append(command);
            }
        }
    }
}
