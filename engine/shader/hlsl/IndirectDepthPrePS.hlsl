#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

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

struct VaringStruct
{
    float4 vertex_position : SV_POSITION;
};

VaringStruct VSMain(VertexInput input)
{
    VaringStruct output;

    StructuredBuffer<PerRenderableMeshData> g_RenderableMeshDatas = ResourceDescriptorHeap[perRenderableMeshDataIndex];
    PerRenderableMeshData renderableMeshData = g_RenderableMeshDatas[meshIndex];

    float4x4 localToWorldMatrix    = renderableMeshData.worldFromModelMatrix;
    float4x4 projectionViewMatrix  = g_FrameUniform.cameraUniform.clipFromWorldMatrix;

    float4 vertex_worldPosition = mul(localToWorldMatrix, float4(input.position, 1.0f));
    output.vertex_position = mul(projectionViewMatrix, vertex_worldPosition);

    return output;
}
