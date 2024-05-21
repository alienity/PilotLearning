#include "d3d12.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"
#include "ShadingParameters.hlsli"
#include "Quaternion.hlsli"
#include "Packing.hlsli"

cbuffer RootConstants : register(b0, space0)
{
    uint perFrameBufferIndex;
    uint albedoIndex;
    uint worldNormalIndex;
    uint ambientOcclusionIndex;
    uint metallic_Roughness_Reflectance_AOIndex;
    uint clearCoat_ClearCoatRoughness_AnisotropyIndex;
    uint depthIndex;
    uint ssrResolveIndex;
    //uint volumeLight3DIndex;

    uint outSSSDiffuseLightingIndex;
    uint outColorIndex;
};

SamplerState defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

void InflateMaterialInputs(float2 uv, uint2 viewCoord, inout MaterialInputs materialInputs)
{
    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[albedoIndex];
    Texture2D<float4> worldNormalTexture = ResourceDescriptorHeap[worldNormalIndex];
    Texture2D<uint> ambientCollusionTexture = ResourceDescriptorHeap[ambientOcclusionIndex];
    Texture2D<float4> metallic_Roughness_Reflectance_AO_Texture = ResourceDescriptorHeap[metallic_Roughness_Reflectance_AOIndex];
    Texture2D<float4> clearCoat_ClearCoatRoughness_Anisotropy_Texture = ResourceDescriptorHeap[clearCoat_ClearCoatRoughness_AnisotropyIndex];
    Texture2D<float4> ssrResolveTexture = ResourceDescriptorHeap[ssrResolveIndex];
    
    float4 mrra = metallic_Roughness_Reflectance_AO_Texture.Sample(defaultSampler, uv).rgba;
    float runtimeAO = saturate((ambientCollusionTexture.Load(int3(viewCoord.xy, 0)).x - 0.5f) / 255.0f);
    float3 cra = clearCoat_ClearCoatRoughness_Anisotropy_Texture.Sample(defaultSampler, uv).rgb;
    
    materialInputs.baseColor = albedoTexture.Sample(defaultSampler, uv).rgba;
    materialInputs.metallic = mrra.r;
    materialInputs.roughness = mrra.g;
    materialInputs.reflectance = mrra.b;
    materialInputs.ambientOcclusion = mrra.a * runtimeAO;
    materialInputs.emissive = float4(0, 0, 0, 0);
    materialInputs.reflection = ssrResolveTexture.Sample(defaultSampler, uv).rgba;
    materialInputs.clearCoat = cra.r;
    materialInputs.clearCoatRoughness = cra.g;
    materialInputs.anisotropy = cra.b;
    materialInputs.normal = worldNormalTexture.Sample(defaultSampler, uv).rgb * 2.0f - 1.0f;
}

void InflateCommonShadingStruct(float2 uv, float depth, const FrameUniforms frameUniform, MaterialInputs materialInputs, inout CommonShadingStruct commonShadingStruct)
{
    float4x4 worldFromClipMatrix = frameUniform.cameraUniform.curFrameUniform.worldFromClipMatrix;
    float4x4 projectionMatrix = frameUniform.cameraUniform.curFrameUniform.clipFromViewMatrix;
    float4x4 worldFromViewMatrix = frameUniform.cameraUniform.curFrameUniform.worldFromViewMatrix;
    float4x4 worldFromViewMatrixTranspose = transpose(worldFromViewMatrix);
    float4 zBufferParams = frameUniform.cameraUniform.curFrameUniform.zBufferParams;
    
    float3 worldPos = reconstructWorldPosFromDepth(uv, depth, worldFromClipMatrix);
    
    float3 sv = select(isPerspectiveProjection(projectionMatrix),
        (worldFromViewMatrixTranspose[3].xyz - worldPos),
        worldFromViewMatrixTranspose[2].xyz); // ortho camera backward dir
    commonShadingStruct.shading_view = normalize(sv);
    
    commonShadingStruct.shading_position = worldPos;
    commonShadingStruct.shading_normal = normalize(materialInputs.normal);
    commonShadingStruct.shading_reflected = reflect(-commonShadingStruct.shading_view, commonShadingStruct.shading_normal);
    commonShadingStruct.shading_NoV = clampNoV(dot(commonShadingStruct.shading_normal, commonShadingStruct.shading_view));
    commonShadingStruct.shading_bentNormal = normalize(materialInputs.bentNormal);
    commonShadingStruct.shading_clearCoatNormal = normalize(materialInputs.clearCoatNormal);
    commonShadingStruct.shading_normalizedViewportCoord = uv;
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    ConstantBuffer<FrameUniforms> frameUniform = ResourceDescriptorHeap[perFrameBufferIndex];
    Texture2D<float4> depthTexture = ResourceDescriptorHeap[depthIndex];
    //Texture3D<float4> volumeLight3DTexture = ResourceDescriptorHeap[volumeLight3DIndex];
    
    RWTexture2D<float4> outSSSDiffuseLighting = ResourceDescriptorHeap[outSSSDiffuseLightingIndex];
    RWTexture2D<float4> outColorTexture = ResourceDescriptorHeap[outColorIndex];

    uint width, height;
    outColorTexture.GetDimensions(width, height);

    float2 uv = float2(DTid.x + 0.5f, DTid.y + 0.5f)/float2(width, height);

    MaterialInputs materialInputs;
    initMaterial(materialInputs);
    InflateMaterialInputs(uv, DTid.xy, materialInputs);

    float depth = depthTexture.Sample(defaultSampler, uv).r;
    
    CommonShadingStruct commonShadingStruct;
    InflateCommonShadingStruct(uv, depth, frameUniform, materialInputs, commonShadingStruct);
    
    //float4 volumeLightVal = evaluateVolumeDepth(frameUniform, volumeLight3DTexture, defaultSampler, uv.xy, depth);
    
    SamplerStruct samplerStruct;
    samplerStruct.defSampler = defaultSampler;
    samplerStruct.sdSampler = shadowmapSampler;

    //float4 fragColor = evaluateMaterial(frameUniform, commonShadingStruct, materialInputs, samplerStruct);    
    //fragColor.rgb = lerp(fragColor.rgb, volumeLightVal.rgb, 1.0f-volumeLightVal.a);
    //outColorTexture[DTid.xy] = fragColor;
    
    LightLoopOutput lightloopOut = LightLoop(frameUniform, commonShadingStruct, materialInputs, samplerStruct);

    outSSSDiffuseLighting[DTid.xy] = float4(lightloopOut.diffuseLighting, 1);
    outColorTexture[DTid.xy] = float4(lightloopOut.specularLighting, 1);
}
