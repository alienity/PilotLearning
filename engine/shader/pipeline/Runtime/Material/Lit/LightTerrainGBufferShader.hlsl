//-------------------------------------------------------------------------------------
// Define
//-------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------
// Include
//-------------------------------------------------------------------------------------

#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../RenderPipeline/ShaderPass/FragInputs.hlsl"
#include "../../RenderPipeline/ShaderPass/ShaderPass.hlsl"

//-------------------------------------------------------------------------------------
// variable declaration
//-------------------------------------------------------------------------------------

#include "../../Material/Lit/LitProperties.hlsl"

//-------------------------------------------------------------------------------------
// Forward Pass
//-------------------------------------------------------------------------------------

#define PROBE_VOLUMES_OFF
#define SCREEN_SPACE_SHADOWS_OFF
#define SHADERPASS SHADERPASS_GBUFFER
#define _NORMALMAP
#define _NORMALMAP_TANGENT_SPACE
#define _MASKMAP
#define SHADOW_LOW
#define UNITY_NO_DXT5nm

#define ENABLE_LOD_SEAMLESS

#include "../../Material/Material.hlsl"
#include "../../Material/Lit/Lit.hlsl"
#include "../../Material/Lit/ShaderPass/LitSharePass.hlsl"
#include "../../Material/Lit/LitData.hlsl"

#include "../../Tools/Terrain/TerrainCommonInput.hlsl"

cbuffer RootConstants : register(b0, space0)
{
    uint frameUniformIndex;
    uint patchListBufferIndex;
    uint terrainHeightMapIndex;
    uint terrainNormalMapIndex;
    uint terrainRenderDataIndex;
    uint terrainPropertyDataIndex;
};

SamplerState s_point_clamp_sampler      : register(s10);
SamplerState s_linear_clamp_sampler     : register(s11);
SamplerState s_linear_repeat_sampler    : register(s12);
SamplerState s_trilinear_clamp_sampler  : register(s13);
SamplerState s_trilinear_repeat_sampler : register(s14);
SamplerComparisonState s_linear_clamp_compare_sampler : register(s15);

RenderDataPerDraw GetRenderDataPerDraw(TerrainRenderData terrainRenderData)
{
    RenderDataPerDraw renderData;
    ZERO_INITIALIZE(RenderDataPerDraw, renderData);
    renderData.objectToWorldMatrix = terrainRenderData.objectToWorldMatrix;
    renderData.worldToObjectMatrix = terrainRenderData.worldToObjectMatrix;
    renderData.prevObjectToWorldMatrix = terrainRenderData.prevObjectToWorldMatrix;
    renderData.prevWorldToObjectMatrix = terrainRenderData.prevWorldToObjectMatrix;
    return renderData;
}

TerrainRenderData GetTerrainRenderData()
{
    StructuredBuffer<TerrainRenderData> terrainRenderDataBuffer = ResourceDescriptorHeap[terrainRenderDataIndex];
    return terrainRenderDataBuffer[0];
}

TerrainRenderPatch GetTerrainRenderPatch(uint instanceIndex)
{
    StructuredBuffer<TerrainRenderPatch> terrainRenderPatchBuffer = ResourceDescriptorHeap[patchListBufferIndex];
    TerrainRenderPatch renderPatch = terrainRenderPatchBuffer[instanceIndex];
    return renderPatch;
}

PropertiesPerMaterial GetTerrainPeroperties()
{
    StructuredBuffer<PropertiesPerMaterial> props = ResourceDescriptorHeap[terrainPropertyDataIndex];
    return props[0];
}

Texture2D<float> GetTerrainHeightMap()
{
    Texture2D<float> terrainHeightMap = ResourceDescriptorHeap[terrainHeightMapIndex];
    return terrainHeightMap;
}

Texture2D<float3> GetTerrainNormalMap()
{
    Texture2D<float3> terrainNormalMap = ResourceDescriptorHeap[terrainNormalMapIndex];
    return terrainNormalMap;
}

FrameUniforms GetFrameUniforms()
{
    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[frameUniformIndex];
    return frameUniform;
}

SamplerStruct GetSamplerStruct()
{
    SamplerStruct mSamplerStruct;
    mSamplerStruct.SPointClampSampler = s_point_clamp_sampler;
    mSamplerStruct.SLinearClampSampler = s_linear_clamp_sampler;
    mSamplerStruct.SLinearRepeatSampler = s_linear_repeat_sampler;
    mSamplerStruct.STrilinearClampSampler = s_trilinear_clamp_sampler;
    mSamplerStruct.STrilinearRepeatSampler = s_trilinear_repeat_sampler;
    mSamplerStruct.SLinearClampCompareSampler = s_linear_clamp_compare_sampler;
    return mSamplerStruct;
}

void FixLODConnectSeam(inout float4 vertex, inout float2 uv, TerrainRenderPatch patch)
{
    uint4 lodTrans = patch.lodTrans;
    uint2 vertexIndex = floor((vertex.xz + PATCH_MESH_SIZE * 0.5 + 0.01) / PATCH_MESH_GRID_SIZE);
    float uvGridStrip = 1.0/PATCH_MESH_GRID_COUNT;

    uint lodDelta = lodTrans.x;
    if(lodDelta > 0 && vertexIndex.x == 0)
    {
        uint gridStripCount = pow(2,lodDelta);
        uint modIndex = vertexIndex.y % gridStripCount;
        if(modIndex != 0)
        {
            vertex.z -= PATCH_MESH_GRID_SIZE * modIndex;
            uv.y -= uvGridStrip * modIndex;
            return;
        }
    }

    lodDelta = lodTrans.y;
    if(lodDelta > 0 && vertexIndex.y == 0)
    {
        uint gridStripCount = pow(2,lodDelta);
        uint modIndex = vertexIndex.x % gridStripCount;
        if(modIndex != 0)
        {
            vertex.x -= PATCH_MESH_GRID_SIZE * modIndex;
            uv.x -= uvGridStrip * modIndex;
            return;
        }
    }

    lodDelta = lodTrans.z;
    if(lodDelta > 0 && vertexIndex.x == PATCH_MESH_GRID_COUNT)
    {
        uint gridStripCount = pow(2,lodDelta);
        uint modIndex = vertexIndex.y % gridStripCount;
        if(modIndex != 0)
        {
            vertex.z += PATCH_MESH_GRID_SIZE * (gridStripCount - modIndex);
            uv.y += uvGridStrip * (gridStripCount- modIndex);
            return;
        }
    }

    lodDelta = lodTrans.w;
    if(lodDelta > 0 && vertexIndex.y == PATCH_MESH_GRID_COUNT)
    {
        uint gridStripCount = pow(2,lodDelta);
        uint modIndex = vertexIndex.x % gridStripCount;
        if(modIndex != 0)
        {
            vertex.x += PATCH_MESH_GRID_SIZE * (gridStripCount- modIndex);
            uv.x += uvGridStrip * (gridStripCount- modIndex);
            return;
        }
    }
}

#include "../../RenderPipeline/ShaderPass/VertMesh.hlsl"

PackedVaryingsType Vert(AttributesMesh inputMesh, uint instanceIndex : SV_InstanceID)
{
    FrameUniforms frameUniform = GetFrameUniforms();
    
    Texture2D<float> terrainHeightMap = GetTerrainHeightMap();
    Texture2D<float3> terrainNormalMap = GetTerrainNormalMap();
    TerrainRenderPatch patch = GetTerrainRenderPatch(instanceIndex);
    TerrainRenderData terrainRenderData = GetTerrainRenderData();
    
    RenderDataPerDraw renderData = GetRenderDataPerDraw(terrainRenderData);
    
    float4 inVertex = float4(inputMesh.positionOS, 1.0f);
    float2 uv = inputMesh.uv0;
#ifdef ENABLE_LOD_SEAMLESS
    FixLODConnectSeam(inVertex, uv, patch);
#endif
    uint lod = patch.lod;
    float scale = pow(2, lod);

    inVertex.xz *= scale;
    inVertex.xz += patch.position;

    float4 _WorldSize = terrainRenderData.terrainSize;
    
    float2 heightUV = (inVertex.xz + (_WorldSize.xz * 0.5) + 0.5) / (_WorldSize.xz + 1);

    float height = terrainHeightMap.SampleLevel(s_linear_clamp_sampler, heightUV, 0).r;
    inVertex.y = height * _WorldSize.y;

    float3 normal = float3(0, 1, 0);
    normal.xz = terrainNormalMap.SampleLevel(s_linear_clamp_sampler, uv, 0).xy * 2 - 1;
    normal.y = sqrt(max(0, 1 - dot(normal.xz, normal.xz)));

    AttributesMesh pathMesh;
    ZERO_INITIALIZE(AttributesMesh, pathMesh);
    pathMesh.positionOS = inVertex.xyz;
    pathMesh.uv0 = uv * scale * 8;
    pathMesh.tangentOS = inputMesh.tangentOS;
    pathMesh.normalOS = normal;
    
    VaryingsType varyingsType;
    varyingsType.vmesh = VertMesh(frameUniform, renderData, pathMesh);
    return PackVaryingsType(varyingsType);
}

void Frag(PackedVaryingsToPS packedInput, OUTPUT_GBUFFER(outGBuffer))
{
    TerrainRenderData terrainRenderData = GetTerrainRenderData();
    RenderDataPerDraw renderData = GetRenderDataPerDraw(terrainRenderData);
    PropertiesPerMaterial matProperties = GetTerrainPeroperties();
    FrameUniforms frameUniform = GetFrameUniforms();
    SamplerStruct samplerStruct = GetSamplerStruct();

    FragInputs input = UnpackVaryingsToFragInputs(packedInput);
    
    float4 _ScreenSize = frameUniform.baseUniform._ScreenSize;
    
    // input.positionSS is SV_Position
    PositionInputs posInput = GetPositionInput(input.positionSS.xy, _ScreenSize.zw,
        input.positionSS.z, input.positionSS.w, input.positionRWS.xyz);
    
    float3 V = GetWorldSpaceNormalizeViewDir(frameUniform, input.positionRWS);
    
    SurfaceData surfaceData;
    BuiltinData builtinData;
    GetSurfaceAndBuiltinData(frameUniform, renderData, matProperties, samplerStruct, input, V, posInput, surfaceData, builtinData);
    
    ENCODE_INTO_GBUFFER(frameUniform, surfaceData, builtinData, posInput.positionSS, outGBuffer);
}


//#pragma vertex Vert
//#pragma fragment Frag
