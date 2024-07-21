//-------------------------------------------------------------------------------------
// Include
//-------------------------------------------------------------------------------------

#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../RenderPipeline/ShaderPass/ShaderPass.hlsl"

#include "../../Material/Lit/LitProperties.hlsl"

#define PROBE_VOLUMES_OFF
#define SCREEN_SPACE_SHADOWS_OFF
#define SHADERPASS SHADERPASS_DEFERRED_LIGHTING
#define _NORMALMAP
#define _NORMALMAP_TANGENT_SPACE
#define _MASKMAP
#define SHADOW_LOW
#define UNITY_NO_DXT5nm

#include "../../Material/Material.hlsl"
#include "../../Lighting/Lighting.hlsl"

// The light loop (or lighting architecture) is in charge to:
// - Define light list
// - Define the light loop
// - Setup the constant/data
// - Do the reflection hierarchy
// - Provide sampling function for shadowmap, ies, cookie and reflection (depends on the specific use with the light loops like index array or atlas or single and texture format (cubemap/latlong))

#define HAS_LIGHTLOOP


// Note: We have fix as guidelines that we have only one deferred material (with control of GBuffer enabled). Mean a users that add a new
// deferred material must replace the old one here. If in the future we want to support multiple layout (cause a lot of consistency problem),
// the deferred shader will require to use multicompile.

#include "../../Lighting/LightLoop/LightLoopDef.hlsl"
#include "../../Material/Lit/Lit.hlsl"
#include "../../Lighting/LightLoop/LightLoop.hlsl"

// #include "../../Material/Lit/ShaderPass/LitSharePass.hlsl"
// #include "../../Material/Lit/LitData.hlsl"

//-------------------------------------------------------------------------------------
// variable declaration
//-------------------------------------------------------------------------------------

cbuffer RootConstants : register(b0, space0)
{
    uint frameUniformIndex;

    uint inGBuffer0Index;
    uint inGBuffer1Index;
    uint inGBuffer2Index;
    uint inGBuffer3Index;
    
    // uint diffuseLightingUAVIndex; // float3
    uint specularLightingUAVIndex; // float4
};

SamplerState s_point_clamp_sampler      : register(s10);
SamplerState s_linear_clamp_sampler     : register(s11);
SamplerState s_linear_repeat_sampler    : register(s12);
SamplerState s_trilinear_clamp_sampler  : register(s13);
SamplerState s_trilinear_repeat_sampler : register(s14);
SamplerComparisonState s_linear_clamp_compare_sampler : register(s15);

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

#define GROUP_SIZE 8

// Direct
[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void SHADE_OPAQUE_ENTRY(uint3 dispatchThreadId : SV_DispatchThreadID, uint2 groupId : SV_GroupID)
{
    uint2 tileCoord    = (GROUP_SIZE * groupId);
    uint2 pixelCoord   = dispatchThreadId.xy;
    uint  featureFlags = UINT_MAX;

    // RenderDataPerDraw renderData = GetRenderDataPerDraw();
    // PropertiesPerMaterial matProperties = GetPropertiesPerMaterial(GetLightPropertyBufferIndexOffset(renderData));
    FrameUniforms frameUniform = GetFrameUniforms();
    SamplerStruct samplerStruct = GetSamplerStruct();
    
    // This need to stay in sync with deferred.shader
    float4 screenSize = frameUniform.baseUniform._ScreenSize;

    float depth = LoadCameraDepth(frameUniform, pixelCoord.xy);
    PositionInputs posInput = GetPositionInput(pixelCoord.xy, screenSize.zw, depth,
        UNITY_MATRIX_I_VP(frameUniform), UNITY_MATRIX_V(frameUniform), tileCoord);

    // // For indirect case: we can still overlap inside a tile with the sky/background, reject it
    // // Can't rely on stencil as we are in compute shader
    // if (depth == UNITY_RAW_FAR_CLIP_VALUE)
    // {
    //     return;
    // }

    // // This is required for camera stacking and other cases where we might have a valid depth value in the depth buffer, but the pixel was not covered by this camera
    // uint stencilVal = GetStencilValue(LOAD_TEXTURE2D_X(_StencilTexture, pixelCoord.xy));
    // if ((stencilVal & STENCILUSAGE_REQUIRES_DEFERRED_LIGHTING) == 0)
    // {
    //     return;
    // }

    float3 V = GetWorldSpaceNormalizeViewDir(frameUniform, posInput.positionWS);

    BSDFData bsdfData;
    BuiltinData builtinData;
    // DECODE_FROM_GBUFFER(posInput.positionSS, featureFlags, bsdfData, builtinData);
    Texture2D<GBufferType0> inGBuffer0 = ResourceDescriptorHeap[inGBuffer0Index];
    Texture2D<GBufferType1> inGBuffer1 = ResourceDescriptorHeap[inGBuffer1Index];
    Texture2D<GBufferType2> inGBuffer2 = ResourceDescriptorHeap[inGBuffer2Index];
    Texture2D<GBufferType3> inGBuffer3 = ResourceDescriptorHeap[inGBuffer3Index];
    DecodeFromGBuffer(inGBuffer0, inGBuffer1, inGBuffer2, inGBuffer3,
        posInput.positionSS, featureFlags, bsdfData, builtinData);

    PreLightData preLightData = GetPreLightData(frameUniform, samplerStruct, V, posInput, bsdfData);

    LightLoopOutput lightLoopOutput;
    LightLoop(frameUniform, samplerStruct, V, posInput, preLightData, bsdfData, builtinData, featureFlags, lightLoopOutput);

    // Alias
    float3 diffuseLighting = lightLoopOutput.diffuseLighting;
    float3 specularLighting = lightLoopOutput.specularLighting;

    diffuseLighting *= GetCurrentExposureMultiplier(frameUniform);
    specularLighting *= GetCurrentExposureMultiplier(frameUniform);
    
    // if (frameUniform.sssUniform._EnableSubsurfaceScattering != 0 && ShouldOutputSplitLighting(bsdfData))
    // {
    //     Texture2D<float4> specularLightingUAV = ResourceDescriptorHeap[specularLightingUAVIndex];
    //     Texture2D<float3> diffuseLightingUAV = ResourceDescriptorHeap[diffuseLightingUAVIndex];
    //     
    //     specularLightingUAV[pixelCoord] = float4(specularLighting, 1.0);
    //     diffuseLightingUAV[pixelCoord]  = TagLightingForSSS(diffuseLighting);
    // }
    // else
    {
        RWTexture2D<float4> specularLightingUAV = ResourceDescriptorHeap[specularLightingUAVIndex];
        
        specularLightingUAV[pixelCoord] = float4(diffuseLighting + specularLighting, 1.0);
    }
}
