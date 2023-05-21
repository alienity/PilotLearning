#pragma once

//------------------------------------------------------------------------------
// Uniforms access
//------------------------------------------------------------------------------

float getObjectUserData(PerRenderableData objectUniforms) 
{ 
    return objectUniforms.userData; 
}

//------------------------------------------------------------------------------
// Attributes access
//------------------------------------------------------------------------------

#if defined(HAS_ATTRIBUTE_COLOR)
float4 getColor(const VaringStruct varing) 
{ 
    return varing.vertex_color; 
}
#endif

#if defined(HAS_ATTRIBUTE_UV0)
float2 getUV0(const VaringStruct varing) 
{ 
    return varing.vertex_uv01.xy;
}
#endif

#if defined(HAS_ATTRIBUTE_UV1)
float2 getUV1(const VaringStruct varing) 
{ 
    return varing.vertex_uv01.zw;
}
#endif

#if defined(BLEND_MODE_MASKED)
float getMaskThreshold(MaterialParams materialParams)
{ 
    return materialParams._maskThreshold;
}
#endif

#if defined(MATERIAL_HAS_DOUBLE_SIDED_CAPABILITY)
bool isDoubleSided(MaterialParams materialParams) 
{ 
    return materialParams._doubleSided; 
}
#endif

float3x3 getWorldTangentFrame(const CommonShadingStruct params) 
{ 
    return params.shading_tangentToWorld;
}

float3 getWorldPosition(const CommonShadingStruct params)
{
    return params.shading_position;
}

float3 getUserWorldPosition(const CommonShadingStruct params, const FrameUniforms frameUniforms)
{
    return mulMat4x4Float3(getUserWorldFromWorldMatrix(frameUniforms), getWorldPosition(params)).xyz;
}

float3 getWorldViewVector(const CommonShadingStruct params) 
{ 
    return params.shading_view;
}

bool isPerspectiveProjection(const FrameUniforms frameUniforms) 
{ 
    return transpose(frameUniforms.clipFromViewMatrix)[2].w != 0.0;
}

float3 getWorldNormalVector(const CommonShadingStruct params)
{ 
    return params.shading_normal;
}

float3 getWorldGeometricNormalVector(const CommonShadingStruct params)
{ 
    return params.shading_geometricNormal;
}

float3 getWorldReflectedVector(const CommonShadingStruct params)
{
    return params.shading_reflected;
}

float getNdotV(const CommonShadingStruct params) 
{ 
    return params.shading_NoV;
}

float3 getNormalizedPhysicalViewportCoord(const CommonShadingStruct params)
{
    // make sure to handle our reversed-z
    return float3(params.shading_normalizedViewportCoord, SV_Position.z);
}

#if defined(VARIANT_HAS_SHADOWING) && defined(VARIANT_HAS_DYNAMIC_LIGHTING)
float4 getSpotLightSpacePosition(const ShadowUniforms shadowUniforms, const CommonShadingStruct params, uint index, float3 dir, float zLight)
{
    float4x4 lightFromWorldMatrix = shadowUniforms.shadows[index].lightFromWorldMatrix;

    // for spotlights, the bias depends on z
    float bias = shadowUniforms.shadows[index].normalBias * zLight;

    return computeLightSpacePosition(getWorldPosition(params), getWorldNormalVector(params), dir, bias, lightFromWorldMatrix);
}
#endif

/**
 * Returns the cascade index for this fragment (between 0 and CONFIG_MAX_SHADOW_CASCADES - 1).
 */
uint getShadowCascade(const CommonShadingStruct params, const FrameUniforms frameUniforms)
{
    float z = mulMat4x4Float3(getViewFromWorldMatrix(frameUniforms), getWorldPosition(params)).z;
    float4 greaterZ = float4(step(frameUniforms.cascadeSplits, float4(z)));
    uint cascadeCount = frameUniforms.cascades & 0xFu;
    return clamp(greaterZ.x + greaterZ.y + greaterZ.z + greaterZ.w, 0u, cascadeCount - 1u);
}

#if defined(VARIANT_HAS_SHADOWING) && defined(VARIANT_HAS_DIRECTIONAL_LIGHTING)

float4 getCascadeLightSpacePosition(const ShadowUniforms shadowUniforms, const CommonShadingStruct params, 
    const FrameUniforms frameUniforms, const VaringStruct varing, uint cascade)
{
    // For the first cascade, return the interpolated light space position.
    // This branch will be coherent (mostly) for neighboring fragments, and it's worth avoiding
    // the matrix multiply inside computeLightSpacePosition.
    if (cascade == 0u)
    {
        // Note: this branch may cause issues with derivatives
        return varing.vertex_lightSpacePosition;
    }

    return computeLightSpacePosition(getWorldPosition(params),
                                     getWorldNormalVector(params),
                                     frameUniforms.lightDirection,
                                     shadowUniforms.shadows[cascade].normalBias,
                                     shadowUniforms.shadows[cascade].lightFromWorldMatrix);
}

#endif
