
#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
// #include "../../RenderPipeline/ShaderPass/FragInputs.hlsl"
// #include "../../RenderPipeline/ShaderPass/VaryingMesh.hlsl"
// #include "../../RenderPipeline/ShaderPass/VertMesh.hlsl"
// #include "../../Material/Lit/LitBuiltinData.hlsl"
#include "../../Material/Builtin/BuiltinData.hlsl"

cbuffer RootConstants : register(b0, space0)
{
    uint frameUniformIndex;
};

struct Attributes
{
    uint vertexID : SV_VertexID;
};

struct Varyings
{
    float4 positionCS : SV_POSITION;
};

FrameUniforms GetFrameUniforms()
{
    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[frameUniformIndex];
    return frameUniform;
}

Varyings Vert(Attributes input)
{
    Varyings output;
    output.positionCS = GetFullScreenTriangleVertexPosition(input.vertexID);
    return output;
}

void Frag(Varyings input, out float4 outColor : SV_Target0)
{
    FrameUniforms frameUniform = GetFrameUniforms();

    float4 screenSize = frameUniform.baseUniform._ScreenSize;

    int cameraDepthTextureIndex = frameUniform.baseUniform._CameraDepthTextureIndex;
    Texture2D<float> _CameraDepthTexture = ResourceDescriptorHeap[cameraDepthTextureIndex];
    float depth = LoadCameraDepth(_CameraDepthTexture, input.positionCS.xy);

    PositionInputs posInput = GetPositionInput(input.positionCS.xy, screenSize.zw, depth,
        UNITY_MATRIX_I_VP(frameUniform), UNITY_MATRIX_V(frameUniform));

    float4 worldPos = float4(posInput.positionWS, 1.0);
    float4 prevPos = worldPos;

    float4 prevClipPos = mul(UNITY_MATRIX_PREV_VP(frameUniform), prevPos);
    float4 curClipPos = mul(UNITY_MATRIX_UNJITTERED_VP(frameUniform), worldPos);

    float2 previousPositionCS = prevClipPos.xy / prevClipPos.w;
    float2 positionCS = curClipPos.xy / curClipPos.w;

    // Convert from Clip space (-1..1) to NDC 0..1 space
    float2 motionVector = (positionCS - previousPositionCS);

#ifdef KILL_MICRO_MOVEMENT
    motionVector.x = abs(motionVector.x) < MICRO_MOVEMENT_THRESHOLD(screenSize).x ? 0 : motionVector.x;
    motionVector.y = abs(motionVector.y) < MICRO_MOVEMENT_THRESHOLD(screenSize).y ? 0 : motionVector.y;
#endif

    motionVector = clamp(motionVector, -1.0f + MICRO_MOVEMENT_THRESHOLD(screenSize), 1.0f - MICRO_MOVEMENT_THRESHOLD(screenSize));

#if UNITY_UV_STARTS_AT_TOP
    motionVector.y = -motionVector.y;
#endif

    // Convert motionVector from Clip space (-1..1) to NDC 0..1 space
    // Note it doesn't mean we don't have negative value, we store negative or positive offset in NDC space.
    // Note: ((positionCS * 0.5 + 0.5) - (previousPositionCS * 0.5 + 0.5)) = (motionVector * 0.5)
    EncodeMotionVector(motionVector * 0.5, outColor);
}
