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
#define SHADER_STAGE_COMPUTE

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

// Direct
[numthreads(8, 8, 1)]
void SHADE_OPAQUE_ENTRY(uint3 dispatchThreadId : SV_DispatchThreadID, uint2 groupId : SV_GroupID)
{
    uint2 pixelCoord   = dispatchThreadId.xy;
    uint  featureFlags = UINT_MAX;

    FrameUniforms frameUniform = GetFrameUniforms();
    SamplerStruct samplerStruct = GetSamplerStruct();
    
    // This need to stay in sync with deferred.shader
    float4 screenSize = frameUniform.baseUniform._ScreenSize;

    Texture2D<float> cameraDepthTexture = ResourceDescriptorHeap[frameUniform.baseUniform._CameraDepthTextureIndex];
    float depth = LoadCameraDepth(cameraDepthTexture, float2(pixelCoord.x, pixelCoord.y));
    
    PositionInputs posInput = GetPositionInput(float2(pixelCoord.x, (screenSize.y) - pixelCoord.y), screenSize.zw, depth,
        UNITY_MATRIX_I_VP(frameUniform), UNITY_MATRIX_V(frameUniform));

    // For indirect case: we can still overlap inside a tile with the sky/background, reject it
    // Can't rely on stencil as we are in compute shader
    if (depth == UNITY_RAW_FAR_CLIP_VALUE)
    {
        return;
    }

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
        float2(posInput.positionSS.x, (screenSize.y) - posInput.positionSS.y), featureFlags, bsdfData, builtinData);


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

/*
// Direct
[numthreads(8, 8, 1)]
void SHADE_OPAQUE_ENTRY(uint3 dispatchThreadId : SV_DispatchThreadID, uint2 groupId : SV_GroupID)
{
    // uint2 tileCoord    = (GROUP_SIZE * groupId);
    uint2 pixelCoord   = dispatchThreadId.xy;
    uint  featureFlags = UINT_MAX;

    // RenderDataPerDraw renderData = GetRenderDataPerDraw();
    // PropertiesPerMaterial matProperties = GetPropertiesPerMaterial(GetLightPropertyBufferIndexOffset(renderData));
    FrameUniforms frameUniform = GetFrameUniforms();
    SamplerStruct samplerStruct = GetSamplerStruct();
    
    // This need to stay in sync with deferred.shader
    float4 screenSize = frameUniform.baseUniform._ScreenSize;

    float depth = LoadCameraDepth(frameUniform, float2(pixelCoord.x, pixelCoord.y));
    
    PositionInputs posInput = GetPositionInput(float2(pixelCoord.x, pixelCoord.y), screenSize.zw, depth,
        UNITY_MATRIX_I_VP(frameUniform), UNITY_MATRIX_V(frameUniform));

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

    if(dispatchThreadId.x > screenSize.x * 0.5f)
    {
        if(dispatchThreadId.y > screenSize.y * 0.5f)
        {
            float depth2 = LoadCameraDepth(frameUniform, float2(pixelCoord.x, pixelCoord.y));
            PositionInputs posInput2 = GetPositionInput(float2(pixelCoord.x, pixelCoord.y),
    screenSize.zw, depth2, UNITY_MATRIX_I_VP(frameUniform), UNITY_MATRIX_V(frameUniform));
            float4 hpositionWS = mul(UNITY_MATRIX_VP(frameUniform), float4(posInput2.positionWS, 1.0f));
            float3 hpositionNDC = hpositionWS.xyz / hpositionWS.w;
            float depth3 = hpositionNDC.z;
            RWTexture2D<float4> specularLightingUAV = ResourceDescriptorHeap[specularLightingUAVIndex];
            if(dispatchThreadId.x < screenSize.x * 0.75f)
            {
                specularLightingUAV[uint2(pixelCoord.x, pixelCoord.y)] = float4(depth3, 0, 0, 1.0);   
            }
            else if(dispatchThreadId.x < screenSize.x * 0.751f)
            {
                specularLightingUAV[uint2(pixelCoord.x, pixelCoord.y)] = float4(1.0f, 1.0f, 1.0f, 1.0);   
            }
            else
            {
                specularLightingUAV[uint2(pixelCoord.x, pixelCoord.y)] = float4(depth2, 0, 0, 1.0);   
            }
        }
        else
        {
            float4 shadow = float4(0, 1, 0, 1);
            float depth2 = LoadCameraDepth(frameUniform, float2(pixelCoord.x, pixelCoord.y));
            PositionInputs posInput2 = GetPositionInput(float2(pixelCoord.x, screenSize.y - 1 - pixelCoord.y),
                screenSize.zw, depth2, UNITY_MATRIX_I_VP(frameUniform), UNITY_MATRIX_V(frameUniform));
            DirectionalLightData light = frameUniform.lightDataUniform.directionalLightData;
            float3 L = -light.forward;
            // Is it worth sampling the shadow map?
            if ((light.lightDimmer > 0) && (light.shadowDimmer > 0) && IsNonZeroBSDF(V, L, preLightData, bsdfData))
            {
                float3 positionWS = posInput2.positionWS;
                float3 normalWS = bsdfData.normalWS * FastSign(dot(bsdfData.normalWS, L));

                // int shadowIndex = frameUniform.lightDataUniform.directionalShadowData.shadowDataIndex[3];

                for (int tt = 0; tt < 4; tt++)
                {
                    HDShadowData sd = frameUniform.lightDataUniform.shadowDatas[light.shadowIndex + tt]; // 
                    float4 hpositionWS =  mul(sd.viewProjMatrix, float4(positionWS, 1));
                    float3 hpositionNDC = hpositionWS.xyz / hpositionWS.w;
                    if(all(abs(hpositionNDC.xy)<0.98f))
                    {
                        float2 posTC = hpositionNDC.xy * 0.5f + float2(0.5f, 0.5f);
                
                        // float3 posTC = EvalShadow_GetTexcoordsAtlas(sd, sd.shadowMapSize.zw, positionWS, false);
                        Texture2D<float4> tex = ResourceDescriptorHeap[sd.shadowmapIndex];
                        float shadow0 = SAMPLE_TEXTURE2D_LOD(tex, samplerStruct.SLinearClampSampler, float2(posTC.x, 1.0f - posTC.y), 0).x;

                        shadow.x = shadow0;
                        shadow.y = 0;
                        
                        break;
                    }
                }
                
            }
            RWTexture2D<float4> specularLightingUAV = ResourceDescriptorHeap[specularLightingUAVIndex];
            specularLightingUAV[uint2(pixelCoord.x, pixelCoord.y)] = shadow;
        }
    }
    else
    {
        if(dispatchThreadId.y > screenSize.y * 0.5f)
        {
            float4 shadow = float4(0, 1, 0, 1);
            float depth2 = LoadCameraDepth(frameUniform, float2(pixelCoord.x, pixelCoord.y));
            PositionInputs posInput2 = GetPositionInput(float2(pixelCoord.x, screenSize.y - 1 - pixelCoord.y),
                screenSize.zw, depth2, UNITY_MATRIX_I_VP(frameUniform), UNITY_MATRIX_V(frameUniform));
            DirectionalLightData light = frameUniform.lightDataUniform.directionalLightData;
            float3 L = -light.forward;
            // Is it worth sampling the shadow map?
            if ((light.lightDimmer > 0) && (light.shadowDimmer > 0) && IsNonZeroBSDF(V, L, preLightData, bsdfData))
            {
                float3 positionWS = posInput2.positionWS;
                float3 normalWS = bsdfData.normalWS * FastSign(dot(bsdfData.normalWS, L));

                // int shadowIndex = frameUniform.lightDataUniform.directionalShadowData.shadowDataIndex[3];

                for (int tt = 0; tt < 4; tt++)
                {
                    HDShadowData sd = frameUniform.lightDataUniform.shadowDatas[light.shadowIndex + tt]; // 
                    float4 hpositionWS =  mul(sd.viewProjMatrix, float4(positionWS, 1));
                    float3 hpositionNDC = hpositionWS.xyz / hpositionWS.w;
                    if(all(abs(hpositionNDC.xy)<0.98f))
                    {
                        float2 posTC = hpositionNDC.xy * 0.5f + float2(0.5f, 0.5f);
                
                        // float3 posTC = EvalShadow_GetTexcoordsAtlas(sd, sd.shadowMapSize.zw, positionWS, false);
                        Texture2D<float4> tex = ResourceDescriptorHeap[sd.shadowmapIndex];
                        float shadow0 = SAMPLE_TEXTURE2D_LOD(tex, samplerStruct.SLinearClampSampler, float2(posTC.x, 1.0f - posTC.y), 0).x;

                        float shadowVal = hpositionNDC.z >= shadow0-0.0005f * pow(2, tt);
                        
                        shadow.x = shadowVal;
                        shadow.y = 0;
                        
                        break;
                    }
                }
                
            }
            RWTexture2D<float4> specularLightingUAV = ResourceDescriptorHeap[specularLightingUAVIndex];
            specularLightingUAV[uint2(pixelCoord.x, pixelCoord.y)] = shadow;
        }
        else
        {
            float depth2 = LoadCameraDepth(frameUniform, float2(pixelCoord.x, pixelCoord.y));
            PositionInputs posInput2 = GetPositionInput(float2(pixelCoord.x, screenSize.y - 1 - pixelCoord.y),
                screenSize.zw, depth2, UNITY_MATRIX_I_VP(frameUniform), UNITY_MATRIX_V(frameUniform));
            DirectionalLightData light = frameUniform.lightDataUniform.directionalLightData;
            float3 L = -light.forward;
            // Is it worth sampling the shadow map?
            if ((light.lightDimmer > 0) && (light.shadowDimmer > 0) && IsNonZeroBSDF(V, L, preLightData, bsdfData))
            {
                float3 positionWS = posInput2.positionWS;
                // float3 normalWS = bsdfData.normalWS * FastSign(dot(bsdfData.normalWS, L));

                int shadowIndex = frameUniform.lightDataUniform.directionalShadowData.shadowDataIndex[2];
                
                HDShadowData sd = frameUniform.lightDataUniform.shadowDatas[shadowIndex]; // light.shadowIndex + 2

                float4 hpositionWS =  mul(sd.viewProjMatrix, float4(positionWS, 1));
                float3 hpositionNDC = hpositionWS.xyz / hpositionWS.w;
                float2 posTC = hpositionNDC.xy * 0.5f + float2(0.5f, 0.5f);
                
                // float3 posTC = EvalShadow_GetTexcoordsAtlas(sd, sd.shadowMapSize.zw, positionWS, false);
                Texture2D<float4> tex = ResourceDescriptorHeap[sd.shadowmapIndex];
                float shadow = SAMPLE_TEXTURE2D_LOD(tex, samplerStruct.SLinearClampSampler, float2(posTC.x, 1.0f - posTC.y), 0).x;
                
                positionWS.xyz /= 20.0f;
                
                RWTexture2D<float4> specularLightingUAV = ResourceDescriptorHeap[specularLightingUAVIndex];
                specularLightingUAV[uint2(pixelCoord.x, pixelCoord.y)] = abs(float4(positionWS.x, positionWS.y, positionWS.z, 1.0));
            }
        }
    }
    return;
    
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
*/
