#if SHADERPASS != SHADERPASS_GBUFFER
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

void Frag(  PackedVaryingsToPS packedInput,
            OUTPUT_GBUFFER(outGBuffer)
            )
{
    RenderDataPerDraw renderData = GetRenderDataPerDraw();
    PropertiesPerMaterial matProperties = GetPropertiesPerMaterial(GetLightPropertyBufferIndexOffset(renderData));
    FrameUniforms frameUniform = GetFrameUniforms();
    SamplerStruct samplerStruct = GetSamplerStruct();

    FragInputs input = UnpackVaryingsToFragInputs(packedInput);
    
    float4 _ScreenSize = frameUniform.baseUniform._ScreenSize;
    
    // input.positionSS is SV_Position
    PositionInputs posInput = GetPositionInput(input.positionSS.xy, _ScreenSize.zw, input.positionSS.z, input.positionSS.w, input.positionRWS);
    
    float3 V = GetWorldSpaceNormalizeViewDir(frameUniform, input.positionRWS);
    
    SurfaceData surfaceData;
    BuiltinData builtinData;
    GetSurfaceAndBuiltinData(frameUniform, renderData, matProperties, samplerStruct, input, V, posInput, surfaceData, builtinData);
    
    ENCODE_INTO_GBUFFER(frameUniform, surfaceData, builtinData, posInput.positionSS, outGBuffer);
}
