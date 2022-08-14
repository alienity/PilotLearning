#pragma once

#include "runtime/core/math/moyu_math.h"

#include "runtime/function/render/render_type.h"

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#include <glm/glm.hpp>

namespace Pilot
{
    static const uint32_t m_point_light_shadow_map_dimension       = 2048;
    static const uint32_t m_directional_light_shadow_map_dimension = 4096;

    // TODO: 64 may not be the best
    static uint32_t const m_mesh_per_drawcall_max_instance_count = 64;
    static uint32_t const m_mesh_vertex_blending_max_joint_count = 1024;
    static uint32_t const m_max_point_light_count                = 15;
    // should sync the macros in "shader_include/constants.h"

    struct SceneDirectionalLight
    {
        glm::vec3 direction;
        float     _padding_direction;
        glm::vec3 color;
        float     _padding_color;
    };

    struct ScenePointLight
    {
        glm::vec3 position;
        float     radius;
        glm::vec3 intensity;
        float     _padding_intensity;
    };

    struct MeshPerframeStorageBufferObject
    {
        glm::mat4             proj_view_matrix;
        glm::vec3             camera_position;
        float                 _padding_camera_position;
        glm::vec3             ambient_light;
        float                 _padding_ambient_light;
        uint32_t              point_light_num;
        uint32_t              _padding_point_light_num_1;
        uint32_t              _padding_point_light_num_2;
        uint32_t              _padding_point_light_num_3;
        ScenePointLight       scene_point_lights[m_max_point_light_count];
        SceneDirectionalLight scene_directional_light;
        glm::mat4             directional_light_proj_view;
    };

    struct MeshInstance
    {
        float     enable_vertex_blending;
        float     _padding_enable_vertex_blending_1;
        float     _padding_enable_vertex_blending_2;
        float     _padding_enable_vertex_blending_3;
        glm::mat4 model_matrix;
    };

    struct MeshPerdrawcallStorageBufferObject
    {
        MeshInstance mesh_instances[m_mesh_per_drawcall_max_instance_count];
    };

    struct MeshPerdrawcallVertexBlendingStorageBufferObject
    {
        glm::mat4 joint_matrices[m_mesh_vertex_blending_max_joint_count * m_mesh_per_drawcall_max_instance_count];
    };

    struct MeshPerMaterialUniformBufferObject
    {
        Vector4 baseColorFactor {0.0f, 0.0f, 0.0f, 0.0f};

        float metallicFactor    = 0.0f;
        float roughnessFactor   = 0.0f;
        float normalScale       = 0.0f;
        float occlusionStrength = 0.0f;

        Vector3  emissiveFactor  = {0.0f, 0.0f, 0.0f};
        uint32_t is_blend        = 0;
        uint32_t is_double_sided = 0;
    };

    struct MeshPointLightShadowPerframeStorageBufferObject
    {
        uint32_t point_light_num;
        uint32_t _padding_point_light_num_1;
        uint32_t _padding_point_light_num_2;
        uint32_t _padding_point_light_num_3;
        Vector4  point_lights_position_and_radius[m_max_point_light_count];
    };

    struct MeshPointLightShadowPerdrawcallStorageBufferObject
    {
        MeshInstance mesh_instances[m_mesh_per_drawcall_max_instance_count];
    };

    struct MeshPointLightShadowPerdrawcallVertexBlendingStorageBufferObject
    {
        glm::mat4 joint_matrices[m_mesh_vertex_blending_max_joint_count * m_mesh_per_drawcall_max_instance_count];
    };

    struct MeshDirectionalLightShadowPerframeStorageBufferObject
    {
        glm::mat4 light_proj_view;
    };

    struct MeshDirectionalLightShadowPerdrawcallStorageBufferObject
    {
        MeshInstance mesh_instances[m_mesh_per_drawcall_max_instance_count];
    };

    struct MeshDirectionalLightShadowPerdrawcallVertexBlendingStorageBufferObject
    {
        glm::mat4 joint_matrices[m_mesh_vertex_blending_max_joint_count * m_mesh_per_drawcall_max_instance_count];
    };

    struct AxisStorageBufferObject
    {
        glm::mat4 model_matrix  = glm::mat4(1.0);
        uint32_t  selected_axis = 3;
    };

    struct ParticleBillboardPerframeStorageBufferObject
    {
        glm::mat4 proj_view_matrix;
        glm::vec3 eye_position;
        float     _padding_eye_position;
        glm::vec3 up_direction;
        float     _padding_up_direction;
    };

    struct ParticleBillboardPerdrawcallStorageBufferObject
    {
        // TODO: 4096 may not be the best
        Vector4 positions[4096];
    };

    struct MeshInefficientPickPerframeStorageBufferObject
    {
        glm::mat4 proj_view_matrix;
        uint32_t  rt_width;
        uint32_t  rt_height;
    };

    struct MeshInefficientPickPerdrawcallStorageBufferObject
    {
        glm::mat4 model_matrices[m_mesh_per_drawcall_max_instance_count];
        uint32_t  node_ids[m_mesh_per_drawcall_max_instance_count];
        float     enable_vertex_blendings[m_mesh_per_drawcall_max_instance_count];
    };

    struct MeshInefficientPickPerdrawcallVertexBlendingStorageBufferObject
    {
        glm::mat4 joint_matrices[m_mesh_vertex_blending_max_joint_count * m_mesh_per_drawcall_max_instance_count];
    };

    // mesh
    struct D3D12Mesh
    {
        bool enable_vertex_blending;

        uint32_t mesh_vertex_count;

        RHI::D3D12Buffer mesh_vertex_buffer;

        uint32_t mesh_index_count;

        RHI::D3D12Buffer mesh_index_buffer;


    };

    // material
    struct D3D12PBRMaterial
    {
        RHI::D3D12Texture            base_color_texture_image;
        RHI::D3D12ShaderResourceView base_color_image_view;

        RHI::D3D12Texture            metallic_roughness_texture_image;
        RHI::D3D12ShaderResourceView metallic_roughness_image_view;

        RHI::D3D12Texture            normal_texture_image;
        RHI::D3D12ShaderResourceView normal_image_view;

        RHI::D3D12Texture            occlusion_texture_image;
        RHI::D3D12ShaderResourceView occlusion_image_view;

        RHI::D3D12Texture            emissive_texture_image;
        RHI::D3D12ShaderResourceView emissive_image_view;

        RHI::D3D12Buffer             material_uniform_buffer;
        RHI::D3D12ShaderResourceView material_uniform_buffer_view;
        
        //VkDescriptorSet material_descriptor_set;
    };

    // nodes
    struct RenderMeshNode
    {
        glm::mat4          model_matrix;
        glm::mat4          joint_matrices[m_mesh_vertex_blending_max_joint_count];
        D3D12Mesh*         ref_mesh     = nullptr;
        D3D12PBRMaterial*  ref_material = nullptr;
        uint32_t           node_id;
        glm::vec3          bounding_box_min;
        glm::vec3          bounding_box_max;
        bool               enable_vertex_blending = false;
    };

    struct RenderAxisNode
    {
        glm::mat4  model_matrix {glm::mat4(1.0f)};
        D3D12Mesh* ref_mesh {nullptr};
        uint32_t   node_id;
        bool       enable_vertex_blending {false};
    };

    struct RenderParticleBillboardNode
    {
        std::vector<Vector4> positions;
    };

    struct TextureDataToUpdate
    {
        void*              base_color_image_pixels;
        uint32_t           base_color_image_width;
        uint32_t           base_color_image_height;
        DXGI_FORMAT        base_color_image_format;
        void*              metallic_roughness_image_pixels;
        uint32_t           metallic_roughness_image_width;
        uint32_t           metallic_roughness_image_height;
        DXGI_FORMAT        metallic_roughness_image_format;
        void*              normal_roughness_image_pixels;
        uint32_t           normal_roughness_image_width;
        uint32_t           normal_roughness_image_height;
        DXGI_FORMAT        normal_roughness_image_format;
        void*              occlusion_image_pixels;
        uint32_t           occlusion_image_width;
        uint32_t           occlusion_image_height;
        DXGI_FORMAT        occlusion_image_format;
        void*              emissive_image_pixels;
        uint32_t           emissive_image_width;
        uint32_t           emissive_image_height;
        DXGI_FORMAT        emissive_image_format;
        D3D12PBRMaterial* now_material;
    };
} // namespace Pilot

namespace HLSL
{

    static constexpr size_t MaterialLimit = 8192;
    static constexpr size_t LightLimit    = 32;
    static constexpr size_t MeshLimit     = 8192;

    static const uint32_t m_point_light_shadow_map_dimension       = 2048;
    static const uint32_t m_directional_light_shadow_map_dimension = 4096;

    static uint32_t const m_mesh_per_drawcall_max_instance_count = 64;
    static uint32_t const m_mesh_vertex_blending_max_joint_count = 1024;
    static uint32_t const m_max_point_light_count                = 15;

    struct SceneDirectionalLight
    {
        glm::vec3 direction;
        float     _padding_direction;
        glm::vec3 color;
        float     _padding_color;
    };

    struct ScenePointLight
    {
        glm::vec3 position;
        float     radius;
        glm::vec3 intensity;
        float     _padding_intensity;
    };

    struct CameraInstance
    {
        glm::mat4 view_matrix;
        glm::mat4 proj_matrix;
        glm::mat4 proj_view_matrix;
        glm::vec3 camera_position;
        float     _padding_camera_position;

    };

    struct MeshPerframeStorageBufferObject
    {
        CameraInstance        cameraInstance;
        glm::vec3             ambient_light;
        float                 _padding_ambient_light;
        uint32_t              point_light_num;
        uint32_t              _padding_point_light_num_1;
        uint32_t              _padding_point_light_num_2;
        uint32_t              _padding_point_light_num_3;
        ScenePointLight       scene_point_lights[m_max_point_light_count];
        SceneDirectionalLight scene_directional_light;
        glm::mat4             directional_light_proj_view;
    };

    struct MeshInstance
    {
        float     enable_vertex_blending;
        float     _padding_enable_vertex_blending_1;
        float     _padding_enable_vertex_blending_2;
        float     _padding_enable_vertex_blending_3;
        glm::mat4 model_matrix;

        D3D12_VERTEX_BUFFER_VIEW vertexBuffer;
        D3D12_INDEX_BUFFER_VIEW  indexBuffer;

        D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;

        glm::vec3 bounding_box_min;
        float     _bounding_box_min_padding;
        glm::vec3 bounding_box_max;
        float     _bounding_box_max_padding;

        uint32_t materialIndex;
        uint32_t _padding_View0;
        uint32_t _padding_View1;
        uint32_t _padding_View2;
    };

    struct MaterialInstance
    {
        uint32_t uniformBufferViewIndex; 
        uint32_t baseColorViewIndex; 
        uint32_t metallicRoughnessViewIndex;
        uint32_t normalViewIndex;
        uint32_t occlusionViewIndex;
        uint32_t emissionViewIndex;
    };

    struct MeshPointLightShadowPerframeStorageBufferObject
    {
        uint32_t  point_light_num;
        uint32_t  _padding_point_light_num_1;
        uint32_t  _padding_point_light_num_2;
        uint32_t  _padding_point_light_num_3;
        glm::vec4 point_lights_position_and_radius[m_max_point_light_count];
    };

    struct MeshDirectionalLightShadowPerframeStorageBufferObject
    {
        glm::mat4 light_proj_view;
        uint32_t  shadow_width;
        uint32_t  shadow_height;
        uint32_t  shadow_depth;
        uint32_t  _padding_shadow;
    };

    struct CommandSignatureParams
    {
        uint32_t                     MeshIndex;
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };






}
