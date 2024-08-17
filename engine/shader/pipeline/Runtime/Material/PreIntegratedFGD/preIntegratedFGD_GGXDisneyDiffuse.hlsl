
#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ImageBasedLighting.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"

#define FGDTEXTURE_RESOLUTION (64)

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
    // We want the LUT to contain the entire [0, 1] range, without losing half a texel at each side.
    float2 coordLUT = RemapHalfTexelCoordTo01(input.texCoord, FGDTEXTURE_RESOLUTION);

    // The FGD texture is parametrized as follows:
    // X = sqrt(dot(N, V))
    // Y = perceptualRoughness
    // These coordinate sampling must match the decoding in GetPreIntegratedDFG in Lit.hlsl,
    // i.e here we use perceptualRoughness, must be the same in shader
    // Note: with this angular parametrization, the LUT is almost perfectly linear,
    // except for the grazing angle when (NdotV -> 0).
    float NdotV = coordLUT.x * coordLUT.x;
    float perceptualRoughness = coordLUT.y;

    // Pre integrate GGX with smithJoint visibility as well as DisneyDiffuse
    float4 preFGD = IntegrateGGXAndDisneyDiffuseFGD(NdotV, PerceptualRoughnessToRoughness(perceptualRoughness));

    return float4(preFGD.xyz, 1.0);
}
