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

StructuredBuffer<MeshInstance> g_MeshesInstance : register(t0, space0);

ByteAddressBuffer g_CounterBuffer : register(t1, space0);

ByteAddressBuffer g_SortIndexDisBuffer : register(t2, space0);

AppendStructuredBuffer<CommandSignatureParams> g_DrawCommandBuffer : register(u0, space0);


[numthreads(8, 1, 1)]
void CSMain(CSParams Params) {
    uint ListCount = g_CounterBuffer.Load(0);
    uint index = (Params.GroupID.x * 8) + Params.GroupIndex;
    if (index < ListCount)
    {
        uint meshIndex = g_SortIndexDisBuffer.Load(index * 8);

        MeshInstance mesh = g_MeshesInstance[meshIndex];
        
        CommandSignatureParams command;
        command.MeshIndex            = meshIndex;
        command.VertexBuffer         = mesh.vertexBuffer;
        command.IndexBuffer          = mesh.indexBuffer;
        command.DrawIndexedArguments = mesh.drawIndexedArguments;

        g_DrawCommandBuffer.Append(command);
    }
}
