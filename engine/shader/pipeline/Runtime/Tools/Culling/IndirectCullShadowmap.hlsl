#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Material/Lit/LitProperties.hlsl"

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

StructuredBuffer<RenderDataPerDraw> g_MeshesInstance : register(t0, space0);
StructuredBuffer<PropertiesPerMaterial> g_MaterialsInstance : register(t1, space0);

AppendStructuredBuffer<CommandSignatureParams> g_DrawSceneCommandBuffer : register(u0, space0);

[numthreads(128, 1, 1)]
void CSMain(uint3 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex) {
    // Each thread processes one mesh instance
    // Compute index and ensure is within bounds
    uint index = (GroupID.x * 128) + GroupIndex;
    if (index < g_FrameUniforms.meshUniform.totalMeshCount)
    {
        RenderDataPerDraw renderData = g_MeshesInstance[index];
        uint lightPropertyBufferIndexOffset = GetLightPropertyBufferIndexOffset(renderData);
        BoundingBox aabb = GetRendererBounds(renderData);
        
        PropertiesPerMaterial propertiesPerMaterial = g_MaterialsInstance[lightPropertyBufferIndexOffset];

        BoundingBox worldAABB = aabb.Transform(renderData.objectToWorldMatrix);

#if defined(DIRECTIONSHADOW)
        Frustum frustum = ExtractPlanesDX(g_FrameUniforms.directionalLight.directionalLightShadowmap.light_proj_view[cascadeLevel]);
#elif defined(SPOTSHADOW)
        Frustum frustum = ExtractPlanesDX(g_FrameUniforms.spotLightUniform.spotLightStructs[spotIndex].spotLightShadowmap.light_proj_view);
#else
        Frustum frustum = ExtractPlanesDX(g_FrameUniforms.cameraUniform._CurFrameUniform.clipFromWorldMatrix);
#endif

        bool visible = FrustumContainsBoundingBox(frustum, worldAABB) != CONTAINMENT_DISJOINT;
        if (visible)
        {
            if (propertiesPerMaterial._BlendMode != 1)
            {
                CommandSignatureParams command;
                command.MeshIndex            = index;
                command.VertexBuffer         = GetVertexBufferView(renderData);
                command.IndexBuffer          = GetIndexBufferView(renderData);
                command.DrawIndexedArguments = GetDrawIndexedArguments(renderData);
                g_DrawSceneCommandBuffer.Append(command);
            }
        }
    }
}
