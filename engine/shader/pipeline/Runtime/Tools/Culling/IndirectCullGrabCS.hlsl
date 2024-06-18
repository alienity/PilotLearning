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

StructuredBuffer<RenderDataPerDraw> g_MeshesInstance : register(t0, space0);

ByteAddressBuffer g_CounterBuffer : register(t1, space0);

ByteAddressBuffer g_SortIndexDisBuffer : register(t2, space0);

AppendStructuredBuffer<CommandSignatureParams> g_DrawCommandBuffer : register(u0, space0);


[numthreads(8, 1, 1)]
void CSMain(uint3 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex) {
    uint ListCount = g_CounterBuffer.Load(0);
    uint index = (GroupID.x * 8) + GroupIndex;
    if (index < ListCount)
    {
        uint meshIndex = g_SortIndexDisBuffer.Load(index * 8);

        RenderDataPerDraw renderData = g_MeshesInstance[meshIndex];
        
        CommandSignatureParams command;
        command.MeshIndex            = meshIndex;
        command.VertexBuffer         = GetVertexBufferView(renderData);
        command.IndexBuffer          = GetIndexBufferView(renderData);
        command.DrawIndexedArguments = GetDrawIndexedArguments(renderData);

        g_DrawCommandBuffer.Append(command);
    }
}
