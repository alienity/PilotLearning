#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "SSRLib.hlsli"

#define KERNEL_SIZE 16
#define FLT_EPS 0.00000001f;

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint motionVectorBufferIndex;
    uint mainColorBufferIndex;
    uint depthBufferIndex;
    uint reprojectionBufferReadIndex;
    uint reprojectionBufferWriteIndex;
	uint outColorBufferIndex;
};

SamplerState defaultSampler : register(s10);

float clampDelta(float2 start, float2 end, float2 delta)
{
    float2 dir = abs(end - start);
    return length(float2(min(dir.x, delta.x), min(dir.y, delta.y)));
}

float4 rayMatch(float3 viewPos, float3 viewDir, float3 screenPos, float2 uv, int numSteps, float thickness, 
    float4x4 projectionMatrix, float2 screenDelta2, float4 zBufferParams, Texture2D<float4> minDepthTex)
{
    float4 rayProj = mul(projectionMatrix, float4(viewPos + viewDir, 1.0f));
    float3 rayDir = normalize(rayProj.xyz / rayProj.w - screenPos);
    rayDir.xy *= 0.5;
    float3 rayStart = float3(uv, screenPos.z);
    float d = clampDelta(rayStart.xy, rayStart.xy + rayDir.xy, screenDelta2);
    float3 samplePos = rayStart + rayDir * d;
    int level = 0;
    
    float mask = 0;
    [loop]
    for (int i = 0; i < numSteps; i++)
    {
        float2 currentDelta = screenDelta2 * exp2(level + 1);
        float distance = clampDelta(samplePos.xy, samplePos.xy + rayDir.xy, currentDelta);
        float3 nextSamplePos = samplePos + rayDir * distance;
        float sampleMinDepth = minDepthTex.SampleLevel(defaultSampler, nextSamplePos.xy, level);
        float nextDepth = nextSamplePos.z;
        
        [flatten]
        if (sampleMinDepth < nextDepth)
        {
            level = min(level + 1, 6);
            samplePos = nextSamplePos;
        }
        else
        {
            level--;
        }
        
        [branch]
        if (level < 0)
        {
            float delta = (-LinearEyeDepth(sampleMinDepth, zBufferParams)) - (-LinearEyeDepth(samplePos.z, zBufferParams));
            mask = delta <= thickness && i > 0;
            return float4(samplePos, mask);
        }
    }
}

[numthreads(KERNEL_SIZE, KERNEL_SIZE, 1)]
void CSRaycast(uint3 id : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    float4 RayCastSize;
    
    float2 uv = (id.xy + 0.5) * RayCastSize.zw;
    
    
}







[numthreads( 8, 8, 1 )]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float4> mainColorBuffer = ResourceDescriptorHeap[mainColorBufferIndex];
    Texture2D<float4> motionVectorBuffer = ResourceDescriptorHeap[motionVectorBufferIndex];
    Texture2D<float> depthBuffer = ResourceDescriptorHeap[depthBufferIndex];
    Texture2D<float4> reprojectionBufferRead = ResourceDescriptorHeap[reprojectionBufferReadIndex];
    RWTexture2D<float4> reprojectionBufferWrite = ResourceDescriptorHeap[reprojectionBufferWriteIndex];
	RWTexture2D<float4> outColorBuffer = ResourceDescriptorHeap[outColorBufferIndex];

    uint width, height;
    depthBuffer.GetDimensions(width, height);

    if(DTid.x >= width || DTid.y >= height)
        return;

    TAAUniform taaUniform = mFrameUniforms.taaUniform;

    float4x4 projectionMatrix = mFrameUniforms.cameraUniform.curFrameUniform.clipFromViewMatrix;

    float4 jitterUV = taaUniform.jitterUV;
    float feedbackMin = taaUniform.feedbackMin;
    float feedbackMax = taaUniform.feedbackMax;
    float motionScale = taaUniform.motionScale;

	float2 ss_txc = (DTid.xy + float2(0.5, 0.5)) / float2(width, height);
    float2 texel_size = 1.0f / float2(width, height);

    float2 uv = ss_txc - jitterUV.xy;
	float _time = mFrameUniforms.baseUniform.time;
	float sin_time = sin(_time);

        //--- 3x3 nearest (good)
    float3 c_frag = find_closest_fragment_3x3(depthBuffer, texel_size, uv);
    float2 ss_vel = motionVectorBuffer.Sample(defaultSampler, c_frag.xy).xy;
    float vs_dist = depth_resolve_linear(c_frag.z, projectionMatrix._m22, projectionMatrix._m23);
	// float2 ss_vel = motionVectorBuffer.Sample(defaultSampler, uv.xy).xy;
	// float vs_dist = depthBuffer.Sample(defaultSampler, uv.xy).x;

    // temporal resolve
    float4 color_temporal = temporal_reprojection(mainColorBuffer, reprojectionBufferRead, texel_size, 
		jitterUV.xy, feedbackMin, feedbackMax, ss_txc, ss_vel, vs_dist);

    // prepare outputs
    float4 to_buffer = resolve_color(color_temporal);
    float4 to_screen = resolve_color(color_temporal);

    // add noise
    float4 noise4 = PDsrand4(ss_txc + sin_time + 0.6959174) / 510.0;

    float4 outbuffer = max(to_buffer + noise4, 0);
    float4 outscreen = max(to_screen + noise4, 0);

	reprojectionBufferWrite[DTid.xy] = outbuffer;
	outColorBuffer[DTid.xy] = outscreen;
}

