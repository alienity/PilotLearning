struct VertexAttributes
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

cbuffer RootConstants : register(b0, space0) { uint texIndex; };

SamplerState defaultSampler : register(s10);

float4 PSMain(VertexAttributes input) : SV_Target0
{
    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[texIndex];

    float4 outColor = baseColorTex.Sample(defaultSampler, input.texcoord);

    const float gamma = 2.2f;

    outColor = pow(outColor, 1.0f / gamma);

    return float4(outColor.rgb, 1);
}
