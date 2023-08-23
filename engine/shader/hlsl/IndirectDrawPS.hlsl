#define HAS_ATTRIBUTE_TANGENTS
#define MATERIAL_NEEDS_TBN
#define HAS_ATTRIBUTE_UV0
#define VARIANT_HAS_DIRECTIONAL_LIGHTING
#define VARIANT_HAS_DYNAMIC_LIGHTING

#include "d3d12.hlsli"
//#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ShadingParameters.hlsli"

cbuffer RootConstants : register(b0, space0) { uint meshIndex; };
ConstantBuffer<FrameUniforms> g_FramUniforms : register(b1, space0);
StructuredBuffer<PerhRenderableMeshData> g_RenderableMeshDatas : register(t0, space0);
StructuredBuffer<PerMaterialViewIndexBuffer> g_MaterialViewIndexBuffers : register(t1, space0);

SamplerState           defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
};

VaringStruct VSMain(VertexInput input)
{
    VaringStruct output;

    PerhRenderableMeshData renderableMeshData = g_RenderableMeshDatas[meshIndex];

    float4x4 localToWorldMatrix    = renderableMeshData.localToWorldMatrix;
    float4x4 localToWorldMatrixInv = renderableMeshData.localToWorldMatrixInverse;
    float4x4 projectionMatrix      = g_FramUniforms.cameraInstance.projViewMatrix;

    output.vertex_worldPosition = mul(localToWorldMatrix, float4(input.position, 1.0f));
    output.vertex_position = mul(projectionMatrix, output.vertex_worldPosition);

    output.vertex_uv01 = input.texcoord;

    float3x3 normalMat = transpose((float3x3)localToWorldMatrixInv);

    output.vertex_worldNormal      = normalize(mul(normalMat, input.normal));
    output.vertex_worldTangent.xyz = normalize(mul(normalMat, input.tangent.xyz));
    output.vertex_worldTangent.w   = input.tangent.w;

    return output;
}

float4 VSMain(VaringStruct varingStruct) : SV_Target0
{
    PerhRenderableMeshData renderableMeshData = g_RenderableMeshDatas[meshIndex];
    PerMaterialViewIndexBuffer materialViewIndexBuffer = g_MaterialViewIndexBuffers[renderableMeshData.materialIndex];

    MaterialInputs materialInputs;
    inflateMaterial(varingStruct, materialViewIndexBuffer, defaultSampler, materialInputs);

    CommonShadingStruct commonShadingStruct;
    computeShadingParams(g_FramUniforms, varingStruct, commonShadingStruct);
    prepareMaterial(materialInputs, commonShadingStruct);

    float4 fragColor = evaluateMaterial(g_FramUniforms, commonShadingStruct, materialInputs);

    return fragColor;
}
