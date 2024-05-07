// Volumetric clouds shader inspired by https://www.guerrilla-games.com/read/the-real-time-volumetric-cloudscapes-of-horizon-zero-dawn
// Also inspired by https://github.com/fede-vaccaro/TerrainEngine-OpenGL
// https://thebookofshaders.com/13/?lan=ch

#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "VolumetricCloudDefinitions.hlsl"

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint weatherTexIndex;
    uint cloudTexIndex; // 3d
    uint worlyTexIndex; // 3d
    uint cloudConstBufferIndex; // constant buffer
    uint outShadowIndex;
    
    uint shadowWidth;
    uint shadowHeight;
};

struct CloudTextures
{
    Texture2D<float3> weatherTexure;
    Texture3D<float4> cloudTexure;
    Texture3D<float4> worleyTexure;
};

struct CloudSamplers
{
    SamplerState colorSampler;
    SamplerState simpleSampler;
};

SamplerState colorSampler : register(s10);
SamplerState simpleSampler : register(s11);

static const float BAYER_FACTOR = 1.0f / 16.0f;
static const float BAYER_FILTER[16] =
{
    0.0f * BAYER_FACTOR, 8.0f * BAYER_FACTOR, 2.0f * BAYER_FACTOR, 10.0f * BAYER_FACTOR,
	12.0f * BAYER_FACTOR, 4.0f * BAYER_FACTOR, 14.0f * BAYER_FACTOR, 6.0f * BAYER_FACTOR,
	3.0f * BAYER_FACTOR, 11.0f * BAYER_FACTOR, 1.0f * BAYER_FACTOR, 9.0f * BAYER_FACTOR,
	15.0f * BAYER_FACTOR, 7.0f * BAYER_FACTOR, 13.0f * BAYER_FACTOR, 5.0f * BAYER_FACTOR
};

// Cone sampling random offsets (for light)
static float3 NOISE_KERNEL_CONE_SAMPLING[6] =
{
    float3(0.38051305, 0.92453449, -0.02111345),
	float3(-0.50625799, -0.03590792, -0.86163418),
	float3(-0.32509218, -0.94557439, 0.01428793),
	float3(0.09026238, -0.27376545, 0.95755165),
	float3(0.28128598, 0.42443639, -0.86065785),
	float3(-0.16852403, 0.14748697, 0.97460106)
};

// Cloud types height density gradients
#define STRATUS_GRADIENT float4(0.0f, 0.1f, 0.2f, 0.3f)
#define STRATOCUMULUS_GRADIENT float4(0.02f, 0.2f, 0.48f, 0.625f)
#define CUMULUS_GRADIENT float4(0.0f, 0.1625f, 0.88f, 0.98f)

float3 toClipSpaceCoord(float2 tex)
{
    float2 ray;
    ray.x = 2.0 * tex.x - 1.0;
    ray.y = 1.0 - tex.y * 2.0;
    
    return float3(ray, 1.0);
}

float BeerLaw(float d)
{
    return exp(-d);
}

float SugarPowder(float d)
{
    return (1.0f - exp(-2.0f * d));
}

float HenyeyGreenstein(float sundotrd, float g)
{
    float gg = g * g;
    return (1.0f - gg) / pow(1.0f + gg - 2.0f * g * sundotrd, 1.5f);
}

bool RaySphereIntersection(float3 rayDir, float radius, out float3 posHit)
{
    float t;
    float3 center = float3(0.0, 0.0, 0.0);
    float radius2 = radius * radius;

    float3 L = -center;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, L);
    float c = dot(L, L) - radius2;

    float discr = b * b - 4.0 * a * c;
    t = max(0.0, (-b + sqrt(discr)) / 2);

    posHit = rayDir * t;

    return true;
}

bool RaySphereIntersectionFromOriginPoint(float3 planetCenter, float3 rayOrigin, float3 rayDir, float radius, out float3 posHit)
{
    float t;
    float radius2 = radius * radius;

    float3 L = rayOrigin - planetCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, L);
    float c = dot(L, L) - radius2;

    float discr = b * b - 4.0 * a * c;
    if (discr < 0.0)
        return false;
    t = max(0.0, (-b + sqrt(discr)) / 2);
    if (t == 0.0)
    {
        return false;
    }
    posHit = rayOrigin + rayDir * t;

    return true;
}

float GetHeightFraction(CloudsConstants cloudsConstants, float3 inPos)
{
    float innerRadius = cloudsConstants.PlanetRadius + cloudsConstants.CloudsBottomHeight;
    float outerRadius = innerRadius + cloudsConstants.CloudsTopHeight;
    return (length(inPos.y + cloudsConstants.PlanetRadius) - innerRadius) / (outerRadius - innerRadius);
}

float Remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax)
{
    return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

float2 GetUVProjection(CloudsConstants cloudsConstants, float3 p)
{
    float innerRadius = cloudsConstants.PlanetRadius + cloudsConstants.CloudsBottomHeight;
    return p.xz / innerRadius + 0.5f;
}

float GetDensityForCloud(float heightFraction, float cloudType)
{
    float stratusFactor = 1.0 - clamp(cloudType * 2.0, 0.0, 1.0);
    float stratoCumulusFactor = 1.0 - abs(cloudType - 0.5) * 2.0;
    float cumulusFactor = clamp(cloudType - 0.5, 0.0, 1.0) * 2.0;

    float4 baseGradient = stratusFactor * STRATUS_GRADIENT + stratoCumulusFactor * STRATOCUMULUS_GRADIENT + cumulusFactor * CUMULUS_GRADIENT;
    // gradicent computation (see Siggraph 2017 Nubis-Decima talk)
    //return remap(heightFraction, baseGradient.x, baseGradient.y, 0.0, 1.0) * remap(heightFraction, baseGradient.z, baseGradient.w, 1.0, 0.0);
    return smoothstep(baseGradient.x, baseGradient.y, heightFraction) - smoothstep(baseGradient.z, baseGradient.w, heightFraction);
}

float SampleCloudDensity(
    CloudsConstants cloudsCons, CloudTextures cloudTexs, CloudSamplers cloudSam, float3 p, bool useHighFreq, float lod)
{
    float heightFraction = GetHeightFraction(cloudsCons, p);
    float3 scroll = cloudsCons.WindDir.xyz * (heightFraction * 750.0f + cloudsCons.Time * cloudsCons.WindSpeed);
    
    float2 UV = GetUVProjection(cloudsCons, p);
    float2 dynamicUV = GetUVProjection(cloudsCons, p + scroll);

    if (heightFraction < 0.0f || heightFraction > 1.0f)
        return 0.0f;

    // low frequency sample
    float4 lowFreqNoise =
        cloudTexs.cloudTexure.SampleLevel(cloudSam.colorSampler, float3(UV * cloudsCons.Crispiness, heightFraction), lod);
    float lowFreqFBM = dot(lowFreqNoise.gba, float3(0.625, 0.25, 0.125));
    float cloudSample = Remap(lowFreqNoise.r, -(1.0f - lowFreqFBM), 1.0f, 0.0f, 1.0f);
	 
    float density = GetDensityForCloud(heightFraction, 1.0f);
    cloudSample *= (density / heightFraction);

    float3 weatherNoise = cloudTexs.weatherTexure.SampleLevel(cloudSam.colorSampler, dynamicUV, 0).rgb;
    float cloudWeatherCoverage = weatherNoise.r * cloudsCons.Coverage;
    float cloudSampleWithCoverage = Remap(cloudSample, cloudWeatherCoverage, 1.0f, 0.0f, 1.0f);
    cloudSampleWithCoverage *= cloudWeatherCoverage;

    // high frequency sample
    if (useHighFreq)
    {
        float3 highFreqNoise =
            cloudTexs.worleyTexure.SampleLevel(cloudSam.colorSampler,
                float3(dynamicUV * cloudsCons.Crispiness, heightFraction) * cloudsCons.Curliness, lod).rgb;
        float highFreqFBM = dot(highFreqNoise.rgb, float3(0.625, 0.25, 0.125));
        float highFreqNoiseModifier = lerp(highFreqFBM, 1.0f - highFreqFBM, clamp(heightFraction * 10.0f, 0.0f, 1.0f));
        cloudSampleWithCoverage = cloudSampleWithCoverage - highFreqNoiseModifier * (1.0 - cloudSampleWithCoverage);
        cloudSampleWithCoverage = Remap(cloudSampleWithCoverage * 2.0, highFreqNoiseModifier * 0.2, 1.0f, 0.0f, 1.0f);
    }

    return clamp(cloudSampleWithCoverage, 0.0f, 1.0f);
}

float RaymarchToCloudTransmittance(
    CloudsConstants cloudsCons, CloudTextures cloudTexs, CloudSamplers cloudSam,
    float2 texCoord, float3 startPos, float3 endPos, float2 outputDim)
{
    const float minTransmittance = 0.1f;
    const int steps = 64;
    
    float3 path = endPos - startPos;
    float len = length(path);
	
    float deltaStep = len / (float) steps;
    float3 dir = path / len;
    dir *= deltaStep;
    
    float2 fragCoord = texCoord * outputDim / cloudsCons.UpsampleRatio;
    int a = int(fragCoord.x) % 4;
    int b = int(fragCoord.y) % 4;
    startPos += dir * BAYER_FILTER[a * 4 + b];

    float3 pos = startPos;
    
    float finalTransmittance = 1.0f;
    float sigmaDeltaStep = -deltaStep * cloudsCons.DensityFactor;
    
    for (int i = 0; i < steps; ++i)
    {
        float densitySample = SampleCloudDensity(cloudsCons, cloudTexs, cloudSam, pos, true, i / 16.0f);
        if (densitySample > 0.0f)
        {
            float deltaTransmittance = exp(densitySample * sigmaDeltaStep);
            finalTransmittance *= deltaTransmittance;
        }

        if (finalTransmittance <= minTransmittance)
            break;
        
        pos += dir;
    }
    return finalTransmittance;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<CloudsConstantsCB> cloudConsCB = ResourceDescriptorHeap[cloudConstBufferIndex];
    Texture2D<float3> weatherTex2D = ResourceDescriptorHeap[weatherTexIndex];
    Texture3D<float4> cloudTex3D = ResourceDescriptorHeap[cloudTexIndex];
    Texture3D<float4> worlyTex3D = ResourceDescriptorHeap[worlyTexIndex];
    RWTexture2D<float> outShadowTex = ResourceDescriptorHeap[outShadowIndex];
    
    CloudsConstants cloudCons = cloudConsCB.cloudConstants;
    
    uint width = shadowWidth;
    uint height = shadowHeight;
    
    float2 texCoord = float2(DTid.xy + 0.5.xx) / float2(width / cloudCons.UpsampleRatio.x, height / cloudCons.UpsampleRatio.y);
    
    float3 lightDir = -frameUniform.directionalLight.lightDirection;
    float3 lightPosition = float3(0, cloudCons.PlanetRadius * 1000, 0);
    
    float innerRadius = cloudCons.PlanetRadius + cloudCons.CloudsBottomHeight;
    float outerRadius = innerRadius + cloudCons.CloudsTopHeight;
    
    float3 planetCenter = float3(0, -cloudCons.PlanetRadius, 0);
    
    float3 startPos, endPos;
    
    RaySphereIntersectionFromOriginPoint(planetCenter, lightPosition.xyz, lightDir, outerRadius, startPos);
    RaySphereIntersectionFromOriginPoint(planetCenter, lightPosition.xyz, lightDir, innerRadius, endPos);
    
    CloudTextures cloudTex;
    cloudTex.weatherTexure = weatherTex2D;
    cloudTex.cloudTexure = cloudTex3D;
    cloudTex.worleyTexure = worlyTex3D;

    CloudSamplers cloudSamper;
    cloudSamper.colorSampler = colorSampler;
    cloudSamper.simpleSampler = simpleSampler;
    
    float transmittance = RaymarchToCloudTransmittance(
        cloudCons, cloudTex, cloudSamper, texCoord, startPos, endPos, float2(width, height));
    
    outShadowTex[DTid.xy] = transmittance;
}
