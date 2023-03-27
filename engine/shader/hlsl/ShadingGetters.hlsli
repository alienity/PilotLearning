#pragma once

//------------------------------------------------------------------------------
// Uniforms access
//------------------------------------------------------------------------------

float getObjectUserData(PerRenderableData objectUniforms) { return objectUniforms.userData; }

//------------------------------------------------------------------------------
// Attributes access
//------------------------------------------------------------------------------

#if defined(HAS_ATTRIBUTE_COLOR)
float4 getColor(const VaringStruct varing) { return varing.vertex_color; }
#endif

#if defined(HAS_ATTRIBUTE_UV0)
float2 getUV0(const VaringStruct varing) { return varing.vertex_uv01.xy; }
#endif

#if defined(HAS_ATTRIBUTE_UV1)
float2 getUV1(const VaringStruct varing) { return varing.vertex_uv01.zw; }
#endif

#if defined(BLEND_MODE_MASKED)
float getMaskThreshold() { return materialParams._maskThreshold; }
#endif

float3x3 getWorldTangentFrame(const CommonShadingParams params) { return params.shading_tangentToWorld; }

float3 getWorldPosition(const CommonShadingParams params) { return params.shading_position; }

float3 getUserWorldPosition(const CommonShadingParams params, const FrameUniforms frameUniforms)
{
    return mulMat4x4Float3(getUserWorldFromWorldMatrix(frameUniforms), getWorldPosition(params)).xyz;
}

float3 getWorldViewVector(const CommonShadingParams params) { return params.shading_view; }

bool isPerspectiveProjection(const FrameUniforms frameUniforms) { return frameUniforms.clipFromViewMatrix[2].w != 0.0; }

float3 getWorldNormalVector(const CommonShadingParams params) { return params.shading_normal; }

float3 getWorldGeometricNormalVector(const CommonShadingParams params) { return params.shading_geometricNormal; }

float3 getWorldReflectedVector(const CommonShadingParams params) { return params.shading_reflected; }

float getNdotV(const CommonShadingParams params) { return params.shading_NoV; }

float3 getNormalizedPhysicalViewportCoord(const CommonShadingParams params)
{
    // make sure to handle our reversed-z
    return float3(params.shading_normalizedViewportCoord, SV_Position.z);
}

/**
 * Returns the normalized [0, 1] logical viewport coordinates with the origin at the viewport's
 * bottom-left. Z coordinate is in the [1, 0] range (reversed).
 *
 * @public-api
 */
float3 getNormalizedViewportCoord()
{
    // make sure to handle our reversed-z
    float2 scale     = frameUniforms.logicalViewportScale;
    float2 offset    = frameUniforms.logicalViewportOffset;
    float2 logicalUv = shading_normalizedViewportCoord * scale + offset;
    return float3(logicalUv, SV_Position.z);
}

#if defined(VARIANT_HAS_SHADOWING) && defined(VARIANT_HAS_DYNAMIC_LIGHTING)
float4 getSpotLightSpacePosition(const CommonShadingParams params, uint index, float3 dir, float zLight)
{
    float4x4 lightFromWorldMatrix = shadowUniforms.shadows[index].lightFromWorldMatrix;

    // for spotlights, the bias depends on z
    float bias = shadowUniforms.shadows[index].normalBias * zLight;

    return computeLightSpacePosition(getWorldPosition(params), getWorldNormalVector(params), dir, bias, lightFromWorldMatrix);
}
#endif

#if defined(MATERIAL_HAS_DOUBLE_SIDED_CAPABILITY)
bool isDoubleSided() { return materialParams._doubleSided; }
#endif

/**
 * Returns the cascade index for this fragment (between 0 and CONFIG_MAX_SHADOW_CASCADES - 1).
 */
uint getShadowCascade()
{
    float z            = mulMat4x4Float3(getViewFromWorldMatrix(), getWorldPosition()).z;
    ufloat4       greaterZ     = ufloat4(greaterThan(frameUniforms.cascadeSplits, float4(z)));
    uint        cascadeCount = frameUniforms.cascades & 0xFu;
    return clamp(greaterZ.x + greaterZ.y + greaterZ.z + greaterZ.w, 0u, cascadeCount - 1u);
}

#if defined(VARIANT_HAS_SHADOWING) && defined(VARIANT_HAS_DIRECTIONAL_LIGHTING)

float4 getCascadeLightSpacePosition(uint cascade)
{
    // For the first cascade, return the interpolated light space position.
    // This branch will be coherent (mostly) for neighboring fragments, and it's worth avoiding
    // the matrix multiply inside computeLightSpacePosition.
    if (cascade == 0u)
    {
        // Note: this branch may cause issues with derivatives
        return vertex_lightSpacePosition;
    }

    return computeLightSpacePosition(getWorldPosition(),
                                     getWorldNormalVector(),
                                     frameUniforms.lightDirection,
                                     shadowUniforms.shadows[cascade].normalBias,
                                     shadowUniforms.shadows[cascade].lightFromWorldMatrix);
}

#endif
