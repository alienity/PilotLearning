#if SHADERPASS != SHADERPASS_MOTION_VECTORS
#error SHADERPASS_is_not_correctly_define
#endif

#include "../../../Runtime/RenderPipeline/ShaderPass/MotionVectorVertexShaderCommon.hlsl"

PackedVaryingsType Vert(AttributesMesh inputMesh/*, AttributesPass inputPass*/)
{
    AttributesPass inputPass = (AttributesPass)0;
    inputPass.previousPositionOS = inputMesh.positionOS;
    
    VaryingsType varyingsType;

    RenderDataPerDraw renderData = GetRenderDataPerDraw();
    FrameUniforms frameUniform = GetFrameUniforms();
    varyingsType.vmesh = VertMesh(frameUniform, renderData, inputMesh);
    return MotionVectorVS(frameUniform, renderData, varyingsType, inputMesh, inputPass);
}

#define SV_TARGET_NORMAL SV_Target1

// Caution: Motion vector pass is different from Depth prepass, it render normal buffer last instead of decal buffer last
// and thus, we force a write of 0 if _DISABLE_DECALS so we always write in the decal buffer.
// This is required as we can't make distinction  between deferred (write normal buffer) and forward (write normal buffer)
// in the context of the motion vector pass. The cost is acceptable as it is only do object with motion vector (usualy skin object)
// that most of the time use Forward Material (so are already writing motion vector data).
// So note that here unlike for depth prepass we don't check && !defined(_DISABLE_DECALS)
void Frag(  PackedVaryingsToPS packedInput
            // When no MSAA, the motion vector is always the first buffer
            , out float4 outMotionVector : SV_Target0
            // Decal buffer must be last as it is bind but we can optionally write into it (based on _DISABLE_DECALS)
            #ifdef WRITE_NORMAL_BUFFER
            , out float4 outNormalBuffer : SV_TARGET_NORMAL
            #endif

            #ifdef _DEPTHOFFSET_ON
            , out float outputDepth : DEPTH_OFFSET_SEMANTIC
            #endif
        )
{

    RenderDataPerDraw renderData = GetRenderDataPerDraw();
    PropertiesPerMaterial matProperties = GetPropertiesPerMaterial(GetLightPropertyBufferIndexOffset(renderData));
    FrameUniforms frameUniform = GetFrameUniforms();
    SamplerStruct samplerStruct = GetSamplerStruct();

    float4 _ScreenSize = frameUniform.baseUniform._ScreenSize;
    
    FragInputs input = UnpackVaryingsToFragInputs(packedInput);
    
    // input.positionSS is SV_Position
    PositionInputs posInput = GetPositionInput(input.positionSS.xy, _ScreenSize.zw, input.positionSS.z, input.positionSS.w, input.positionRWS);

#ifdef VARYINGS_NEED_POSITION_WS
    float3 V = GetWorldSpaceNormalizeViewDir(frameUniform, input.positionRWS);
#else
    // Unused
    float3 V = float3(1.0, 1.0, 1.0); // Avoid the division by 0
#endif

    SurfaceData surfaceData;
    BuiltinData builtinData;
    GetSurfaceAndBuiltinData(frameUniform, renderData, matProperties, samplerStruct, input, V, posInput, surfaceData, builtinData);

    VaryingsPassToPS inputPass = UnpackVaryingsPassToPS(packedInput.vpass);
#ifdef _DEPTHOFFSET_ON
    inputPass.positionCS.w += builtinData.depthOffset;
    inputPass.previousPositionCS.w += builtinData.depthOffset;
#endif

    // TODO: How to allow overriden motion vector from GetSurfaceAndBuiltinData ?
    float2 motionVector = CalculateMotionVector(inputPass.positionCS, inputPass.previousPositionCS, _ScreenSize);

    // Convert from Clip space (-1..1) to NDC 0..1 space.
    // Note it doesn't mean we don't have negative value, we store negative or positive offset in NDC space.
    // Note: ((positionCS * 0.5 + 0.5) - (previousPositionCS * 0.5 + 0.5)) = (motionVector * 0.5)
    EncodeMotionVector(motionVector * 0.5, outMotionVector);

    // Note: unity_MotionVectorsParams.y is 0 is forceNoMotion is enabled
    bool forceNoMotion = frameUniform.cameraUniform.unity_MotionVectorsParams.y == 0.0;

    // Setting the motionVector to a value more than 2 set as a flag for "force no motion". This is valid because, given that the velocities are in NDC,
    // a value of >1 can never happen naturally, unless explicitely set.
    if (forceNoMotion)
        outMotionVector = float4(2.0, 0.0, 0.0, 0.0);

// Normal Buffer Processing
#ifdef WRITE_NORMAL_BUFFER
    EncodeIntoNormalBuffer(ConvertSurfaceDataToNormalData(surfaceData), outNormalBuffer);
#endif

#ifdef _DEPTHOFFSET_ON
    outputDepth = posInput.deviceDepth;
#endif
}
