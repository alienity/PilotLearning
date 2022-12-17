#include "runtime/function/render/glm_wrapper.h"
#include "runtime/function/render/render_camera.h"

#include "runtime/function/render/render_mesh.h"
#include "runtime/function/render/render_resource.h"

#include "runtime/function/render/rhi/d3d12/d3d12_core.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resource.h"
#include "runtime/function/render/rhi/d3d12/d3d12_descriptor.h"

#include "runtime/function/render/rhi/hlsl_data_types.h"

#include "runtime/core/base/macro.h"

#include <stdexcept>

namespace Pilot
{
    void RenderResource::uploadGlobalRenderResource(LevelResourceDesc level_resource_desc)
    {
        // sky box irradiance
        SkyBoxIrradianceMap skybox_irradiance_map        = level_resource_desc.m_ibl_resource_desc.m_skybox_irradiance_map;
        std::shared_ptr<TextureData> irradiace_pos_x_map = loadTextureHDR(skybox_irradiance_map.m_positive_x_map);
        std::shared_ptr<TextureData> irradiace_neg_x_map = loadTextureHDR(skybox_irradiance_map.m_negative_x_map);
        std::shared_ptr<TextureData> irradiace_pos_y_map = loadTextureHDR(skybox_irradiance_map.m_positive_y_map);
        std::shared_ptr<TextureData> irradiace_neg_y_map = loadTextureHDR(skybox_irradiance_map.m_negative_y_map);
        std::shared_ptr<TextureData> irradiace_pos_z_map = loadTextureHDR(skybox_irradiance_map.m_positive_z_map);
        std::shared_ptr<TextureData> irradiace_neg_z_map = loadTextureHDR(skybox_irradiance_map.m_negative_z_map);

        // sky box specular
        SkyBoxSpecularMap            skybox_specular_map = level_resource_desc.m_ibl_resource_desc.m_skybox_specular_map;
        std::shared_ptr<TextureData> specular_pos_x_map  = loadTextureHDR(skybox_specular_map.m_positive_x_map);
        std::shared_ptr<TextureData> specular_neg_x_map  = loadTextureHDR(skybox_specular_map.m_negative_x_map);
        std::shared_ptr<TextureData> specular_pos_y_map  = loadTextureHDR(skybox_specular_map.m_positive_y_map);
        std::shared_ptr<TextureData> specular_neg_y_map  = loadTextureHDR(skybox_specular_map.m_negative_y_map);
        std::shared_ptr<TextureData> specular_pos_z_map  = loadTextureHDR(skybox_specular_map.m_positive_z_map);
        std::shared_ptr<TextureData> specular_neg_z_map  = loadTextureHDR(skybox_specular_map.m_negative_z_map);

        // create IBL textures, take care of the texture order
        std::array<std::shared_ptr<TextureData>, 6> irradiance_maps = {irradiace_pos_x_map,
                                                                       irradiace_neg_x_map,
                                                                       irradiace_pos_z_map,
                                                                       irradiace_neg_z_map,
                                                                       irradiace_pos_y_map,
                                                                       irradiace_neg_y_map};
        std::array<std::shared_ptr<TextureData>, 6> specular_maps   = {specular_pos_x_map,
                                                                     specular_neg_x_map,
                                                                     specular_pos_z_map,
                                                                     specular_neg_z_map,
                                                                     specular_pos_y_map,
                                                                     specular_neg_y_map};

        // brdf
        std::shared_ptr<TextureData> brdf_map = loadTextureHDR(level_resource_desc.m_ibl_resource_desc.m_brdf_map);

        // color grading
        std::shared_ptr<TextureData> color_grading_map = loadTexture(level_resource_desc.m_color_grading_resource_desc.m_color_grading_map);

        startUploadBatch();
        {
            // create irradiance cubemap
            auto irridiance_tex = createCubeMap(irradiance_maps);
            m_global_render_resource._ibl_resource._irradiance_texture_image = irridiance_tex;

            // create specular cubemap
            auto specular_tex = createCubeMap(specular_maps);
            m_global_render_resource._ibl_resource._specular_texture_image = specular_tex;

            // create brdf lut texture
            auto lut_tex = createTex2D(brdf_map);
            m_global_render_resource._ibl_resource._brdfLUT_texture_image = lut_tex;

            // create color grading texture
            auto color_grading_tex = createTex2D(color_grading_map);
            m_global_render_resource._color_grading_resource._color_grading_LUT_texture_image = color_grading_tex;
        }
        {
            auto genDefaultData = ([](bool isWhite) {
                std::shared_ptr<TextureData> default_map = std::make_shared<TextureData>();

                char val = isWhite ? 255 : 0;

                default_map->m_width        = 1;
                default_map->m_height       = 1;
                default_map->m_pixels       = new char[4] {val, val, val, val};
                default_map->m_need_release = true;
                default_map->m_format       = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
                default_map->m_depth        = 1;
                default_map->m_array_layers = 1;
                default_map->m_mip_levels   = 1;

                return default_map;
                });

            std::shared_ptr<TextureData> default_white_data = genDefaultData(true);
            auto white_tex = createTex2D(default_white_data);
            m_default_resource._white_texture2d_image       = white_tex;

            std::shared_ptr<TextureData> default_black_data = genDefaultData(false);
            auto black_tex = createTex2D(default_black_data);
            m_default_resource._black_texture2d_image       = black_tex;

        }
        endUploadBatch();
    }

    void RenderResource::uploadGameObjectRenderResource(RenderEntity       render_entity,
                                                        RenderMeshData     mesh_data,
                                                        RenderMaterialData material_data)
    {
        getOrCreateD3D12Mesh(render_entity, mesh_data);
        getOrCreateD3D12Material(render_entity, material_data);
    }

    void RenderResource::uploadGameObjectRenderResource(RenderEntity         render_entity,
                                                        RenderMeshData       mesh_data)
    {
        getOrCreateD3D12Mesh(render_entity, mesh_data);
    }

    void RenderResource::uploadGameObjectRenderResource(RenderEntity         render_entity,
                                                        RenderMaterialData   material_data)
    {
        getOrCreateD3D12Material(render_entity, material_data);
    }

    void RenderResource::updatePerFrameBuffer(std::shared_ptr<RenderScene>  render_scene,
                                              std::shared_ptr<RenderCamera> camera)
    {
        Matrix4x4 view_matrix      = camera->getViewMatrix();
        Matrix4x4 proj_matrix      = camera->getPersProjMatrix();
        Vector3   camera_position  = camera->position();
        glm::mat4 proj_view_matrix = GLMUtil::fromMat4x4(proj_matrix * view_matrix);

        // camera instance
        HLSL::CameraInstance cameraInstance;
        cameraInstance.view_matrix      = GLMUtil::fromMat4x4(view_matrix);
        cameraInstance.proj_matrix      = GLMUtil::fromMat4x4(proj_matrix);
        cameraInstance.proj_view_matrix = proj_view_matrix;
        cameraInstance.camera_position  = GLMUtil::fromVec3(camera_position);

        // ambient light
        Vector3  ambient_light   = render_scene->m_ambient_light.m_color.toVector3();
        uint32_t point_light_num = static_cast<uint32_t>(render_scene->m_point_light_list.size());
        uint32_t spot_light_num  = static_cast<uint32_t>(render_scene->m_spot_light_list.size());

        // set ubo data
        m_mesh_perframe_storage_buffer_object.cameraInstance   = cameraInstance;
        m_mesh_perframe_storage_buffer_object.ambient_light    = GLMUtil::fromVec3(ambient_light);
        m_mesh_perframe_storage_buffer_object.point_light_num  = point_light_num;
        m_mesh_perframe_storage_buffer_object.spot_light_num   = spot_light_num;

        m_mesh_point_light_shadow_perframe_storage_buffer_object.point_light_num = point_light_num;

        // point lights
        for (uint32_t i = 0; i < point_light_num; i++)
        {
            Vector3 point_light_position = render_scene->m_point_light_list[i].m_position;
            float point_light_intensity = render_scene->m_point_light_list[i].m_intensity;

            float radius = render_scene->m_point_light_list[i].m_radius;
            Color color  = render_scene->m_point_light_list[i].m_color;

            m_mesh_perframe_storage_buffer_object.scene_point_lights[i].position = GLMUtil::fromVec3(point_light_position);
            m_mesh_perframe_storage_buffer_object.scene_point_lights[i].radius = radius;
            m_mesh_perframe_storage_buffer_object.scene_point_lights[i].color = GLMUtil::fromVec3(Vector3(color.r, color.g, color.b));
            m_mesh_perframe_storage_buffer_object.scene_point_lights[i].intensity = point_light_intensity;

            m_mesh_point_light_shadow_perframe_storage_buffer_object.point_lights_position_and_radius[i] = 
                glm::vec4(point_light_position.x, point_light_position.y, point_light_position.z, radius);
        }

        m_mesh_spot_light_shadow_perframe_storage_buffer_object.spot_light_num = spot_light_num;

        // spot lights
        for (uint32_t i = 0; i < spot_light_num; i++)
        {
            Vector3 spot_light_position  = render_scene->m_spot_light_list[i].m_position;
            Vector3 spot_light_direction = render_scene->m_spot_light_list[i].m_direction;
            float   spot_light_intensity = render_scene->m_spot_light_list[i].m_intensity;

            Color color  = render_scene->m_spot_light_list[i].m_color;

            m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].position = GLMUtil::fromVec3(spot_light_position);
            m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].radius = render_scene->m_spot_light_list[i].m_radius;
            m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].color = GLMUtil::fromVec3(Vector3(color.r, color.g, color.b));
            m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].intensity = spot_light_intensity;
            m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].direction = GLMUtil::fromVec3(spot_light_direction);
            m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].inner_radians = render_scene->m_spot_light_list[i].m_inner_radians;
            m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].outer_radians = render_scene->m_spot_light_list[i].m_outer_radians;

            m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].shadowmap = render_scene->m_spot_light_list[i].m_shadowmap;
            m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].shadowmap_width = render_scene->m_spot_light_list[i].m_shadowmap_size.x;
            m_mesh_perframe_storage_buffer_object.scene_spot_lights[i].spot_light_proj_view = GLMUtil::fromMat4x4(render_scene->m_spot_light_list[i].m_shadow_view_proj_mat);
        }

        {
            // directional light
            m_mesh_perframe_storage_buffer_object.scene_directional_light.direction = GLMUtil::fromVec3(Vector3::normalize(render_scene->m_directional_light.m_direction));
            m_mesh_perframe_storage_buffer_object.scene_directional_light.color = GLMUtil::fromVec3(render_scene->m_directional_light.m_color.toVector3());
            m_mesh_perframe_storage_buffer_object.scene_directional_light.intensity = render_scene->m_directional_light.m_intensity;
            m_mesh_perframe_storage_buffer_object.scene_directional_light.shadowmap = render_scene->m_directional_light.m_shadowmap ? 1 : 0;
            m_mesh_perframe_storage_buffer_object.scene_directional_light.shadowmap_width = render_scene->m_directional_light.m_shadowmap_size.x;
            m_mesh_perframe_storage_buffer_object.scene_directional_light.directional_light_proj_view = GLMUtil::fromMat4x4(render_scene->m_directional_light.m_shadow_view_proj_mat);
        }
    }

    D3D12Mesh& RenderResource::getOrCreateD3D12Mesh(RenderEntity entity, RenderMeshData mesh_data)
    {
        size_t assetid = entity.m_mesh_asset_id;

        auto it = m_d3d12_meshes.find(assetid);
        if (it != m_d3d12_meshes.end())
        {
            return it->second;
        }
        else
        {
            D3D12Mesh temp;
            auto      res = m_d3d12_meshes.insert(std::make_pair(assetid, std::move(temp)));
            assert(res.second);

            uint32_t index_buffer_size = static_cast<uint32_t>(mesh_data.m_static_mesh_data.m_index_buffer->m_size);
            void*    index_buffer_data = mesh_data.m_static_mesh_data.m_index_buffer->m_data;

            uint32_t vertex_buffer_size = static_cast<uint32_t>(mesh_data.m_static_mesh_data.m_vertex_buffer->m_size);
            MeshVertexDataDefinition* vertex_buffer_data =
                reinterpret_cast<MeshVertexDataDefinition*>(mesh_data.m_static_mesh_data.m_vertex_buffer->m_data);

            D3D12Mesh& now_mesh = res.first->second;

            if (mesh_data.m_skeleton_binding_buffer)
            {
                uint32_t joint_binding_buffer_size = (uint32_t)mesh_data.m_skeleton_binding_buffer->m_size;
                MeshVertexBindingDataDefinition* joint_binding_buffer_data =
                    reinterpret_cast<MeshVertexBindingDataDefinition*>(mesh_data.m_skeleton_binding_buffer->m_data);
                updateMeshData(true,
                               index_buffer_size,
                               index_buffer_data,
                               vertex_buffer_size,
                               vertex_buffer_data,
                               joint_binding_buffer_size,
                               joint_binding_buffer_data,
                               now_mesh);
            }
            else
            {
                updateMeshData(false,
                               index_buffer_size,
                               index_buffer_data,
                               vertex_buffer_size,
                               vertex_buffer_data,
                               0,
                               nullptr,
                               now_mesh);
            }

            return now_mesh;
        }
    }

    D3D12PBRMaterial& RenderResource::getOrCreateD3D12Material(RenderEntity entity, RenderMaterialData material_data)
    {
        size_t assetid = entity.m_material_asset_id;

        auto it = m_d3d12_pbr_materials.find(assetid);
        if (it != m_d3d12_pbr_materials.end())
        {
            return it->second;
        }
        else
        {
            D3D12PBRMaterial temp;
            auto res = m_d3d12_pbr_materials.insert(std::make_pair(assetid, std::move(temp)));
            assert(res.second);

            float empty_image[] = {0.5f, 0.5f, 0.5f, 0.5f};

            void* base_color_image_pixels = empty_image;
            
            std::shared_ptr<TextureData> base_color_texture_data = material_data.m_base_color_texture;
            if (base_color_texture_data == nullptr)
            {
                base_color_texture_data                 = std::make_shared<TextureData>();
                base_color_texture_data->m_width        = 1;
                base_color_texture_data->m_height       = 1;
                base_color_texture_data->m_depth        = 0;
                base_color_texture_data->m_mip_levels   = 0;
                base_color_texture_data->m_array_layers = 0;
                base_color_texture_data->m_is_srgb      = true;
                base_color_texture_data->m_format       = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                base_color_texture_data->m_pixels       = empty_image;
            }

            std::shared_ptr<TextureData> metallic_roughness_texture_data = material_data.m_metallic_roughness_texture;
            if (metallic_roughness_texture_data == nullptr)
            {
                metallic_roughness_texture_data                 = std::make_shared<TextureData>();
                metallic_roughness_texture_data->m_width        = 1;
                metallic_roughness_texture_data->m_height       = 1;
                metallic_roughness_texture_data->m_depth        = 0;
                metallic_roughness_texture_data->m_mip_levels   = 0;
                metallic_roughness_texture_data->m_array_layers = 0;
                metallic_roughness_texture_data->m_is_srgb      = false;
                metallic_roughness_texture_data->m_format       = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
                metallic_roughness_texture_data->m_pixels       = empty_image;
            }

            std::shared_ptr<TextureData> normal_texture_data = material_data.m_normal_texture;
            if (normal_texture_data == nullptr)
            {
                normal_texture_data                 = std::make_shared<TextureData>();
                normal_texture_data->m_width        = 1;
                normal_texture_data->m_height       = 1;
                normal_texture_data->m_depth        = 0;
                normal_texture_data->m_mip_levels   = 0;
                normal_texture_data->m_array_layers = 0;
                normal_texture_data->m_is_srgb      = false;
                normal_texture_data->m_format       = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
                normal_texture_data->m_pixels       = empty_image;
            }

            std::shared_ptr<TextureData> occlusion_texture_data = material_data.m_occlusion_texture;
            if (occlusion_texture_data == nullptr)
            {
                occlusion_texture_data                 = std::make_shared<TextureData>();
                occlusion_texture_data->m_width        = 1;
                occlusion_texture_data->m_height       = 1;
                occlusion_texture_data->m_depth        = 0;
                occlusion_texture_data->m_mip_levels   = 0;
                occlusion_texture_data->m_array_layers = 0;
                occlusion_texture_data->m_is_srgb      = false;
                occlusion_texture_data->m_format       = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
                occlusion_texture_data->m_pixels       = empty_image;
            }

            std::shared_ptr<TextureData> emission_texture_data = material_data.m_emissive_texture;
            if (emission_texture_data == nullptr)
            {
                emission_texture_data                  = std::make_shared<TextureData>();
                emission_texture_data->m_width         = 1;
                emission_texture_data->m_height        = 1;
                emission_texture_data->m_depth         = 0;
                emission_texture_data->m_mip_levels    = 0;
                emission_texture_data->m_array_layers  = 0;
                emission_texture_data->m_is_srgb       = false;
                emission_texture_data->m_format        = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
                emission_texture_data->m_pixels        = empty_image;
            }

            D3D12PBRMaterial& now_material = res.first->second;
            {
                now_material.m_blend        = material_data.m_blend;
                now_material.m_double_sided = material_data.m_double_sided;

                now_material.m_base_color_factor  = material_data.m_base_color_factor;
                now_material.m_metallic_factor    = material_data.m_metallic_factor;
                now_material.m_roughness_factor   = material_data.m_roughness_factor;
                now_material.m_normal_scale       = material_data.m_normal_scale;
                now_material.m_occlusion_strength = material_data.m_occlusion_strength;
                now_material.m_emissive_factor    = material_data.m_emissive_factor;
            }
            {
                startUploadBatch();

                HLSL::MeshPerMaterialUniformBufferObject material_uniform_buffer_info;
                material_uniform_buffer_info.is_blend          = material_data.m_blend;
                material_uniform_buffer_info.is_double_sided   = material_data.m_double_sided;
                material_uniform_buffer_info.baseColorFactor   = GLMUtil::fromVec4(material_data.m_base_color_factor);
                material_uniform_buffer_info.metallicFactor    = material_data.m_metallic_factor;
                material_uniform_buffer_info.roughnessFactor   = material_data.m_roughness_factor;
                material_uniform_buffer_info.normalScale       = material_data.m_normal_scale;
                material_uniform_buffer_info.occlusionStrength = material_data.m_occlusion_strength;
                material_uniform_buffer_info.emissiveFactor    = GLMUtil::fromVec3(material_data.m_emissive_factor);

                uint32_t buffer_size = sizeof(HLSL::MeshPerMaterialUniformBufferObject);
                auto uniform_buffer =
                    createStaticBuffer(&material_uniform_buffer_info, buffer_size, buffer_size, false, false);
                now_material.material_uniform_buffer      = uniform_buffer;

                auto base_color_tex = createTex2D(base_color_texture_data->m_width,
                                                   base_color_texture_data->m_height,
                                                   base_color_texture_data->m_pixels,
                                                   base_color_texture_data->m_format,
                                                   true,
                                                   true,
                                                   false);
                now_material.base_color_texture_image = base_color_tex;

                auto metal_rough_tex = createTex2D(metallic_roughness_texture_data->m_width,
                                                    metallic_roughness_texture_data->m_height,
                                                    metallic_roughness_texture_data->m_pixels,
                                                    metallic_roughness_texture_data->m_format,
                                                    false,
                                                    true,
                                                    false);
                now_material.metallic_roughness_texture_image = metal_rough_tex;

                auto normal_rough_tex = createTex2D(normal_texture_data->m_width,
                                                     normal_texture_data->m_height,
                                                     normal_texture_data->m_pixels,
                                                     normal_texture_data->m_format,
                                                     false,
                                                     true,
                                                     false);
                now_material.normal_texture_image = normal_rough_tex;

                auto occlusion_tex = createTex2D(occlusion_texture_data->m_width,
                                                       occlusion_texture_data->m_height,
                                                       occlusion_texture_data->m_pixels,
                                                       occlusion_texture_data->m_format,
                                                       false,
                                                       true,
                                                       false);
                now_material.occlusion_texture_image = occlusion_tex;

                auto emissive_tex = createTex2D(emission_texture_data->m_width,
                                                      emission_texture_data->m_height,
                                                      emission_texture_data->m_pixels,
                                                      emission_texture_data->m_format,
                                                      false,
                                                      true,
                                                      false);
                now_material.emissive_texture_image = emissive_tex;

                endUploadBatch();
            }

            return now_material;
        }
    }

    void RenderResource::updateMeshData(bool                                   enable_vertex_blending,
                                        uint32_t                               index_buffer_size,
                                        void*                                  index_buffer_data,
                                        uint32_t                               vertex_buffer_size,
                                        MeshVertexDataDefinition const*        vertex_buffer_data,
                                        uint32_t                               joint_binding_buffer_size,
                                        MeshVertexBindingDataDefinition const* joint_binding_buffer_data,
                                        D3D12Mesh&                             now_mesh)
    {
        now_mesh.enable_vertex_blending = enable_vertex_blending;
        assert(0 == (vertex_buffer_size % sizeof(MeshVertexDataDefinition)));
        now_mesh.mesh_vertex_count = vertex_buffer_size / sizeof(MeshVertexDataDefinition);
        updateVertexBuffer(enable_vertex_blending,
                           vertex_buffer_size,
                           vertex_buffer_data,
                           joint_binding_buffer_size,
                           joint_binding_buffer_data,
                           now_mesh);
        assert(0 == (index_buffer_size % sizeof(uint32_t)));
        now_mesh.mesh_index_count = index_buffer_size / sizeof(uint32_t);
        updateIndexBuffer(index_buffer_size, index_buffer_data, now_mesh);
    }

    void RenderResource::updateVertexBuffer(bool                                   enable_vertex_blending,
                                            uint32_t                               vertex_buffer_size,
                                            MeshVertexDataDefinition const*        vertex_buffer_data,
                                            uint32_t                               joint_binding_buffer_size,
                                            MeshVertexBindingDataDefinition const* joint_binding_buffer_data,
                                            D3D12Mesh&                             now_mesh)
    {
        if (enable_vertex_blending)
        {
            assert(0 == (vertex_buffer_size % sizeof(MeshVertexDataDefinition)));
            uint32_t vertex_count = vertex_buffer_size / sizeof(MeshVertexDataDefinition);
            
            uint32_t vertex_buffer_size =
                sizeof(MeshVertex::D3D12MeshVertexPositionNormalTangentTexture) * vertex_count;

            uint32_t vertex_buffer_offset = 0;

            // TODO:还不太理解这个indices和weight的实际作用，先放着，后面再来弄

            /*
            uint32_t vertex_position_buffer_size = sizeof(MeshVertex::D3D12MeshVertexPostition) * vertex_count;
            uint32_t vertex_varying_enable_blending_buffer_size =
                sizeof(MeshVertex::D3D12MeshVertexVaryingEnableBlending) * vertex_count;
            uint32_t vertex_varying_buffer_size       = sizeof(MeshVertex::D3D12MeshVertexVarying) * vertex_count;
            uint32_t vertex_joint_binding_buffer_size = sizeof(MeshVertex::D3D12MeshVertexJointBinding) * index_count;

            uint32_t vertex_position_buffer_offset = 0;
            uint32_t vertex_varying_enable_blending_buffer_offset =
                vertex_position_buffer_offset + vertex_position_buffer_size;
            uint32_t vertex_varying_buffer_offset =
                vertex_varying_enable_blending_buffer_offset + vertex_varying_enable_blending_buffer_size;
            uint32_t vertex_joint_binding_buffer_offset = vertex_varying_buffer_offset + vertex_varying_buffer_size;

            // temporary staging buffer
            uint32_t inefficient_staging_buffer_size =
                vertex_position_buffer_size + vertex_varying_enable_blending_buffer_size + vertex_varying_buffer_size +
                vertex_joint_binding_buffer_size;
            VkBuffer       inefficient_staging_buffer        = VK_NULL_HANDLE;
            VkDeviceMemory inefficient_staging_buffer_memory = VK_NULL_HANDLE;
            VulkanUtil::createBuffer(vulkan_context->m_physical_device,
                                     vulkan_context->m_device,
                                     inefficient_staging_buffer_size,
                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                     inefficient_staging_buffer,
                                     inefficient_staging_buffer_memory);

            void* inefficient_staging_buffer_data;
            vkMapMemory(vulkan_context->m_device,
                        inefficient_staging_buffer_memory,
                        0,
                        VK_WHOLE_SIZE,
                        0,
                        &inefficient_staging_buffer_data);

            MeshVertex::D3D12MeshVertexPostition* mesh_vertex_positions =
                reinterpret_cast<MeshVertex::D3D12MeshVertexPostition*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) + vertex_position_buffer_offset);
            MeshVertex::D3D12MeshVertexVaryingEnableBlending* mesh_vertex_blending_varyings =
                reinterpret_cast<MeshVertex::D3D12MeshVertexVaryingEnableBlending*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) +
                    vertex_varying_enable_blending_buffer_offset);
            MeshVertex::D3D12MeshVertexVarying* mesh_vertex_varyings =
                reinterpret_cast<MeshVertex::D3D12MeshVertexVarying*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) + vertex_varying_buffer_offset);
            MeshVertex::D3D12MeshVertexJointBinding* mesh_vertex_joint_binding =
                reinterpret_cast<MeshVertex::D3D12MeshVertexJointBinding*>(
                    reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data) + vertex_joint_binding_buffer_offset);

            for (uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
            {
                glm::vec3 normal  = glm::vec3(vertex_buffer_data[vertex_index].nx,
                                             vertex_buffer_data[vertex_index].ny,
                                             vertex_buffer_data[vertex_index].nz);
                glm::vec3 tangent = glm::vec3(vertex_buffer_data[vertex_index].tx,
                                              vertex_buffer_data[vertex_index].ty,
                                              vertex_buffer_data[vertex_index].tz);

                mesh_vertex_positions[vertex_index].position = glm::vec3(vertex_buffer_data[vertex_index].x,
                                                                         vertex_buffer_data[vertex_index].y,
                                                                         vertex_buffer_data[vertex_index].z);

                mesh_vertex_blending_varyings[vertex_index].normal  = normal;
                mesh_vertex_blending_varyings[vertex_index].tangent = tangent;

                mesh_vertex_varyings[vertex_index].texcoord =
                    glm::vec2(vertex_buffer_data[vertex_index].u, vertex_buffer_data[vertex_index].v);
            }

            for (uint32_t index_index = 0; index_index < index_count; ++index_index)
            {
                uint32_t vertex_buffer_index = index_buffer_data[index_index];

                // TODO: move to assets loading process

                mesh_vertex_joint_binding[index_index].indices =
                    glm::ivec4(joint_binding_buffer_data[vertex_buffer_index].m_index0,
                               joint_binding_buffer_data[vertex_buffer_index].m_index1,
                               joint_binding_buffer_data[vertex_buffer_index].m_index2,
                               joint_binding_buffer_data[vertex_buffer_index].m_index3);

                float inv_total_weight = joint_binding_buffer_data[vertex_buffer_index].m_weight0 +
                                         joint_binding_buffer_data[vertex_buffer_index].m_weight1 +
                                         joint_binding_buffer_data[vertex_buffer_index].m_weight2 +
                                         joint_binding_buffer_data[vertex_buffer_index].m_weight3;

                inv_total_weight = (inv_total_weight != 0.0) ? 1 / inv_total_weight : 1.0;

                mesh_vertex_joint_binding[index_index].weights =
                    glm::vec4(joint_binding_buffer_data[vertex_buffer_index].m_weight0 * inv_total_weight,
                              joint_binding_buffer_data[vertex_buffer_index].m_weight1 * inv_total_weight,
                              joint_binding_buffer_data[vertex_buffer_index].m_weight2 * inv_total_weight,
                              joint_binding_buffer_data[vertex_buffer_index].m_weight3 * inv_total_weight);
            }

            vkUnmapMemory(vulkan_context->m_device, inefficient_staging_buffer_memory);

            // use the vmaAllocator to allocate asset vertex buffer
            VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

            bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.size  = vertex_position_buffer_size;
            vmaCreateBuffer(vulkan_context->m_assets_allocator,
                            &bufferInfo,
                            &allocInfo,
                            &now_mesh.mesh_vertex_position_buffer,
                            &now_mesh.mesh_vertex_position_buffer_allocation,
                            NULL);
            bufferInfo.size = vertex_varying_enable_blending_buffer_size;
            vmaCreateBuffer(vulkan_context->m_assets_allocator,
                            &bufferInfo,
                            &allocInfo,
                            &now_mesh.mesh_vertex_varying_enable_blending_buffer,
                            &now_mesh.mesh_vertex_varying_enable_blending_buffer_allocation,
                            NULL);
            bufferInfo.size = vertex_varying_buffer_size;
            vmaCreateBuffer(vulkan_context->m_assets_allocator,
                            &bufferInfo,
                            &allocInfo,
                            &now_mesh.mesh_vertex_varying_buffer,
                            &now_mesh.mesh_vertex_varying_buffer_allocation,
                            NULL);

            bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.size  = vertex_joint_binding_buffer_size;
            vmaCreateBuffer(vulkan_context->m_assets_allocator,
                            &bufferInfo,
                            &allocInfo,
                            &now_mesh.mesh_vertex_joint_binding_buffer,
                            &now_mesh.mesh_vertex_joint_binding_buffer_allocation,
                            NULL);

            // use the data from staging buffer
            VulkanUtil::copyBuffer(rhi.get(),
                                   inefficient_staging_buffer,
                                   now_mesh.mesh_vertex_position_buffer,
                                   vertex_position_buffer_offset,
                                   0,
                                   vertex_position_buffer_size);
            VulkanUtil::copyBuffer(rhi.get(),
                                   inefficient_staging_buffer,
                                   now_mesh.mesh_vertex_varying_enable_blending_buffer,
                                   vertex_varying_enable_blending_buffer_offset,
                                   0,
                                   vertex_varying_enable_blending_buffer_size);
            VulkanUtil::copyBuffer(rhi.get(),
                                   inefficient_staging_buffer,
                                   now_mesh.mesh_vertex_varying_buffer,
                                   vertex_varying_buffer_offset,
                                   0,
                                   vertex_varying_buffer_size);
            VulkanUtil::copyBuffer(rhi.get(),
                                   inefficient_staging_buffer,
                                   now_mesh.mesh_vertex_joint_binding_buffer,
                                   vertex_joint_binding_buffer_offset,
                                   0,
                                   vertex_joint_binding_buffer_size);

            // release staging buffer
            vkDestroyBuffer(vulkan_context->m_device, inefficient_staging_buffer, nullptr);
            vkFreeMemory(vulkan_context->m_device, inefficient_staging_buffer_memory, nullptr);

            // update descriptor set
            VkDescriptorSetAllocateInfo mesh_vertex_blending_per_mesh_descriptor_set_alloc_info;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.pNext          = NULL;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.descriptorPool = vulkan_context->m_descriptor_pool;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.descriptorSetCount = 1;
            mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.pSetLayouts        = m_mesh_descriptor_set_layout;

            if (VK_SUCCESS != vkAllocateDescriptorSets(vulkan_context->m_device,
                                                       &mesh_vertex_blending_per_mesh_descriptor_set_alloc_info,
                                                       &now_mesh.mesh_vertex_blending_descriptor_set))
            {
                throw std::runtime_error("allocate mesh vertex blending per mesh descriptor set");
            }

            VkDescriptorBufferInfo mesh_vertex_Joint_binding_storage_buffer_info = {};
            mesh_vertex_Joint_binding_storage_buffer_info.offset                 = 0;
            mesh_vertex_Joint_binding_storage_buffer_info.range                  = vertex_joint_binding_buffer_size;
            mesh_vertex_Joint_binding_storage_buffer_info.buffer = now_mesh.mesh_vertex_joint_binding_buffer;
            assert(mesh_vertex_Joint_binding_storage_buffer_info.range <
                   m_global_render_resource._storage_buffer._max_storage_buffer_range);

            VkDescriptorSet descriptor_set_to_write = now_mesh.mesh_vertex_blending_descriptor_set;

            VkWriteDescriptorSet descriptor_writes[1];

            VkWriteDescriptorSet& mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info =
                descriptor_writes[0];
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.sType =
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.pNext      = NULL;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.dstSet     = descriptor_set_to_write;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.dstBinding = 0;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.dstArrayElement = 0;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.descriptorType =
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.descriptorCount = 1;
            mesh_vertex_blending_vertex_Joint_binding_storage_buffer_write_info.pBufferInfo =
                &mesh_vertex_Joint_binding_storage_buffer_info;

            vkUpdateDescriptorSets(vulkan_context->m_device,
                                   (sizeof(descriptor_writes) / sizeof(descriptor_writes[0])),
                                   descriptor_writes,
                                   0,
                                   NULL);
            */
        }
        else
        {
            assert(0 == (vertex_buffer_size % sizeof(MeshVertexDataDefinition)));
            uint32_t vertex_count = vertex_buffer_size / sizeof(MeshVertexDataDefinition);

            uint32_t inputLayoutStride = sizeof(MeshVertex::D3D12MeshVertexPositionNormalTangentTexture);
            uint32_t vertex_buffer_size = inputLayoutStride * vertex_count;

            uint32_t vertex_buffer_offset = 0;
            
            // temporary staging buffer
            uint32_t inefficient_staging_buffer_size = vertex_buffer_size;

            DirectX::SharedGraphicsResource inefficient_staging_buffer =
                m_GraphicsMemory->Allocate(inefficient_staging_buffer_size);
            void* inefficient_staging_buffer_data = inefficient_staging_buffer.Memory();

            MeshVertex::D3D12MeshVertexPositionNormalTangentTexture* mesh_vertexs =
                reinterpret_cast<MeshVertex::D3D12MeshVertexPositionNormalTangentTexture*>(
                    reinterpret_cast<uint8_t*>(inefficient_staging_buffer_data) + vertex_buffer_offset);
            
            for (uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
            {
                glm::vec3 normal  = glm::vec3(vertex_buffer_data[vertex_index].nx,
                                             vertex_buffer_data[vertex_index].ny,
                                             vertex_buffer_data[vertex_index].nz);
                glm::vec4 tangent = glm::vec4(vertex_buffer_data[vertex_index].tx,
                                              vertex_buffer_data[vertex_index].ty,
                                              vertex_buffer_data[vertex_index].tz,
                                              vertex_buffer_data[vertex_index].tw);

                mesh_vertexs[vertex_index].position = glm::vec3(vertex_buffer_data[vertex_index].x,
                                                                vertex_buffer_data[vertex_index].y,
                                                                vertex_buffer_data[vertex_index].z);

                mesh_vertexs[vertex_index].normal  = normal;
                mesh_vertexs[vertex_index].tangent = tangent;

                mesh_vertexs[vertex_index].texcoord =
                    glm::vec2(vertex_buffer_data[vertex_index].u, vertex_buffer_data[vertex_index].v);
            }

            now_mesh.p_mesh_vertex_buffer =
                RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                         RHI::RHIBufferTargetVertex,
                                         vertex_count,
                                         sizeof(MeshVertexDataDefinition),
                                         L"MeshVertexBuffer",
                                         RHI::RHIBufferModeImmutable,
                                         D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            {
                startUploadBatch();

                {
                    D3D12_RESOURCE_STATES buf_ori_state =
                        now_mesh.p_mesh_vertex_buffer->GetResourceState().GetSubresourceState(0);

                    m_ResourceUpload->Transition(now_mesh.p_mesh_vertex_buffer->GetResource(),
                                                 buf_ori_state,
                                                 D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

                    m_ResourceUpload->Upload(now_mesh.p_mesh_vertex_buffer->GetResource(), inefficient_staging_buffer);

                    m_ResourceUpload->Transition(now_mesh.p_mesh_vertex_buffer->GetResource(),
                                                 D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST,
                                                 buf_ori_state);
                }

                endUploadBatch();
            }

        }
    }

    void RenderResource::updateIndexBuffer(uint32_t index_buffer_size, void* index_buffer_data, D3D12Mesh& now_mesh)
    {
        uint32_t buffer_size = index_buffer_size;

        uint32_t index_count = index_buffer_size / sizeof(uint32_t);

        DirectX::SharedGraphicsResource inefficient_staging_buffer = m_GraphicsMemory->Allocate(buffer_size);

        void* staging_buffer_data = inefficient_staging_buffer.Memory();

        memcpy(staging_buffer_data, index_buffer_data, (size_t)buffer_size);

        now_mesh.p_mesh_index_buffer =
            RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                     RHI::RHIBufferTargetIndex,
                                     index_count,
                                     sizeof(uint32_t),
                                     L"MeshIndexBuffer",
                                     RHI::RHIBufferModeImmutable,
                                     D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER);

        {
            startUploadBatch();

            {
                D3D12_RESOURCE_STATES buf_ori_state =
                    now_mesh.p_mesh_index_buffer->GetResourceState().GetSubresourceState(0);

                m_ResourceUpload->Transition(now_mesh.p_mesh_index_buffer->GetResource(),
                                             buf_ori_state,
                                             D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

                m_ResourceUpload->Upload(now_mesh.p_mesh_index_buffer->GetResource(), inefficient_staging_buffer);

                m_ResourceUpload->Transition(now_mesh.p_mesh_index_buffer->GetResource(),
                                             D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST,
                                             buf_ori_state);
            }

            endUploadBatch();
        }

    }

    D3D12Mesh& RenderResource::getEntityMesh(RenderEntity entity)
    {
        size_t assetid = entity.m_mesh_asset_id;

        auto it = m_d3d12_meshes.find(assetid);
        if (it != m_d3d12_meshes.end())
        {
            return it->second;
        }
        else
        {
            throw std::runtime_error("failed to get entity mesh");
        }
    }

    D3D12PBRMaterial& RenderResource::getEntityMaterial(RenderEntity entity)
    {
        size_t assetid = entity.m_material_asset_id;

        auto it = m_d3d12_pbr_materials.find(assetid);
        if (it != m_d3d12_pbr_materials.end())
        {
            return it->second;
        }
        else
        {
            throw std::runtime_error("failed to get entity material");
        }
    }

    std::size_t RenderResource::hashTexture2D(uint32_t    width,
                                              uint32_t    height,
                                              uint32_t    mip_levels,
                                              void*       pixels_ptr,
                                              DXGI_FORMAT format)
    {
        std::size_t h0 = std::hash<uint32_t> {}(width);
        h0 = (h0 ^ (std::hash<uint32_t> {}(height) << 1));
        h0 = (h0 ^ (std::hash<uint32_t> {}(mip_levels) << 1));
        h0 = (h0 ^ (std::hash<void*> {}(pixels_ptr) << 1));
        h0 = (h0 ^ (std::hash<uint32_t> {}(format) << 1));
        return h0;
    }

    //-------------------------------------------------------------------------------------------
    std::shared_ptr<RHI::D3D12Buffer> RenderResource::createDynamicBuffer(void* buffer_data, uint32_t buffer_size, uint32_t buffer_stride)
    {
        assert(buffer_size % buffer_stride == 0);
        UINT numelement = buffer_size / buffer_stride;
        std::shared_ptr<RHI::D3D12Buffer> dynamicBuffer =
            RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                     RHI::RHIBufferTarget::RHIBufferTargetStructured,
                                     numelement,
                                     buffer_stride,
                                     L"DynamicBuffer",
                                     RHI::RHIBufferModeDynamic);

        void* bufferPtr = dynamicBuffer->GetCpuVirtualAddress<void>();
        memcpy(bufferPtr, buffer_data, buffer_size);
        return dynamicBuffer;
    }

    std::shared_ptr<RHI::D3D12Buffer> RenderResource::createDynamicBuffer(std::shared_ptr<BufferData>& buffer_data)
    {
        return createDynamicBuffer(buffer_data->m_data, buffer_data->m_size, buffer_data->m_size);
    }
    
    std::shared_ptr<RHI::D3D12Buffer> RenderResource::createStaticBuffer(void* buffer_data, uint32_t buffer_size, uint32_t buffer_stride, bool raw, bool batch)
    {
        assert(buffer_size % buffer_stride == 0);

        if (batch)
            this->startUploadBatch();

        std::shared_ptr<RHI::D3D12Buffer> staticBuffer =
            std::make_shared<RHI::D3D12Buffer>(m_Device->GetLinkedDevice(),
                                               buffer_size,
                                               buffer_stride,
                                               D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT,
                                               D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
                                               D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        uint32_t number_elements = buffer_size / buffer_stride;

        D3D12_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
        resourceViewDesc.Format                          = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
        resourceViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_BUFFER;
        resourceViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        resourceViewDesc.Buffer.FirstElement             = 0;
        resourceViewDesc.Buffer.NumElements              = number_elements;
        resourceViewDesc.Buffer.StructureByteStride      = buffer_stride;
        resourceViewDesc.Buffer.Flags                    = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
        if (raw)
        {
            resourceViewDesc.Format                     = DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS;
            resourceViewDesc.Buffer.FirstElement        = 0;
            resourceViewDesc.Buffer.NumElements         = buffer_size / 4;
            resourceViewDesc.Buffer.StructureByteStride = 0;
            resourceViewDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_RAW;
        }

        D3D12_RESOURCE_STATES tex2d_ori_state = staticBuffer->GetResourceState().GetSubresourceState(0);

        m_ResourceUpload->Transition(staticBuffer->GetResource(), tex2d_ori_state, D3D12_RESOURCE_STATE_COPY_DEST);

        D3D12_SUBRESOURCE_DATA resourceInitData = {buffer_data, buffer_size, buffer_size};
        m_ResourceUpload->Upload(staticBuffer->GetResource(), 0, &resourceInitData, 1);

        m_ResourceUpload->Transition(staticBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, tex2d_ori_state);

        if (batch)
            this->endUploadBatch();

        return staticBuffer;
    }

    std::shared_ptr<RHI::D3D12Buffer> RenderResource::createStaticBuffer(std::shared_ptr<BufferData>& buffer_data, bool raw, bool batch)
    {
        return createStaticBuffer(buffer_data->m_data, buffer_data->m_size, buffer_data->m_size, raw, batch);
    }

    std::shared_ptr<RHI::D3D12Texture> RenderResource::createTex2D(uint32_t width, uint32_t height, void* pixels, DXGI_FORMAT format, bool is_srgb, bool genMips, bool batch)
    {

        uint32_t tex2d_miplevels = 1;
        if (genMips)
        {
            tex2d_miplevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
        }

        std::size_t tex_hash = hashTexture2D(width, height, tex2d_miplevels, pixels, format);
        auto tex_iter = _Texture_Caches.find(tex_hash);
        if (tex_iter != _Texture_Caches.end())
        {
            return tex_iter->second;
        }
        else
        {
            if (batch)
                this->startUploadBatch();

            RHI::RHISurfaceCreateFlags texflags = RHI::RHISurfaceCreateFlagNone;
            if (genMips)
                texflags |= RHI::RHISurfaceCreateMipmap;
            if (is_srgb)
                texflags |= RHI::RHISurfaceCreateSRGB;

            std::shared_ptr<RHI::D3D12Texture> tex2d = RHI::D3D12Texture::Create2D(
                m_Device->GetLinkedDevice(), width, height, tex2d_miplevels, format, texflags);

            D3D12_SUBRESOURCE_DATA resourceInitData;
            resourceInitData.pData      = pixels;
            resourceInitData.RowPitch   = width * D3D12RHIUtils::BytesPerPixel(format);
            resourceInitData.SlicePitch = resourceInitData.RowPitch * height;

            D3D12_RESOURCE_STATES tex2d_ori_state = tex2d->GetResourceState().GetSubresourceState(0);

            if (tex2d_ori_state != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                m_ResourceUpload->Transition(tex2d->GetResource(), tex2d_ori_state, D3D12_RESOURCE_STATE_COPY_DEST);
            }

            m_ResourceUpload->Upload(tex2d->GetResource(), 0, &resourceInitData, 1);

            D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COPY_DEST;

            if (genMips)
            {
                currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                m_ResourceUpload->Transition(
                    tex2d->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                m_ResourceUpload->GenerateMips(tex2d->GetResource());
            }

            if (currentState != tex2d_ori_state)
            {
                m_ResourceUpload->Transition(tex2d->GetResource(), currentState, tex2d_ori_state);
            }

            if (batch)
                this->endUploadBatch();

            return tex2d;
        }
    }

    std::shared_ptr<RHI::D3D12Texture> RenderResource::createTex2D(std::shared_ptr<TextureData>& tex2d_data, bool genMips, bool batch)
    {
        return createTex2D(tex2d_data->m_width,
                           tex2d_data->m_height,
                           tex2d_data->m_pixels,
                           tex2d_data->m_format,
                           tex2d_data->m_is_srgb,
                           genMips,
                           batch);
    }
    
    std::shared_ptr<RHI::D3D12Texture> RenderResource::createCubeMap(std::array<std::shared_ptr<TextureData>, 6>& cube_maps, bool genMips, bool batch)
    {
        if (batch)
            this->startUploadBatch();

        // assume all textures have same width, height and format
        uint32_t cubemap_miplevels = 1;
        if (genMips)
            cubemap_miplevels = static_cast<uint32_t>(std::floor(std::log2(std::max(cube_maps[0]->m_width, cube_maps[0]->m_height)))) + 1;

        UINT        width  = cube_maps[0]->m_width;
        UINT        height = cube_maps[0]->m_height;
        DXGI_FORMAT format = cube_maps[0]->m_format;

        RHI::RHISurfaceCreateFlags texflags = RHI::RHISurfaceCreateFlagNone | RHI::RHISurfaceCreateRandomWrite;
        if (genMips)
            texflags |= RHI::RHISurfaceCreateMipmap;
        //if (is_srgb)
        //    texflags |= RHI::RHISurfaceCreateSRGB;

        std::shared_ptr<RHI::D3D12Texture> cube_tex = RHI::D3D12Texture::CreateCubeMap(
            m_Device->GetLinkedDevice(), width, height, cubemap_miplevels, format, texflags);

        D3D12_RESOURCE_STATES tex2d_ori_state = cube_tex->GetResourceState().GetSubresourceState(0);

        m_ResourceUpload->Transition(cube_tex->GetResource(), tex2d_ori_state, D3D12_RESOURCE_STATE_COPY_DEST);

        UINT bytesPerPixel = D3D12RHIUtils::BytesPerPixel(format);

        for (size_t i = 0; i < 6; i++)
        {
            D3D12_SUBRESOURCE_DATA resourceInitData;
            resourceInitData.pData      = cube_maps[i]->m_pixels;
            resourceInitData.RowPitch   = cube_maps[i]->m_width * bytesPerPixel;
            resourceInitData.SlicePitch = resourceInitData.RowPitch * height;

            m_ResourceUpload->Upload(cube_tex->GetResource(), cubemap_miplevels * i, &resourceInitData, 1);

            if (genMips)
                m_ResourceUpload->GenerateMips(cube_tex->GetResource());
        }

        m_ResourceUpload->Transition(cube_tex->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, tex2d_ori_state);

        if (batch)
            this->endUploadBatch();

        return cube_tex;
    }


} // namespace Pilot
