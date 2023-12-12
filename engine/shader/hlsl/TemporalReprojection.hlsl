#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

#define ZCMP_GT(a, b) (a < b)

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

// https://software.intel.com/en-us/node/503873
float3 RGB_YCoCg(float3 c)
{
	// Y = R/4 + G/2 + B/4
	// Co = R/2 - B/2
	// Cg = -R/4 + G/2 - B/4
	return float3(
			c.x/4.0 + c.y/2.0 + c.z/4.0,
			c.x/2.0 - c.z/2.0,
		-c.x/4.0 + c.y/2.0 - c.z/4.0
	);
}

// https://software.intel.com/en-us/node/503873
float3 YCoCg_RGB(float3 c)
{
	// R = Y + Co - Cg
	// G = Y + Cg
	// B = Y - Co - Cg
	return saturate(float3(
		c.x + c.y - c.z,
		c.x + c.z,
		c.x - c.y - c.z
	));
}

float4 sample_color(Texture2D<float4> tex, float2 uv)
{
	float4 c = tex.Sample(defaultSampler, uv);
	return float4(RGB_YCoCg(c.rgb), c.a);
}

float4 resolve_color(float4 c)
{
	return float4(YCoCg_RGB(c.rgb).rgb, c.a);
}

//note: normalized random, float=[0;1[
float PDnrand( float2 n ) {
	return frac( sin(dot(n.xy, float2(12.9898f, 78.233f)))* 43758.5453f );
}
float2 PDnrand2( float2 n ) {
	return frac( sin(dot(n.xy, float2(12.9898f, 78.233f)))* float2(43758.5453f, 28001.8384f) );
}
float3 PDnrand3( float2 n ) {
	return frac( sin(dot(n.xy, float2(12.9898f, 78.233f)))* float3(43758.5453f, 28001.8384f, 50849.4141f ) );
}
float4 PDnrand4( float2 n ) {
	return frac( sin(dot(n.xy, float2(12.9898f, 78.233f)))* float4(43758.5453f, 28001.8384f, 50849.4141f, 12996.89f) );
}

//====
//note: signed random, float=[-1;1[
float PDsrand( float2 n ) {
	return PDnrand( n ) * 2 - 1;
}
float2 PDsrand2( float2 n ) {
	return PDnrand2( n ) * 2 - 1;
}
float3 PDsrand3( float2 n ) {
	return PDnrand3( n ) * 2 - 1;
}
float4 PDsrand4( float2 n ) {
	return PDnrand4( n ) * 2 - 1;
}

// Convert rgb to luminance
// with rgb in linear space with sRGB primaries and D65 white point
float Luminance(float3 linearRgb)
{
	return dot(linearRgb, float3(0.2126729, 0.7151522, 0.0721750));
}

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

float4 temporal_reprojection(Texture2D<float4> mainTex, Texture2D<float4> prevTex, float2 mainTexTexelSize, 
	float2 jitterUV, float feedbackMin, float feedbackMax, float2 ss_txc, float2 ss_vel, float vs_dist)
{
	// read texels
	float4 texel0 = sample_color(mainTex, ss_txc - jitterUV.xy);
	float4 texel1 = sample_color(prevTex, ss_txc - ss_vel);

	// calc min-max of current neighbourhood
	float2 uv = ss_txc/* - jitterUV.xy*/;

	float2 du = float2(mainTexTexelSize.x, 0.0);
	float2 dv = float2(0.0, mainTexTexelSize.y);

	float4 ctl = sample_color(mainTex, uv - dv - du);
	float4 ctc = sample_color(mainTex, uv - dv);
	float4 ctr = sample_color(mainTex, uv - dv + du);
	float4 cml = sample_color(mainTex, uv - du);
	float4 cmc = sample_color(mainTex, uv);
	float4 cmr = sample_color(mainTex, uv + du);
	float4 cbl = sample_color(mainTex, uv + dv - du);
	float4 cbc = sample_color(mainTex, uv + dv);
	float4 cbr = sample_color(mainTex, uv + dv + du);

	float4 cmin = min(ctl, min(ctc, min(ctr, min(cml, min(cmc, min(cmr, min(cbl, min(cbc, cbr))))))));
	float4 cmax = max(ctl, max(ctc, max(ctr, max(cml, max(cmc, max(cmr, max(cbl, max(cbc, cbr))))))));

    float4 cavg = (ctl + ctc + ctr + cml + cmc + cmr + cbl + cbc + cbr) / 9.0;

	float4 cmin5 = min(ctc, min(cml, min(cmc, min(cmr, cbc))));
	float4 cmax5 = max(ctc, max(cml, max(cmc, max(cmr, cbc))));
	float4 cavg5 = (ctc + cml + cmc + cmr + cbc) / 5.0;
	cmin = 0.5 * (cmin + cmin5);
	cmax = 0.5 * (cmax + cmax5);
	cavg = 0.5 * (cavg + cavg5);

	// shrink chroma min-max
	float2 chroma_extent = 0.25 * 0.5 * (cmax.r - cmin.r);
	float2 chroma_center = texel0.gb;
	cmin.yz = chroma_center - chroma_extent;
	cmax.yz = chroma_center + chroma_extent;
	cavg.yz = chroma_center;

	// clamp to neighbourhood of current sample
	texel1 = clip_aabb(cmin.xyz, cmax.xyz, clamp(cavg, cmin, cmax), texel1);

	// feedback weight from unbiased luminance diff (t.lottes)
	float lum0 = texel0.r;
	float lum1 = texel1.r;
	// float lum0 = Luminance(texel0.rgb);
	// float lum1 = Luminance(texel1.rgb);

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

