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
    float4 metallic_Roughness_Reflectance_AO : SV_Target4;
    float4 clearCoat_ClearCoatRoughness_Anisotropy : SV_Target5; // float3
    float4 motionVector : SV_Target6;
};

VaringStruct VSMain(VertexInput input)
{
    StructuredBuffer<PerRenderableMeshData> g_RenderableMeshDatas = ResourceDescriptorHeap[perRenderableMeshDataIndex];
    
    VaringStruct output;

    PerRenderableMeshData renderableMeshData = g_RenderableMeshDatas[meshIndex];

    float4x4 localToWorldMatrix    = renderableMeshData.worldFromModelMatrix;
    float4x4 localToWorldMatrixInv = renderableMeshData.modelFromWorldMatrix;
    float4x4 projectionViewMatrix  = g_FrameUniform.cameraUniform.curFrameUniform.clipFromWorldMatrix;

    float4x4 projectionMatrix = g_FrameUniform.cameraUniform.curFrameUniform.clipFromViewMatrix;
    float4x4 lastviewFromWorldMatrix = g_FrameUniform.cameraUniform.lastFrameUniform.viewFromWorldMatrix;
    float4x4 projectionViewMatrixPrev = mul(projectionMatrix, lastviewFromWorldMatrix);

    output.ws_position = mul(localToWorldMatrix, float4(input.position, 1.0f));
    output.cs_pos = mul(projectionViewMatrix, output.ws_position);

    output.cs_xy_curr = output.cs_pos.xyw;
    output.cs_xy_prev = mul(projectionViewMatrixPrev, output.ws_position).xyw;

    output.vertex_uv01 = input.texcoord;

    float3x3 normalMat = transpose((float3x3)localToWorldMatrixInv);

    output.ws_normal      = normalize(mul(normalMat, input.normal));
    output.ws_tangent.xyz = normalize(mul(normalMat, input.tangent.xyz));
    output.ws_tangent.w   = input.tangent.w;

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
    output.worldNormal = float4(varingStruct.ws_normal.xyz * 0.5 + 0.5f, 0);
    output.worldTangent = float4(varingStruct.ws_tangent.xyzw * 0.5 + 0.5f);
    output.materialNormal = float4(materialInputs.normal.xyz * 0.5f + 0.5f, 0);
    output.metallic_Roughness_Reflectance_AO.xyzw = float4(materialInputs.metallic, materialInputs.roughness, materialInputs.reflectance, materialInputs.ambientOcclusion);
    output.clearCoat_ClearCoatRoughness_Anisotropy = float4(materialInputs.clearCoat, materialInputs.clearCoatRoughness, materialInputs.anisotropy, 0.0f);
    
    // compute velocity in ndc
    float2 ndc_curr = varingStruct.cs_xy_curr.xy/varingStruct.cs_xy_curr.z;
    float2 ndc_prev = varingStruct.cs_xy_prev.xy/varingStruct.cs_xy_prev.z;
    // compute screen space velocity [0,1;0,1]
    float2 mv = 0.5f * (ndc_curr - ndc_prev);
    mv.y = -mv.y;
    output.motionVector = float4(mv.xy, 0, 0);

    return output;
}
