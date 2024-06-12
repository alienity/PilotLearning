
//--------------------------------------------------------------------------------------------------
// Included headers
//--------------------------------------------------------------------------------------------------
#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ShadingParameters.hlsli"

//--------------------------------------------------------------------------------------------------
// Inputs & outputs
//--------------------------------------------------------------------------------------------------

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint depthTextureIndex;
    uint volumeLight3DIndex;
    uint specularSourceIndex;
    uint outColorTextureIndex;
};

SamplerState defaultSampler : register(s10);

[numthreads(8, 8, 1)]
void CombineLightingMain(uint3 groupId : SV_GroupID,
                          uint groupThreadId : SV_GroupThreadID,
                          uint3 dispatchThreadId : SV_DispatchThreadID)
{
    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float> depthTexture = ResourceDescriptorHeap[depthTextureIndex];
    Texture3D<float4> volumeLight3DTexture = ResourceDescriptorHeap[volumeLight3DIndex];
    Texture2D<float4> specularSource = ResourceDescriptorHeap[specularSourceIndex];
    RWTexture2D<float4> outColorTexture = ResourceDescriptorHeap[outColorTextureIndex];
    
    uint width, height;
    specularSource.GetDimensions(width, height);

    float2 uv = (dispatchThreadId.xy + float2(0.5, 0.5)) / float2(width, height);
    
    float depth = depthTexture.Sample(defaultSampler, uv).r;
    
    float4 volumeLightVal = evaluateVolumeDepth(frameUniform, volumeLight3DTexture, defaultSampler, uv.xy, depth);
    
    float3 specular = specularSource.Sample(defaultSampler, uv).rgb;
    float4 outColor = outColorTexture[dispatchThreadId.xy];
    
    outColor.rgb += specular;
    
    outColor.rgb = lerp(outColor.rgb, volumeLightVal.rgb, 1.0f - volumeLightVal.a);
    
    outColorTexture[dispatchThreadId.xy] = outColor;
}