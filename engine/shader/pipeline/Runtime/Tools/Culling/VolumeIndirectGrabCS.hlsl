#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Tools/VolumeLighting/VolumetricLightingCommon.hlsl"

struct CommandSignatureParams
{
    uint _VolumetricFogGlobalIndex;
    D3D12_DRAW_ARGUMENTS DrawArguments;
};

// ConstantBuffer<FrameUniforms> frameUniform : register(b1, space0);
// StructuredBuffer<LocalVolumetricFogDatas> localVolumetricDatas : register(t0, space0);
ByteAddressBuffer g_CounterBuffer : register(t1, space0);
ByteAddressBuffer g_SortIndexDisBuffer : register(t2, space0);
AppendStructuredBuffer<CommandSignatureParams> g_DrawCommandBuffer : register(u0, space0);


[numthreads(8, 1, 1)]
void CSMain(uint3 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
    uint ListCount = g_CounterBuffer.Load(0);
    uint index = (GroupID.x * 8) + GroupIndex;
    if (index < ListCount)
    {
        uint fogGlobalIndex = g_SortIndexDisBuffer.Load(index * 8);

        // LocalVolumetricFogDatas renderData = localVolumetricDatas[fogGlobalIndex];

        D3D12_DRAW_ARGUMENTS drawArgs;
        drawArgs.VertexCountPerInstance = 3;
        drawArgs.InstanceCount = 1;
        drawArgs.StartVertexLocation = 0;
        drawArgs.StartInstanceLocation = 0;
        
        CommandSignatureParams command;
        command._VolumetricFogGlobalIndex = fogGlobalIndex;
        command.DrawArguments = drawArgs;

        g_DrawCommandBuffer.Append(command);
    }
}
