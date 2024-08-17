
#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ImageBasedLighting.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"

// ----------------------------------------------------------------------------
// Pre-Integration
// ----------------------------------------------------------------------------

struct Attributes
{
    uint vertexID : SV_VertexID;
};

struct Varyings
{
    float4 positionCS : SV_POSITION;
    float2 texCoord   : TEXCOORD0;
};

Varyings Vert(Attributes input)
{
    Varyings output;

    output.positionCS = GetFullScreenTriangleVertexPosition(input.vertexID);
    output.texCoord   = GetFullScreenTriangleTexCoord(input.vertexID);

    return output;
}

float4 Frag(Varyings input) : SV_Target
{
    // Currently, we do not implement the pre-integration of Marschner for two reason:
    // 1) Area Light support for anisotropic LTC is not supported, and we fall back to GGX.
    // 2) Environment lighting is evaluated with the BSDF directly.

    float4 preFGD = 0;

    return float4(preFGD.xyz, 1.0);
}
