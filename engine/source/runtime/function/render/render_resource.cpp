#include "runtime/function/render/glm_wrapper.h"
#include "runtime/function/render/render_camera.h"

#include "runtime/function/render/render_mesh.h"
#include "runtime/function/render/render_resource.h"

#include "runtime/function/render/rhi/d3d12/d3d12_core.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resource.h"
#include "runtime/function/render/rhi/d3d12/d3d12_descriptor.h"

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
        std::shared_ptr<TextureData> color_grading_map =
            loadTexture(level_resource_desc.m_color_grading_resource_desc.m_color_grading_map);

        startUploadBatch();
        {
            // create irradiance cubemap
            createCubeMap(irradiance_maps,
                          m_global_render_resource._ibl_resource._irradiance_texture_image,
                          m_global_render_resource._ibl_resource._irradiance_texture_image_view);

            // create specular cubemap
            createCubeMap(specular_maps,
                          m_global_render_resource._ibl_resource._specular_texture_image,
                          m_global_render_resource._ibl_resource._specular_texture_image_view);

            // create brdf lut texture
            createTex2D(brdf_map,
                        m_global_render_resource._ibl_resource._brdfLUT_texture_image,
                        m_global_render_resource._ibl_resource._brdfLUT_texture_image_view);

            // create color grading texture
            createTex2D(color_grading_map,
                        m_global_render_resource._color_grading_resource._color_grading_LUT_texture_image,
                        m_global_render_resource._color_grading_resource._color_grading_LUT_texture_image_view);
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
        Vector3  ambient_light   = render_scene->m_ambient_light.m_irradiance;
        uint32_t point_light_num = static_cast<uint32_t>(render_scene->m_point_light_list.m_lights.size());

        // set ubo data
        m_mesh_perframe_storage_buffer_object.cameraInstance   = cameraInstance;
        m_mesh_perframe_storage_buffer_object.ambient_light    = GLMUtil::fromVec3(ambient_light);
        m_mesh_perframe_storage_buffer_object.point_light_num  = point_light_num;

        m_mesh_point_light_shadow_perframe_storage_buffer_object.point_light_num = point_light_num;

        // point lights
        for (uint32_t i = 0; i < point_light_num; i++)
        {
            Vector3 point_light_position = render_scene->m_point_light_list.m_lights[i].m_position;
            Vector3 point_light_intensity =
                render_scene->m_point_light_list.m_lights[i].m_flux / (4.0f * glm::pi<float>());

            float radius = render_scene->m_point_light_list.m_lights[i].calculateRadius();

            m_mesh_perframe_storage_buffer_object.scene_point_lights[i].position =
                GLMUtil::fromVec3(point_light_position);
            m_mesh_perframe_storage_buffer_object.scene_point_lights[i].radius = radius;
            m_mesh_perframe_storage_buffer_object.scene_point_lights[i].intensity =
                GLMUtil::fromVec3(point_light_intensity);

            m_mesh_point_light_shadow_perframe_storage_buffer_object.point_lights_position_and_radius[i] =
                glm::vec4(point_light_position.x, point_light_position.y, point_light_position.z, radius);
        }

        // directional light
        m_mesh_perframe_storage_buffer_object.scene_directional_light.direction =
            GLMUtil::fromVec3(Vector3::normalize(render_scene->m_directional_light.m_direction));
        m_mesh_perframe_storage_buffer_object.scene_directional_light.color =
            GLMUtil::fromVec3(render_scene->m_directional_light.m_color);

        /*
        // pick pass view projection matrix
        m_mesh_inefficient_pick_perframe_storage_buffer_object.proj_view_matrix = proj_view_matrix;

        m_particlebillboard_perframe_storage_buffer_object.proj_view_matrix = proj_view_matrix;
        m_particlebillboard_perframe_storage_buffer_object.eye_position     = GLMUtil::fromVec3(camera_position);
        m_particlebillboard_perframe_storage_buffer_object.up_direction     = GLMUtil::fromVec3(camera->up());
        */
    }

    /*
    void RenderResource::updatePerFrameBuffer(std::shared_ptr<RenderScene>  render_scene,
                                              std::shared_ptr<RenderCamera> camera)
    {
        Matrix4x4 view_matrix      = camera->getViewMatrix();
        Matrix4x4 proj_matrix      = camera->getPersProjMatrix();
        Vector3   camera_position  = camera->position();
        glm::mat4 proj_view_matrix = GLMUtil::fromMat4x4(proj_matrix * view_matrix);

        // ambient light
        Vector3  ambient_light   = render_scene->m_ambient_light.m_irradiance;
        uint32_t point_light_num = static_cast<uint32_t>(render_scene->m_point_light_list.m_lights.size());

        // set ubo data
        m_mesh_perframe_storage_buffer_object.proj_view_matrix = proj_view_matrix;
        m_mesh_perframe_storage_buffer_object.camera_position  = GLMUtil::fromVec3(camera_position);
        m_mesh_perframe_storage_buffer_object.ambient_light    = GLMUtil::fromVec3(ambient_light);
        m_mesh_perframe_storage_buffer_object.point_light_num  = point_light_num;

        m_mesh_point_light_shadow_perframe_storage_buffer_object.point_light_num = point_light_num;

        // point lights
        for (uint32_t i = 0; i < point_light_num; i++)
        {
            Vector3 point_light_position = render_scene->m_point_light_list.m_lights[i].m_position;
            Vector3 point_light_intensity =
                render_scene->m_point_light_list.m_lights[i].m_flux / (4.0f * glm::pi<float>());

            float radius = render_scene->m_point_light_list.m_lights[i].calculateRadius();

            m_mesh_perframe_storage_buffer_object.scene_point_lights[i].position =
                GLMUtil::fromVec3(point_light_position);
            m_mesh_perframe_storage_buffer_object.scene_point_lights[i].radius    = radius;
            m_mesh_perframe_storage_buffer_object.scene_point_lights[i].intensity =
                GLMUtil::fromVec3(point_light_intensity);

            m_mesh_point_light_shadow_perframe_storage_buffer_object.point_lights_position_and_radius[i] =
                Vector4(point_light_position, radius);
        }

        // directional light
        m_mesh_perframe_storage_buffer_object.scene_directional_light.direction =
            GLMUtil::fromVec3(Vector3::normalize(render_scene->m_directional_light.m_direction));
        m_mesh_perframe_storage_buffer_object.scene_directional_light.color =
            GLMUtil::fromVec3(render_scene->m_directional_light.m_color);

        // pick pass view projection matrix
        m_mesh_inefficient_pick_perframe_storage_buffer_object.proj_view_matrix = proj_view_matrix;

        m_particlebillboard_perframe_storage_buffer_object.proj_view_matrix = proj_view_matrix;
        m_particlebillboard_perframe_storage_buffer_object.eye_position     = GLMUtil::fromVec3(camera_position);
        m_particlebillboard_perframe_storage_buffer_object.up_direction     = GLMUtil::fromVec3(camera->up());
    }
    */

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
            auto              res = m_d3d12_pbr_materials.insert(std::make_pair(assetid, std::move(temp)));
            assert(res.second);

            float empty_image[] = {0.5f, 0.5f, 0.5f, 0.5f};

            void*              base_color_image_pixels = empty_image;
            uint32_t           base_color_image_width  = 1;
            uint32_t           base_color_image_height = 1;
            DXGI_FORMAT        base_color_image_format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            if (material_data.m_base_color_texture)
            {
                base_color_image_pixels = material_data.m_base_color_texture->m_pixels;
                base_color_image_width  = static_cast<uint32_t>(material_data.m_base_color_texture->m_width);
                base_color_image_height = static_cast<uint32_t>(material_data.m_base_color_texture->m_height);
                base_color_image_format = material_data.m_base_color_texture->m_format;
            }

            void*              metallic_roughness_image_pixels = empty_image;
            uint32_t           metallic_roughness_width        = 1;
            uint32_t           metallic_roughness_height       = 1;
            DXGI_FORMAT        metallic_roughness_format       = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
            if (material_data.m_metallic_roughness_texture)
            {
                metallic_roughness_image_pixels = material_data.m_metallic_roughness_texture->m_pixels;
                metallic_roughness_width  = static_cast<uint32_t>(material_data.m_metallic_roughness_texture->m_width);
                metallic_roughness_height = static_cast<uint32_t>(material_data.m_metallic_roughness_texture->m_height);
                metallic_roughness_format = material_data.m_metallic_roughness_texture->m_format;
            }

            void*              normal_roughness_image_pixels = empty_image;
            uint32_t           normal_roughness_width        = 1;
            uint32_t           normal_roughness_height       = 1;
            DXGI_FORMAT        normal_roughness_format       = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
            if (material_data.m_normal_texture)
            {
                normal_roughness_image_pixels = material_data.m_normal_texture->m_pixels;
                normal_roughness_width        = static_cast<uint32_t>(material_data.m_normal_texture->m_width);
                normal_roughness_height       = static_cast<uint32_t>(material_data.m_normal_texture->m_height);
                normal_roughness_format       = material_data.m_normal_texture->m_format;
            }

            void*              occlusion_image_pixels = empty_image;
            uint32_t           occlusion_image_width  = 1;
            uint32_t           occlusion_image_height = 1;
            DXGI_FORMAT        occlusion_image_format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
            if (material_data.m_occlusion_texture)
            {
                occlusion_image_pixels = material_data.m_occlusion_texture->m_pixels;
                occlusion_image_width  = static_cast<uint32_t>(material_data.m_occlusion_texture->m_width);
                occlusion_image_height = static_cast<uint32_t>(material_data.m_occlusion_texture->m_height);
                occlusion_image_format = material_data.m_occlusion_texture->m_format;
            }

            void*              emissive_image_pixels = empty_image;
            uint32_t           emissive_image_width  = 1;
            uint32_t           emissive_image_height = 1;
            DXGI_FORMAT        emissive_image_format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
            if (material_data.m_emissive_texture)
            {
                emissive_image_pixels = material_data.m_emissive_texture->m_pixels;
                emissive_image_width  = static_cast<uint32_t>(material_data.m_emissive_texture->m_width);
                emissive_image_height = static_cast<uint32_t>(material_data.m_emissive_texture->m_height);
                emissive_image_format = material_data.m_emissive_texture->m_format;
            }

            D3D12PBRMaterial& now_material = res.first->second;

            MeshPerMaterialUniformBufferObject material_uniform_buffer_info = {};
            material_uniform_buffer_info.is_blend                           = entity.m_blend;
            material_uniform_buffer_info.is_double_sided                    = entity.m_double_sided;
            material_uniform_buffer_info.baseColorFactor                    = entity.m_base_color_factor;
            material_uniform_buffer_info.metallicFactor                     = entity.m_metallic_factor;
            material_uniform_buffer_info.roughnessFactor                    = entity.m_roughness_factor;
            material_uniform_buffer_info.normalScale                        = entity.m_normal_scale;
            material_uniform_buffer_info.occlusionStrength                  = entity.m_occlusion_strength;
            material_uniform_buffer_info.emissiveFactor                     = entity.m_emissive_factor;

            TextureDataToUpdate update_texture_data;
            update_texture_data.base_color_image_pixels         = base_color_image_pixels;
            update_texture_data.base_color_image_width          = base_color_image_width;
            update_texture_data.base_color_image_height         = base_color_image_height;
            update_texture_data.base_color_image_format         = base_color_image_format;
            update_texture_data.metallic_roughness_image_pixels = metallic_roughness_image_pixels;
            update_texture_data.metallic_roughness_image_width  = metallic_roughness_width;
            update_texture_data.metallic_roughness_image_height = metallic_roughness_height;
            update_texture_data.metallic_roughness_image_format = metallic_roughness_format;
            update_texture_data.normal_roughness_image_pixels   = normal_roughness_image_pixels;
            update_texture_data.normal_roughness_image_width    = normal_roughness_width;
            update_texture_data.normal_roughness_image_height   = normal_roughness_height;
            update_texture_data.normal_roughness_image_format   = normal_roughness_format;
            update_texture_data.occlusion_image_pixels          = occlusion_image_pixels;
            update_texture_data.occlusion_image_width           = occlusion_image_width;
            update_texture_data.occlusion_image_height          = occlusion_image_height;
            update_texture_data.occlusion_image_format          = occlusion_image_format;
            update_texture_data.emissive_image_pixels           = emissive_image_pixels;
            update_texture_data.emissive_image_width            = emissive_image_width;
            update_texture_data.emissive_image_height           = emissive_image_height;
            update_texture_data.emissive_image_format           = emissive_image_format;
            update_texture_data.now_material                    = &now_material;

            {
                startUploadBatch();

                uint32_t buffer_size = sizeof(MeshPerMaterialUniformBufferObject);
                createStaticBuffer(&material_uniform_buffer_info,
                                   buffer_size,
                                   buffer_size,
                                   now_material.material_uniform_buffer,
                                   false,
                                   now_material.material_uniform_buffer_view,
                                   false);

                createTex2D(update_texture_data.base_color_image_width,
                            update_texture_data.base_color_image_height,
                            update_texture_data.base_color_image_pixels,
                            update_texture_data.base_color_image_format,
                            true,
                            now_material.base_color_texture_image,
                            now_material.base_color_image_view,
                            true,
                            false);

                createTex2D(update_texture_data.metallic_roughness_image_width,
                            update_texture_data.metallic_roughness_image_height,
                            update_texture_data.metallic_roughness_image_pixels,
                            update_texture_data.metallic_roughness_image_format,
                            false,
                            now_material.metallic_roughness_texture_image,
                            now_material.metallic_roughness_image_view,
                            true,
                            false);

                createTex2D(update_texture_data.normal_roughness_image_width,
                            update_texture_data.normal_roughness_image_height,
                            update_texture_data.normal_roughness_image_pixels,
                            update_texture_data.normal_roughness_image_format,
                            false,
                            now_material.normal_texture_image,
                            now_material.normal_image_view,
                            true,
                            false);

                if (material_data.m_occlusion_texture)
                {
                    createTex2D(update_texture_data.occlusion_image_width,
                                update_texture_data.occlusion_image_height,
                                update_texture_data.occlusion_image_pixels,
                                update_texture_data.occlusion_image_format,
                                false,
                                now_material.occlusion_texture_image,
                                now_material.occlusion_image_view,
                                true,
                                false);
                }
                else
                {
                    // TODO: 设置一个空白的纹理
                }

                if (material_data.m_emissive_texture)
                {
                    createTex2D(update_texture_data.emissive_image_width,
                                update_texture_data.emissive_image_height,
                                update_texture_data.emissive_image_pixels,
                                update_texture_data.emissive_image_format,
                                false,
                                now_material.emissive_texture_image,
                                now_material.emissive_image_view,
                                true,
                                false);
                }
                else
                {
                    // TODO: 设置一个空白的纹理
                }

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
                           index_buffer_size,
                           reinterpret_cast<uint32_t*>(index_buffer_data),
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
                                            uint32_t                               index_buffer_size,
                                            uint32_t*                              index_buffer_data,
                                            D3D12Mesh&                             now_mesh)
    {
        if (enable_vertex_blending)
        {
            assert(0 == (vertex_buffer_size % sizeof(MeshVertexDataDefinition)));
            uint32_t vertex_count = vertex_buffer_size / sizeof(MeshVertexDataDefinition);
            assert(0 == (index_buffer_size % sizeof(uint32_t)));
            uint32_t index_count = index_buffer_size / sizeof(uint32_t);

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

            uint32_t vertex_buffer_size =
                sizeof(MeshVertex::D3D12MeshVertexPositionNormalTangentTexture) * vertex_count;

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

            now_mesh.mesh_vertex_buffer =
                RHI::D3D12Buffer(m_Device->GetLinkedDevice(),
                                 vertex_buffer_size,
                                 vertex_buffer_size,
                                 D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT,
                                 D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
                                 D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            {
                startUploadBatch();

                {
                    D3D12_RESOURCE_STATES buf_ori_state =
                        now_mesh.mesh_vertex_buffer.GetResourceState().GetSubresourceState(0);

                    m_ResourceUpload->Transition(now_mesh.mesh_vertex_buffer.GetResource(),
                                                 buf_ori_state,
                                                 D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

                    m_ResourceUpload->Upload(now_mesh.mesh_vertex_buffer.GetResource(), inefficient_staging_buffer);

                    m_ResourceUpload->Transition(now_mesh.mesh_vertex_buffer.GetResource(),
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

        DirectX::SharedGraphicsResource inefficient_staging_buffer = m_GraphicsMemory->Allocate(buffer_size);

        void* staging_buffer_data = inefficient_staging_buffer.Memory();

        memcpy(staging_buffer_data, index_buffer_data, (size_t)buffer_size);

        now_mesh.mesh_index_buffer = RHI::D3D12Buffer(m_Device->GetLinkedDevice(),
                                                      buffer_size,
                                                      buffer_size,
                                                      D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT,
                                                      D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
                                                      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER);
        {
            startUploadBatch();

            {
                D3D12_RESOURCE_STATES buf_ori_state =
                    now_mesh.mesh_index_buffer.GetResourceState().GetSubresourceState(0);

                m_ResourceUpload->Transition(now_mesh.mesh_index_buffer.GetResource(),
                                             buf_ori_state,
                                             D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

                m_ResourceUpload->Upload(now_mesh.mesh_index_buffer.GetResource(), inefficient_staging_buffer);

                m_ResourceUpload->Transition(now_mesh.mesh_index_buffer.GetResource(),
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

    //-------------------------------------------------------------------------------------------
    void RenderResource::createDynamicBuffer(void*             buffer_data,
                                             uint32_t          buffer_size,
                                             uint32_t          buffer_stride,
                                             RHI::D3D12Buffer& dynamicBuffer)
    {
        assert(buffer_size % buffer_stride == 0);
        dynamicBuffer = RHI::D3D12Buffer(m_Device->GetLinkedDevice(),
                                         buffer_size,
                                         buffer_stride,
                                         D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD,
                                         D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE);
        void* bufferPtr = dynamicBuffer.GetCpuVirtualAddress<void>();
        memcpy(bufferPtr, buffer_data, buffer_size);
    }

    void RenderResource::createDynamicBuffer(std::shared_ptr<BufferData>& buffer_data,
                                             RHI::D3D12Buffer&            dynamicBuffer)
    {
        createDynamicBuffer(buffer_data->m_data, buffer_data->m_size, buffer_data->m_size, dynamicBuffer);
    }
    
    void RenderResource::createStaticBuffer(void*                         buffer_data,
                                            uint32_t                      buffer_size,
                                            uint32_t                      buffer_stride,
                                            RHI::D3D12Buffer&             staticBuffer,
                                            bool                          raw,
                                            RHI::D3D12ShaderResourceView& staticBuffer_view,
                                            bool                          batch)
    {
        assert(buffer_size % buffer_stride == 0);

        if (batch)
            this->startUploadBatch();

        staticBuffer = RHI::D3D12Buffer(m_Device->GetLinkedDevice(),
                                        buffer_size,
                                        buffer_stride,
                                        D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT,
                                        D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
                                        D3D12_RESOURCE_STATE_COPY_DEST);

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
            resourceViewDesc.Buffer.NumElements         = number_elements / 4;
            resourceViewDesc.Buffer.StructureByteStride = 0;
            resourceViewDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_RAW;
        }

        staticBuffer_view = RHI::D3D12ShaderResourceView(m_Device->GetLinkedDevice(), resourceViewDesc, &staticBuffer);

        m_ResourceUpload->Transition(staticBuffer.GetResource(),
                                     staticBuffer.GetResourceState().GetSubresourceState(0),
                                     D3D12_RESOURCE_STATE_COPY_DEST);

        D3D12_SUBRESOURCE_DATA resourceInitData = {buffer_data, buffer_size, buffer_size};
        m_ResourceUpload->Upload(staticBuffer.GetResource(), 0, &resourceInitData, 1);

        m_ResourceUpload->Transition(
            staticBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        if (batch)
            this->endUploadBatch();
    }

    void RenderResource::createStaticBuffer(std::shared_ptr<BufferData>&  buffer_data,
                                            RHI::D3D12Buffer&             staticBuffer,
                                            bool                          raw,
                                            RHI::D3D12ShaderResourceView& staticBuffer_view,
                                            bool                          batch)
    {
        createStaticBuffer(
            buffer_data->m_data, buffer_data->m_size, buffer_data->m_size, staticBuffer, raw, staticBuffer_view, batch);
    }

    void RenderResource::createTex2D(uint32_t                      width,
                                     uint32_t                      height,
                                     void*                         pixels,
                                     DXGI_FORMAT                   format,
                                     bool                          is_srgb,
                                     RHI::D3D12Texture&            tex2d,
                                     RHI::D3D12ShaderResourceView& tex2d_view,
                                     bool                          genMips,
                                     bool                          batch)
    {
        if (batch)
            this->startUploadBatch();

        uint32_t tex2d_miplevels = 1;
        if (genMips)
            tex2d_miplevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

        D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE/* | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS*/;
        D3D12_RESOURCE_DESC  resourceDesc =
            CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, tex2d_miplevels, 1, 0, resourceFlags);

        tex2d = RHI::D3D12Texture(m_Device->GetLinkedDevice(), resourceDesc, std::nullopt, false);

        D3D12_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
        resourceViewDesc.Format        = is_srgb ? D3D12RHIUtils::MakeSRGB(resourceDesc.Format) : resourceDesc.Format;
        resourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        resourceViewDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        resourceViewDesc.Texture2D.MostDetailedMip     = 0;
        resourceViewDesc.Texture2D.MipLevels           = resourceDesc.MipLevels;
        resourceViewDesc.Texture2D.PlaneSlice          = 0;
        resourceViewDesc.Texture2D.ResourceMinLODClamp = 0;

        tex2d_view = RHI::D3D12ShaderResourceView(m_Device->GetLinkedDevice(), resourceViewDesc, &tex2d);

        size_t RowPitchBytes = width * D3D12RHIUtils::BytesPerPixel(format);

        D3D12_SUBRESOURCE_DATA resourceInitData;
        resourceInitData.pData      = pixels;
        resourceInitData.RowPitch   = resourceDesc.Width * D3D12RHIUtils::BytesPerPixel(format);
        resourceInitData.SlicePitch = resourceInitData.RowPitch * resourceDesc.Height;

        D3D12_RESOURCE_STATES tex2d_ori_state = tex2d.GetResourceState().GetSubresourceState(0);

        m_ResourceUpload->Transition(tex2d.GetResource(), tex2d_ori_state, D3D12_RESOURCE_STATE_COPY_DEST);

        m_ResourceUpload->Upload(tex2d.GetResource(), 0, &resourceInitData, 1);

        m_ResourceUpload->Transition(
            tex2d.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        if (genMips)
            m_ResourceUpload->GenerateMips(tex2d.GetResource());

        m_ResourceUpload->Transition(tex2d.GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, tex2d_ori_state);

        if (batch)
            this->endUploadBatch();
    }

    void RenderResource::createTex2D(std::shared_ptr<TextureData>& tex2d_data,
                                     RHI::D3D12Texture&            tex2d,
                                     RHI::D3D12ShaderResourceView& tex2d_view,
                                     bool                          genMips,
                                     bool                          batch)
    {
        createTex2D(tex2d_data->m_width,
                    tex2d_data->m_height,
                    tex2d_data->m_pixels,
                    tex2d_data->m_format,
                    tex2d_data->m_is_srgb,
                    tex2d,
                    tex2d_view,
                    genMips,
                    batch);
    }


    void RenderResource::createCubeMap(std::array<std::shared_ptr<TextureData>, 6>& cube_maps,
                                       RHI::D3D12Texture&                           cube_tex,
                                       RHI::D3D12ShaderResourceView&                cube_tex_view,
                                       bool                                         genMips,
                                       bool                                         batch)
    {
        if (batch)
            this->startUploadBatch();

        // assume all textures have same width, height and format
        uint32_t cubemap_miplevels = 1;
        if (genMips)
            cubemap_miplevels =
                static_cast<uint32_t>(std::floor(std::log2(std::max(cube_maps[0]->m_width, cube_maps[0]->m_height)))) +
                1;

        D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        D3D12_RESOURCE_DESC  resourceDesc  = CD3DX12_RESOURCE_DESC::Tex2D(cube_maps[0]->m_format,
                                                                        cube_maps[0]->m_width,
                                                                        cube_maps[0]->m_height,
                                                                        6,
                                                                        cubemap_miplevels,
                                                                        1,
                                                                        0,
                                                                        resourceFlags);

        cube_tex = RHI::D3D12Texture(m_Device->GetLinkedDevice(), resourceDesc, std::nullopt, true);

        D3D12_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
        resourceViewDesc.Format                          = resourceDesc.Format;
        resourceViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        resourceViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        resourceViewDesc.TextureCube.MostDetailedMip     = 0;
        resourceViewDesc.TextureCube.MipLevels           = resourceDesc.MipLevels;
        resourceViewDesc.TextureCube.ResourceMinLODClamp = 0;

        cube_tex_view = RHI::D3D12ShaderResourceView(m_Device->GetLinkedDevice(), resourceViewDesc, &cube_tex);

        D3D12_RESOURCE_STATES tex2d_ori_state = cube_tex.GetResourceState().GetSubresourceState(0);

        m_ResourceUpload->Transition(cube_tex.GetResource(), tex2d_ori_state, D3D12_RESOURCE_STATE_COPY_DEST);

        UINT bytesPerPixel = D3D12RHIUtils::BytesPerPixel(resourceViewDesc.Format);

        for (size_t i = 0; i < 6; i++)
        {
            D3D12_SUBRESOURCE_DATA resourceInitData;
            resourceInitData.pData      = cube_maps[i]->m_pixels;
            resourceInitData.RowPitch   = cube_maps[i]->m_width * bytesPerPixel;
            resourceInitData.SlicePitch = resourceInitData.RowPitch * resourceDesc.Height;

            m_ResourceUpload->Upload(cube_tex.GetResource(), cubemap_miplevels * i, &resourceInitData, 1);

            if (genMips)
                m_ResourceUpload->GenerateMips(cube_tex.GetResource());
        }

        m_ResourceUpload->Transition(cube_tex.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, tex2d_ori_state);

        if (batch)
            this->endUploadBatch();
    }


} // namespace Pilot
