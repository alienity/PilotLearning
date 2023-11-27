#include "d3d12.hlsli"
#include "Shader.hlsli"
#include "CommonMath.hlsli"
#include "InputTypes.hlsli"

cbuffer RootConstants : register(b0, space0) { uint meshIndex; };
ConstantBuffer<FrameUniforms> g_FrameUniform : register(b1, space0);
StructuredBuffer<PerRenderableMeshData> g_MeshesInstance : register(t0, space0);
StructuredBuffer<PerMaterialViewIndexBuffer> g_MaterialsInstance : register(t1, space0);

SamplerState defaultSampler : register(s10);
SamplerComparisonState shadowmapSampler : register(s11);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 positionWS : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    PerRenderableMeshData mesh = g_MeshesInstance[meshIndex];

    output.positionWS = mul(mesh.worldFromModelMatrix, float4(input.position, 1.0f)).xyz;
    output.position   = mul(g_FrameUniform.cameraUniform.curFrameUniform.clipFromWorldMatrix, float4(output.positionWS, 1.0f));

    output.texcoord    = input.texcoord;

    float3x3 normalMat = transpose((float3x3)mesh.modelFromWorldMatrix);

    output.normal      = normalize(mul(normalMat, input.normal));
    output.tangent.xyz = normalize(mul(normalMat, input.tangent.xyz));
    output.tangent.w   = input.tangent.w;

    return output;
}

void getShadowCascade(
    float4x4 viewFromWorldMatrix, float3 worldPosition, uint4 shadow_bounds, uint cascadeCount, 
    out uint cascadeLevel, out float2 shadow_bound_offset) 
{
    float2 posInView = mulMat4x4Float3(viewFromWorldMatrix, worldPosition).xy;
    float x = max(abs(posInView.x), abs(posInView.y));
    uint4 greaterZ = uint4(step(shadow_bounds * 0.9f * 0.5f, float4(x, x, x, x)));
    cascadeLevel = greaterZ.x + greaterZ.y + greaterZ.z + greaterZ.w;
    shadow_bound_offset = step(1, cascadeLevel & uint2(0x1, 0x2)) * 0.5;
}

float sampleDepth(
    const Texture2D<float> shadowmap, SamplerComparisonState shadowmapSampler, 
    const float2 offseted_uv, const float depth)
{
    float tempfShadow = shadowmap.SampleCmpLevelZero(shadowmapSampler, offseted_uv, depth);
    return tempfShadow;
}

float shadowSample_PCF_Low(
    const Texture2D<float> shadowmap, SamplerComparisonState shadowmapSampler, 
    const float4x4 light_proj_view, uint2 shadowmap_size, float2 shadow_bound_offset, const float3 positionWS, const float depthBias)
{
    float4 fragPosLightSpace = mul(light_proj_view, float4(positionWS, 1.0f));

    // perform perspective divide
    float3 shadowPosition = fragPosLightSpace.xyz / fragPosLightSpace.w;

    shadowPosition.xy = shadowPosition.xy * 0.5 + 0.5;
    shadowPosition.y = 1 - shadowPosition.y;

    float2 texelSize = float2(1.0f, 1.0f) / shadowmap_size;

    //  Castaño, 2013, "Shadow Mapping Summary Part 1"
    float depth = shadowPosition.z + depthBias;

    float2 offset = float2(0.5, 0.5);
    float2 uv = (shadowPosition.xy * shadowmap_size) + offset;
    float2 base = (floor(uv) - offset) * texelSize;
    float2 st = frac(uv);

    float2 uw = float2(3.0 - 2.0 * st.x, 1.0 + 2.0 * st.x);
    float2 vw = float2(3.0 - 2.0 * st.y, 1.0 + 2.0 * st.y);

    float2 u = float2((2.0 - st.x) / uw.x - 1.0, st.x / uw.y + 1.0);
    float2 v = float2((2.0 - st.y) / vw.x - 1.0, st.y / vw.y + 1.0);

    u *= texelSize.x;
    v *= texelSize.y;

    float sum = 0.0;
    sum += uw.x * vw.x * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.x, v.x)) * 0.5 + shadow_bound_offset, depth);
    sum += uw.y * vw.x * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.y, v.x)) * 0.5 + shadow_bound_offset, depth);
    sum += uw.x * vw.y * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.x, v.y)) * 0.5 + shadow_bound_offset, depth);
    sum += uw.y * vw.y * sampleDepth(shadowmap, shadowmapSampler, (base + float2(u.y, v.y)) * 0.5 + shadow_bound_offset, depth);
    return sum * (1.0 / 16.0);
}

// use hardware assisted PCF + 3x3 gaussian filter
float shadowSample_PCF(
    const Texture2D<float> shadowmap, SamplerComparisonState shadowmapSampler, 
    const float4x4 light_view_matrix, const float4x4 light_proj_view[4], 
    const float4 shadow_bounds, uint2 shadowmap_size, uint cascadeCount, const float3 positionWS, const float ndotl)
{
    uint cascadeLevel;
    float2 shadow_bound_offset;
    getShadowCascade(light_view_matrix, positionWS, shadow_bounds, cascadeCount, cascadeLevel, shadow_bound_offset);

    if(cascadeLevel == 4)
        return 1.0f;

    float shadow_bias = max(0.0015f * exp2(cascadeLevel) * (1.0 - ndotl), 0.0003f);  

    return shadowSample_PCF_Low(shadowmap, shadowmapSampler, 
        light_proj_view[cascadeLevel], shadowmap_size, shadow_bound_offset, positionWS, shadow_bias);
}

float4 PSMain(VertexOutput input) : SV_Target0
{
    PerRenderableMeshData mesh = g_MeshesInstance[meshIndex];
    PerMaterialViewIndexBuffer material = g_MaterialsInstance[mesh.perMaterialViewIndexBufferIndex];

    StructuredBuffer<PerMaterialParametersBuffer> matParamsBuffer = ResourceDescriptorHeap[material.parametersBufferIndex];

    float4 baseColorFactor = matParamsBuffer[0].baseColorFactor;

    float2 baseColorTilling = matParamsBuffer[0].base_color_tilling;
    float2 normalTexTilling = matParamsBuffer[0].normal_tilling;

    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[material.baseColorIndex];
    Texture2D<float3> normalTex    = ResourceDescriptorHeap[material.normalIndex];

    float4 baseColor = baseColorTex.Sample(defaultSampler, input.texcoord * baseColorTilling);
    float3 vNt       = normalTex.Sample(defaultSampler, input.texcoord * baseColorTilling) * 2.0f - 1.0f;

    baseColor = baseColor * baseColorFactor;

    // http://www.mikktspace.com/
    float3 vNormal    = input.normal.xyz;
    float3 vTangent   = input.tangent.xyz;
    float3 vBiTangent = input.tangent.w * cross(vNormal, vTangent);
    float3 vNout      = normalize(vNt.x * vTangent + vNt.y * vBiTangent + vNt.z * vNormal);

    float3 positionWS = input.positionWS;
    float3 viewPos    = g_FrameUniform.cameraUniform.curFrameUniform.cameraPosition;

    float3 outColor = float3(0, 0, 0);

    // direction light
    {
        float3 lightForward   = g_FrameUniform.directionalLight.lightDirection;
        float3 lightColor     = g_FrameUniform.directionalLight.lightColorIntensity.rgb;
        float  lightStrength  = g_FrameUniform.directionalLight.lightColorIntensity.a;
        uint   shadowmap      = g_FrameUniform.directionalLight.shadowType;

        lightColor = lightColor * lightStrength;

        float3 lightDir = -lightForward;

        float3 viewDir    = normalize(viewPos - positionWS);
        float3 halfwayDir = normalize(lightDir + viewDir);

        float  diff    = max(dot(vNout, lightDir), 0.0f);
        float3 diffuse = lightColor * diff;

        float  spec     = pow(max(dot(vNout, halfwayDir), 0.0f), 32);
        float3 specular = lightColor * spec;

        float3 directionLightColor = diffuse + specular;

        if (shadowmap == 1)
        {
            float4x4 lightViewMat = g_FrameUniform.directionalLight.directionalLightShadowmap.light_view_matrix;
            float4x4 lightProjViews[4] = g_FrameUniform.directionalLight.directionalLightShadowmap.light_proj_view;
            Texture2D<float> shadowMap = ResourceDescriptorHeap[g_FrameUniform.directionalLight.directionalLightShadowmap.shadowmap_srv_index];
            float4 shadow_bounds = g_FrameUniform.directionalLight.directionalLightShadowmap.shadow_bounds;
            uint2 shadowmap_size = g_FrameUniform.directionalLight.directionalLightShadowmap.shadowmap_size;
            uint cascadeCount = g_FrameUniform.directionalLight.directionalLightShadowmap.cascadeCount;
            float ndotl = dot(vNout, lightDir);
            float fShadow = shadowSample_PCF(shadowMap, shadowmapSampler, lightViewMat, lightProjViews, shadow_bounds, shadowmap_size, cascadeCount, positionWS, ndotl);
            directionLightColor *= fShadow;
            // directionLightColor.xyz += __fShadow;

            /*
            float4x4 lightViewMat = g_FrameUniform.directionalLight.directionalLightShadowmap.light_view_matrix;

            float4 positionInLightSpace = mul(lightViewMat, float4(positionWS.xyz, 1.0f));

            float posWidthInLightSpace = max(abs(positionInLightSpace.x), abs(positionInLightSpace.y));

            float4 shadow_bounds = g_FrameUniform.directionalLight.directionalLightShadowmap.shadow_bounds * 0.9f * 0.5f;

            uint4 greaterZ = uint4(step(shadow_bounds, float4(posWidthInLightSpace, posWidthInLightSpace, posWidthInLightSpace, posWidthInLightSpace)));

            int shadow_bound_index = greaterZ.x + greaterZ.y + greaterZ.z + greaterZ.w;

            float2 shadow_bound_offset = step(1, shadow_bound_index & uint2(0x1, 0x2)) * 0.5;

            if(shadow_bound_index != 4)
            {
                float4x4 lightViewProjMat = g_FrameUniform.directionalLight.directionalLightShadowmap.light_proj_view[shadow_bound_index];

                float4 fragPosLightSpace = mul(lightViewProjMat, float4(positionWS, 1.0f));

                // perform perspective divide
                float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
                // transform to [0,1] range
                projCoords.xy = projCoords.xy * 0.5 + 0.5;

                float2 shadowmap_uv = float2(projCoords.x, 1 - projCoords.y) * 0.5 + shadow_bound_offset;

                // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
                Texture2D<float> shadowMap = ResourceDescriptorHeap[g_FrameUniform.directionalLight.directionalLightShadowmap.shadowmap_srv_index];

                //float shadow_bias = 0.0004f;
                float shadow_bias = max(0.0015f * exp2(shadow_bound_index) * (1.0 - dot(vNormal, lightDir)), 0.0003f);  

                float shadowTexelSize = 1.0f / (g_FrameUniform.directionalLight.directionalLightShadowmap.shadowmap_width * 0.5f);
                
                // PCF
                float pcfDepth = projCoords.z;

                float fShadow = 0.0f;
                for (int x = -1; x <= 1; x++)
                {
                    for (int y = -1; y <= 1; y++)
                    {
                        float2 tempShadowTexCoord = float2(shadowmap_uv.x, shadowmap_uv.y) + float2(x, y) * shadowTexelSize;
                        float tempfShadow = shadowMap.SampleCmpLevelZero(shadowmapSampler, tempShadowTexCoord, pcfDepth + shadow_bias);
                        fShadow += tempfShadow;
                    }
                }
                fShadow /= 9.0f;

                directionLightColor *= fShadow;
            }
            */


            /*
            float posInLightSpaceX = abs(positionInLightSpace.x);
            float posInLightSpaceY = abs(positionInLightSpace.y);

            float shadow_bound_0 = g_FrameUniform.directionalLight.directionalLightShadowmap.shadow_bounds.x * 0.9f * 0.5f;
            float shadow_bound_1 = g_FrameUniform.directionalLight.directionalLightShadowmap.shadow_bounds.y * 0.9f * 0.5f;
            float shadow_bound_2 = g_FrameUniform.directionalLight.directionalLightShadowmap.shadow_bounds.z * 0.9f * 0.5f;
            float shadow_bound_3 = g_FrameUniform.directionalLight.directionalLightShadowmap.shadow_bounds.w * 0.9f * 0.5f;

            int shadow_bound_index = -1;
            float2 shadow_bound_offset = float2(0, 0);

            float x = abs(mulMat4x4Float3(viewFromWorldMatrix, worldPosition).x);
            uint4 greaterZ = uint4(step(shadow_bounds, float4(x * 0.5)));
            return clamp(greaterZ.x + greaterZ.y + greaterZ.z + greaterZ.w, 0u, cascadeCount - 1u);

            if (posInLightSpaceX < shadow_bound_0 && posInLightSpaceY < shadow_bound_0)
            {
                shadow_bound_index = 0;
                shadow_bound_offset = float2(0, 0);
            }
            else if (posInLightSpaceX < shadow_bound_1 && posInLightSpaceY < shadow_bound_1)
            {
                shadow_bound_index = 1;
                shadow_bound_offset = float2(0.5, 0);
            }
            else if (posInLightSpaceX < shadow_bound_2 && posInLightSpaceY < shadow_bound_2)
            {
                shadow_bound_index = 2;
                shadow_bound_offset = float2(0, 0.5);
            }
            else if (posInLightSpaceX < shadow_bound_3 && posInLightSpaceY < shadow_bound_3)
            {
                shadow_bound_index = 3;
                shadow_bound_offset = float2(0.5, 0.5);
            }

            if (shadow_bound_index != -1)
            {
                float4x4 lightViewProjMat = g_FrameUniform.directionalLight.directionalLightShadowmap.light_proj_view[shadow_bound_index];

                float4 fragPosLightSpace = mul(lightViewProjMat, float4(positionWS, 1.0f));

                // perform perspective divide
                float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
                // transform to [0,1] range
                projCoords.xy = projCoords.xy * 0.5 + 0.5;

                float2 shadowmap_uv = float2(projCoords.x, 1 - projCoords.y) * 0.5 + shadow_bound_offset;

                // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
                Texture2D<float> shadowMap = ResourceDescriptorHeap[g_FrameUniform.directionalLight.directionalLightShadowmap.shadowmap_srv_index];

                //float shadow_bias = 0.0004f;
                float shadow_bias = max(0.0015f * exp2(shadow_bound_index) * (1.0 - dot(vNormal, lightDir)), 0.0003f);  

                float shadowTexelSize = 1.0f / (g_FrameUniform.directionalLight.directionalLightShadowmap.shadowmap_width * 0.5f);
                
                // PCF
                float pcfDepth = projCoords.z;

                float fShadow = 0.0f;
                for (int x = -1; x <= 1; x++)
                {
                    for (int y = -1; y <= 1; y++)
                    {
                        float2 tempShadowTexCoord = float2(shadowmap_uv.x, shadowmap_uv.y) + float2(x, y) * shadowTexelSize;
                        float tempfShadow = shadowMap.SampleCmpLevelZero(shadowmapSampler, tempShadowTexCoord, pcfDepth + shadow_bias);
                        fShadow += tempfShadow;
                    }
                }
                fShadow /= 9.0f;

                directionLightColor *= fShadow;
            }
            */
        }
        outColor = outColor + directionLightColor;
    }

    uint point_light_num = g_FrameUniform.pointLightUniform.pointLightCounts;
    uint i = 0;
    for (i = 0; i < point_light_num; i++)
    {
        float3 lightPos      = g_FrameUniform.pointLightUniform.pointLightStructs[i].lightPosition;
        float3 lightColor    = g_FrameUniform.pointLightUniform.pointLightStructs[i].lightIntensity.rgb;
        float  lightStrength = g_FrameUniform.pointLightUniform.pointLightStructs[i].lightIntensity.a;
        float  lightRadius   = g_FrameUniform.pointLightUniform.pointLightStructs[i].lightRadius;

        lightColor = lightColor * lightStrength;

        float lightDistance = length(lightPos - positionWS);
        float attenuation   = saturate(1.0f - lightDistance / lightRadius);
        attenuation         = attenuation * attenuation;

        float3 lightDir = normalize(lightPos - positionWS);
        float3 viewDir  = normalize(viewPos - positionWS);
        float3 halfwayDir = normalize(lightDir + viewDir);

        float  diff    = max(dot(vNout, lightDir), 0.0f);
        float3 diffuse = lightColor * diff * attenuation;

        float  spec     = pow(max(dot(vNout, halfwayDir), 0.0f), 32);
        float3 specular = lightColor * spec * attenuation;

        outColor = outColor + diffuse + specular;
    }

    uint spot_light_num = g_FrameUniform.spotLightUniform.spotLightCounts;
    for (i = 0; i < spot_light_num; i++)
    {
        float3 lightPos          = g_FrameUniform.spotLightUniform.spotLightStructs[i].lightPosition;
        float3 lightForward      = g_FrameUniform.spotLightUniform.spotLightStructs[i].lightDirection;
        float3 lightColor        = g_FrameUniform.spotLightUniform.spotLightStructs[i].lightIntensity.rgb;
        float  lightStrength     = g_FrameUniform.spotLightUniform.spotLightStructs[i].lightIntensity.a;
        float  lightRadius       = g_FrameUniform.spotLightUniform.spotLightStructs[i].lightRadius;
        float  lightInnerRadians = g_FrameUniform.spotLightUniform.spotLightStructs[i].inner_radians;
        float  lightOuterRadians = g_FrameUniform.spotLightUniform.spotLightStructs[i].outer_radians;
        uint   shadowmap         = g_FrameUniform.spotLightUniform.spotLightStructs[i].shadowType;

        float lightInnerCutoff = cos(lightInnerRadians * 0.5f);
        float lightOuterCutoff = cos(lightOuterRadians * 0.5f);

        lightColor = lightColor * lightStrength;

        float3 lightDir = normalize(lightPos - positionWS);

        float theta = dot(lightDir, normalize(-lightForward));

        if (theta < lightOuterCutoff)
            continue;

        float lightDistance = length(lightPos - positionWS);
        float attenuation   = saturate(1.0f - lightDistance / lightRadius);
        attenuation         = attenuation * attenuation;

        float epsilon = lightInnerCutoff - lightOuterCutoff;
        float intensity = saturate((theta - lightOuterCutoff) / epsilon); 

        float3 viewDir    = normalize(viewPos - positionWS);
        float3 halfwayDir = normalize(lightDir + viewDir);

        float  diff    = max(dot(vNout, lightDir), 0.0f);
        float3 diffuse = lightColor * diff * attenuation * intensity;

        float  spec     = pow(max(dot(vNout, halfwayDir), 0.0f), 32);
        float3 specular = lightColor * spec * attenuation * intensity;

        float3 spotLightColor = diffuse + specular;

        if (shadowmap == 1)
        {
            float4x4 lightViewProjMat = g_FrameUniform.spotLightUniform.spotLightStructs[i].spotLightShadowmap.light_proj_view;

            float4 fragPosLightSpace = mul(lightViewProjMat, float4(positionWS, 1.0f));

            // perform perspective divide
            float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
            // transform to [0,1] range
            projCoords.xy = projCoords.xy * 0.5 + 0.5;

            // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
            Texture2D<float> shadowMap = ResourceDescriptorHeap[g_FrameUniform.spotLightUniform.spotLightStructs[i].spotLightShadowmap.shadowmap_srv_index];

            // float shadow_bias = 0.0004f;
            float shadow_bias = max(0.001f * (1.0 - dot(vNormal, lightDir)), 0.0003f);

            float shadowTexelSize = 1.0f / g_FrameUniform.spotLightUniform.spotLightStructs[i].spotLightShadowmap.shadowmap_size.x;

            // PCF
            float pcfDepth = projCoords.z;

            float fShadow = 0.0f;
            for (int x = -1; x <= 1; x++)
            {
                for (int y = -1; y <= 1; y++)
                {
                    float2 tempShadowTexCoord = float2(projCoords.x, 1.0f - projCoords.y) + float2(x, y) * shadowTexelSize;
                    float  tempfShadow = shadowMap.SampleCmpLevelZero(shadowmapSampler, tempShadowTexCoord, pcfDepth + shadow_bias);
                    fShadow += tempfShadow;
                }
            }
            fShadow /= 9.0f;

            spotLightColor *= fShadow;
        }

        outColor = outColor + spotLightColor;
    }

    outColor = outColor * baseColor.rgb;

    return float4(outColor.rgb, baseColor.a);
}


/*
// https://devblogs.microsoft.com/directx/hlsl-shader-model-6-6/
// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
// https://devblogs.microsoft.com/directx/in-the-works-hlsl-shader-model-6-6/
float4 PSMain(VertexOutput input) : SV_Target0
{
    MeshInstance     mesh     = g_MeshesInstance[meshIndex];
    MaterialInstance material = g_MaterialsInstance[mesh.materialIndex];

    //// https://github.com/microsoft/DirectXShaderCompiler/issues/2193
    //ByteAddressBuffer matFactorsBuffer = ResourceDescriptorHeap[material.uniformBufferIndex];
    //float4 baseColorFactor = matFactorsBuffer.Load<float4>(0);

    // https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-cs-resources
    StructuredBuffer<PerMaterialUniformBuffer> matFactors = ResourceDescriptorHeap[material.uniformBufferIndex];
    float4 baseColorFactor = matFactors[0].baseColorFactor;

    Texture2D<float4> baseColorTex = ResourceDescriptorHeap[material.baseColorIndex];
    Texture2D<float3> normalTex = ResourceDescriptorHeap[material.normalIndex];

    float4 baseColor = baseColorTex.Sample(defaultSampler, input.texcoord);

    float3 normalVal = normalTex.Sample(defaultSampler, input.texcoord) * 2.0f - 1.0f;

    baseColor = baseColor * baseColorFactor;

    return float4(normalVal.rgb, 1);
}
*/