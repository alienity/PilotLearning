#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ShadingParameters.hlsli"
#include "Quaternion.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint albedoIndex;
    uint worldNormalIndex;
    uint worldTangentIndex;
    uint materialNormalIndex;
    uint emissiveIndex;
    uint ambientOcclusionIndex;
    uint metallic_Roughness_Reflectance_AOIndex;
    uint clearCoat_ClearCoatRoughness_AnisotropyIndex;
    uint depthIndex;
    uint outColorIndex;
};
ConstantBuffer<FrameUniforms> g_FrameUniform : register(b1, space0);

SamplerState defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

[numthreads( 8, 8, 1 )]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[albedoIndex];
    Texture2D<float4> worldNormalTexture = ResourceDescriptorHeap[worldNormalIndex];
    Texture2D<float4> worldTangentTexture = ResourceDescriptorHeap[worldTangentIndex];
    Texture2D<float4> materialNormalTexture = ResourceDescriptorHeap[materialNormalIndex];
    Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[emissiveIndex];
    Texture2D<float4> ambientCollusionTexture = ResourceDescriptorHeap[ambientOcclusionIndex];
    Texture2D<float4> metallic_Roughness_Reflectance_AO_Texture = ResourceDescriptorHeap[metallic_Roughness_Reflectance_AOIndex];
    Texture2D<float4> clearCoat_ClearCoatRoughness_Anisotropy_Texture = ResourceDescriptorHeap[clearCoat_ClearCoatRoughness_AnisotropyIndex];
    Texture2D<float4> depthTexture = ResourceDescriptorHeap[depthIndex];
    RWTexture2D<float4> outColorTexture = ResourceDescriptorHeap[outColorIndex];

    uint width, height;
    outColorTexture.GetDimensions(width, height);

    float2 uv = float2(DTid.x + 0.5f, DTid.y + 0.5f)/float2(width, height);

    MaterialInputs materialInputs;
    initMaterial(materialInputs);

    {
        materialInputs.baseColor = albedoTexture.Sample(defaultSampler, uv).rgba;
        float4 mrra = metallic_Roughness_Reflectance_AO_Texture.Sample(defaultSampler, uv).rgba;
        materialInputs.metallic = mrra.r;
        materialInputs.roughness = mrra.g;
        materialInputs.reflectance = mrra.b;
        materialInputs.ambientOcclusion = mrra.a;
        materialInputs.ambientOcclusion *= ambientCollusionTexture.Sample(defaultSampler, uv).r;
        materialInputs.emissive = emissiveTexture.Sample(defaultSampler, uv).rgba;
        float3 cra = clearCoat_ClearCoatRoughness_Anisotropy_Texture.Sample(defaultSampler, uv).rgb;
        materialInputs.clearCoat = cra.r;
        materialInputs.clearCoatRoughness = cra.g;
        materialInputs.anisotropy = cra.b;
        materialInputs.normal = materialNormalTexture.Sample(defaultSampler, uv).rgb * 2.0f - 1.0f;
    }

    CommonShadingStruct commonShadingStruct;
    // computeShadingParams(g_FrameUniform, varingStruct, commonShadingStruct);
    {
        // http://www.mikktspace.com/
        float3 n = worldNormalTexture.Sample(defaultSampler, uv).rgb * 2.0f - 1.0f;
        float4 vertex_worldTangent = worldTangentTexture.Sample(defaultSampler, uv).xyzw * 2.0f - 1.0f;
        float3 t = vertex_worldTangent.xyz;
        float3 b = cross(n, t) * vertex_worldTangent.w;

        commonShadingStruct.shading_geometricNormal = normalize(n);

        // We use unnormalized post-interpolation values, assuming mikktspace tangents
        commonShadingStruct.shading_tangentToWorld = transpose(float3x3(t, b, n));

        float depth = depthTexture.Sample(defaultSampler, uv).r;
        float3 clipPos = float3(uv.x*2.0f-1.0f, (1-uv.y)*2.0f-1.0f, depth);
        float4 vertexPos = mul(g_FrameUniform.cameraUniform.worldFromClipMatrix, float4(clipPos, 1.0f));
        commonShadingStruct.shading_position.xyz = vertexPos.xyz / vertexPos.w;

        // With perspective camera, the view vector is cast from the fragment pos to the eye position,
        // With ortho camera, however, the view vector is the same for all fragments:
        float4x4 projectionMatrix = g_FrameUniform.cameraUniform.clipFromViewMatrix;
        float4x4 worldFromViewMatrix = g_FrameUniform.cameraUniform.worldFromViewMatrix;
        float4x4 _worldFromViewMatrixTranspose = transpose(worldFromViewMatrix);
        float3 sv = select(isPerspectiveProjection(projectionMatrix), 
            (_worldFromViewMatrixTranspose[3].xyz - commonShadingStruct.shading_position), _worldFromViewMatrixTranspose[2].xyz); // ortho camera backward dir
        commonShadingStruct.shading_view = normalize(sv);
        commonShadingStruct.shading_normalizedViewportCoord = uv;

        prepareMaterial(materialInputs, commonShadingStruct);
    }

    SamplerStruct samplerStruct;
    samplerStruct.defSampler = defaultSampler;
    samplerStruct.sdSampler = shadowmapSampler;

    float4 fragColor = evaluateMaterial(g_FrameUniform, commonShadingStruct, materialInputs, samplerStruct);

    outColorTexture[DTid.xy] = fragColor;
    // outColorTexture[DTid.xy] = float4(commonShadingStruct.shading_geometricNormal.xyz, 1);
}
