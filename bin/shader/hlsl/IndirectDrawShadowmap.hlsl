#include "d3d12.hlsli"
// #include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

cbuffer RootConstants : register(b0, space0) 
{ 
    uint meshIndex;
#if defined(DIRECTIONSHADOW)
    uint cascadeLevel;
#elif defined(SPOTSHADOW)
    uint spotIndex;
#endif
};

ConstantBuffer<FrameUniforms> g_FramUniforms : register(b1, space0);
StructuredBuffer<PerRenderableMeshData> g_RenderableMeshDatas : register(t0, space0);
StructuredBuffer<PerMaterialViewIndexBuffer> g_MaterialViewIndexBuffers : register(t1, space0);

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

    PerRenderableMeshData mesh = g_RenderableMeshDatas[meshIndex];

	#if defined(DIRECTIONSHADOW)
    float4x4 view_proj_mat = g_FramUniforms.directionalLight.directionalLightShadowmap.light_proj_view[cascadeLevel];
    #elif defined(SPOTSHADOW)
    float4x4 view_proj_mat = g_FramUniforms.spotLightUniform.spotLightStructs[spotIndex].spotLightShadowmap.light_proj_view;
    #else
    float4x4 view_proj_mat = g_FramUniforms.cameraUniform.clipFromWorldMatrix;
    #endif

    output.positionWS = mul(mesh.worldFromModelMatrix, float4(input.position, 1.0f)).xyz;
    output.position   = mul(view_proj_mat, float4(output.positionWS, 1.0f));

    output.texcoord    = input.texcoord;

    float3x3 normalMat = transpose((float3x3)mesh.modelFromWorldMatrix);

    output.normal      = normalize(mul(normalMat, input.normal));
    output.tangent.xyz = normalize(mul(normalMat, input.tangent.xyz));
    output.tangent.w   = input.tangent.w;

    return output;
}

float4 PSMain(VertexOutput input) : SV_Target0
{
    PerRenderableMeshData mesh = g_RenderableMeshDatas[meshIndex];
    PerMaterialViewIndexBuffer material = g_MaterialViewIndexBuffers[mesh.perMaterialViewIndexBufferIndex];

    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[material.baseColorIndex];

    float4 baseColor = baseColorTex.Sample(defaultSampler, input.texcoord);

    clip(baseColor.a < 0.05f ? -1 : 1);

    return float4(baseColor.rgb, 1);
}
