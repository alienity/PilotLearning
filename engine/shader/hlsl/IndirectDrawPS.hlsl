#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ShadingParameters.hlsli"

cbuffer RootConstants : register(b0, space0) { uint meshIndex; };
ConstantBuffer<FrameUniforms> g_FrameUniform : register(b1, space0);
StructuredBuffer<PerRenderableMeshData> g_RenderableMeshDatas : register(t0, space0);
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

    PerRenderableMeshData renderableMeshData = g_RenderableMeshDatas[meshIndex];

    float4x4 localToWorldMatrix    = renderableMeshData.worldFromModelMatrix;
    float4x4 localToWorldMatrixInv = renderableMeshData.modelFromWorldMatrix;
    float4x4 projectionViewMatrix  = g_FrameUniform.cameraUniform.curFrameUniform.clipFromWorldMatrix;

    output.ws_position = mul(localToWorldMatrix, float4(input.position, 1.0f));
    output.cs_pos = mul(projectionViewMatrix, output.ws_position);

    output.vertex_uv01 = input.texcoord;

    float3x3 normalMat = transpose((float3x3)localToWorldMatrixInv);

    output.ws_normal      = normalize(mul(normalMat, input.normal));
    output.ws_tangent.xyz = normalize(mul(normalMat, input.tangent.xyz));
    output.ws_tangent.w   = input.tangent.w;

    return output;
}

float4 PSMain(VaringStruct varingStruct) : SV_Target0
{
    PerRenderableMeshData renderableMeshData = g_RenderableMeshDatas[meshIndex];
    PerMaterialViewIndexBuffer materialViewIndexBuffer = g_MaterialViewIndexBuffers[renderableMeshData.perMaterialViewIndexBufferIndex];

    MaterialInputs materialInputs;
    initMaterial(materialInputs);
    inflateMaterial(varingStruct, materialViewIndexBuffer, defaultSampler, materialInputs);

    CommonShadingStruct commonShadingStruct;
    computeShadingParams(g_FrameUniform, varingStruct, commonShadingStruct);
    prepareMaterial(materialInputs, commonShadingStruct);

    SamplerStruct samplerStruct;
    samplerStruct.defSampler = defaultSampler;
    samplerStruct.sdSampler = shadowmapSampler;

    float4 fragColor = evaluateMaterial(g_FrameUniform, commonShadingStruct, materialInputs, samplerStruct);

    return fragColor;
}
