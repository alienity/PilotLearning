#include "SSAO.hlsli"
#include "InputTypes.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint normalIndex;
    uint depthIndex;
    uint ssaoIndex;
};
ConstantBuffer<FrameUniforms> g_FrameUniform : register(b1, space0);

SamplerState defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

float toLinearZ(float fz, float m22, float m23)
{
    return -m23/(fz+m22);
}

// https://gamedev.stackexchange.com/questions/32681/random-number-hlsl
// https://learnopengl.com/Advanced-Lighting/SSAO
float ssao(SSAOInput ssaoInput, float2 uv, uint2 screenSize)
{
    // Parameters used in coordinate conversion
    float4x4 projMatInv = ssaoInput.frameUniforms.cameraUniform.viewFromClipMatrix;
    float4x4 projMatrix = ssaoInput.frameUniforms.cameraUniform.clipFromViewMatrix;
    float3x3 normalMat = transpose((float3x3)ssaoInput.frameUniforms.cameraUniform.worldFromViewMatrix);

    int alphaNumber = 8;
    int betaNumber = 8;
    float _Radius = 0.01f;
    // float _ZDiff = 0.0001f;
    float _Attenuation = 0.2f;

    float raw_depth_o = ssaoInput.depthTex.Sample(ssaoInput.defaultSampler, uv).r;
    float4 pos_ss = float4(uv.xy*2-1, raw_depth_o, 1.0f);
    float4 _pos_view = mul(projMatInv, pos_ss);
    float3 pos_view = _pos_view.xyz / _pos_view.w;

    float3 norm_world = ssaoInput.worldNormalMap.Sample(ssaoInput.defaultSampler, uv).rgb * 2 - 1;
    float3 norm_view = mul(normalMat, norm_world);

    float3 tangent_view = float3(1,0,0);
    float3 bitangent_view = float3(0,1,0);
    const float3 up_view = float3(0,0,1);
    if(dot(norm_view, up_view) > FLT_MIN)
    {
        tangent_view = normalize(cross(up_view, norm_view));
        bitangent_view = cross(norm_view, tangent_view);
    }
    float3x3 tbn_view = float3x3(tangent_view, bitangent_view, norm_view);

    float occlusionAmount = 0;
    for(int i = 0; i < betaNumber; i++) {
        for(int j = 0; j < alphaNumber; j++) {
            float alpha = 0.5f * F_PI * float(j) / alphaNumber;
            // float alpha = F_PI * float(j) / alphaNumber;
            float beta = 2 * F_PI * float(i) / betaNumber;
            float3 dir = float3(sin(alpha)*sin(beta), sin(alpha)*cos(beta), cos(alpha));
            float3 sampleDir = normalize(mul(tbn_view, dir)) * _Radius;
            float3 pos_view_new = pos_view + sampleDir;
            float4 _pos_ss_new = mul(projMatrix, float4(pos_view_new, 1.0f));
            float3 pos_ss_new = _pos_ss_new.xyz / _pos_ss_new.w;

            float2 uv_new = pos_ss_new.xy*0.5+0.5;
            float raw_depth_new = ssaoInput.depthTex.Sample(ssaoInput.defaultSampler, uv_new).r;

            float diff = step(raw_depth_new, pos_ss_new.z);
            occlusionAmount += diff;
        }
    }

    return occlusionAmount / (betaNumber * alphaNumber);
    
}


[numthreads( 8, 8, 1 )]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalIndex];
    Texture2D<float4> depthTexture = ResourceDescriptorHeap[depthIndex];
    RWTexture2D<float4> ssaoTexture = ResourceDescriptorHeap[ssaoIndex];

    uint width, height;
    ssaoTexture.GetDimensions(width, height);

    if(DTid.x >= width || DTid.y >= height)
        return;

    float2 uv = float2(DTid.x + 0.5f, DTid.y + 0.5f)/float2(width, height);

    SSAOInput _input;
    _input.frameUniforms = g_FrameUniform;
    _input.depthTex = depthTexture;
    _input.worldNormalMap = normalTexture;
    _input.defaultSampler = defaultSampler;

    // float3 ao__ = ssao(_input, uv, uint2(width, height));
    // ssaoTexture[DTid.xy] = float4(ao__, 1.0f);

    float ao = ssao(_input, uv, uint2(width, height));
    // ao = 1 - ao;
    float4 outAO = float4(ao, 0, 0, 1.0f);
    ssaoTexture[DTid.xy] = outAO;
    // ssaoTexture[DTid.xy] = float4(uv, 0, 1);
}
