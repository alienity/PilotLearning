#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "Math.hlsli"
#include "SharedTypes.hlsli"

cbuffer RootConstants : register(b0, space0) { uint meshIndex; };

ConstantBuffer<MeshPerframeStorageBufferObject> g_ConstantBufferParams : register(b1, space0);

StructuredBuffer<MeshInstance> g_MeshesInstance : register(t0, space0);

//StructuredBuffer<Material> g_Materials : register(t0, space0);

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
    float2 texcoord : TEXCOORD;
    float3 normal   : NORMAL;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    MeshInstance mesh = g_MeshesInstance[meshIndex];

    output.position = mul(mesh.localToWorldMatrix, float4(input.position, 1.0f));
    output.position = mul(g_ConstantBufferParams.cameraInstance.projViewMatrix, output.position);

    output.texcoord = input.texcoord;
    output.normal   = normalize(mul((float3x3)mesh.localToWorldMatrix, input.normal));
    
    return output;
}

float4 PSMain(VertexOutput input) : SV_Target0
{
    return float4(1,0,0,1);
    //return float4(input.texcoord.x, input.texcoord.y, 0, 1);
}
