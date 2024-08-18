
#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/Color.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../Lighting/Lighting.hlsl"
#include "../../Material/Material.hlsl"
#include "../../Material/NormalBuffer.hlsl"
#include "../../Lighting/LightLoop/LightLoopDef.hlsl"
#include "../../Material/BuiltinGIUtilities.hlsl"
#include "../../Material/MaterialEvaluation.hlsl"
#include "../../Lighting/LightEvaluation.hlsl"

// Raytracing includes (should probably be in generic files)
#include "../../RenderPipeline/Raytracing/Shaders/RaytracingSampling.hlsl"
#include "../../RenderPipeline/Raytracing/Shaders/RayTracingCommon.hlsl"

// The dispatch tile resolution
#define INDIRECT_DIFFUSE_TILE_SIZE 8

// Defines the mip offset for the color buffer
#define SSGI_MIP_OFFSET 1

#define SSGI_CLAMP_VALUE 7.0f

cbuffer RootConstants : register(b0, space0)
{
    uint frameUniformIndex;
    uint depthTextureIndex;
    uint indirectDiffuseHitPointTextureRWIndex;
};

#include "../../Lighting/ScreenSpaceLighting/RayMarching.hlsl"














