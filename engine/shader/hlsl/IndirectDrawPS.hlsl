#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "SharedTypes.hlsli"
#include "Varings.hlsli"
#include "ShadingLit.hlsli"

cbuffer RootConstants : register(b0, space0) { uint meshIndex; };

ConstantBuffer<MeshPerframeBuffer> g_ConstantBufferParams : register(b1, space0);

StructuredBuffer<MeshInstance> g_MeshesInstance : register(t0, space0);

StructuredBuffer<MaterialInstance> g_MaterialsInstance : register(t1, space0);

SamplerState           defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
};

/*
struct VaringStruct
{
    float4 vertex_worldPosition;

#if defined(HAS_ATTRIBUTE_TANGENTS)
    float3 vertex_worldNormal;
#if defined(MATERIAL_NEEDS_TBN)
    float4 vertex_worldTangent;
#endif
#endif

    float4 vertex_position;

    int instance_index;

#if defined(HAS_ATTRIBUTE_COLOR)
    float4 vertex_color;
#endif

#if defined(HAS_ATTRIBUTE_UV0) && !defined(HAS_ATTRIBUTE_UV1)
    float2 vertex_uv01;
#elif defined(HAS_ATTRIBUTE_UV1)
    float4 vertex_uv01;
#endif

#if defined(VARIANT_HAS_SHADOWING) && defined(VARIANT_HAS_DIRECTIONAL_LIGHTING)
    float4 vertex_lightSpacePosition;
#endif
};
*/

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    MeshInstance mesh = g_MeshesInstance[meshIndex];

    output.positionWS = mul(mesh.localToWorldMatrix, float4(input.position, 1.0f)).xyz;
    output.position   = mul(g_ConstantBufferParams.cameraInstance.projViewMatrix, float4(output.positionWS, 1.0f));

    output.texcoord = input.texcoord;

    float3x3 normalMat = transpose((float3x3)mesh.localToWorldMatrixInverse);

    output.normal      = normalize(mul(normalMat, input.normal));
    output.tangent.xyz = normalize(mul(normalMat, input.tangent.xyz));
    output.tangent.w   = input.tangent.w;

    return output;
}

float4 VSMain(VaringStruct varingStruct) : SV_Target0
{
    CommonShadingStruct commonShadingStruct;
    
    FramUniforms   frameUniforms; 
    MaterialParams materialParams;
    MaterialInputs materialInput;

    computeShadingParams(materialParams, varingStruct, commonShadingStruct);

    // Моід materialInput
    materialInput.baseColor = float4(1, 1, 1, 1);

    prepareMaterial(materialInput, commonShadingStruct);

    float4 fragColor = evaluateMaterial(frameUniforms, materialParams, commonShadingStruct, materialInput);

    return fragColor;
}
