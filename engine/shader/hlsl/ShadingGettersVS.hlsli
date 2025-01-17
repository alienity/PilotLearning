
//------------------------------------------------------------------------------
// Uniforms access
//------------------------------------------------------------------------------

float4x4 getWorldFromModelMatrix(const PerRenderableData perRenderableDdata)
{
    return perRenderableDdata.worldFromModelMatrix;
}

float3x3 getWorldFromModelNormalMatrix(const PerRenderableData perRenderableDdata)
{
    return perRenderableDdata.worldFromModelNormalMatrix;
}

float getObjectUserData(const PerRenderableData perRenderableDdata)
{
    return perRenderableDdata.userData;
}

//------------------------------------------------------------------------------
// Attributes access
//------------------------------------------------------------------------------

int getVertexIndex()
{
    return SV_VertexID;
}

#if defined(VARIANT_HAS_SKINNING_OR_MORPHING)
float3 mulBoneNormal(float3 n, uint i) {

    float3x3 cof;

    // the first 8 elements of the cofactor matrix are stored as fp16
    float2 x0y0 = unpackHalf2x16(bonesUniforms.bones[i].cof[0]);
    float2 z0x1 = unpackHalf2x16(bonesUniforms.bones[i].cof[1]);
    float2 y1z1 = unpackHalf2x16(bonesUniforms.bones[i].cof[2]);
    float2 x2y2 = unpackHalf2x16(bonesUniforms.bones[i].cof[3]);

    // the last element must be computed by hand
    float a = bonesUniforms.bones[i].transform[0][0];
    float b = bonesUniforms.bones[i].transform[0][1];
    float d = bonesUniforms.bones[i].transform[1][0];
    float e = bonesUniforms.bones[i].transform[1][1];

    cof[0].xyz = float3(x0y0, z0x1.x);
    cof[1].xyz = float3(z0x1.y, y1z1);
    cof[2].xyz = float3(x2y2, a * e - b * d);

    return normalize(cof * n);
}

float3 mulBoneVertex(float3 v, uint i) {
    // last row of bonesUniforms.transform[i] (row major) is assumed to be [0,0,0,1]
    float4x3 m = transpose(bonesUniforms.bones[i].transform);
    return v.x * m[0].xyz + (v.y * m[1].xyz + (v.z * m[2].xyz + m[3].xyz));
}

void skinNormal(inout float3 n, const float4 ids, const float4 weights) {
    n =   mulBoneNormal(n, ids.x) * weights.x
        + mulBoneNormal(n, ids.y) * weights.y
        + mulBoneNormal(n, ids.z) * weights.z
        + mulBoneNormal(n, ids.w) * weights.w;
}

void skinPosition(inout float3 p, const float4 ids, const float4 weights) {
    p =   mulBoneVertex(p, ids.x) * weights.x
        + mulBoneVertex(p, ids.y) * weights.y
        + mulBoneVertex(p, ids.z) * weights.z
        + mulBoneVertex(p, ids.w) * weights.w;
}

#define MAX_MORPH_TARGET_BUFFER_WIDTH 2048

void morphPosition(inout float4 p) {
    ifloat3 texcoord = float3(getVertexIndex() % MAX_MORPH_TARGET_BUFFER_WIDTH, getVertexIndex() / MAX_MORPH_TARGET_BUFFER_WIDTH, 0);
    uint c = object_uniforms.morphTargetCount;
    for (uint i = 0u; i < c; ++i) {
        float w = morphingUniforms.weights[i][0];
        if (w != 0.0) {
            texcoord.z = int(i);
            p += w * texelFetch(morphTargetBuffer_positions, texcoord, 0);
        }
    }
}

void morphNormal(inout float3 n) {
    float3 baseNormal = n;
    ifloat3 texcoord = ifloat3(getVertexIndex() % MAX_MORPH_TARGET_BUFFER_WIDTH, getVertexIndex() / MAX_MORPH_TARGET_BUFFER_WIDTH, 0);
    uint c = object_uniforms.morphTargetCount;
    for (uint i = 0u; i < c; ++i) {
        float w = morphingUniforms.weights[i][0];
        if (w != 0.0) {
            texcoord.z = int(i);
            ifloat4 tangent = texelFetch(morphTargetBuffer_tangents, texcoord, 0);
            float3 normal;
            toTangentFrame(float4(tangent) * (1.0 / 32767.0), normal);
            n += w * (normal - baseNormal);
        }
    }
}
#endif

float4 getPosition() {
    float4 pos = mesh_position;

#if defined(VARIANT_HAS_SKINNING_OR_MORPHING)

    if ((object_uniforms.flagsChannels & FILAMENT_OBJECT_MORPHING_ENABLED_BIT) != 0u) {
#if defined(LEGACY_MORPHING)
        pos += morphingUniforms.weights[0] * mesh_custom0;
        pos += morphingUniforms.weights[1] * mesh_custom1;
        pos += morphingUniforms.weights[2] * mesh_custom2;
        pos += morphingUniforms.weights[3] * mesh_custom3;
#else
        morphPosition(pos);
#endif
    }

    if ((object_uniforms.flagsChannels & FILAMENT_OBJECT_SKINNING_ENABLED_BIT) != 0u) {
        skinPosition(pos.xyz, mesh_bone_indices, mesh_bone_weights);
    }

#endif

    return pos;
}

#if defined(HAS_ATTRIBUTE_CUSTOM0)
float4 getCustom0() { return mesh_custom0; }
#endif
#if defined(HAS_ATTRIBUTE_CUSTOM1)
float4 getCustom1() { return mesh_custom1; }
#endif
#if defined(HAS_ATTRIBUTE_CUSTOM2)
float4 getCustom2() { return mesh_custom2; }
#endif
#if defined(HAS_ATTRIBUTE_CUSTOM3)
float4 getCustom3() { return mesh_custom3; }
#endif
#if defined(HAS_ATTRIBUTE_CUSTOM4)
float4 getCustom4() { return mesh_custom4; }
#endif
#if defined(HAS_ATTRIBUTE_CUSTOM5)
float4 getCustom5() { return mesh_custom5; }
#endif
#if defined(HAS_ATTRIBUTE_CUSTOM6)
float4 getCustom6() { return mesh_custom6; }
#endif
#if defined(HAS_ATTRIBUTE_CUSTOM7)
float4 getCustom7() { return mesh_custom7; }
#endif

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

/**
 * Computes and returns the position in world space of the current vertex.
 * The world position computation depends on the current vertex domain. This
 * function optionally applies vertex skinning if needed.
 *
 * NOTE: the "transform" and "position" temporaries are necessary to work around
 * an issue with Adreno drivers (b/110851741).
 */

float4 computeWorldPosition()
{
#if defined(VERTEX_DOMAIN_OBJECT)
    mat4 transform = getWorldFromModelMatrix();
    vec3 position  = getPosition().xyz;
    return mulMat4x4Float3(transform, position);
#elif defined(VERTEX_DOMAIN_WORLD)
    return float4(getPosition().xyz, 1.0);
#elif defined(VERTEX_DOMAIN_VIEW)
    mat4 transform = getWorldFromViewMatrix();
    vec3 position  = getPosition().xyz;
    return mulMat4x4Float3(transform, position);
#elif defined(VERTEX_DOMAIN_DEVICE)
    mat4 transform = getWorldFromClipMatrix();
    float4 p         = getPosition();
    // GL convention to inverted DX convention
    p.z           = p.z * -0.5 + 0.5;
    float4 position = transform * p;
    // w could be zero (e.g.: with the skybox) which corresponds to an infinite distance in
    // world-space. However, we want to avoid infinites and divides-by-zero, so we use a very
    // small number instead in that case (2^-63 seem to work well).
    const float ALMOST_ZERO_FLT = 1.08420217249e-19f;
    if (abs(position.w) < ALMOST_ZERO_FLT)
    {
        position.w = select(position.w < 0.0, -ALMOST_ZERO_FLT, ALMOST_ZERO_FLT);
    }
    return position * (1.0 / position.w);
#else
#error Unknown Vertex Domain
#endif
}
