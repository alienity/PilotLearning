#include "CommonMath.hlsli"

#define IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT 64

// https://google.github.io/filament/Filament.html#annex/importancesamplingfortheibl

float2 hammersley(uint i, float numSamples)
{
    uint bits = i;
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
    bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
    bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
    bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
    return float2(i / numSamples, bits / exp2(32));
}

float3 importanceSampleGGX( float2 Xi, float Roughness, float3 N )
{
    float a = Roughness * Roughness;
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a*a - 1) * Xi.y ) );
    float SinTheta = sqrt( 1 - CosTheta * CosTheta );
    float3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;
    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize( cross( UpVector, N ) );
    float3 TangentY = cross( N, TangentX );
    // Tangent to world space
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

float GDFG(float NoV, float NoL, float a)
{
    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return (2 * NoL) / (GGXV + GGXL);
}

// implementation of the LDFG term
float2 DFG(float NoV, float a)
{
    float3 V;
    V.x = sqrt(1.0f - NoV*NoV);
    V.y = 0.0f;
    V.z = NoV;

    const uint sampleCount = 1024u;

    float2 r = 0.0f;
    for (uint i = 0; i < sampleCount; i++)
    {
        float2 Xi = hammersley(i, sampleCount);
        float3 H = importanceSampleGGX(Xi, a, N);
        float3 L = 2.0f * dot(V, H) * H - V;

        float VoH = saturate(dot(V, H));
        float NoL = saturate(L.z);
        float NoH = saturate(H.z);

        if (NoL > 0.0f) {
            float G = GDFG(NoV, NoL, a);
            float Gv = G * VoH / NoH;
            float Fc = pow(1 - VoH, 5.0f);
            r.x += Gv * (1 - Fc);
            r.y += Gv * Fc;
        }
    }
    return r * (1.0f / sampleCount);
}

// pre-filter the environment, based on some input roughness that varies over each mipmap level 
// of the pre-filter cubemap (from 0.0 to 1.0), and store the result in prefilteredColor.
// 所以这里用一次Draw一个Cube来实现是最方便的，因为光栅化会自动对Cube的每个像素都进行处理
// 针对每个lod等级，都做一次绘制，每个lod等级使用的roughness不一样，
// lod 0 --- α 0
// lod 1 --- α 0.2
// lod 2 --- α 0.4
// lod 3 --- α 0.6
// lod 4 --- α 0.8


// implementation of the LD term
float4 LD(TextureCube<float4> envMap, SamplerState enMapSampler, float3 localPos, float roughness)
{
    float3 N = normalize(localPos);
    float3 R = N;
    float3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;   
    float3 prefilteredColor = float3(0.0, 0.0, 0.0);     
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = hammersley(i, SAMPLE_COUNT);
        float3 H = importanceSampleGGX(Xi, roughness, N);
        float3 L = 2.0f * dot(V, H) * H - V;

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            prefilteredColor += envMap.Sample(enMapSampler, L).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    return float4(prefilteredColor, 1.0);
}






















// 简单参考下glsl
/*
prefilterShader.use();
prefilterShader.setInt("environmentMap", 0);
prefilterShader.setMat4("projection", captureProjection);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
unsigned int maxMipLevels = 5;
for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
{
    // reisze framebuffer according to mip-level size.
    unsigned int mipWidth  = 128 * std::pow(0.5, mip);
    unsigned int mipHeight = 128 * std::pow(0.5, mip);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
    glViewport(0, 0, mipWidth, mipHeight);

    float roughness = (float)mip / (float)(maxMipLevels - 1);
    prefilterShader.setFloat("roughness", roughness);
    for (unsigned int i = 0; i < 6; ++i)
    {
        prefilterShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderCube();
    }
}
glBindFramebuffer(GL_FRAMEBUFFER, 0);   
*/

// 在d3d12中写入到指定mip中，当然如果直接用ComputeShader去做计算，就可以一次性计算出所有的mip，那我还是用cS做吧
// https://www.gamedev.net/forums/topic/688777-render-to-specific-cubemap-mip/5343021/



// 计算iradians map也可以使用上面的方法



/*
\begin{align*}
    \alpha       &= perceptualRoughness^2                        \\
    lod_{\alpha} &= \alpha^{\frac{1}{2}} = perceptualRoughness   \\
\end{align*}
*/
