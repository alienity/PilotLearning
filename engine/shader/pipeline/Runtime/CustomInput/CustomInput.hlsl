#ifndef __CUSTOM_INPUT_HLSL__
#define __CUSTOM_INPUT_HLSL__

cbuffer RootConstants : register(b0, space0)
{
    uint meshIndex;
    uint renderDataPerDrawIndex;
    uint propertiesPerMaterialIndex;
    uint frameUniformIndex;
};

SamplerState s_point_clamp_sampler      : register(s10);
SamplerState s_linear_clamp_sampler     : register(s11);
SamplerState s_linear_repeat_sampler    : register(s12);
SamplerState s_trilinear_clamp_sampler  : register(s13);
SamplerState s_trilinear_repeat_sampler : register(s14);
SamplerComparisonState s_linear_clamp_compare_sampler : register(s15);

RenderDataPerDraw GetRenderDataPerDraw()
{
    StructuredBuffer<RenderDataPerDraw> renderDataPerDrawBuffer = ResourceDescriptorHeap[renderDataPerDrawIndex];
    RenderDataPerDraw renderData = renderDataPerDrawBuffer[meshIndex];
    return renderData;
}

PropertiesPerMaterial GetPropertiesPerMaterial(uint meshMatOffset)
{
    StructuredBuffer<PropertiesPerMaterial> propertiesPerMaterialBuffer = ResourceDescriptorHeap[propertiesPerMaterialIndex];
    PropertiesPerMaterial propertiesPerMaterial = propertiesPerMaterialBuffer[meshMatOffset];
    return propertiesPerMaterial;
}

FrameUniforms GetFrameUniforms()
{
    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[frameUniformIndex];
    return frameUniform;
}

#endif
