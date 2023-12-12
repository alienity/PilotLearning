#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ShadingParameters.hlsli"
#include "Quaternion.hlsli"

#define EAST 1
#define SOUTH 2
#define WEST 4
#define NORTH 8

#define TillingScale 256

cbuffer RootConstants : register(b0, space0)
{
    uint terrainPatchNodeIndex;
    uint meshPerFrameBufferIndex;
    uint terrainHeightmapIndex;
    uint terrainNormalmapIndex;
    uint terrainDrawIdxBufferIndex;
    uint terrainMateiralBufferIndex;
    uint terrainMatIndexBufferIndex;
    uint terrainMatTillingBufferIndex;
};
// ConstantBuffer<FrameUniforms> g_FrameUniform : register(b1, space0);
// StructuredBuffer<PerRenderableMeshData> g_RenderableMeshDatas : register(t0, space0);
// StructuredBuffer<PerMaterialViewIndexBuffer> g_MaterialViewIndexBuffers : register(t1, space0);

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
    float4 metallic_Roughness_Reflectance_AO : SV_Target4;
    float4 clearCoat_ClearCoatRoughness_Anisotropy : SV_Target5; // float3
    float4 motionVector : SV_Target6;
};

VaringStruct VSMain(VertexInput input)
{
    StructuredBuffer<TerrainPatchNode> mTerrainPatchNodes = ResourceDescriptorHeap[terrainPatchNodeIndex];
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[meshPerFrameBufferIndex];

    // StructuredBuffer<MaterialIndexStruct> mMaterialIndexBuffer = ResourceDescriptorHeap[terrainMatIndexBufferIndex];
    
    Texture2D<float4> terrainHeightmap = ResourceDescriptorHeap[terrainHeightmapIndex];
    Texture2D<float4> terrainNormalmap = ResourceDescriptorHeap[terrainNormalmapIndex];

    StructuredBuffer<uint> mDrawIdxBuffer = ResourceDescriptorHeap[terrainDrawIdxBufferIndex];

    StructuredBuffer<MaterialIndexStruct> mMaterialIndexBuffers = ResourceDescriptorHeap[terrainMatIndexBufferIndex];
    StructuredBuffer<MaterialTillingStruct> mMaterialTillingBuffers = ResourceDescriptorHeap[terrainMatTillingBufferIndex];
    
    Texture2D<float3> displacementTexture = ResourceDescriptorHeap[mMaterialIndexBuffers[0].displacementIndex];
    float2 displacementTilling = mMaterialTillingBuffers[0].displacementTilling * TillingScale;

    uint patchNodeIndex = mDrawIdxBuffer[input.instanceID];

    TerrainPatchNode _terrainPatchNode = mTerrainPatchNodes[patchNodeIndex];

    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    float4x4 localToWorldMatrixInv = mFrameUniforms.terrainUniform.world2LocalMatrix;
    float4x4 projectionViewMatrix  = mFrameUniforms.cameraUniform.curFrameUniform.clipFromWorldMatrix;

    float4x4 projectionMatrix         = mFrameUniforms.cameraUniform.curFrameUniform.clipFromViewMatrix;
    float4x4 lastviewFromWorldMatrix  = mFrameUniforms.cameraUniform.lastFrameUniform.viewFromWorldMatrix;
    float4x4 projectionViewMatrixPrev = mul(projectionMatrix, lastviewFromWorldMatrix);

    float terrainMaxHeight = mFrameUniforms.terrainUniform.terrainMaxHeight;
    float terrainSize = mFrameUniforms.terrainUniform.terrainSize;

    float3 localPosition = input.position;

    float edgeOffsetScale = pow(2, max(0, _terrainPatchNode.mipLevel-1));

    if(_terrainPatchNode.neighbor & EAST)
    {
        localPosition += float3(0, 0, -input.color.a) * edgeOffsetScale;
    }
    if(_terrainPatchNode.neighbor & SOUTH)
    {
        localPosition += float3(input.color.r, 0, 0) * edgeOffsetScale;
    }
    if(_terrainPatchNode.neighbor & WEST)
    {
        localPosition += float3(0, 0, input.color.b) * edgeOffsetScale;
    }
    if(_terrainPatchNode.neighbor & NORTH)
    {
        localPosition += float3(-input.color.g, 0, 0) * edgeOffsetScale;
    }

    localPosition = localPosition * _terrainPatchNode.nodeWidth + 
        float3(_terrainPatchNode.patchMinPos.x, 0, _terrainPatchNode.patchMinPos.y);

    float2 terrainUV = (localPosition.xz) / float2(terrainSize, terrainSize);
    float curHeight = terrainHeightmap.SampleLevel(defaultSampler, terrainUV, 0).b;

    // float displacement = displacementTexture.SampleLevel(defaultSampler, terrainUV * displacementTilling, 0).r;

    localPosition.y = curHeight * terrainMaxHeight;
    // localPosition.y += displacement * 0.5f;

    VaringStruct output;
    output.ws_position = mul(localToWorldMatrix, float4(localPosition, 1.0f));
    output.cs_pos = mul(projectionViewMatrix, output.ws_position);

    float4x4 prevLocalToWorldMatrix = mFrameUniforms.terrainUniform.prevLocal2WorldMatrix;
    float4 prev_ws_position = mul(prevLocalToWorldMatrix, float4(localPosition, 1.0f));
    output.cs_xy_curr = output.cs_pos.xyw;
    output.cs_xy_prev = mul(projectionViewMatrixPrev, prev_ws_position).xyw;

    // output.vertex_uv01 = input.texcoord;
    output.vertex_uv01 = terrainUV;

    float3 localNormal = input.normal.xyz;
    localNormal = normalize(localNormal * 2 - 1);
    float3 localTangent = input.tangent.xyz;
    float3 localBitangent = cross(localNormal, localTangent);
    localTangent = cross(localBitangent, localNormal);
    float3x3 normalMat = transpose((float3x3)localToWorldMatrixInv);

    output.ws_normal      = normalize(mul(normalMat, localNormal));    
    output.ws_tangent.xyz = normalize(mul(normalMat, localTangent));
    output.ws_tangent.w   = input.tangent.w;

    return output;
}

PSOutputGBuffer PSMain(VaringStruct varingStruct)
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[meshPerFrameBufferIndex];
    float4x4 localToWorldMatrix = mFrameUniforms.terrainUniform.local2WorldMatrix;
    float4x4 localToWorldMatrixInv = mFrameUniforms.terrainUniform.world2LocalMatrix;
    float3x3 normalMat = transpose((float3x3)localToWorldMatrixInv);

    float2 terrainUV = varingStruct.vertex_uv01.xy;

    Texture2D<float4> terrainNormalmap = ResourceDescriptorHeap[terrainNormalmapIndex];

    float3 localNormal = terrainNormalmap.SampleLevel(defaultSampler, terrainUV, 0).rgb;
    localNormal = normalize(mul(normalMat, (localNormal * 2 - 1)));
    localNormal.x = -localNormal.x;
    float3 localTangent = normalize(mul(normalMat, float3(1, 0, 0)));
    float3 localBitangent = cross(localNormal, localTangent);
    localTangent = cross(localBitangent, localNormal);

    /*
    int albedoIndex;
    int armIndex;
    int displacementIndex;
    int normalIndex;
    */

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

    PSOutputGBuffer output;

    // output.albedo = float4(0,0,0,1);
    // output.albedo = float4(varingStruct.vertex_uv01.x, varingStruct.vertex_uv01.y, 0, 1);
    output.albedo = float4(albedo.rgb, 1);
    output.worldNormal = float4(localNormal.xyz * 0.5 + 0.5f, 0);
    output.worldTangent = float4(localTangent.xyz, 1) * 0.5 + 0.5f;
    output.materialNormal = float4(normal.xyz * 0.5 + 0.5f, 0);
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
