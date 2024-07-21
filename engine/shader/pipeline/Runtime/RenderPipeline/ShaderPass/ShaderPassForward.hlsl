#if SHADERPASS != SHADERPASS_FORWARD
#error SHADERPASS_is_not_correctly_define
#endif

#include "../../RenderPipeline/ShaderPass/VertMesh.hlsl"

PackedVaryingsType Vert(AttributesMesh inputMesh)
{
    VaryingsType varyingsType;

    RenderDataPerDraw renderData = GetRenderDataPerDraw();
    FrameUniforms frameUniform = GetFrameUniforms();
    varyingsType.vmesh = VertMesh(frameUniform, renderData, inputMesh);

    return PackVaryingsType(varyingsType);
}


//NOTE: some shaders set target1 to be
//   Blend 1 SrcAlpha OneMinusSrcAlpha
//The reason for this blend mode is to let virtual texturing alpha dither work.
//Anything using Target1 should write 1.0 or 0.0 in alpha to write / not write into the target.

#ifdef OUTPUT_SPLIT_LIGHTING
#define DIFFUSE_LIGHTING_TARGET SV_Target1
#define SSS_BUFFER_TARGET SV_Target2
#elif defined(_WRITE_TRANSPARENT_MOTION_VECTOR)
#define MOTION_VECTOR_TARGET SV_Target1
#endif

void Frag(PackedVaryingsToPS packedInput
    , out float4 outColor : SV_Target0  // outSpecularLighting when outputting split lighting
    #ifdef OUTPUT_SPLIT_LIGHTING
        , out float4 outDiffuseLighting : DIFFUSE_LIGHTING_TARGET
        , OUTPUT_SSSBUFFER(outSSSBuffer) : SSS_BUFFER_TARGET
    #elif defined(_WRITE_TRANSPARENT_MOTION_VECTOR)
          , out float4 outMotionVec : MOTION_VECTOR_TARGET
    #endif
    #ifdef _DEPTHOFFSET_ON
        , out float outputDepth : DEPTH_OFFSET_SEMANTIC
    #endif
)
{
#ifdef _WRITE_TRANSPARENT_MOTION_VECTOR
    // Init outMotionVector here to solve compiler warning (potentially unitialized variable)
    // It is init to the value of forceNoMotion (with 2.0)
    // Always write 1.0 in alpha since blend mode could be active on this target as a side effect of VT feedback buffer
    // motion vector expected output format is RG16
    outMotionVec = float4(2.0, 0.0, 0.0, 1.0);
#endif

    RenderDataPerDraw renderData = GetRenderDataPerDraw();
    PropertiesPerMaterial matProperties = GetPropertiesPerMaterial(GetLightPropertyBufferIndexOffset(renderData));
    FrameUniforms frameUniform = GetFrameUniforms();
    SamplerStruct samplerStruct = GetSamplerStruct();
    
    FragInputs input = UnpackVaryingsToFragInputs(packedInput);

    uint2 tileIndex = uint2(input.positionSS.xy);

    float4 _ScreenSize = frameUniform.baseUniform._ScreenSize;
    
    // input.positionSS is SV_Position
    PositionInputs posInput = GetPositionInput(input.positionSS.xy, _ScreenSize.zw, input.positionSS.z, input.positionSS.w, input.positionRWS.xyz, tileIndex);

#ifdef VARYINGS_NEED_POSITION_WS
    float3 V = GetWorldSpaceNormalizeViewDir(frameUniform, input.positionRWS);
#else
    // Unused
    float3 V = float3(1.0, 1.0, 1.0); // Avoid the division by 0
#endif

    SurfaceData surfaceData;
    BuiltinData builtinData;
    GetSurfaceAndBuiltinData(frameUniform, renderData, matProperties, samplerStruct, input, V, posInput, surfaceData, builtinData);

    BSDFData bsdfData = ConvertSurfaceDataToBSDFData(input.positionSS.xy, surfaceData);
    
    PreLightData preLightData = GetPreLightData(frameUniform, renderData, matProperties, samplerStruct, V, posInput, bsdfData);

    outColor = float4(0.0, 0.0, 0.0, 0.0);

    // We need to skip lighting when doing debug pass because the debug pass is done before lighting so some buffers may not be properly initialized potentially causing crashes on PS4.

    {
#ifdef _SURFACE_TYPE_TRANSPARENT
        uint featureFlags = LIGHT_FEATURE_MASK_FLAGS_TRANSPARENT;
#else
        uint featureFlags = LIGHT_FEATURE_MASK_FLAGS_OPAQUE;
#endif
        LightLoopOutput lightLoopOutput;
        LightLoop(frameUniform, renderData, matProperties, samplerStruct, V, posInput, preLightData, bsdfData, builtinData, featureFlags, lightLoopOutput);

        // Alias
        float3 diffuseLighting = lightLoopOutput.diffuseLighting;
        float3 specularLighting = lightLoopOutput.specularLighting;

        diffuseLighting *= GetCurrentExposureMultiplier(frameUniform);
        specularLighting *= GetCurrentExposureMultiplier(frameUniform);

#ifdef OUTPUT_SPLIT_LIGHTING
        if (_EnableSubsurfaceScattering != 0 && ShouldOutputSplitLighting(bsdfData))
        {
            outColor = float4(specularLighting, 1.0);
            // Always write 1.0 in alpha since blend mode could be active on this target as a side effect of VT feedback buffer
            // Diffuse output is expected to be RGB10, so alpha must always be 1 to ensure it is written.
            outDiffuseLighting = float4(TagLightingForSSS(diffuseLighting), 1.0);
        }
        else
        {
            outColor = float4(diffuseLighting + specularLighting, 1.0);
            // Always write 1.0 in alpha since blend mode could be active on this target as a side effect of VT feedback buffer
            // Diffuse output is expected to be RGB10, so alpha must always be 1 to ensure it is written.
            outDiffuseLighting = float4(0, 0, 0, 1);
        }
        ENCODE_INTO_SSSBUFFER(surfaceData, posInput.positionSS, outSSSBuffer);
#else
        outColor = ApplyBlendMode(matProperties, diffuseLighting, specularLighting, builtinData.opacity);
        outColor = EvaluateAtmosphericScattering(posInput, V, outColor);
#endif

#ifdef _WRITE_TRANSPARENT_MOTION_VECTOR
        VaryingsPassToPS inputPass = UnpackVaryingsPassToPS(packedInput.vpass);
        bool forceNoMotion = any(unity_MotionVectorsParams.yw == 0.0);
        // outMotionVec is already initialize at the value of forceNoMotion (see above)

            //Motion vector is enabled in SG but not active in VFX
#if defined(HAVE_VFX_MODIFICATION) && !VFX_FEATURE_MOTION_VECTORS
        forceNoMotion = true;
#endif

        if (!forceNoMotion)
        {
            float2 motionVec = CalculateMotionVector(inputPass.positionCS, inputPass.previousPositionCS);
            EncodeMotionVector(motionVec * 0.5, outMotionVec);

            // Always write 1.0 in alpha since blend mode could be active on this target as a side effect of VT feedback buffer
            // motion vector expected output format is RG16
            outMotionVec.zw = 1.0;
        }
#endif
    }

#ifdef _DEPTHOFFSET_ON
    outputDepth = posInput.deviceDepth;
#endif
    
}
