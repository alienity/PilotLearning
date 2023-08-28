#include "runtime/function/render/glm_wrapper.h"
#include "runtime/function/render/render_camera.h"

#include "runtime/function/render/render_mesh_loader.h"
#include "runtime/function/render/render_resource.h"

#include "runtime/function/render/rhi/d3d12/d3d12_core.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resource.h"
#include "runtime/function/render/rhi/d3d12/d3d12_descriptor.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsMemory.h"

#include "runtime/function/render/rhi/hlsl_data_types.h"
#include "runtime/core/base/macro.h"

#include "runtime/platform/path/path.h"
#include "runtime/platform/system/systemtime.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include <stdexcept>

namespace MoYu
{
    /*
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
        
        endUploadBatch();
    }
    */
    bool RenderResource::updateGlobalRenderResource(RenderScene* m_render_scene, GlobalRenderingRes level_resource_desc)
    {
        // ambient light
        m_render_scene->m_ambient_light.m_color = level_resource_desc.m_ambient_light;

        // direction light
        m_render_scene->m_directional_light.m_direction = level_resource_desc.m_directional_light.m_direction;
        m_render_scene->m_directional_light.m_color     = level_resource_desc.m_directional_light.m_color;

        // sky box irradiance
        MoYu::CubeMap skybox_irradiance_map = level_resource_desc.m_skybox_irradiance_map;
        std::shared_ptr<TextureData> irradiace_pos_x_map = loadTextureHDR(skybox_irradiance_map.m_positive_x_map);
        std::shared_ptr<TextureData> irradiace_neg_x_map = loadTextureHDR(skybox_irradiance_map.m_negative_x_map);
        std::shared_ptr<TextureData> irradiace_pos_y_map = loadTextureHDR(skybox_irradiance_map.m_positive_y_map);
        std::shared_ptr<TextureData> irradiace_neg_y_map = loadTextureHDR(skybox_irradiance_map.m_negative_y_map);
        std::shared_ptr<TextureData> irradiace_pos_z_map = loadTextureHDR(skybox_irradiance_map.m_positive_z_map);
        std::shared_ptr<TextureData> irradiace_neg_z_map = loadTextureHDR(skybox_irradiance_map.m_negative_z_map);

        // sky box specular
        MoYu::CubeMap skybox_specular_map = level_resource_desc.m_skybox_specular_map;
        std::shared_ptr<TextureData> specular_pos_x_map = loadTextureHDR(skybox_specular_map.m_positive_x_map);
        std::shared_ptr<TextureData> specular_neg_x_map = loadTextureHDR(skybox_specular_map.m_negative_x_map);
        std::shared_ptr<TextureData> specular_pos_y_map = loadTextureHDR(skybox_specular_map.m_positive_y_map);
        std::shared_ptr<TextureData> specular_neg_y_map = loadTextureHDR(skybox_specular_map.m_negative_y_map);
        std::shared_ptr<TextureData> specular_pos_z_map = loadTextureHDR(skybox_specular_map.m_positive_z_map);
        std::shared_ptr<TextureData> specular_neg_z_map = loadTextureHDR(skybox_specular_map.m_negative_z_map);

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

        startUploadBatch();
        {
            // create irradiance cubemap
            auto irridiance_tex = createCubeMap(irradiance_maps, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, true, false, false);
            m_render_scene->m_skybox_map.m_skybox_irradiance_map = irridiance_tex;

            // create specular cubemap
            auto specular_tex = createCubeMap(specular_maps, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, false, false, false);
            m_render_scene->m_skybox_map.m_skybox_specular_map = specular_tex;
        }
        endUploadBatch();

        return true;
    }

    void RenderResource::updateFrameUniforms(RenderScene* render_scene, RenderCamera* camera)
    {
        Matrix4x4 view_matrix     = camera->getViewMatrix();
        Matrix4x4 proj_matrix     = camera->getPersProjMatrix();
        Vector3   camera_position = camera->position();
        
        // FrameUniforms
        HLSL::FrameUniforms* _frameUniforms = &m_FrameUniforms;

        // Camera Uniform
        HLSL::CameraUniform _cameraUniform;
        _cameraUniform.viewFromWorldMatrix = GLMUtil::fromMat4x4(view_matrix);
        _cameraUniform.worldFromViewMatrix = glm::inverse(_cameraUniform.viewFromWorldMatrix);
        _cameraUniform.clipFromViewMatrix  = GLMUtil::fromMat4x4(proj_matrix);
        _cameraUniform.viewFromClipMatrix  = glm::inverse(_cameraUniform.clipFromViewMatrix);
        _cameraUniform.clipFromWorldMatrix = GLMUtil::fromMat4x4(proj_matrix * view_matrix);
        _cameraUniform.worldFromClipMatrix = glm::inverse(_cameraUniform.clipFromWorldMatrix);
        _cameraUniform.clipTransform       = HLSL::float4(1, 1, 1, 1);
        _cameraUniform.cameraPosition      = GLMUtil::fromVec3(camera_position);

        _cameraUniform.resolution = HLSL::float4(camera->m_width, camera->m_height, 1.0f / camera->m_width, 1.0f / camera->m_height);
        _cameraUniform.logicalViewportScale = HLSL::float2(1.0f, 1.0f);
        _cameraUniform.logicalViewportOffset = HLSL::float2(0.0f, 0.0f);

        _cameraUniform.cameraNear = camera->m_znear;
        _cameraUniform.cameraFar = camera->m_zfar;

        _cameraUniform.exposure = 1.0f;
        _cameraUniform.ev100 = 1.0f;

        _frameUniforms->cameraUniform = _cameraUniform;


        // Base Uniform
        HLSL::BaseUniform _baseUniform;
        _baseUniform.time     = g_SystemTime.GetTimeSecs();
        _baseUniform.userTime = HLSL::float4(g_SystemTime.GetDeltaTimeSecs(), 0, 0, 0);
        _baseUniform.lodBias  = 0;

        _frameUniforms->baseUniform = _baseUniform;


        HLSL::MeshUniform _meshUniform;
        _meshUniform.totalMeshCount = render_scene->m_mesh_renderers.size();

        _frameUniforms->meshUniform = _meshUniform;

        HLSL::DirectionalLightStruct _directionalLightStruct;
        _directionalLightStruct.lightColorIntensity =
            HLSL::float4(GLMUtil::fromVec3(render_scene->m_directional_light.m_color.toVector3()), render_scene->m_directional_light.m_intensity);
        _directionalLightStruct.lightPosition = GLMUtil::fromVec3(render_scene->m_directional_light.m_position);
        _directionalLightStruct.lightRadius = 1.0f;
        _directionalLightStruct.lightDirection =
            GLMUtil::fromVec3(Vector3::normalize(render_scene->m_directional_light.m_direction));
        _directionalLightStruct.useShadowmap = render_scene->m_directional_light.m_shadowmap ? 1 : 0;

        HLSL::DirectionalLightShadowmap _directionalLightShadowmap;
        _directionalLightShadowmap.cascadeCount = render_scene->m_directional_light.m_cascade;
        _directionalLightShadowmap.shadowmap_width = render_scene->m_directional_light.m_shadowmap_size.x;
        _directionalLightShadowmap.shadowmap_height = render_scene->m_directional_light.m_shadowmap_size.x;
        _directionalLightShadowmap.light_view_matrix =
            GLMUtil::fromMat4x4(render_scene->m_directional_light.m_shadow_view_mat);
        for (size_t i = 0; i < render_scene->m_directional_light.m_cascade; i++)
        {
            _directionalLightShadowmap.shadow_bounds[i] = (int)render_scene->m_directional_light.m_shadow_bounds.x << i;
            _directionalLightShadowmap.light_proj_matrix[i] =
                GLMUtil::fromMat4x4(render_scene->m_directional_light.m_shadow_proj_mats[i]);
            _directionalLightShadowmap.light_proj_view_matrix[i] =
                GLMUtil::fromMat4x4(render_scene->m_directional_light.m_shadow_view_proj_mats[i]);
        }
        _directionalLightStruct.directionalLightShadowmap = _directionalLightShadowmap;

        _frameUniforms->directionalLight = _directionalLightStruct;


        HLSL::PointLightUniform _pointLightUniform;
        _pointLightUniform.pointLightCounts = render_scene->m_point_light_list.size();
        for (uint32_t i = 0; i < render_scene->m_point_light_list.size(); i++)
        {
            Vector3 point_light_position  = render_scene->m_point_light_list[i].m_position;
            float   point_light_intensity = render_scene->m_point_light_list[i].m_intensity;

            float radius = render_scene->m_point_light_list[i].m_radius;
            Color color  = render_scene->m_point_light_list[i].m_color;

            HLSL::PointLightStruct _pointLightStruct;
            _pointLightStruct.lightPosition = GLMUtil::fromVec3(point_light_position);
            _pointLightStruct.lightRadius = radius;
            _pointLightStruct.lightIntensity =
                HLSL::float4(GLMUtil::fromVec3(Vector3(color.r, color.g, color.b)), point_light_intensity);

            _pointLightUniform.pointLightStructs[i] = _pointLightStruct;
        }
        _frameUniforms->pointLightUniform = _pointLightUniform;


        HLSL::SpotLightUniform _spotLightUniform;
        _spotLightUniform.spotLightCounts = render_scene->m_spot_light_list.size();
        for (uint32_t i = 0; i < render_scene->m_spot_light_list.size(); i++)
        {
            Vector3 spot_light_position  = render_scene->m_spot_light_list[i].m_position;
            Vector3 spot_light_direction = render_scene->m_spot_light_list[i].m_direction;
            float   spot_light_intensity = render_scene->m_spot_light_list[i].m_intensity;

            float radius = render_scene->m_spot_light_list[i].m_radius;
            Color color  = render_scene->m_spot_light_list[i].m_color;

            HLSL::SpotLightStruct _spotLightStruct;
            _spotLightStruct.lightPosition = GLMUtil::fromVec3(spot_light_position);
            _spotLightStruct.lightRadius   = radius;
            _spotLightStruct.lightIntensity =
                HLSL::float4(GLMUtil::fromVec3(Vector3(color.r, color.g, color.b)), spot_light_intensity);
            _spotLightStruct.lightDirection = GLMUtil::fromVec3(spot_light_direction);
            _spotLightStruct.inner_radians  = Math::degreesToRadians(render_scene->m_spot_light_list[i].m_inner_degree);
            _spotLightStruct.outer_radians  = Math::degreesToRadians(render_scene->m_spot_light_list[i].m_outer_degree);
            _spotLightStruct.useShadowmap   = render_scene->m_spot_light_list[i].m_shadowmap;

            HLSL::SpotLightSgadowmap _spotLightSgadowmap;
            _spotLightSgadowmap.shadowmap_width = render_scene->m_spot_light_list[i].m_shadowmap_size.x;
            _spotLightSgadowmap.light_proj_view =
                GLMUtil::fromMat4x4(render_scene->m_spot_light_list[i].m_shadow_view_proj_mat);

            _spotLightStruct.spotLightShadowmap = _spotLightSgadowmap;

            _spotLightUniform.scene_spot_lights[i] = _spotLightStruct;
        }
        _frameUniforms->spotLightUniform = _spotLightUniform;
    }

    bool RenderResource::updateInternalMeshRenderer(SceneMeshRenderer scene_mesh_renderer, SceneMeshRenderer& cached_mesh_renderer, InternalMeshRenderer& internal_mesh_renderer, bool has_initialized)
    {
        updateInternalMesh(scene_mesh_renderer.m_scene_mesh, cached_mesh_renderer.m_scene_mesh, internal_mesh_renderer.ref_mesh, has_initialized);
        updateInternalMaterial(scene_mesh_renderer.m_material, cached_mesh_renderer.m_material, internal_mesh_renderer.ref_material, has_initialized);

        cached_mesh_renderer = scene_mesh_renderer;

        return true;
    }

    std::shared_ptr<RHI::D3D12Texture> RenderResource::SceneImageToTexture(const SceneImage& _sceneimage)
    {
        uint32_t    m_width    = 1;
        uint32_t    m_height   = 1;
        void*       m_pixels   = empty_image;
        DXGI_FORMAT m_format   = _sceneimage.m_is_srgb ? DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
        bool        m_is_srgb  = _sceneimage.m_is_srgb;
        bool        m_gen_mips = _sceneimage.m_auto_mips;

        if (_sceneimage.m_image_file != "")
        {
            std::shared_ptr<TextureData> _image_data = loadTexture(_sceneimage.m_image_file, 4);
            m_width  = _image_data->m_width;
            m_height = _image_data->m_height;
            m_pixels = _image_data->m_pixels;
        }

        return createTex2D(m_width, m_height, m_pixels, m_format, m_is_srgb, m_gen_mips, false);
    }

    bool RenderResource::updateInternalMaterial(SceneMaterial scene_material, SceneMaterial& cached_material, InternalMaterial& internal_material, bool has_initialized)
    {
        internal_material.m_shader_name = scene_material.m_shader_name;

        ScenePBRMaterial m_mat_data = scene_material.m_mat_data;
        ScenePBRMaterial m_cached_mat_data = cached_material.m_mat_data;

        this->startUploadBatch();

        InternalPBRMaterial& now_material = internal_material.m_intenral_pbr_mat;
        {
            now_material.m_blend        = m_mat_data.m_blend;
            now_material.m_double_sided = m_mat_data.m_double_sided;
        }
        {
            bool is_uniform_same = m_mat_data.m_blend == m_cached_mat_data.m_blend;
            is_uniform_same &= m_mat_data.m_double_sided == m_cached_mat_data.m_double_sided;
            is_uniform_same &= m_mat_data.m_base_color_factor == m_cached_mat_data.m_base_color_factor;
            is_uniform_same &= m_mat_data.m_metallic_factor == m_cached_mat_data.m_metallic_factor;
            is_uniform_same &= m_mat_data.m_roughness_factor == m_cached_mat_data.m_roughness_factor;
            is_uniform_same &= m_mat_data.m_normal_scale == m_cached_mat_data.m_normal_scale;
            is_uniform_same &= m_mat_data.m_occlusion_strength == m_cached_mat_data.m_occlusion_strength;
            is_uniform_same &= m_mat_data.m_emissive_factor == m_cached_mat_data.m_emissive_factor;
            is_uniform_same &= m_mat_data.m_base_color_texture_file.m_tilling == m_cached_mat_data.m_base_color_texture_file.m_tilling;
            is_uniform_same &= m_mat_data.m_metallic_roughness_texture_file.m_tilling == m_cached_mat_data.m_metallic_roughness_texture_file.m_tilling;
            is_uniform_same &= m_mat_data.m_normal_texture_file.m_tilling == m_cached_mat_data.m_normal_texture_file.m_tilling;
            is_uniform_same &= m_mat_data.m_occlusion_texture_file.m_tilling == m_cached_mat_data.m_occlusion_texture_file.m_tilling;
            is_uniform_same &= m_mat_data.m_emissive_texture_file.m_tilling == m_cached_mat_data.m_emissive_texture_file.m_tilling;

            if (!is_uniform_same || !has_initialized)
            {
                HLSL::PerMaterialParametersBuffer material_uniform_buffer_info;
                material_uniform_buffer_info.is_blend          = m_mat_data.m_blend;
                material_uniform_buffer_info.is_double_sided   = m_mat_data.m_double_sided;
                material_uniform_buffer_info.baseColorFactor   = GLMUtil::fromVec4(m_mat_data.m_base_color_factor);
                material_uniform_buffer_info.metallicFactor    = m_mat_data.m_metallic_factor;
                material_uniform_buffer_info.roughnessFactor   = m_mat_data.m_roughness_factor;
                material_uniform_buffer_info.reflectanceFactor = 1.0f;

                material_uniform_buffer_info.normalScale       = m_mat_data.m_normal_scale;
                material_uniform_buffer_info.occlusionStrength = m_mat_data.m_occlusion_strength;
                material_uniform_buffer_info.emissiveFactor    = m_mat_data.m_emissive_factor.x;

                material_uniform_buffer_info.base_color_tilling = GLMUtil::fromVec2(m_mat_data.m_base_color_texture_file.m_tilling);
                material_uniform_buffer_info.metallic_roughness_tilling = GLMUtil::fromVec2(m_mat_data.m_metallic_roughness_texture_file.m_tilling);
                material_uniform_buffer_info.normal_tilling     = GLMUtil::fromVec2(m_mat_data.m_normal_texture_file.m_tilling);
                material_uniform_buffer_info.occlusion_tilling  = GLMUtil::fromVec2(m_mat_data.m_occlusion_texture_file.m_tilling);
                material_uniform_buffer_info.emissive_tilling   = GLMUtil::fromVec2(m_mat_data.m_emissive_texture_file.m_tilling);

                if (now_material.material_uniform_buffer == nullptr)
                {
                    uint32_t buffer_size = sizeof(HLSL::PerMaterialParametersBuffer);
                    auto uniform_buffer = createStaticBuffer(&material_uniform_buffer_info, buffer_size, buffer_size, false, false);
                    now_material.material_uniform_buffer = uniform_buffer;
                }
                else
                {
                    now_material.material_uniform_buffer->InflateBuffer((BYTE*)&material_uniform_buffer_info, sizeof(HLSL::PerMaterialParametersBuffer));
                }
            }
        }
        {
            //if (!IsImageSame(m_base_color_texture_file) || !has_initialized)
            if (!(m_mat_data.m_base_color_texture_file.m_image == m_cached_mat_data.m_base_color_texture_file.m_image) || !has_initialized)
            {
                SceneImage base_color_image = m_mat_data.m_base_color_texture_file.m_image;
                if (now_material.base_color_texture_image != nullptr)
                    now_material.base_color_texture_image = nullptr;
                now_material.base_color_texture_image = SceneImageToTexture(base_color_image);
            }
        }
        {
            if (!(m_mat_data.m_metallic_roughness_texture_file.m_image == m_cached_mat_data.m_metallic_roughness_texture_file.m_image) || !has_initialized)
            {
                SceneImage metallic_roughness_image = m_mat_data.m_metallic_roughness_texture_file.m_image;
                if (now_material.metallic_roughness_texture_image != nullptr)
                    now_material.metallic_roughness_texture_image = nullptr;
                now_material.metallic_roughness_texture_image = SceneImageToTexture(metallic_roughness_image);
            }
        }
        {
            if (!(m_mat_data.m_normal_texture_file.m_image == m_cached_mat_data.m_normal_texture_file.m_image) || !has_initialized)
            {
                SceneImage normal_image = m_mat_data.m_normal_texture_file.m_image;
                if (now_material.normal_texture_image != nullptr)
                    now_material.normal_texture_image = nullptr;
                now_material.normal_texture_image = SceneImageToTexture(normal_image);
            }
        }
        {
            if (!(m_mat_data.m_occlusion_texture_file.m_image == m_cached_mat_data.m_occlusion_texture_file.m_image) || !has_initialized)
            {
                SceneImage occlusion_image = m_mat_data.m_occlusion_texture_file.m_image;
                if (now_material.occlusion_texture_image != nullptr)
                    now_material.occlusion_texture_image = nullptr;
                now_material.occlusion_texture_image = SceneImageToTexture(occlusion_image);
            }
        }
        {
            if (!(m_mat_data.m_emissive_texture_file.m_image == m_cached_mat_data.m_emissive_texture_file.m_image) || !has_initialized)
            {
                SceneImage emissive_image = m_mat_data.m_emissive_texture_file.m_image;
                if (now_material.emissive_texture_image != nullptr)
                    now_material.emissive_texture_image = nullptr;
                now_material.emissive_texture_image = SceneImageToTexture(emissive_image);
            }
        }

        this->endUploadBatch();

        internal_material.m_intenral_pbr_mat = now_material;

        cached_material = scene_material;

        return true;
    }

    bool RenderResource::updateInternalMesh(SceneMesh scene_mesh, SceneMesh& cached_mesh, InternalMesh& internal_mesh, bool has_initialized)
    {
        bool is_scene_mesh_same = scene_mesh.m_is_mesh_data == cached_mesh.m_is_mesh_data;
        is_scene_mesh_same &= scene_mesh.m_sub_mesh_file == cached_mesh.m_sub_mesh_file;
        is_scene_mesh_same &= scene_mesh.m_mesh_data_path == cached_mesh.m_mesh_data_path;

        if (!is_scene_mesh_same || !has_initialized)
        {
            internal_mesh = createInternalMesh(scene_mesh);
        }

        cached_mesh = scene_mesh;

        return true;
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

        uint32_t number_elements = buffer_size / buffer_stride;

        RHI::RHIBufferTarget bufferTarget = raw ? RHI::RHIBufferTargetRaw : RHI::RHIBufferTargetStructured;

        std::shared_ptr<RHI::D3D12Buffer> staticBuffer =
            RHI::D3D12Buffer::Create(m_Device->GetLinkedDevice(),
                                     bufferTarget,
                                     number_elements,
                                     buffer_stride,
                                     L"StaticBuffer",
                                     RHI::RHIBufferModeImmutable,
                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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

        if (batch)
            this->startUploadBatch();

        RHI::RHISurfaceCreateFlags texflags = RHI::RHISurfaceCreateFlagNone;
        if (genMips)
            texflags |= RHI::RHISurfaceCreateMipmap;
        if (is_srgb)
            texflags |= RHI::RHISurfaceCreateSRGB;

        std::shared_ptr<RHI::D3D12Texture> tex2d =
            RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(), width, height, tex2d_miplevels, format, texflags);

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

    std::shared_ptr<RHI::D3D12Texture> RenderResource::createTex2D(std::shared_ptr<TextureData>& tex2d_data, DXGI_FORMAT format, bool is_srgb, bool genMips, bool batch)
    {
        return createTex2D(tex2d_data->m_width, tex2d_data->m_height, tex2d_data->m_pixels, format, is_srgb, genMips, batch);
    }
    
    std::shared_ptr<RHI::D3D12Texture> RenderResource::createCubeMap(std::array<std::shared_ptr<TextureData>, 6>& cube_maps, DXGI_FORMAT format, bool is_srgb, bool genMips, bool batch)
    {
        if (batch)
            this->startUploadBatch();

        // assume all textures have same width, height and format
        uint32_t cubemap_miplevels = 1;
        if (genMips)
            cubemap_miplevels = static_cast<uint32_t>(std::floor(std::log2(std::max(cube_maps[0]->m_width, cube_maps[0]->m_height)))) + 1;

        UINT width  = cube_maps[0]->m_width;
        UINT height = cube_maps[0]->m_height;
        
        RHI::RHISurfaceCreateFlags texflags = RHI::RHISurfaceCreateFlagNone | RHI::RHISurfaceCreateRandomWrite;
        if (genMips)
            texflags |= RHI::RHISurfaceCreateMipmap;
        if (is_srgb)
            texflags |= RHI::RHISurfaceCreateSRGB;

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

    InternalMesh RenderResource::createInternalMesh(SceneMesh scene_mesh)
    {
        std::filesystem::path root_folder = g_runtime_global_context.m_config_manager->getRootFolder();

        RenderMeshData render_mesh_data = {};
        if (scene_mesh.m_is_mesh_data)
        {
            const std::filesystem::path& relative_path = std::filesystem::absolute(root_folder / scene_mesh.m_mesh_data_path);
            render_mesh_data = loadMeshData(relative_path.string());
        }
        else
        {
            const std::filesystem::path& relative_path = std::filesystem::absolute(root_folder / scene_mesh.m_sub_mesh_file);
            render_mesh_data = loadMeshData(relative_path.string());
        }

        return createInternalMesh(render_mesh_data);
    }

    InternalMesh RenderResource::createInternalMesh(RenderMeshData mesh_data)
    {
        InternalVertexBuffer vertex_buffer = createVertexBuffer(mesh_data.m_static_mesh_data.m_InputElementDefinition,
                                                                mesh_data.m_static_mesh_data.m_vertex_buffer);

        InternalIndexBuffer index_buffer = createIndexBuffer(mesh_data.m_static_mesh_data.m_index_buffer);

        return InternalMesh {false, mesh_data.m_axis_aligned_box, index_buffer, vertex_buffer};
    }

    InternalVertexBuffer RenderResource::createVertexBuffer(InputDefinition input_definition, std::shared_ptr<BufferData> vertex_buffer)
    {
        typedef D3D12MeshVertexPositionNormalTangentTexture MeshVertexDataDefinition;

        ASSERT(input_definition == MeshVertexDataDefinition::InputElementDefinition);
        uint32_t vertex_buffer_size = vertex_buffer->m_size;
        MeshVertexDataDefinition* vertex_buffer_data = static_cast<MeshVertexDataDefinition*>(vertex_buffer->m_data);
        uint32_t inputLayoutStride = sizeof(MeshVertexDataDefinition);
        ASSERT(0 == (vertex_buffer_size % inputLayoutStride));
        uint32_t vertex_count = vertex_buffer_size / inputLayoutStride;
        //uint32_t vertex_buffer_size = inputLayoutStride * vertex_count;

        uint32_t vertex_buffer_offset = 0;

        // temporary staging buffer
        uint32_t inefficient_staging_buffer_size = vertex_buffer_size;

        RHI::SharedGraphicsResource inefficient_staging_buffer =
            m_GraphicsMemory->Allocate(inefficient_staging_buffer_size);
        void* inefficient_staging_buffer_data = inefficient_staging_buffer.Memory();

        MeshVertexDataDefinition* mesh_vertexs = reinterpret_cast<MeshVertexDataDefinition*>(
            reinterpret_cast<uint8_t*>(inefficient_staging_buffer_data) + vertex_buffer_offset);

        for (uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
        {
            mesh_vertexs[vertex_index].position = vertex_buffer_data[vertex_index].position;
            mesh_vertexs[vertex_index].normal   = vertex_buffer_data[vertex_index].normal;
            mesh_vertexs[vertex_index].tangent  = vertex_buffer_data[vertex_index].tangent;
            mesh_vertexs[vertex_index].texcoord = vertex_buffer_data[vertex_index].texcoord;
        }

        std::shared_ptr<RHI::D3D12Buffer> p_mesh_vertex_buffer =
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
                D3D12_RESOURCE_STATES buf_ori_state = p_mesh_vertex_buffer->GetResourceState().GetSubresourceState(0);
                m_ResourceUpload->Transition(p_mesh_vertex_buffer->GetResource(), buf_ori_state, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                m_ResourceUpload->Upload(p_mesh_vertex_buffer->GetResource(), 0, inefficient_staging_buffer);
                m_ResourceUpload->Transition(p_mesh_vertex_buffer->GetResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, buf_ori_state);
            }

            endUploadBatch();
        }

        return InternalVertexBuffer {input_definition, vertex_count, p_mesh_vertex_buffer};
    }

    InternalIndexBuffer RenderResource::createIndexBuffer(std::shared_ptr<BufferData> index_buffer)
    {
        uint32_t index_buffer_size = index_buffer->m_size;

        uint32_t index_count = index_buffer_size / sizeof(uint32_t);

        RHI::SharedGraphicsResource inefficient_staging_buffer = m_GraphicsMemory->Allocate(index_buffer_size);

        void* staging_buffer_data = inefficient_staging_buffer.Memory();

        memcpy(staging_buffer_data, index_buffer->m_data, (size_t)index_buffer_size);

        std::shared_ptr<RHI::D3D12Buffer> p_mesh_index_buffer =
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
                D3D12_RESOURCE_STATES buf_ori_state = p_mesh_index_buffer->GetResourceState().GetSubresourceState(0);
                m_ResourceUpload->Transition(p_mesh_index_buffer->GetResource(), buf_ori_state, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
                m_ResourceUpload->Upload(p_mesh_index_buffer->GetResource(), 0, inefficient_staging_buffer);
                m_ResourceUpload->Transition(p_mesh_index_buffer->GetResource(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, buf_ori_state);
            }

            endUploadBatch();
        }

        return InternalIndexBuffer {sizeof(uint32_t), index_count, p_mesh_index_buffer};
    }


} // namespace MoYu
