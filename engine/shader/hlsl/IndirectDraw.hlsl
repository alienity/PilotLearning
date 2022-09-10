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

// https://devblogs.microsoft.com/directx/hlsl-shader-model-6-6/
// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
// https://devblogs.microsoft.com/directx/in-the-works-hlsl-shader-model-6-6/
float4 PSMain(VertexOutput input) : SV_Target0
{
    MeshInstance     mesh     = g_MeshesInstance[meshIndex];
    MaterialInstance material = g_MaterialsInstance[mesh.materialIndex];

    // https://github.com/microsoft/DirectXShaderCompiler/issues/2193
    ByteAddressBuffer matFactorsBuffer = ResourceDescriptorHeap[material.uniformBufferIndex];
    float4 baseColorFactor = matFactorsBuffer.Load<float4>(0);

    //ConstantBuffer<PerMaterialUniformBufferObject> matFactors = ResourceDescriptorHeap[material.uniformBufferIndex];

    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[material.baseColorIndex];

    float4 baseColor = baseColorTex.Sample(defaultSampler, input.texcoord);

    baseColor = baseColor * baseColorFactor;

    return float4(baseColor.rgb, 1);
}
