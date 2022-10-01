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
    output.position   = mul(g_ConstantBufferParams.cameraInstance.projViewMatrix, float4(output.positionWS, 1.0f));

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

    StructuredBuffer<PerMaterialUniformBufferObject> matBuffer = ResourceDescriptorHeap[material.uniformBufferIndex];

    float4 baseColorFactor = matBuffer[0].baseColorFactor;
    float  normalStength    = matBuffer[0].normalScale;

    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[material.baseColorIndex];
    Texture2D<float3> normalTex    = ResourceDescriptorHeap[material.normalIndex];

    float4 baseColor = baseColorTex.Sample(defaultSampler, input.texcoord);
    float3 vNt       = normalTex.Sample(defaultSampler, input.texcoord) * 2.0f - 1.0f;

    baseColor = baseColor * baseColorFactor;

    // http://www.mikktspace.com/
    float3 vNormal    = input.normal.xyz;
    float3 vTangent   = input.tangent.xyz;
    float3 vBiTangent = input.tangent.w * cross(vNormal, vTangent);
    float3 vNout      = normalize(vNt.x * vTangent + vNt.y * vBiTangent + vNt.z * vNormal);

    vNout = vNout * normalStength;

    float3 positionWS = input.positionWS;
    float3 viewPos    = g_ConstantBufferParams.cameraInstance.cameraPosition;

    float3 outColor = g_ConstantBufferParams.ambient_light * 0.0f;

    uint point_light_num = g_ConstantBufferParams.point_light_num;
    for (uint i = 0; i < point_light_num; i++)
    {
        float3 lightPos   = g_ConstantBufferParams.scene_point_lights[i].position;
        float3 lightColor = g_ConstantBufferParams.scene_point_lights[i].color;
        float  lightStrength = g_ConstantBufferParams.scene_point_lights[i].intensity;

        float3 lightDir = normalize(lightPos - positionWS);
        float3 viewDir  = normalize(viewPos - positionWS);
        float3 halfwayDir = normalize(lightDir + viewDir);

        float specColor = pow(max(dot(vNout, halfwayDir), 0.0f), 4);
        float3 specularColor = lightColor * lightStrength * specColor;

        outColor += specularColor;
    }
    outColor = outColor * baseColor.rgb;

    return float4(outColor.rgb, 1);
}


/*
// https://devblogs.microsoft.com/directx/hlsl-shader-model-6-6/
// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
// https://devblogs.microsoft.com/directx/in-the-works-hlsl-shader-model-6-6/
float4 PSMain(VertexOutput input) : SV_Target0
{
    MeshInstance     mesh     = g_MeshesInstance[meshIndex];
    MaterialInstance material = g_MaterialsInstance[mesh.materialIndex];

    //// https://github.com/microsoft/DirectXShaderCompiler/issues/2193
    //ByteAddressBuffer matFactorsBuffer = ResourceDescriptorHeap[material.uniformBufferIndex];
    //float4 baseColorFactor = matFactorsBuffer.Load<float4>(0);

    // https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-cs-resources
    StructuredBuffer<PerMaterialUniformBufferObject> matFactors = ResourceDescriptorHeap[material.uniformBufferIndex];
    float4 baseColorFactor = matFactors[0].baseColorFactor;

    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[material.baseColorIndex];
    Texture2D<float3> normalTex = ResourceDescriptorHeap[material.normalIndex];

    float4 baseColor = baseColorTex.Sample(defaultSampler, input.texcoord);

    float3 normalVal = normalTex.Sample(defaultSampler, input.texcoord) * 2.0f - 1.0f;

    baseColor = baseColor * baseColorFactor;

    return float4(normalVal.rgb, 1);
}
*/