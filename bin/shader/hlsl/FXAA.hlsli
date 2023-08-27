#include "CommonMath.hlsli"
#include "Shader.hlsli"
#include "InputTypes.hlsli"
#include "d3d12.hlsli"

cbuffer Constants : register(b0)
{
    float _ContrastThreshold;
    float _RelativeThreshold;
    float _SubpixelBlending;
    uint  _BaseMapIndex;
    uint  _OutputMapIndex;
};

SamplerState defaultSampler : register(s10);

float SampleLuminance(Texture2D<float4> baseMap, SamplerState baseMapSampler, float2 uv)
{
#if defined(LUMINANCE_GREEN)
    return baseMap.Sample(baseMapSampler, uv).g;
#else
    return baseMap.Sample(baseMapSampler, uv).a;
#endif
}

float SampleLuminance(Texture2D<float4> baseMap, SamplerState baseMapSampler, float2 baseMapTexelSize, float2 uv, float uOffset, float vOffset)
{
    uv += float2(uOffset, vOffset) * baseMapTexelSize.xy;
    return SampleLuminance(baseMap, baseMapSampler, uv);
}

struct LuminanceData
{
    float m, n, e, s, w;
    float ne, nw, se, sw;
    float highest, lowest, contrast;
};

LuminanceData SampleLuminanceNeighborhood(Texture2D<float4> baseMap, SamplerState baseMapSampler, float2 baseMapTexelSize, float2 uv)
{
    LuminanceData l;
    l.m = SampleLuminance(baseMap, baseMapSampler, baseMapTexelSize, uv, 0, 0);
    l.n = SampleLuminance(baseMap, baseMapSampler, baseMapTexelSize, uv, 0, 1);
    l.e = SampleLuminance(baseMap, baseMapSampler, baseMapTexelSize, uv, 1, 0);
    l.s = SampleLuminance(baseMap, baseMapSampler, baseMapTexelSize, uv, 0, -1);
    l.w = SampleLuminance(baseMap, baseMapSampler, baseMapTexelSize, uv, -1, 0);

    l.ne = SampleLuminance(baseMap, baseMapSampler, baseMapTexelSize, uv, 1, 1);
    l.nw = SampleLuminance(baseMap, baseMapSampler, baseMapTexelSize, uv, -1, 1);
    l.se = SampleLuminance(baseMap, baseMapSampler, baseMapTexelSize, uv, 1, -1);
    l.sw = SampleLuminance(baseMap, baseMapSampler, baseMapTexelSize, uv, -1, -1);

    l.highest = max(max(max(max(l.n, l.e), l.s), l.w), l.m);
    l.lowest  = min(min(min(min(l.n, l.e), l.s), l.w), l.m);
    l.contrast = l.highest - l.lowest;
    return l;
}

bool ShouldSkipPixel(LuminanceData l)
{
    float threshold = max(_ContrastThreshold, _RelativeThreshold * l.highest);
    return l.contrast < threshold;
}

float DeterminePixelBlendFactor(LuminanceData l)
{
    float filter = 2 * (l.n + l.e + l.s + l.w);
    filter += l.ne + l.nw + l.se + l.sw;
    filter *= 1.0 / 12;
    filter = abs(filter - l.m);
    filter = saturate(filter / l.contrast);

    float blendFactor = smoothstep(0, 1, filter);
    return blendFactor * blendFactor * _SubpixelBlending;
}

struct EdgeData
{
    bool  isHorizontal;
    float pixelStep;
    float oppositeLuminance, gradient;
};

EdgeData DetermineEdge(LuminanceData l, float2 baseMapTexelSize)
{
    EdgeData e;
    float    horizontal = abs(l.n + l.s - 2 * l.m) * 2 + abs(l.ne + l.se - 2 * l.e) + abs(l.nw + l.sw - 2 * l.w);
    float    vertical   = abs(l.e + l.w - 2 * l.m) * 2 + abs(l.ne + l.nw - 2 * l.n) + abs(l.se + l.sw - 2 * l.s);
    e.isHorizontal      = horizontal >= vertical;

    float pLuminance = e.isHorizontal ? l.n : l.e;
    float nLuminance = e.isHorizontal ? l.s : l.w;
    float pGradient  = abs(pLuminance - l.m);
    float nGradient  = abs(nLuminance - l.m);

    e.pixelStep = e.isHorizontal ? baseMapTexelSize.y : baseMapTexelSize.x;

    if (pGradient < nGradient)
    {
        e.pixelStep         = -e.pixelStep;
        e.oppositeLuminance = nLuminance;
        e.gradient          = nGradient;
    }
    else
    {
        e.oppositeLuminance = pLuminance;
        e.gradient          = pGradient;
    }

    return e;
}

#if defined(LOW_QUALITY)
#define EDGE_STEP_COUNT 4
#define EDGE_STEPS 1, 1.5, 2, 4
#define EDGE_GUESS 12
#else
#define EDGE_STEP_COUNT 10
#define EDGE_STEPS 1, 1.5, 2, 2, 2, 2, 2, 2, 2, 4
#define EDGE_GUESS 8
#endif

static const float edgeSteps[EDGE_STEP_COUNT] = {EDGE_STEPS};

float DetermineEdgeBlendFactor(LuminanceData l, EdgeData e, Texture2D<float4> baseMap, SamplerState baseMapSampler, float2 baseMapTexelSize, float2 uv)
{
    float2 uvEdge = uv;
    float2 edgeStep;
    if (e.isHorizontal)
    {
        uvEdge.y += e.pixelStep * 0.5;
        edgeStep = float2(baseMapTexelSize.x - 1, 0);
    }
    else
    {
        uvEdge.x += e.pixelStep * 0.5;
        edgeStep = float2(0, baseMapTexelSize.y - 1);
    }

    float edgeLuminance     = (l.m + e.oppositeLuminance) * 0.5;
    float gradientThreshold = e.gradient * 0.25;

    float2 puv             = uvEdge + edgeStep * edgeSteps[0];
    float  pLuminanceDelta = SampleLuminance(baseMap, baseMapSampler, puv) - edgeLuminance;
    bool   pAtEnd          = abs(pLuminanceDelta) >= gradientThreshold;

    int i = 1;
    [unroll]
    for (i = 1; i < EDGE_STEP_COUNT && !pAtEnd; i++)
    {
        puv += edgeStep * edgeSteps[i];
        pLuminanceDelta = SampleLuminance(baseMap, baseMapSampler, puv) - edgeLuminance;
        pAtEnd          = abs(pLuminanceDelta) >= gradientThreshold;
    }
    if (!pAtEnd)
    {
        puv += edgeStep * EDGE_GUESS;
    }

    float2 nuv             = uvEdge - edgeStep * edgeSteps[0];
    float  nLuminanceDelta = SampleLuminance(baseMap, baseMapSampler, nuv) - edgeLuminance;
    bool   nAtEnd          = abs(nLuminanceDelta) >= gradientThreshold;

    [unroll]
    for (i = 1; i < EDGE_STEP_COUNT && !nAtEnd; i++)
    {
        nuv -= edgeStep * edgeSteps[i];
        nLuminanceDelta = SampleLuminance(baseMap, baseMapSampler, nuv) - edgeLuminance;
        nAtEnd          = abs(nLuminanceDelta) >= gradientThreshold;
    }
    if (!nAtEnd)
    {
        nuv -= edgeStep * EDGE_GUESS;
    }

    float pDistance, nDistance;
    if (e.isHorizontal)
    {
        pDistance = puv.x - uv.x;
        nDistance = uv.x - nuv.x;
    }
    else
    {
        pDistance = puv.y - uv.y;
        nDistance = uv.y - nuv.y;
    }

    float shortestDistance;
    bool  deltaSign;
    if (pDistance <= nDistance)
    {
        shortestDistance = pDistance;
        deltaSign        = pLuminanceDelta >= 0;
    }
    else
    {
        shortestDistance = nDistance;
        deltaSign        = nLuminanceDelta >= 0;
    }

    if (deltaSign == (l.m - edgeLuminance >= 0))
    {
        return 0;
    }
    return 0.5 - shortestDistance / (pDistance + nDistance);
}

float4 ApplyFXAA(Texture2D<float4> baseMap, SamplerState baseMapSampler, float2 baseMapTexelSize, float2 uv)
{
    LuminanceData l = SampleLuminanceNeighborhood(baseMap, baseMapSampler, baseMapTexelSize, uv);

    if (ShouldSkipPixel(l))
    {
        return baseMap.Sample(baseMapSampler, uv);
    }

    float    pixelBlend = DeterminePixelBlendFactor(l);
    EdgeData e          = DetermineEdge(l, baseMapTexelSize);
    float    edgeBlend  = DetermineEdgeBlendFactor(l, e, baseMap, baseMapSampler, baseMapTexelSize, uv);
    float    finalBlend = max(pixelBlend, edgeBlend);

    if (e.isHorizontal)
    {
        uv.y += e.pixelStep * finalBlend;
    }
    else
    {
        uv.x += e.pixelStep * finalBlend;
    }
    return float4(baseMap.Sample(baseMapSampler, uv).rgb, l.m);
}


[numthreads(8, 8, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID) {

    Texture2D<float4> m_BaseMapTex = ResourceDescriptorHeap[_BaseMapIndex];
    RWTexture2D<float4> m_OutputMapTex = ResourceDescriptorHeap[_OutputMapIndex];

    uint width, height;
    m_BaseMapTex.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    float2 baseMapTexelSize = 1.0f / float2(width, height);
    float2 uv = DTid.xy * baseMapTexelSize;

    m_OutputMapTex[DTid.xy] = ApplyFXAA(m_BaseMapTex, defaultSampler, baseMapTexelSize, uv);
}
