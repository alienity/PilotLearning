// Volumetric clouds shader inspired by https://www.guerrilla-games.com/read/the-real-time-volumetric-cloudscapes-of-horizon-zero-dawn
// Also inspired by https://github.com/fede-vaccaro/TerrainEngine-OpenGL
// https://thebookofshaders.com/13/?lan=ch

#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "VolumetricCloudDefinitions.hlsl"

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint sceneColorIndex;
    uint sceneDepthIndex;
    uint weatherTexIndex;
    uint cloudTexIndex; // 3d
    uint worlyTexIndex; // 3d
    uint cloudConstBufferIndex; // constant buffer
    uint outColorIndex;
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

float RaymarchToLight(
    CloudsConstants cloudsCons, CloudTextures cloudTexs, CloudSamplers cloudSam, 
    float3 origin, float stepSize, float3 lightDir, float originalDensity, float lightDotEye)
{
    float3 startPos = origin;
    
    float deltaStep = stepSize * 6.0f;
    float3 rayStep = lightDir * deltaStep;
    const float coneStep = 1.0f / 6.0f;
    float coneRadius = 1.0f;
    float coneDensity = 0.0;
    
    float density = 0.0;
    const float densityThreshold = 0.3f;
    
    float invDepth = 1.0 / deltaStep;
    float sigmaDeltaStep = -deltaStep * cloudsCons.Absorption;
    float3 pos;

    float finalTransmittance = 1.0;

    for (int i = 0; i < 6; i++)
    {
        pos = startPos + coneRadius * NOISE_KERNEL_CONE_SAMPLING[i] * float(i);

        float heightFraction = GetHeightFraction(cloudsCons, pos);
        if (heightFraction >= 0)
        {
            float cloudDensity = SampleCloudDensity(cloudsCons, cloudTexs, cloudSam, pos, density > densityThreshold, i / 16.0f);
            if (cloudDensity > 0.0)
            {
                float curTransmittance = exp(cloudDensity * sigmaDeltaStep);
                finalTransmittance *= curTransmittance;
                density += cloudDensity;
            }
        }
        startPos += rayStep;
        coneRadius += coneStep;
    }
    return finalTransmittance;
}

float4 RaymarchToCloud(
    CloudsConstants cloudsCons, CloudTextures cloudTexs, CloudSamplers cloudSam, 
    float3 lightDir, float3 lightColor, float3 ambientColor, float3 skyColor, 
    float2 texCoord, float3 startPos, float3 endPos, float2 outputDim, out float4 cloudPos)
{
    const float minTransmittance = 0.1f;
    const int steps = 64;
    float4 finalColor = float4(0.0, 0.0, 0.0, 0.0);
    
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
    float LdotV = dot(normalize(lightDir.rgb), normalize(dir));

    float finalTransmittance = 1.0f;
    float sigmaDeltaStep = -deltaStep * cloudsCons.DensityFactor;
    bool entered = false;
    
    for (int i = 0; i < steps; ++i)
    {
        float densitySample = SampleCloudDensity(cloudsCons, cloudTexs, cloudSam, pos, true, i / 16.0f);
        if (densitySample > 0.0f)
        {
            if (!entered)
            {
                cloudPos = float4(pos, 1.0);
                entered = true;
            }
            //float height = GetHeightFraction(cloudsCons, pos);
            float lightDensity = RaymarchToLight(cloudsCons, cloudTexs, cloudSam, pos, deltaStep * 0.1f, lightDir.rgb, densitySample, LdotV);
            float scattering = max(lerp(HenyeyGreenstein(LdotV, -0.08f), HenyeyGreenstein(LdotV, 0.08f), clamp(LdotV * 0.5f + 0.5f, 0.0f, 1.0f)), 1.0f);
            //float powderEffect = SugarPowder(densitySample);
			
            float3 S = 0.6f * (lerp(lerp(ambientColor.rgb * 1.8f, skyColor, 0.2f), scattering * lightColor.rgb, lightDensity)) * densitySample;
            float deltaTransmittance = exp(densitySample * sigmaDeltaStep);
            float3 Sint = (S - S * deltaTransmittance) * (1.0f / densitySample);
            finalColor.rgb += finalTransmittance * Sint;
            finalTransmittance *= deltaTransmittance;
        }

        if (finalTransmittance <= minTransmittance)
            break;
        
        pos += dir;
    }
    finalColor.a = 1.0 - finalTransmittance;
    return finalColor;
}

float3 ReconstructWorldPosFromDepth(float2 uv, float depth, float4x4 invProj, float4x4 invView)
{
    float ndcX = uv.x * 2 - 1;
    float ndcY = 1 - uv.y * 2; // Remember to flip y!!!
    float4 viewPos = mul(invProj, float4(ndcX, ndcY, depth, 1.0f));
    viewPos = viewPos / viewPos.w;
    return mul(invView, viewPos).xyz;
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID )
{
    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[perFrameBufferIndex];
    ConstantBuffer<CloudsConstantsCB> cloudConsCB = ResourceDescriptorHeap[cloudConstBufferIndex];
    Texture2D<float4> sceneColorTex = ResourceDescriptorHeap[sceneColorIndex];
    Texture2D<float> sceneDepthTex = ResourceDescriptorHeap[sceneDepthIndex];
    Texture2D<float3> weatherTex2D = ResourceDescriptorHeap[weatherTexIndex];
    Texture3D<float4> cloudTex3D = ResourceDescriptorHeap[cloudTexIndex];
    Texture3D<float4> worlyTex3D = ResourceDescriptorHeap[worlyTexIndex];
    RWTexture2D<float4> outColorTex = ResourceDescriptorHeap[outColorIndex];
    
    CloudsConstants cloudCons = cloudConsCB.cloudConstants;
    
    uint width, height;
    sceneColorTex.GetDimensions(width, height);
    
    float2 texCoord = float2(DTid.xy + 0.5.xx) / float2(width / cloudCons.UpsampleRatio.x, height / cloudCons.UpsampleRatio.y);
    float4 baseColor = sceneColorTex.SampleLevel(simpleSampler, texCoord, 0);
    float4 cloudsColor = float4(0.0, 0.0, 0.0, 0.0f);
    
    float4x4 InvProj = frameUniform.cameraUniform._CurFrameUniform.unJitterProjectionMatrixInv;
    float4x4 InvView = frameUniform.cameraUniform._CurFrameUniform.worldFromViewMatrix;
    float3 cameraPos = frameUniform.cameraUniform._CurFrameUniform._WorldSpaceCameraPos;
    
    float depth = sceneDepthTex.SampleLevel(simpleSampler, texCoord, 0).r;
    float3 worldPos = ReconstructWorldPosFromDepth(texCoord, depth, InvProj, InvView);
    if (depth > FLT_MIN || worldPos.y < FLT_MIN)
    {
        outColorTex[DTid.xy] = baseColor;
        return;
    }
    
	//compute ray direction
    float4 rayClipSpace = float4(toClipSpaceCoord(texCoord), 1.0);
    float4 rayView = mul(InvProj, rayClipSpace);
    rayView = float4(rayView.xy, -1.0, 0.0);
    
    float3 worldDir = mul(InvView, rayView).xyz;
    worldDir = normalize(worldDir);
    
    
    float3 startPos, endPos;
    float innerRadius = cloudCons.PlanetRadius + cloudCons.CloudsBottomHeight;
    float outerRadius = innerRadius + cloudCons.CloudsTopHeight;
    
    float3 planetCenter = float3(0, -cloudCons.PlanetRadius, 0);
    
    if (cameraPos.y < innerRadius - cloudCons.PlanetRadius)
    {
        RaySphereIntersectionFromOriginPoint(planetCenter, cameraPos.xyz, worldDir, innerRadius, startPos);
        RaySphereIntersectionFromOriginPoint(planetCenter, cameraPos.xyz, worldDir, outerRadius, endPos);
    }
    else if (cameraPos.y > innerRadius - cloudCons.PlanetRadius && cameraPos.y < outerRadius - cloudCons.PlanetRadius)
    {
        startPos = cameraPos.xyz;
        RaySphereIntersectionFromOriginPoint(planetCenter, cameraPos.xyz, worldDir, outerRadius, endPos);
    }
    else
    {
        RaySphereIntersectionFromOriginPoint(planetCenter, cameraPos.xyz, worldDir, outerRadius, startPos);
        RaySphereIntersectionFromOriginPoint(planetCenter, cameraPos.xyz, worldDir, innerRadius, endPos);
    }
    
    float3 skyColor = baseColor.rgb;
    float3 outputColor = skyColor;

    CloudTextures cloudTex;
    cloudTex.weatherTexure = weatherTex2D;
    cloudTex.cloudTexure = cloudTex3D;
    cloudTex.worleyTexure = worlyTex3D;

    CloudSamplers cloudSamper;
    cloudSamper.colorSampler = colorSampler;
    cloudSamper.simpleSampler = simpleSampler;
    
    float3 lightDir = -frameUniform.lightDataUniform.directionalLightData.forward;
    float3 lightColor = frameUniform.lightDataUniform.directionalLightData.color.rgb;
    float3 ambientColor = float3(0.1, 0.1, 0.1);
    
    float4 cloudPos;
    cloudsColor = RaymarchToCloud(
        cloudCons, cloudTex, cloudSamper, 
        lightDir, lightColor, ambientColor, skyColor, 
        texCoord, startPos, endPos, float2(width, height), cloudPos);
    cloudsColor.rgb = cloudsColor.rgb * 1.8f - 0.1f;

    baseColor.rgb = lerp(baseColor.rgb, baseColor.rgb * (1.0f - cloudsColor.a) + cloudsColor.rgb, cloudsColor.a * 1.0);

    float cloudToCameraDistance = distance(cameraPos.xyz, cloudPos.xyz);
    if (cloudToCameraDistance <= cloudCons.DistanceToFadeFrom)
    {
        outputColor = baseColor.rgb;
    }
    else if (cloudToCameraDistance > cloudCons.DistanceToFadeFrom && cloudToCameraDistance <= cloudCons.DistanceToFadeFrom + cloudCons.DistanceOfFade)
    {
        float factor = (cloudToCameraDistance - cloudCons.DistanceToFadeFrom) / cloudCons.DistanceOfFade;
        outputColor = lerp(baseColor.rgb, skyColor.rgb, factor);
    }

    outColorTex[DTid.xy] = float4(outputColor, 1.0f);
}
