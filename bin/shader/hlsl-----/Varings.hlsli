#pragma once

//------------------------------------------------------------------------------
// Varyings
//------------------------------------------------------------------------------

struct VaringStruct
{
    float4 vertex_worldPosition;

#if defined(HAS_ATTRIBUTE_TANGENTS)
    float3 vertex_worldNormal;
#if defined(MATERIAL_NEEDS_TBN)
    float4 vertex_worldTangent;
#endif
#endif

    float4 vertex_position;

    int instance_index;

#if defined(HAS_ATTRIBUTE_COLOR)
    float4 vertex_color;
#endif

#if defined(HAS_ATTRIBUTE_UV0) && !defined(HAS_ATTRIBUTE_UV1)
    float2 vertex_uv01;
#elif defined(HAS_ATTRIBUTE_UV1)
    float4 vertex_uv01;
#endif

#if defined(VARIANT_HAS_SHADOWING) && defined(VARIANT_HAS_DIRECTIONAL_LIGHTING)
    float4 vertex_lightSpacePosition;
#endif
};

// Note that fragColor is an output and is not declared here; see main.fs and depth_main.fs
