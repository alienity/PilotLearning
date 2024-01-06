#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ShadingParameters.hlsli"
#include "Quaternion.hlsli"

#define TillingScale 256

cbuffer RootConstants : register(b0, space0)
{
    uint clipmapIndex;

    uint transformBufferIndex;
    uint perFrameBufferIndex;
    uint terrainHeightmapIndex;
    uint terrainNormalmapIndex;
    uint terrainMatIndexBufferIndex;
    uint terrainMatTillingBufferIndex;
};

SamplerState defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

struct MaterialIndexStruct
{
    int albedoIndex;
    int armIndex;
    int displacementIndex;
    int normalIndex;
};

struct MaterialTillingStruct
{
    float2 albedoTilling;
    float2 armTilling;
    float2 displacementTilling;
    float2 normalTilling;
};

struct VertexInput
{
    float3 position : POSITION;
};

struct PSOutputGBuffer
{
    float4 albedo : SV_Target0;
    float4 worldNormal : SV_Target1; // float3
    float4 metallic_Roughness_Reflectance_AO : SV_Target2;
    float4 clearCoat_ClearCoatRoughness_Anisotropy : SV_Target3; // float3
    float4 motionVector : SV_Target4;
};

VaringStruct VSMain(VertexInput input)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];

    Texture2D<float4> terrainHeightmap = ResourceDescriptorHeap[terrainHeightmapIndex];
    Texture2D<float4> terrainNormalmap = ResourceDescriptorHeap[terrainNormalmapIndex];

    StructuredBuffer<ClipmapTransform> clipmapTransformBuffers = ResourceDescriptorHeap[transformBufferIndex];

    StructuredBuffer<MaterialIndexStruct> mMaterialIndexBuffers = ResourceDescriptorHeap[terrainMatIndexBufferIndex];
    StructuredBuffer<MaterialTillingStruct> mMaterialTillingBuffers = ResourceDescriptorHeap[terrainMatTillingBufferIndex];

    Texture2D<float3> displacementTexture = ResourceDescriptorHeap[mMaterialIndexBuffers[0].displacementIndex];
    float2 displacementTilling = mMaterialTillingBuffers[0].displacementTilling * TillingScale;    

    ClipmapTransform clipTransform = clipmapTransformBuffers[clipmapIndex];

    float4x4 tLocalTransform = clipTransform.transform;
    // int tMeshType = clipTransform.mesh_type;

    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    float4x4 localToWorldMatrixInv = mFrameUniforms.terrainUniform.world2LocalMatrix;
    float4x4 projectionViewMatrix  = mFrameUniforms.cameraUniform.curFrameUniform.clipFromWorldMatrix;

    float4x4 projectionMatrix         = mFrameUniforms.cameraUniform.curFrameUniform.clipFromViewMatrix;
    float4x4 lastviewFromWorldMatrix  = mFrameUniforms.cameraUniform.lastFrameUniform.viewFromWorldMatrix;
    float4x4 projectionViewMatrixPrev = mul(projectionMatrix, lastviewFromWorldMatrix);

    float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;
    float terrainSize = mFrameUniforms.terrainUniform.terrainSize;

    float3 localPosition = input.position;
    localPosition = mul(tLocalTransform, float4(localPosition, 1)).xyz;
    float3 worldPosition = mul(localToWorldMatrix, float4(localPosition, 1)).xyz;
    
    float2 terrainUV = (worldPosition.xz) / float2(terrainSize, terrainSize);
    float curHeight = terrainHeightmap.SampleLevel(defaultSampler, terrainUV, 0).b;

    worldPosition.y += curHeight * terrainMaxHeight;

    VaringStruct output;
    output.ws_position = float4(worldPosition, 1.0f);
    output.cs_pos = mul(projectionViewMatrix, output.ws_position);

    float4x4 prevLocalToWorldMatrix = mFrameUniforms.terrainUniform.prevLocal2WorldMatrix;
    float4 prev_ws_position = mul(prevLocalToWorldMatrix, float4(localPosition, 1.0f));

    float2 prevTerrainUV = (prev_ws_position.xz) / float2(terrainSize, terrainSize);
    float prevHeight = terrainHeightmap.SampleLevel(defaultSampler, prevTerrainUV, 0).b;

    prev_ws_position.y += prevHeight * terrainMaxHeight;

    output.cs_xy_curr = output.cs_pos.xyw;
    output.cs_xy_prev = mul(projectionViewMatrixPrev, prev_ws_position).xyw;

    // output.vertex_uv01 = input.texcoord;
    output.vertex_uv01 = terrainUV;

    float3 localNormal = float3(0,1,0);
    float3 localTangent = float3(1,0,0);
    float3x3 normalMat = transpose((float3x3)localToWorldMatrixInv);

    output.ws_normal      = normalize(mul(normalMat, localNormal));    
    output.ws_tangent.xyz = normalize(mul(normalMat, localTangent));
    output.ws_tangent.w   = 1;

    return output;
}

PSOutputGBuffer PSMain(VaringStruct varingStruct)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    float4x4 localToWorldMatrixInv = mFrameUniforms.terrainUniform.world2LocalMatrix;
    float3x3 normalMat = transpose((float3x3)localToWorldMatrixInv);

    float2 terrainUV = varingStruct.vertex_uv01.xy;

    Texture2D<float4> terrainNormalmap = ResourceDescriptorHeap[terrainNormalmapIndex];

    // float3 localTerrainNormal = terrainNormalmap.SampleLevel(defaultSampler, terrainUV, 0).rgb * 2 - 1;
    // float3 worldTerrainNormal = normalize(mul(normalMat, localTerrainNormal));

    float3 worldTerrainNormal = normalize(cross(ddy(varingStruct.ws_position.xyz), ddx(varingStruct.ws_position.xyz)));

    float3x3 tbnWorld = ToTBNMatrix(worldTerrainNormal);

    StructuredBuffer<MaterialIndexStruct> mMaterialIndexBuffers = ResourceDescriptorHeap[terrainMatIndexBufferIndex];
    StructuredBuffer<MaterialTillingStruct> mMaterialTillingBuffers = ResourceDescriptorHeap[terrainMatTillingBufferIndex];
    Texture2D<float3> albedoTexture = ResourceDescriptorHeap[mMaterialIndexBuffers[0].albedoIndex];
    float2 albedoTilling = mMaterialTillingBuffers[0].albedoTilling * TillingScale;
    Texture2D<float3> aoroughnessmetalTexture = ResourceDescriptorHeap[mMaterialIndexBuffers[0].armIndex];
    float2 armTilling = mMaterialTillingBuffers[0].armTilling * TillingScale;
    Texture2D<float3> normalTexture = ResourceDescriptorHeap[mMaterialIndexBuffers[0].normalIndex];
    float2 normalTilling = mMaterialTillingBuffers[0].normalTilling * TillingScale;

    float3 albedo = albedoTexture.Sample(defaultSampler, terrainUV * albedoTilling).rgb;
    float3 aoRoughnessMetallic = albedoTexture.Sample(defaultSampler, terrainUV * armTilling).rgb;
    float3 normal = normalTexture.Sample(defaultSampler, terrainUV * normalTilling).rgb*2-1;
    // normal.y = -normal.y;

    float3 worldNormal = normalize(mul(tbnWorld, normal));

    PSOutputGBuffer output;

    // output.albedo = float4(0,0,0,1);
    // output.albedo = float4(varingStruct.vertex_uv01.x, varingStruct.vertex_uv01.y, 0, 1);
    output.albedo = float4(albedo.rgb, 1);
    output.worldNormal = float4(worldNormal.xyz * 0.5 + 0.5f, 0);
    output.metallic_Roughness_Reflectance_AO.xyzw = float4(aoRoughnessMetallic.z, aoRoughnessMetallic.y, 0, aoRoughnessMetallic.x);
    output.clearCoat_ClearCoatRoughness_Anisotropy = float4(0, 0, 0, 0.0f);

    // compute velocity in ndc
    float2 ndc_curr = varingStruct.cs_xy_curr.xy/varingStruct.cs_xy_curr.z;
    float2 ndc_prev = varingStruct.cs_xy_prev.xy/varingStruct.cs_xy_prev.z;
    // compute screen space velocity [0,1;0,1]
    float2 mv = 0.5f * (ndc_curr - ndc_prev);
    mv.y = -mv.y;
    output.motionVector = float4(mv.xy, 0, 0);

    return output;
}
