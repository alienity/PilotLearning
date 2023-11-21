#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ShadingParameters.hlsli"
#include "Quaternion.hlsli"

#define EAST 1
#define SOUTH 2
#define WEST 4
#define NORTH 8

cbuffer RootConstants : register(b0, space0)
{
    uint terrainPatchNodeIndex;
    uint meshPerFrameBufferIndex;
    uint terrainHeightmapIndex;
    uint terrainNormalmapIndex;
};
// ConstantBuffer<FrameUniforms> g_FrameUniform : register(b1, space0);
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
    float4 color    : COLOR;
    
    uint instanceID : SV_InstanceID;
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
    StructuredBuffer<TerrainPatchNode> mTerrainPatchNodes = ResourceDescriptorHeap[terrainPatchNodeIndex];
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[meshPerFrameBufferIndex];

    Texture2D<float4> terrainHeightmap = ResourceDescriptorHeap[terrainHeightmapIndex];
    Texture2D<float4> terrainNormalmap = ResourceDescriptorHeap[terrainNormalmapIndex];

    TerrainPatchNode _terrainPatchNode = mTerrainPatchNodes[input.instanceID];

    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    float4x4 localToWorldMatrixInv = mFrameUniforms.terrainUniform.world2LocalMatrix;
    float4x4 projectionViewMatrix  = mFrameUniforms.cameraUniform.clipFromWorldMatrix;
    float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;
    float terrainSize = mFrameUniforms.terrainUniform.terrainSize;

    float3 localPosition = input.position;

    if(_terrainPatchNode.neighbor & EAST)
    {
        localPosition += -float3(input.color.a, 0, 0);
    }
    if(_terrainPatchNode.neighbor & SOUTH)
    {
        localPosition += float3(0, 0, input.color.g);
    }
    if(_terrainPatchNode.neighbor & WEST)
    {
        localPosition += float3(input.color.b, 0, 0);
    }
    if(_terrainPatchNode.neighbor & NORTH)
    {
        localPosition += -float3(0, 0, input.color.g);
    }

    localPosition = localPosition * _terrainPatchNode.nodeWidth + 
        float3(_terrainPatchNode.patchMinPos.x, 0, _terrainPatchNode.patchMinPos.y);

    float2 terrainUV = float2(localPosition.x, localPosition.z) / float2(terrainSize + 1, terrainSize + 1);
    float curHeight = terrainHeightmap.SampleLevel(defaultSampler, terrainUV, 0).b;

    localPosition.y = curHeight * terrainMaxHeight;

    // localPosition.xyz *= 0.01f;
    localPosition.y += 1.5f;

    VaringStruct output;
    output.vertex_worldPosition = mul(localToWorldMatrix, float4(localPosition, 1.0f));
    output.vertex_position = mul(projectionViewMatrix, output.vertex_worldPosition);

    // output.vertex_uv01 = input.texcoord;
    output.vertex_uv01 = terrainUV;

    float3 localNormal = terrainNormalmap.SampleLevel(defaultSampler, terrainUV, 0).rgb;
    localNormal = normalize(localNormal * 2 - 1);

    float3 localTangent = float3(1, 0, 0);
    float3 localBitangent = cross(localNormal, localTangent);
    localTangent = cross(localBitangent, localNormal);

    float3x3 normalMat = transpose((float3x3)localToWorldMatrixInv);

    // output.vertex_worldNormal      = normalize(mul(normalMat, input.normal));    
    output.vertex_worldNormal      = normalize(mul(normalMat, localNormal));
    output.vertex_worldTangent.xyz = normalize(mul(normalMat, localTangent));
    output.vertex_worldTangent.w   = input.tangent.w;

    return output;
}

PSOutputGBuffer PSMain(VaringStruct varingStruct)
{
    float2 terrainUV = varingStruct.vertex_uv01.xy;

    Texture2D<float4> terrainNormalmap = ResourceDescriptorHeap[terrainNormalmapIndex];

    float3 localNormal = terrainNormalmap.SampleLevel(defaultSampler, terrainUV, 0).rgb;
    localNormal = normalize(localNormal * 2 - 1);
    float3 localTangent = float3(1, 0, 0);
    float3 localBitangent = cross(localNormal, localTangent);
    localTangent = cross(localBitangent, localNormal);


    PSOutputGBuffer output;

    // output.albedo = float4(1,1,1,1);
    // output.albedo = float4(varingStruct.vertex_uv01.x, varingStruct.vertex_uv01.y, 0, 1);
    output.albedo = float4(localNormal.xyz ,1);
    output.worldNormal = float4(localNormal.xyz, 0);
    output.worldTangent = float4(localTangent.xyz, 1);
    output.materialNormal = float4(0, 1, 0, 0);
    output.emissive = float4(0, 0, 0, 0);
    output.metallic_Roughness_Reflectance_AO.xyzw = float4(0, 1, 0, 0);
    output.clearCoat_ClearCoatRoughness_Anisotropy = float4(0, 0, 0, 0.0f);

    return output;
}
