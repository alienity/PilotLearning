#if SHADERPASS != SHADERPASS_GBUFFER
#error SHADERPASS_is_not_correctly_define
#endif

#include "../../RenderPipeline/ShaderPass/VertMesh.hlsl"

PackedVaryingsType Vert(AttributesMesh inputMesh)
{
    VaryingsType varyingsType;

    varyingsType.vmesh = VertMesh(inputMesh);

    return PackVaryingsType(varyingsType);
}

void Frag(  PackedVaryingsToPS packedInput,
            OUTPUT_GBUFFER(outGBuffer)
            )
{
    // FrameUniforms frameUniform = (FrameUniforms)0;
    //
    // FragInputs input = UnpackVaryingsToFragInputs(packedInput);
    //
    // float4 _ScreenSize = frameUniform.baseUniform._ScreenSize;
    //
    // // input.positionSS is SV_Position
    // PositionInputs posInput = GetPositionInput(input.positionSS.xy, _ScreenSize.zw, input.positionSS.z, input.positionSS.w, input.positionRWS);
    //
    // float3 V = GetWorldSpaceNormalizeViewDir(frameUniform, input.positionRWS);
    //
    // SurfaceData surfaceData;
    // BuiltinData builtinData;
    // GetSurfaceAndBuiltinData(input, V, posInput, surfaceData, builtinData);
    //
    // ENCODE_INTO_GBUFFER(surfaceData, builtinData, posInput.positionSS, outGBuffer);
    
    outGBuffer0 = float4(0, 0, 0, 0);
    outGBuffer1 = float4(0, 0, 0, 0);
    outGBuffer2 = float4(0, 0, 0, 0);
    outGBuffer3 = float4(0, 0, 0, 0);
}
