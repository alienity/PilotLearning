#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"

ConstantBuffer<FrameUniforms> g_FramUniforms : register(b1, space0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 viewDir : TEXCOORD3;
};

VSOutput VSMain(uint VertID : SV_VertexID)
{
    float4x4 ProjInverse = g_FramUniforms.cameraUniform._InvProjMatrix;
    float3x3 ViewInverse = (float3x3)g_FramUniforms.cameraUniform._InvViewMatrix;

    float2 ScreenUV = float2(uint2(VertID, VertID << 1) & 2);
    float4 ProjectedPos = float4(lerp(float2(-1, 1), float2(1, -1), ScreenUV), 0, 1);
    float4 PosViewSpace = mul(ProjInverse, ProjectedPos);

    VSOutput vsOutput;
    vsOutput.position = ProjectedPos;
    vsOutput.viewDir = mul(ViewInverse, PosViewSpace.xyz / PosViewSpace.w);

    return vsOutput;
}
