#pragma once
#include "rhi.h"

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#include <glm/glm.hpp>

#define ConstantBufferStruct struct alignas(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)

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

    struct SceneDirectionalLight
    {
        glm::vec3 direction;
        float     _padding_0;
        glm::vec3 color;
        float     intensity;
        uint32_t  shadowmap; // 1 - use shadowmap, 0 - do not use shadowmap
        uint32_t  cascade; // how many cascade level, default 4
        float     shadowmap_width; // shadowmap size
        uint32_t  shadowmap_srv_index; // shadowmap srv in descriptorheap index
        uint32_t  shadow_bounds[4]; // shadow bounds for each cascade level
        glm::mat4 direction_light_view_matrix;
        glm::mat4 direction_light_projs[4];
        glm::mat4 direction_light_proj_views[4];
    };

    struct ScenePointLight
    {
        glm::vec3 position;
        float     radius;
        glm::vec3 color;
        float     intensity;
    };

    struct SceneSpotLight
    {
        glm::vec3 position;
        float     radius;
        glm::vec3 color;
        float     intensity;
        glm::vec3 direction;
        float     _padding_direction;
        float     inner_radians;
        float     outer_radians;
        glm::vec2 _padding_radians;

        uint32_t  shadowmap;           // 1 - use shadowmap, 0 - do not use shadowmap
        uint32_t  shadowmap_srv_index; // shadowmap srv in descriptorheap index
        float     shadowmap_width;
        float     _padding_shadowmap;
        glm::mat4 spot_light_proj_view;
    };

    struct CameraInstance
    {
        glm::mat4 view_matrix;
        glm::mat4 proj_matrix;
        glm::mat4 proj_view_matrix;
        glm::mat4 view_matrix_inverse;
        glm::mat4 proj_matrix_inverse;
        glm::mat4 proj_view_matrix_inverse;
        glm::vec3 camera_position;
        float     _padding_camera_position;
    };

    ConstantBufferStruct MeshPerframeStorageBufferObject
    {
        CameraInstance        cameraInstance;
        glm::vec3             ambient_light;
        float                 _padding_ambient_light;
        uint32_t              point_light_num;
        uint32_t              spot_light_num;
        uint32_t              total_mesh_num;
        uint32_t              _padding_point_light_num_3;
        ScenePointLight       scene_point_lights[m_max_point_light_count];
        SceneSpotLight        scene_spot_lights[m_max_spot_light_count];
        SceneDirectionalLight scene_directional_light;
    };

    struct BoundingBox
    {
        glm::vec3 center;
        float     _padding_center;
        glm::vec3 extents;
        float     _padding_extents;
    };

    struct MeshInstance
    {
        float     enable_vertex_blending;
        glm::vec3 _padding_enable_vertex_blending;

        glm::mat4 model_matrix;
        glm::mat4 model_matrix_inverse;

        D3D12_VERTEX_BUFFER_VIEW vertexBuffer;
        D3D12_INDEX_BUFFER_VIEW  indexBuffer;

        D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
        glm::vec3 _padding_drawArguments;

        BoundingBox boundingBox;

        uint32_t materialIndex;
        glm::uvec3 _padding_material_index;
    };

    struct MaterialInstance
    {
        uint32_t uniformBufferViewIndex;
        uint32_t baseColorViewIndex;
        uint32_t metallicRoughnessViewIndex;
        uint32_t normalViewIndex;
        uint32_t occlusionViewIndex;
        uint32_t emissionViewIndex;

        glm::uvec2 _padding_material;
    };

    struct MeshPointLightShadowPerframeStorageBufferObject
    {
        uint32_t   point_light_num;
        glm::uvec3 _padding_point_light_num;
        glm::vec4  point_lights_position_and_radius[m_max_point_light_count];
    };

    struct MeshSpotLightShadowPerframeStorageBufferObject
    {
        uint32_t   spot_light_num;
        glm::uvec3 _padding_spot_light_num;
        glm::vec4  spot_lights_position_and_radius[m_max_spot_light_count];
    };

    struct MeshDirectionalLightShadowPerframeStorageBufferObject
    {
        glm::mat4 light_proj_view;
        uint32_t  shadow_width;
        uint32_t  shadow_height;
        uint32_t  shadow_depth;
        uint32_t  _padding_shadow;
    };

    struct BitonicSortCommandSigParams
    {
        uint32_t MeshIndex;
        float    MeshToCamDis;
    };

    struct CommandSignatureParams
    {
        uint32_t                     MeshIndex;
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };

    struct MeshPerMaterialUniformBuffer
    {
        glm::vec4 baseColorFactor {1.0f, 1.0f, 1.0f, 1.0f};

        float metallicFactor           = 0.0f;
        float roughnessFactor          = 0.0f;
        float reflectanceFactor        = 0.0f;
        float clearCoatFactor          = 0.0f;
        float clearCoatRoughnessFactor = 0.0f;
        float anisotropyFactor         = 0.0f;

        float normalScale              = 0.0f;
        float occlusionStrength        = 0.0f;

        glm::vec2 base_color_tilling         = {1.0f, 1.0f};
        glm::vec2 metallic_roughness_tilling = {1.0f, 1.0f};
        glm::vec2 normal_tilling             = {1.0f, 1.0f};
        glm::vec2 occlusion_tilling          = {1.0f, 1.0f};
        glm::vec2 emissive_tilling           = {1.0f, 1.0f};

        glm::vec2 _padding_uniform_00;

        float     emissiveFactor  = 1.0f;
        uint32_t  is_blend        = 0;
        uint32_t  is_double_sided = 0;

        uint32_t _padding_uniform_1;
    };

    const std::uint64_t totalCommandBufferSizeInBytes = HLSL::MeshLimit * sizeof(HLSL::CommandSignatureParams);

    const std::uint64_t totalIndexCommandBufferInBytes = HLSL::MeshLimit * sizeof(HLSL::BitonicSortCommandSigParams);

} // namespace HLSL
