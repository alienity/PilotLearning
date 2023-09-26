#pragma once
#include "rhi.h"

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#include <glm/glm.hpp>

#define ConstantBufferStruct struct alignas(alignof(float))

// datas map to hlsl
namespace HLSL
{
    static constexpr size_t MaterialLimit = 2048;
    static constexpr size_t LightLimit    = 32;
    static constexpr size_t MeshLimit     = 2048;

    static const uint32_t m_point_light_shadow_map_dimension       = 2048;
    static const uint32_t m_directional_light_shadow_map_dimension = 4096;

    static uint32_t const m_mesh_per_drawcall_max_instance_count = 64;
    static uint32_t const m_mesh_vertex_blending_max_joint_count = 1024;
    static uint32_t const m_max_point_light_count                = 16;
    static uint32_t const m_max_spot_light_count                 = 16;

    typedef glm::mat4x4 float4x4;
    typedef glm::vec4 float4;
    typedef glm::vec3 float3;
    typedef glm::vec2 float2;
    typedef glm::uvec4 uint4;
    typedef glm::uvec3 uint3;
    typedef glm::uvec2 uint2;
    typedef unsigned int uint;

    struct BoundingBox
    {
        float3 center;
        float  _Padding_Center;
        float3 extents;
        float  _Padding_Extents;
    };


    struct PerMaterialParametersBuffer
    {
        float4 baseColorFactor {1.0f, 1.0f, 1.0f, 1.0f};

        float metallicFactor {1.0f};
        float roughnessFactor {1.0f};
        float reflectanceFactor {1.0f};
        float clearCoatFactor {1.0f};
        float clearCoatRoughnessFactor {1.0f};
        float anisotropyFactor {0.0f};
        float2 _padding0;

        float2 base_color_tilling {1.0f, 1.0f};
        float2 metallic_roughness_tilling {1.0f, 1.0f};
        float2 normal_tilling {1.0f, 1.0f};
        float2 occlusion_tilling {1.0f, 1.0f};
        float2 emissive_tilling {1.0f, 1.0f};

        uint  is_blend {0};
        uint  is_double_sided {0};
    };

    struct PerMaterialViewIndexBuffer
    {
        uint parametersBufferIndex;
        uint baseColorIndex;
        uint metallicRoughnessIndex;
        uint normalIndex;
        uint occlusionIndex;
        uint emissionIndex;

        uint2 _padding_material_0;
    };

    struct PerRenderableMeshData
    {
        // 16
        float  enableVertexBlending;
        float3 _padding_enable_vertex_blending;

        // 64
        float4x4 worldFromModelMatrix;
        // 64
        float4x4 modelFromWorldMatrix;

        // 16
        D3D12_VERTEX_BUFFER_VIEW vertexBuffer;
        // 16
        D3D12_INDEX_BUFFER_VIEW indexBuffer;

        // 20
        D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;

        // 12
        float3 _padding_drawArguments;

        // 32
        BoundingBox boundingBox;

        // 16
        uint  perMaterialViewIndexBufferIndex;
        uint3 _padding_materialIndex2;
    };

    struct CameraUniform
    {
        float4x4 viewFromWorldMatrix; // clip    view <- world    : view matrix
        float4x4 worldFromViewMatrix; // clip    view -> world    : model matrix
        float4x4 clipFromViewMatrix;  // clip <- view    world    : projection matrix
        float4x4 viewFromClipMatrix;  // clip -> view    world    : inverse projection matrix
        float4x4 clipFromWorldMatrix; // clip <- view <- world
        float4x4 worldFromClipMatrix; // clip -> view -> world
        float4   clipTransform;       // [sx, sy, tx, ty] only used by VERTEX_DOMAIN_DEVICE
        float3   cameraPosition;
        float    _baseReserved0;

        float4 resolution;            // physical viewport width, height, 1/width, 1/height
        float2 logicalViewportScale;  // scale-factor to go from physical to logical viewport
        float2 logicalViewportOffset; // offset to go from physical to logical viewport

        // camera position in view space (when camera_at_origin is enabled), i.e. it's (0,0,0).
        float cameraNear;
        float cameraFar; // camera *culling* far-plane distance, always positive (projection far is at +inf)
        float exposure;
        float ev100;
    };

    struct BaseUniform
    {
        float2 clipControl;   // clip control
        float  time;          // time in seconds, with a 1-second period
        float  temporalNoise; // noise [0,1] when TAA is used, 0 otherwise
        float4 userTime;      // time(s), (double)time - (float)time, 0, 0
        float  needsAlphaChannel;
        float  lodBias; // load bias to apply to user materials
        float  refractionLodOffset;
        float  baseReserved0;
    };

    struct AOUniform
    {
        float aoSamplingQualityAndEdgeDistance; // <0: no AO, 0: bilinear, !0: bilateral edge distance
        float aoBentNormals;                    // 0: no AO bent normal, >0.0 AO bent normals
        float aoReserved0;
        float aoReserved1;
    };

    struct IBLUniform
    {
        float4 iblSH[7]; // actually float3 entries (std140 requires float4 alignment)
        float  iblLuminance;
        float  iblRoughnessOneLevel; // level for roughness == 1
        int    dfg_lut_srv_index;    // specular lut dfg
        int    ld_lut_srv_index;     // specular lut ld
        int    radians_srv_index;    // diffuse
        float3 __Reserved0;
    };

    struct SSRUniform
    {
        float4x4 ssrReprojection;
        float4x4 ssrUvFromViewMatrix;
        float    ssrThickness; // ssr thickness, in world units
        float    ssrBias;      // ssr bias, in world units
        float    ssrDistance;  // ssr world raycast distance, 0 when ssr is off
        float    ssrStride;    // ssr texel stride, >= 1.0
    };

    struct DirectionalLightShadowmap
    {
        uint     shadowmap_srv_index; // shadowmap srv in descriptorheap index
        uint     cascadeCount;        // how many cascade level, default 4
        uint2    shadowmap_size;     // shadowmap size
        uint4    shadow_bounds;       // shadow bounds for each cascade level
        float4x4 light_view_matrix;   // direction light view matrix
        float4x4 light_proj_matrix[4];
        float4x4 light_proj_view_matrix[4];
    };

    struct DirectionalLightStruct
    {
        float4 lightColorIntensity; // directional light rgb - color, a - intensity
        float3 lightPosition;       // directional light position
        float  lightRadius;         // sun radius
        float3 lightDirection;      // directional light direction
        uint   shadowType;        // 1 use shadowmap
        float2 lightFarAttenuationParams; // a, a/far (a=1/pct-of-far)
        float2 _padding_0;

        DirectionalLightShadowmap directionalLightShadowmap;
    };

    struct PointLightShadowmap
    {
        uint     shadowmap_srv_index; // shadowmap srv in descriptorheap index
        uint2    shadowmap_size;
        float    _padding_shadowmap;
        float4x4 light_proj_view[6];
    };

    struct PointLightStruct
    {
        float3 lightPosition;
        float  lightRadius;
        float4 lightIntensity; // point light rgb - color, a - intensity
        uint   shadowType;
        float  falloff;
        uint2  _padding_0;

        PointLightShadowmap pointLightShadowmap;
    };

    struct PointLightUniform
    {
        uint             pointLightCounts;
        float3           _padding_0;
        PointLightStruct pointLightStructs[m_max_point_light_count];
    };

    struct SpotLightShadowmap
    {
        uint     shadowmap_srv_index; // shadowmap srv in descriptorheap index
        uint2    shadowmap_size;
        float    _padding_shadowmap;
        float4x4 light_proj_view;
    };

    struct SpotLightStruct
    {
        float3 lightPosition;
        float  lightRadius;
        float4 lightIntensity; // spot light rgb - color, a - intensity
        float3 lightDirection;
        float  inner_radians;
        float  outer_radians;
        uint   shadowType;
        float  falloff;
        uint   _padding_0;

        SpotLightShadowmap spotLightShadowmap;
    };

    struct SpotLightUniform
    {
        uint            spotLightCounts;
        float3          _padding_0;
        SpotLightStruct scene_spot_lights[m_max_spot_light_count];
    };

    struct MeshUniform
    {
        uint   totalMeshCount;
        float3 _padding_0;
    };

    struct FrameUniforms
    {
        CameraUniform          cameraUniform;
        BaseUniform            baseUniform;
        MeshUniform            meshUniform;
        AOUniform              aoUniform;
        IBLUniform             iblUniform;
        SSRUniform             ssrUniform;
        DirectionalLightStruct directionalLight;
        PointLightUniform      pointLightUniform;
        SpotLightUniform       spotLightUniform;
    };

    struct BitonicSortCommandSigParams
    {
        uint  MeshIndex;
        float MeshToCamDis;
    };

    struct CommandSignatureParams
    {
        uint                         MeshIndex;
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };
    
    const std::uint64_t totalCommandBufferSizeInBytes = HLSL::MeshLimit * sizeof(HLSL::CommandSignatureParams);

    const std::uint64_t totalIndexCommandBufferInBytes = HLSL::MeshLimit * sizeof(HLSL::BitonicSortCommandSigParams);

} // namespace HLSL
