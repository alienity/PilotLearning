#pragma once

struct MaterialVertexInputs
{
#ifdef HAS_ATTRIBUTE_COLOR
    float4 color;
#endif
#ifdef HAS_ATTRIBUTE_UV0
    float2 uv0;
#endif
#ifdef HAS_ATTRIBUTE_UV1
    float2 uv1;
#endif
#ifdef VARIABLE_CUSTOM0
    float4 VARIABLE_CUSTOM0;
#endif
#ifdef VARIABLE_CUSTOM1
    float4 VARIABLE_CUSTOM1;
#endif
#ifdef VARIABLE_CUSTOM2
    float4 VARIABLE_CUSTOM2;
#endif
#ifdef VARIABLE_CUSTOM3
    float4 VARIABLE_CUSTOM3;
#endif
#ifdef HAS_ATTRIBUTE_TANGENTS
    float3 worldNormal;
#endif
    float4 worldPosition;
#ifdef VERTEX_DOMAIN_DEVICE
#ifdef MATERIAL_HAS_CLIP_SPACE_TRANSFORM
    float4x4 clipSpaceTransform;
#endif // MATERIAL_HAS_CLIP_SPACE_TRANSFORM
#endif // VERTEX_DOMAIN_DEVICE
};

// Workaround for a driver bug on ARM Bifrost GPUs. Assigning a structure member
// (directly or inside an expression) to an invariant causes a driver crash.
float4 getWorldPosition(const MaterialVertexInputs material)
{
    return material.worldPosition;
}

#ifdef VERTEX_DOMAIN_DEVICE
#ifdef MATERIAL_HAS_CLIP_SPACE_TRANSFORM
float4x4 getMaterialClipSpaceTransform(const MaterialVertexInputs material) 
{ 
    return material.clipSpaceTransform; 
}
#endif // MATERIAL_HAS_CLIP_SPACE_TRANSFORM
#endif // VERTEX_DOMAIN_DEVICE

/*
void initMaterialVertex(out MaterialVertexInputs material)
{
#ifdef HAS_ATTRIBUTE_COLOR
    material.color = mesh_color;
#endif
#ifdef HAS_ATTRIBUTE_UV0
#ifdef FLIP_UV_ATTRIBUTE
    material.uv0 = float2(mesh_uv0.x, 1.0 - mesh_uv0.y);
#else
    material.uv0 = mesh_uv0;
#endif
#endif
#ifdef HAS_ATTRIBUTE_UV1
#ifdef FLIP_UV_ATTRIBUTE
    material.uv1 = float2(mesh_uv1.x, 1.0 - mesh_uv1.y);
#else
    material.uv1 = mesh_uv1;
#endif
#endif
#ifdef VARIABLE_CUSTOM0
    material.VARIABLE_CUSTOM0 = float4(0.0);
#endif
#ifdef VARIABLE_CUSTOM1
    material.VARIABLE_CUSTOM1 = float4(0.0);
#endif
#ifdef VARIABLE_CUSTOM2
    material.VARIABLE_CUSTOM2 = float4(0.0);
#endif
#ifdef VARIABLE_CUSTOM3
    material.VARIABLE_CUSTOM3 = float4(0.0);
#endif
    material.worldPosition = computeWorldPosition();
#ifdef VERTEX_DOMAIN_DEVICE
#ifdef MATERIAL_HAS_CLIP_SPACE_TRANSFORM
    material.clipSpaceTransform = float4x4(1.0);
#endif
#endif
}
*/