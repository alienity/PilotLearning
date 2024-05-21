
//--------------------------------------------------------------------------------------------------
// Included headers
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Inputs & outputs
//--------------------------------------------------------------------------------------------------

cbuffer RootConstants : register(b0, space0)
{
    uint irradianceSourceIndex;
    uint outColorTextureIndex;
};

SamplerState defaultSampler : register(s10);

[numthreads(8, 8, 1)]
void CombineLightingMain(uint3 groupId : SV_GroupID,
                          uint groupThreadId : SV_GroupThreadID,
                          uint3 dispatchThreadId : SV_DispatchThreadID)
{
    Texture2D<float4> irradianceSource = ResourceDescriptorHeap[irradianceSourceIndex];
    RWTexture2D<float4> outColorTexture = ResourceDescriptorHeap[outColorTextureIndex];
    
    uint width, height;
    irradianceSource.GetDimensions(width, height);

    float2 uv = (dispatchThreadId.xy + float2(0.5, 0.5)) / float2(width, height);
    
    float3 irradiance = irradianceSource.Sample(defaultSampler, uv).rgb;
    float4 outColor = outColorTexture[dispatchThreadId.xy];
    
    outColor.rgb += irradiance;
    
    outColorTexture[dispatchThreadId.xy] = outColor;
}
