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

    static constexpr uint32_t GeoClipMeshType = 5;

    static const uint32_t m_point_light_shadow_map_dimension       = 2048;
    static const uint32_t m_directional_light_shadow_map_dimension = 4096;

    static uint32_t const m_mesh_per_drawcall_max_instance_count = 64;
    static uint32_t const m_mesh_vertex_blending_max_joint_count = 1024;
    static uint32_t const m_max_point_light_count                = 16;
    static uint32_t const m_max_spot_light_count                 = 16;

    struct BoundingBox
    {
        glm::float3 center;
        float       _Padding_Center;
        glm::float3 extents;
        float       _Padding_Extents;
    };


    struct PerMaterialParametersBuffer
    {
        glm::float4 baseColorFactor {1.0f, 1.0f, 1.0f, 1.0f};

        float metallicFactor {1.0f};
        float roughnessFactor {0.0f};
        float reflectanceFactor {0.5f};
        float clearCoatFactor {1.0f};
        float clearCoatRoughnessFactor {0.0f};
        float anisotropyFactor {0.0f};
        glm::float2 _padding0;

        glm::float2 base_color_tilling {1.0f, 1.0f};
        glm::float2 metallic_roughness_tilling {1.0f, 1.0f};
        glm::float2 normal_tilling {1.0f, 1.0f};
        glm::float2 occlusion_tilling {1.0f, 1.0f};
        glm::float2 emissive_tilling {1.0f, 1.0f};

        glm::uint  is_blend {0};
        glm::uint is_double_sided {0};
    };

    struct PerMaterialViewIndexBuffer
    {
        glm::uint parametersBufferIndex;
        glm::uint baseColorIndex;
        glm::uint metallicRoughnessIndex;
        glm::uint normalIndex;
        glm::uint occlusionIndex;
        glm::uint emissionIndex;

        glm::uvec2 _padding_material_0;
    };

    struct PerRenderableMeshData
    {
        // 16
        float  enableVertexBlending;
        glm::float3 _padding_enable_vertex_blending;

        // 64
        glm::float4x4 worldFromModelMatrix;
        // 64
        glm::float4x4 modelFromWorldMatrix;

        // 64
        glm::float4x4 prevWorldFromModelMatrix;
        // 64
        glm::float4x4 prevModelFromWorldMatrix;

        // 16
        D3D12_VERTEX_BUFFER_VIEW vertexBuffer;
        // 16
        D3D12_INDEX_BUFFER_VIEW indexBuffer;

        // 20
        D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;

        // 12
        glm::float3 _padding_drawArguments;

        // 32
        BoundingBox boundingBox;

        // 16
        glm::uint  perMaterialViewIndexBufferIndex;
        glm::uvec3 _padding_materialIndex2;
    };

    struct FrameCameraUniform
    {
        glm::float4x4 viewFromWorldMatrix; // clip    view <- world    : view matrix
        glm::float4x4 worldFromViewMatrix; // clip    view -> world    : model matrix
        glm::float4x4 clipFromViewMatrix;  // clip <- view    world    : projection matrix
        glm::float4x4 viewFromClipMatrix;  // clip -> view    world    : inverse projection matrix
        glm::float4x4 unJitterProjectionMatrix; // Unjitter projection matrix
        glm::float4x4 unJitterProjectionMatrixInv; // Unjitter projection matrix inverse
        glm::float4x4 clipFromWorldMatrix; // clip <- view <- world
        glm::float4x4 worldFromClipMatrix; // clip -> view -> world
        glm::float4   clipTransform;       // [sx, sy, tx, ty] only used by VERTEX_DOMAIN_DEVICE
        glm::float3   cameraPosition;      // camera world position
        glm::uint     jitterIndex;         // jitter index
        glm::float4   zBufferParams; // xy - to 01 depth, zw - to eye depth
    };

    struct CameraUniform
    {
        FrameCameraUniform curFrameUniform;
        FrameCameraUniform lastFrameUniform;

        glm::float4 resolution;            // physical viewport width, height, 1/width, 1/height
        glm::float2 logicalViewportScale;  // scale-factor to go from physical to logical viewport
        glm::float2 logicalViewportOffset; // offset to go from physical to logical viewport

        // camera position in view space (when camera_at_origin is enabled), i.e. it's (0,0,0).
        float cameraNear;
        float cameraFar; // camera *culling* far-plane distance, always positive (projection far is at +inf)
        float exposure;
        float ev100;
    };

    struct BaseUniform
    {
        glm::float2 clipControl;   // clip control
        float  time;          // time in seconds, with a 1-second period
        float  temporalNoise; // noise [0,1] when TAA is used, 0 otherwise
        glm::float4 userTime;      // time(s), (double)time - (float)time, 0, 0
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

    struct TAAuniform
    {
        glm::float4 projectionExtents; // xy = frustum extents at distance 1, zw = jitter at distance 1
        glm::float4 jitterUV;
        float       feedbackMin;
        float       feedbackMax;
        float       motionScale;
        float       __Reserved0;
    };

    struct IBLUniform
    {
        glm::float4 iblSH[7]; // actually glm::float3 entries (std140 requires glm::float4 alignment)
        float  iblLuminance;
        float  iblRoughnessOneLevel; // level for roughness == 1
        int    dfg_lut_srv_index;    // specular lut dfg
        int    ld_lut_srv_index;     // specular lut ld
        int    radians_srv_index;    // diffuse
        glm::float3 __Reserved0;
    };

    struct SSRUniform
    {
        glm::float4 ScreenSize;
        glm::float4 ResolveSize;
        glm::float4 RayCastSize;
        glm::float4 JitterSizeAndOffset;
        glm::float4 NoiseSize;
        float  SmoothnessRange;
        float  BRDFBias;
        float  TResponseMin;
        float  TResponseMax;
        float  EdgeFactor;
        float  Thickness;
        int    NumSteps;
        int    MaxMipMap;
    };

    struct DirectionalLightShadowmap
    {
        glm::uint     shadowmap_srv_index; // shadowmap srv in descriptorheap index
        glm::uint     cascadeCount;        // how many cascade level, default 4
        glm::uvec2    shadowmap_size;     // shadowmap size
        glm::uvec4    shadow_bounds;       // shadow bounds for each cascade level
        glm::float4x4 light_view_matrix;   // direction light view matrix
        glm::float4x4 light_proj_matrix[4];
        glm::float4x4 light_proj_view_matrix[4];
    };

    struct DirectionalLightStruct
    {
        glm::float4 lightColorIntensity; // directional light rgb - color, a - intensity
        glm::float3 lightPosition;       // directional light position
        float  lightRadius;         // sun radius
        glm::float3 lightDirection;      // directional light direction
        glm::uint   shadowType;        // 1 use shadowmap
        glm::float2 lightFarAttenuationParams; // a, a/far (a=1/pct-of-far)
        glm::float2 _padding_0;

        DirectionalLightShadowmap directionalLightShadowmap;
    };

    struct PointLightShadowmap
    {
        glm::uint     shadowmap_srv_index; // shadowmap srv in descriptorheap index
        glm::uvec2    shadowmap_size;
        float    _padding_shadowmap;
        glm::float4x4 light_proj_view[6];
    };

    struct PointLightStruct
    {
        glm::float3 lightPosition;
        float       lightRadius;
        glm::float4 lightIntensity; // point light rgb - color, a - intensity
        glm::uint   shadowType;
        float       falloff;
        glm::uvec2  _padding_0;

        PointLightShadowmap pointLightShadowmap;
    };

    struct PointLightUniform
    {
        glm::uint             pointLightCounts;
        glm::float3           _padding_0;
        PointLightStruct pointLightStructs[m_max_point_light_count];
    };

    struct SpotLightShadowmap
    {
        glm::uint     shadowmap_srv_index; // shadowmap srv in descriptorheap index
        glm::uvec2    shadowmap_size;
        float    _padding_shadowmap;
        glm::float4x4 light_proj_view;
    };

    struct SpotLightStruct
    {
        glm::float3 lightPosition;
        float  lightRadius;
        glm::float4 lightIntensity; // spot light rgb - color, a - intensity
        glm::float3 lightDirection;
        float  inner_radians;
        float  outer_radians;
        glm::uint   shadowType;
        float  falloff;
        glm::uint   _padding_0;

        SpotLightShadowmap spotLightShadowmap;
    };

    struct SpotLightUniform
    {
        glm::uint            spotLightCounts;
        glm::float3          _padding_0;
        SpotLightStruct scene_spot_lights[m_max_spot_light_count];
    };

    struct MeshUniform
    {
        glm::uint   totalMeshCount;
        glm::float3 _padding_0;
    };

    struct TerrainUniform
    {
        glm::uint terrainSize;      // 1024*1024
        glm::uint terrainMaxHeight; // 1024
        glm::uint heightMapIndex;
        glm::uint normalMapIndex;
        glm::float4x4 local2WorldMatrix;
        glm::float4x4 world2LocalMatrix;
        glm::float4x4 prevLocal2WorldMatrix;
        glm::float4x4 prevWorld2LocalMatrix;
    };

    struct VolumeLightUniform
    {
        float scattering_coef;
        float extinction_coef;
        float volumetrix_range;
        float skybox_extinction_coef;

        float mieG; // x: 1 - g^2, y: 1 + g^2, z: 2*g, w: 1/4pi
        float noise_scale;
        float noise_intensity;
        float noise_intensity_offset;

        glm::float2 noise_velocity;
        float       ground_level;
        float       height_scale;

        float maxRayLength;
        int   sampleCount;
        int   downscaleMip;
        float minStepSize;
    };

    struct FrameUniforms
    {
        CameraUniform          cameraUniform;
        BaseUniform            baseUniform;
        TerrainUniform         terrainUniform;
        MeshUniform            meshUniform;
        AOUniform              aoUniform;
        TAAuniform             taaUniform;
        IBLUniform             iblUniform;
        SSRUniform             ssrUniform;
        DirectionalLightStruct directionalLight;
        PointLightUniform      pointLightUniform;
        SpotLightUniform       spotLightUniform;
        VolumeLightUniform     volumeLightUniform;
    };

    struct BitonicSortCommandSigParams
    {
        glm::uint  MeshIndex;
        float MeshToCamDis;
    };

    struct CommandSignatureParams
    {
        glm::uint                    MeshIndex;
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };
    
    const std::uint64_t totalCommandBufferSizeInBytes = HLSL::MeshLimit * sizeof(HLSL::CommandSignatureParams);

    const std::uint64_t totalIndexCommandBufferInBytes = HLSL::MeshLimit * sizeof(HLSL::BitonicSortCommandSigParams);


    struct ClipmapTransform
    {
        glm::float4x4 transform;
        int mesh_type;
    };

    struct ClipmapMeshCount
    {
        int tile_count;
        int filler_count;
        int trim_count;
        int seam_count;
        int cross_cunt;
        int total_count;
    };

    struct ClipMeshCommandSigParams
    {
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
        glm::int3                    _Padding_0;
        BoundingBox                  ClipBoundingBox;
    };

    struct ToDrawCommandSignatureParams
    {
        uint32_t                     ClipIndex;
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };

    enum NeighborFace
    {
        None  = 0,
        EAST  = 1 << 0,
        SOUTH = 1 << 1,
        WEST  = 1 << 2,
        NORTH = 1 << 3
    };

    struct TerrainPatchNode
    {
        glm::float2 patchMinPos; // node�����½Ƕ���
        float       maxHeight;   // ��ǰnode���߶�
        float       minHeight;   // ��ǰnode��С�߶�
        float       nodeWidth;   // patchnode�Ŀ���
        int         mipLevel;    // ��ǰnode��mip�ȼ�
        uint32_t    neighbor;    // ����һ��mip��Ϊ�ھӵı�ʶ
    };

    // 512 * 512
    #define MaxTerrainNodeCount 262144

} // namespace HLSL
