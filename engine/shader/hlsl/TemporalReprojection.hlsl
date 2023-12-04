#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

#define ZCMP_GT(a, b) (a < b)

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint mainColorBufferIndex;
    uint motionVectorBufferIndex;
    uint depthBufferIndex;
    uint reprojectionBufferReadIndex;
    uint reprojectionBufferWriteIndex;
};

SamplerState defaultSampler : register(s10);

// Convert rgb to luminance
// with rgb in linear space with sRGB primaries and D65 white point
float Luminance(float3 linearRgb) { return dot(linearRgb, float3(0.2126729, 0.7151522, 0.0721750)); }

float toLinearZ(float fz, float m22, float m23)
{
    return -m23/(fz+m22);
}

float depth_resolve_linear(float z, float m22, float m23)
{
	return toLinearZ(z, m22, m23);
}

float3 find_closest_fragment_3x3(Texture2D<float> depthTexture, float2 depthTexelSize, float2 uv)
{
	float2 dd = abs(depthTexelSize.xy);
	float2 du = float2(dd.x, 0.0);
	float2 dv = float2(0.0, dd.y);

	float3 dtl = float3(-1, -1, depthTexture.Sample(defaultSampler, uv - dv - du).x);
	float3 dtc = float3( 0, -1, depthTexture.Sample(defaultSampler, uv - dv).x);
	float3 dtr = float3( 1, -1, depthTexture.Sample(defaultSampler, uv - dv + du).x);

	float3 dml = float3(-1, 0, depthTexture.Sample(defaultSampler, uv - du).x);
	float3 dmc = float3( 0, 0, depthTexture.Sample(defaultSampler, uv).x);
	float3 dmr = float3( 1, 0, depthTexture.Sample(defaultSampler, uv + du).x);

	float3 dbl = float3(-1, 1, depthTexture.Sample(defaultSampler, uv + dv - du).x);
	float3 dbc = float3( 0, 1, depthTexture.Sample(defaultSampler, uv + dv).x);
	float3 dbr = float3( 1, 1, depthTexture.Sample(defaultSampler, uv + dv + du).x);

	float3 dmin = dtl;
	if (ZCMP_GT(dmin.z, dtc.z)) dmin = dtc;
	if (ZCMP_GT(dmin.z, dtr.z)) dmin = dtr;

	if (ZCMP_GT(dmin.z, dml.z)) dmin = dml;
	if (ZCMP_GT(dmin.z, dmc.z)) dmin = dmc;
	if (ZCMP_GT(dmin.z, dmr.z)) dmin = dmr;

	if (ZCMP_GT(dmin.z, dbl.z)) dmin = dbl;
	if (ZCMP_GT(dmin.z, dbc.z)) dmin = dbc;
	if (ZCMP_GT(dmin.z, dbr.z)) dmin = dbr;

	return float3(uv + dd.xy * dmin.xy, dmin.z);
}

float4 clip_aabb(float3 aabb_min, float3 aabb_max, float4 p, float4 q)
{
	// note: only clips towards aabb center (but fast!)
	float3 p_clip = 0.5 * (aabb_max + aabb_min);
	float3 e_clip = 0.5 * (aabb_max - aabb_min) + FLT_EPS;

	float4 v_clip = q - float4(p_clip, p.w);
	float3 v_unit = v_clip.xyz / e_clip;
	float3 a_unit = abs(v_unit);
	float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

	if (ma_unit > 1.0)
		return float4(p_clip, p.w) + v_clip / ma_unit;
	else
		return q;// point inside aabb
}

float4 temporal_reprojection(Texture2D<float4> mainTex, Texture2D<float4> prevTex, 
    float2 mainTexTexelSize, float feedbackMin, float feedbackMax, float2 ss_txc, float2 ss_vel, float vs_dist)
{
	// read texels
	float4 texel0 = mainTex.Sample(defaultSampler, ss_txc);
	float4 texel1 = prevTex.Sample(defaultSampler, ss_txc - ss_vel);

	// calc min-max of current neighbourhood
	float2 uv = ss_txc;

	float2 du = float2(mainTexTexelSize.x, 0.0);
	float2 dv = float2(0.0, mainTexTexelSize.y);

	float4 ctl = mainTex.Sample(defaultSampler, uv - dv - du);
	float4 ctc = mainTex.Sample(defaultSampler, uv - dv);
	float4 ctr = mainTex.Sample(defaultSampler, uv - dv + du);
	float4 cml = mainTex.Sample(defaultSampler, uv - du);
	float4 cmc = mainTex.Sample(defaultSampler, uv);
	float4 cmr = mainTex.Sample(defaultSampler, uv + du);
	float4 cbl = mainTex.Sample(defaultSampler, uv + dv - du);
	float4 cbc = mainTex.Sample(defaultSampler, uv + dv);
	float4 cbr = mainTex.Sample(defaultSampler, uv + dv + du);

	float4 cmin = min(ctl, min(ctc, min(ctr, min(cml, min(cmc, min(cmr, min(cbl, min(cbc, cbr))))))));
	float4 cmax = max(ctl, max(ctc, max(ctr, max(cml, max(cmc, max(cmr, max(cbl, max(cbc, cbr))))))));

    float4 cavg = (ctl + ctc + ctr + cml + cmc + cmr + cbl + cbc + cbr) / 9.0;

	// clamp to neighbourhood of current sample
	texel1 = clip_aabb(cmin.xyz, cmax.xyz, clamp(cavg, cmin, cmax), texel1);

	// feedback weight from unbiased luminance diff (t.lottes)
	float lum0 = Luminance(texel0.rgb);
	float lum1 = Luminance(texel1.rgb);

	float unbiased_diff = abs(lum0 - lum1) / max(lum0, max(lum1, 0.2));
	float unbiased_weight = 1.0 - unbiased_diff;
	float unbiased_weight_sqr = unbiased_weight * unbiased_weight;
	float k_feedback = lerp(feedbackMin, feedbackMax, unbiased_weight_sqr);

	// output
	return lerp(texel0, texel1, k_feedback);
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    ConstantBuffer<FrameUniforms> mFrameUniforms = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float4> mainColorBuffer = ResourceDescriptorHeap[mainColorBufferIndex];
    Texture2D<float3> motionVectorBuffer = ResourceDescriptorHeap[motionVectorBufferIndex];
    Texture2D<float> depthBuffer = ResourceDescriptorHeap[depthBufferIndex];
    Texture2D<float4> reprojectionBufferRead = ResourceDescriptorHeap[reprojectionBufferReadIndex];
    Texture2D<float4> reprojectionBufferWrite = ResourceDescriptorHeap[reprojectionBufferWriteIndex];

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

    float2 uv = DTid.xy / float2(width, height);
    float2 depthTexelSize = 1.0f / float2(width, height);

        //--- 3x3 nearest (good)
    float3 c_frag = find_closest_fragment_3x3(depthBuffer, depthTexelSize, uv);
    float2 ss_vel = motionVectorBuffer.Sample(defaultSampler, c_frag.xy).xy;
    // float vs_dist = depth_resolve_linear(c_frag.z);
    float vs_dist = depth_resolve_linear(c_frag.z, projectionMatrix._m22, projectionMatrix._m23);

    // temporal resolve
    float4 color_temporal = temporal_reprojection(mainColorBuffer, reprojectionBufferRead, depthTexelSize, feedbackMin, feedbackMax, uv, ss_vel, vs_dist);

    // prepare outputs
    float4 to_buffer = resolve_color(color_temporal);

    float4 to_screen = resolve_color(color_temporal);

    // add noise
    float4 noise4 = PDsrand4(IN.ss_txc + _SinTime.x + 0.6959174) / 510.0;
    float4 outbuffer = saturate(to_buffer + noise4);
    float4 outscreen = saturate(to_screen + noise4);

}

