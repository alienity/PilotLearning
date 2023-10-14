#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ShadingParameters.hlsli"
#include "Quaternion.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint meshIndex;
    uint perRenderableMeshDataIndex;
    uint materialViewViewIndex;
};
ConstantBuffer<FrameUniforms> g_FrameUniform : register(b1, space0);
// StructuredBuffer<PerRenderableMeshData> g_RenderableMeshDatas : register(t0, space0);
// StructuredBuffer<PerMaterialViewIndexBuffer> g_MaterialViewIndexBuffers : register(t1, space0);

SamplerState defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
};

struct PSOutputGBuffer
{
    float4 albedo : SV_Target0;
    float4 worldNormal : SV_Target1; // float3
    float4 worldTangent : SV_Target2; // float4
    float4 materialNormal : SV_Target3; // float3
    float4 emissive : SV_Target4;
    float4 metallic_Roughness_Reflectance_AO : SV_Target5;
    float4 clearCoat_ClearCoatRoughness_Anisotropy : SV_Target6; // float3
};

VaringStruct VSMain(VertexInput input)
{
    StructuredBuffer<PerRenderableMeshData> g_RenderableMeshDatas = ResourceDescriptorHeap[perRenderableMeshDataIndex];
    
    VaringStruct output;

    PerRenderableMeshData renderableMeshData = g_RenderableMeshDatas[meshIndex];

    float4x4 localToWorldMatrix    = renderableMeshData.worldFromModelMatrix;
    float4x4 localToWorldMatrixInv = renderableMeshData.modelFromWorldMatrix;
    float4x4 projectionViewMatrix  = g_FrameUniform.cameraUniform.clipFromWorldMatrix;

    output.vertex_worldPosition = mul(localToWorldMatrix, float4(input.position, 1.0f));
    output.vertex_position = mul(projectionViewMatrix, output.vertex_worldPosition);

    output.vertex_uv01 = input.texcoord;

    float3x3 normalMat = transpose((float3x3)localToWorldMatrixInv);

    output.vertex_worldNormal      = normalize(mul(normalMat, input.normal));
    output.vertex_worldTangent.xyz = normalize(mul(normalMat, input.tangent.xyz));
    output.vertex_worldTangent.w   = input.tangent.w;

    return output;
}

PSOutputGBuffer PSMain(VaringStruct varingStruct)
{
    StructuredBuffer<PerRenderableMeshData> g_RenderableMeshDatas = ResourceDescriptorHeap[perRenderableMeshDataIndex];
    StructuredBuffer<PerMaterialViewIndexBuffer> g_MaterialViewIndexBuffers = ResourceDescriptorHeap[materialViewViewIndex];

    PerRenderableMeshData renderableMeshData = g_RenderableMeshDatas[meshIndex];
    PerMaterialViewIndexBuffer materialViewIndexBuffer = g_MaterialViewIndexBuffers[renderableMeshData.perMaterialViewIndexBufferIndex];

    MaterialInputs materialInputs;
    initMaterial(materialInputs);
    inflateMaterial(varingStruct, materialViewIndexBuffer, defaultSampler, materialInputs);

    CommonShadingStruct commonShadingStruct;
    computeShadingParams(g_FrameUniform, varingStruct, commonShadingStruct);
    prepareMaterial(materialInputs, commonShadingStruct);

    PSOutputGBuffer output;

    output.albedo = materialInputs.baseColor;
    output.worldNormal = float4(varingStruct.vertex_worldNormal.xyz * 0.5 + 0.5f, 0);
    output.worldTangent = float4(varingStruct.vertex_worldTangent.xyzw * 0.5 + 0.5f);
    output.materialNormal = float4(materialInputs.normal.xyz * 0.5f + 0.5f, 0);
    output.emissive = materialInputs.emissive;
    output.metallic_Roughness_Reflectance_AO.xyzw = float4(materialInputs.metallic, materialInputs.roughness, materialInputs.reflectance, materialInputs.ambientOcclusion);
    output.clearCoat_ClearCoatRoughness_Anisotropy = float4(materialInputs.clearCoat, materialInputs.clearCoatRoughness, materialInputs.anisotropy, 0.0f);

    return output;
}