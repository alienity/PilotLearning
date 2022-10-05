#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "Math.hlsli"
#include "SharedTypes.hlsli"

cbuffer RootConstants : register(b0, space0) { uint meshIndex; };

ConstantBuffer<MeshPerframeStorageBufferObject> g_ConstantBufferParams : register(b1, space0);

StructuredBuffer<MeshInstance> g_MeshesInstance : register(t0, space0);

StructuredBuffer<MaterialInstance> g_MaterialsInstance : register(t1, space0);

SamplerState defaultSampler : register(s10);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 positionWS : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    MeshInstance mesh = g_MeshesInstance[meshIndex];

    output.positionWS = mul(mesh.localToWorldMatrix, float4(input.position, 1.0f)).xyz;
    output.position   = mul(g_ConstantBufferParams.scene_directional_light.directional_light_proj_view, float4(output.positionWS, 1.0f));

    output.texcoord    = input.texcoord;

    float3x3 normalMat = transpose((float3x3)mesh.localToWorldMatrixInverse);

    output.normal      = normalize(mul(normalMat, input.normal));
    output.tangent.xyz = normalize(mul(normalMat, input.tangent.xyz));
    output.tangent.w   = input.tangent.w;

    return output;
}

float4 PSMain(VertexOutput input) : SV_Target0
{
    MeshInstance     mesh     = g_MeshesInstance[meshIndex];
    MaterialInstance material = g_MaterialsInstance[mesh.materialIndex];

    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[material.baseColorIndex];

    float4 baseColor = baseColorTex.Sample(defaultSampler, input.texcoord);

    clip(baseColor.a < 0.05f ? -1 : 1);

    return float4(baseColor.rgb, 1);
}
