#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Material/Lit/LitProperties.hlsl"

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
StructuredBuffer<RenderDataPerDraw> g_RenderDatas : register(t0, space0);
StructuredBuffer<PropertiesPerMaterial> g_PropertiesData : register(t1, space0);

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

    RenderDataPerDraw meshData = g_RenderDatas[meshIndex];

	#if defined(DIRECTIONSHADOW)
    HDDirectionalShadowData dirShadowData = g_FramUniforms.lightDataUniform.directionalShadowData;
    uint shadowCascadeIndex = dirShadowData.shadowDataIndex[cascadeLevel];
    HDShadowData shadowData = g_FramUniforms.lightDataUniform.shadowDatas[shadowCascadeIndex];
    float4x4 view_proj_mat = shadowData.viewProjMatrix;
    #elif defined(SPOTSHADOW)
    LightData lightData = g_FramUniforms.lightDataUniform.lightData[spotIndex];
    HDShadowData shadowData = g_FramUniforms.lightDataUniform.shadowDatas[lightData.shadowDataIndex];
    float4x4 view_proj_mat = shadowData.viewProjMatrix;
    #else
    float4x4 view_proj_mat = g_FramUniforms.cameraUniform._CurFrameUniform.clipFromWorldMatrix;
    #endif

    output.positionWS = mul(meshData.objectToWorldMatrix, float4(input.position, 1.0f)).xyz;
    output.position   = mul(view_proj_mat, float4(output.positionWS, 1.0f));

    output.texcoord    = input.texcoord;

    float3x3 normalMat = transpose((float3x3)meshData.worldToObjectMatrix);

    output.normal      = normalize(mul(normalMat, input.normal));
    output.tangent.xyz = normalize(mul(normalMat, input.tangent.xyz));
    output.tangent.w   = input.tangent.w;

    return output;
}

float4 PSMain(VertexOutput input) : SV_Target0
{
    RenderDataPerDraw meshData = g_RenderDatas[meshIndex];
    PropertiesPerMaterial materialProperties = g_PropertiesData[GetLightPropertyBufferIndexOffset(meshData)];

    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[materialProperties._BaseColorMapIndex];

    float4 baseColor = baseColorTex.Sample(defaultSampler, input.texcoord);

    clip(baseColor.a < 0.05f ? -1 : 1);

    return float4(baseColor.rgb, 1);
}
