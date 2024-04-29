#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/render/render_camera.h"

#include "runtime/function/render/render_mesh_loader.h"
#include "runtime/function/render/render_resource.h"

#include "runtime/function/render/rhi/d3d12/d3d12_core.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resource.h"
#include "runtime/function/render/rhi/d3d12/d3d12_descriptor.h"
#include "runtime/function/render/rhi/d3d12/d3d12_graphicsMemory.h"

#include "runtime/function/render/rhi/hlsl_data_types.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"

#include "runtime/platform/path/path.h"
#include "runtime/platform/system/systemtime.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/renderer/tool_pass.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/terrain_render_helper.h"

#include <stdexcept>

namespace MoYu
{
    RenderResource::RenderResource()
    {
        
    }

    RenderResource::~RenderResource()
    {
        ReleaseAllTextures();
    }

    bool RenderResource::updateGlobalRenderResource(RenderScene* m_render_scene, GlobalRenderingRes level_resource_desc)
    {
        // sherical harmonics
        m_render_scene->m_ibl_map.m_SH = level_resource_desc.m_ibl_map.m_SH;

        // ambient light
        m_render_scene->m_ambient_light.m_color = level_resource_desc.m_ambient_light;

        // direction light
        m_render_scene->m_directional_light.m_direction = level_resource_desc.m_directional_light.m_direction;
        m_render_scene->m_directional_light.m_color     = level_resource_desc.m_directional_light.m_color;

        // sky box irradiance
        MoYu::CubeMap skybox_irradiance_map = level_resource_desc.m_skybox_irradiance_map;
        std::shared_ptr<MoYu::MoYuScratchImage> irradiace_pos_x_map = loadImage(skybox_irradiance_map.m_positive_x_map);
        std::shared_ptr<MoYu::MoYuScratchImage> irradiace_neg_x_map = loadImage(skybox_irradiance_map.m_negative_x_map);
        std::shared_ptr<MoYu::MoYuScratchImage> irradiace_pos_y_map = loadImage(skybox_irradiance_map.m_positive_y_map);
        std::shared_ptr<MoYu::MoYuScratchImage> irradiace_neg_y_map = loadImage(skybox_irradiance_map.m_negative_y_map);
        std::shared_ptr<MoYu::MoYuScratchImage> irradiace_pos_z_map = loadImage(skybox_irradiance_map.m_positive_z_map);
        std::shared_ptr<MoYu::MoYuScratchImage> irradiace_neg_z_map = loadImage(skybox_irradiance_map.m_negative_z_map);

        // sky box specular
        MoYu::CubeMap skybox_specular_map = level_resource_desc.m_skybox_specular_map;
        std::shared_ptr<MoYu::MoYuScratchImage> specular_pos_x_map = loadImage(skybox_specular_map.m_positive_x_map);
        std::shared_ptr<MoYu::MoYuScratchImage> specular_neg_x_map = loadImage(skybox_specular_map.m_negative_x_map);
        std::shared_ptr<MoYu::MoYuScratchImage> specular_pos_y_map = loadImage(skybox_specular_map.m_positive_y_map);
        std::shared_ptr<MoYu::MoYuScratchImage> specular_neg_y_map = loadImage(skybox_specular_map.m_negative_y_map);
        std::shared_ptr<MoYu::MoYuScratchImage> specular_pos_z_map = loadImage(skybox_specular_map.m_positive_z_map);
        std::shared_ptr<MoYu::MoYuScratchImage> specular_neg_z_map = loadImage(skybox_specular_map.m_negative_z_map);

        // create skybox textures, take care of the texture order
        std::array<std::shared_ptr<MoYu::MoYuScratchImage>, 6> irradiance_maps = {irradiace_pos_x_map,
                                                                                  irradiace_neg_x_map,
                                                                                  irradiace_pos_z_map,
                                                                                  irradiace_neg_z_map,
                                                                                  irradiace_pos_y_map,
                                                                                  irradiace_neg_y_map};
        std::array<std::shared_ptr<MoYu::MoYuScratchImage>, 6> specular_maps   = {specular_pos_x_map,
                                                                                  specular_neg_x_map,
                                                                                  specular_pos_z_map,
                                                                                  specular_neg_z_map,
                                                                                  specular_pos_y_map,
                                                                                  specular_neg_y_map};

        // create volume cloud
        MoYu::VolumeCloudTexs volume_clouds_texs = level_resource_desc.m_volume_clouds;
        std::shared_ptr<MoYu::MoYuScratchImage> weather2d_map = loadImage(volume_clouds_texs.m_weather2d);
        std::shared_ptr<MoYu::MoYuScratchImage> cloud3d_map = loadImage(volume_clouds_texs.m_cloud3d);
        std::shared_ptr<MoYu::MoYuScratchImage> worley3d_map = loadImage(volume_clouds_texs.m_worley3d);

        // load dfg texture
        std::shared_ptr<MoYu::MoYuScratchImage> _dfg_map = loadImage(level_resource_desc.m_ibl_map.m_dfg_map);
        std::shared_ptr<MoYu::MoYuScratchImage> _ld_map  = loadImage(level_resource_desc.m_ibl_map.m_ld_map);
        std::shared_ptr<MoYu::MoYuScratchImage> _radians_map  = loadImage(level_resource_desc.m_ibl_map.m_irradians_map);

        // load bluenoise texture
        std::shared_ptr<MoYu::MoYuScratchImage> _bluenoise_map = loadImage(level_resource_desc.m_bluenoises.m_bluenoise_map);

        startUploadBatch();
        {
            // create irradiance cubemap
            auto irridiance_tex = createCubeMap(irradiance_maps, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, true, false, false, false);
            m_render_scene->m_skybox_map.m_skybox_irradiance_map = irridiance_tex;

            // create specular cubemap
            auto specular_tex = createCubeMap(specular_maps, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, false, false, false, false);
            m_render_scene->m_skybox_map.m_skybox_specular_map = specular_tex;

            // create volume cloud maps
            auto weather2d_tex = createTex(weather2d_map);
            auto cloud3d_tex = createTex(cloud3d_map);
            auto worley3d_tex = createTex(worley3d_map);
            m_render_scene->m_cloud_map.m_weather2D = weather2d_tex;
            m_render_scene->m_cloud_map.m_cloud3D = cloud3d_tex;
            m_render_scene->m_cloud_map.m_worley3D = worley3d_tex;

            // create ibl dfg
            auto dfg_tex = createTex(_dfg_map);
            m_render_scene->m_ibl_map.m_dfg = dfg_tex;

            // create ibl ld
            auto ld_tex = createTex(_ld_map);
            m_render_scene->m_ibl_map.m_ld = ld_tex;

            // create ibl radians
            auto radians_tex = createTex(_radians_map);
            m_render_scene->m_ibl_map.m_radians = radians_tex;

            // create blue noise
            auto bluenoise_tex = createTex(_bluenoise_map);
            m_render_scene->m_bluenoise_map.m_bluenoise_64x64_uni = bluenoise_tex;
        }
        endUploadBatch();

        return true;
    }

    void RenderResource::updateFrameUniforms(RenderScene* render_scene, RenderCamera* camera)
    {
        glm::float4x4 view_matrix     = camera->getViewMatrix();
        glm::float4x4 proj_matrix     = camera->getPersProjMatrix();
        glm::float3   camera_position = camera->position();

        float _cn = camera->m_nearClipPlane;
        float _cf = camera->m_farClipPlane;

        glm::float4x4 unjitter_proj_matrix = camera->getUnJitterPersProjMatrix();
        
        // FrameUniforms
        HLSL::FrameUniforms* _frameUniforms = &m_FrameUniforms;

        // Camera Uniform
        HLSL::FrameCameraUniform _frameCameraUniform;
        _frameCameraUniform.viewFromWorldMatrix = view_matrix;
        _frameCameraUniform.worldFromViewMatrix = glm::inverse(_frameCameraUniform.viewFromWorldMatrix);
        _frameCameraUniform.clipFromViewMatrix  = proj_matrix;
        _frameCameraUniform.viewFromClipMatrix  = glm::inverse(_frameCameraUniform.clipFromViewMatrix);
        _frameCameraUniform.clipFromWorldMatrix = proj_matrix * view_matrix;
        _frameCameraUniform.worldFromClipMatrix = glm::inverse(_frameCameraUniform.clipFromWorldMatrix);
        _frameCameraUniform.clipTransform       = glm::float4(1, 1, 1, 1);
        _frameCameraUniform.cameraPosition      = camera_position;
        // update camera taa parameters
        _frameCameraUniform.unJitterProjectionMatrix = unjitter_proj_matrix;
        _frameCameraUniform.unJitterProjectionMatrixInv = glm::inverse(unjitter_proj_matrix);

        _frameCameraUniform.zBufferParams = CalculateZBufferParams(_cn, _cf);

        HLSL::FrameCameraUniform _lastFrameCameraUniform = _frameUniforms->cameraUniform.curFrameUniform;

        HLSL::CameraUniform _cameraUniform;
        _cameraUniform.lastFrameUniform = _lastFrameCameraUniform;
        _cameraUniform.curFrameUniform = _frameCameraUniform;

        _cameraUniform.resolution = glm::float4(
            camera->m_pixelWidth, camera->m_pixelHeight, 1.0f / camera->m_pixelWidth, 1.0f / camera->m_pixelHeight);
        _cameraUniform.logicalViewportScale = glm::float2(1.0f, 1.0f);
        _cameraUniform.logicalViewportOffset = glm::float2(0.0f, 0.0f);

        _cameraUniform.cameraNear = camera->m_nearClipPlane;
        _cameraUniform.cameraFar = camera->m_farClipPlane;

        _cameraUniform.exposure = 1.0f;
        _cameraUniform.ev100 = 1.0f;

        _frameUniforms->cameraUniform = _cameraUniform;


        // Base Uniform
        HLSL::BaseUniform _baseUniform;
        _baseUniform.time     = g_SystemTime.GetTimeSecs();
        _baseUniform.userTime = glm::float4(g_SystemTime.GetDeltaTimeSecs(), 0, 0, 0);
        _baseUniform.lodBias  = 0;

        _frameUniforms->baseUniform = _baseUniform;

        // terrain Uniform
        HLSL::TerrainUniform _terrainUniform;
        _terrainUniform.terrainSize = 1024;
        _terrainUniform.terrainMaxHeight = 1024;
        if (render_scene->m_terrain_renderers.size() != 0)
        {
            _terrainUniform.prevLocal2WorldMatrix = _frameUniforms->terrainUniform.local2WorldMatrix;
            _terrainUniform.local2WorldMatrix = render_scene->m_terrain_renderers[0].internalTerrainRenderer.model_matrix;
            //_terrainUniform.prevLocal2WorldMatrix = render_scene->m_terrain_renderers[0].internalTerrainRenderer.prev_model_matrix;
        }
        else
        {
            _terrainUniform.local2WorldMatrix = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
            _terrainUniform.prevLocal2WorldMatrix = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        }
        _terrainUniform.world2LocalMatrix = glm::inverse(_terrainUniform.local2WorldMatrix);
        _terrainUniform.prevWorld2LocalMatrix = glm::inverse(_terrainUniform.prevLocal2WorldMatrix);

        _frameUniforms->terrainUniform = _terrainUniform;

        // mesh Uniform
        HLSL::MeshUniform _meshUniform;
        _meshUniform.totalMeshCount = render_scene->m_mesh_renderers.size();

        _frameUniforms->meshUniform = _meshUniform;

        // IBL Uniform
        HLSL::IBLUniform _iblUniform;
        //_iblUniform.iblSH
        _iblUniform.iblRoughnessOneLevel = 4;
        _iblUniform.dfg_lut_srv_index    = render_scene->m_ibl_map.m_dfg->GetDefaultSRV()->GetIndex();
        _iblUniform.ld_lut_srv_index     = render_scene->m_ibl_map.m_ld->GetDefaultSRV()->GetIndex();
        _iblUniform.radians_srv_index    = render_scene->m_ibl_map.m_radians->GetDefaultSRV()->GetIndex();
        for (size_t i = 0; i < 7; i++)
        {
            _iblUniform.iblSH[i] = EngineConfig::g_SHConfig._GSH[i];
            //_iblUniform.iblSH[i] = GLMUtil::fromVec4(render_scene->m_ibl_map.m_SH[i]);
        }

        _frameUniforms->iblUniform = _iblUniform;

        HLSL::DirectionalLightStruct _directionalLightStruct;
        _directionalLightStruct.lightColorIntensity = glm::float4(
            render_scene->m_directional_light.m_color.toVector3(), render_scene->m_directional_light.m_intensity);
        _directionalLightStruct.lightPosition = render_scene->m_directional_light.m_position;
        _directionalLightStruct.lightRadius = 1.0f;
        _directionalLightStruct.lightDirection = glm::normalize(render_scene->m_directional_light.m_direction);
        _directionalLightStruct.shadowType = render_scene->m_directional_light.m_shadowmap ? 1 : 0;
        _directionalLightStruct.lightFarAttenuationParams = glm::float2(100, 0.0001);

        HLSL::DirectionalLightShadowmap _directionalLightShadowmap;
        _directionalLightShadowmap.cascadeCount = render_scene->m_directional_light.m_cascade;
        _directionalLightShadowmap.shadowmap_size = glm::uvec2(render_scene->m_directional_light.m_shadowmap_size.x,
                                                                render_scene->m_directional_light.m_shadowmap_size.y);
        _directionalLightShadowmap.light_view_matrix = render_scene->m_directional_light.m_shadow_view_mat;
        for (size_t i = 0; i < render_scene->m_directional_light.m_cascade; i++)
        {
            _directionalLightShadowmap.shadow_bounds[i] = (int)render_scene->m_directional_light.m_shadow_bounds.x << i;
            _directionalLightShadowmap.light_proj_matrix[i] = render_scene->m_directional_light.m_shadow_proj_mats[i];
            _directionalLightShadowmap.light_proj_view_matrix[i] =
                render_scene->m_directional_light.m_shadow_view_proj_mats[i];
        }
        _directionalLightStruct.directionalLightShadowmap = _directionalLightShadowmap;

        _frameUniforms->directionalLight = _directionalLightStruct;

        HLSL::PointLightUniform _pointLightUniform;
        _pointLightUniform.pointLightCounts = render_scene->m_point_light_list.size();
        for (uint32_t i = 0; i < render_scene->m_point_light_list.size(); i++)
        {
            glm::float3 point_light_position  = render_scene->m_point_light_list[i].m_position;
            float   point_light_intensity = render_scene->m_point_light_list[i].m_intensity;

            float radius = render_scene->m_point_light_list[i].m_radius;
            Color color  = render_scene->m_point_light_list[i].m_color;

            HLSL::PointLightStruct _pointLightStruct;
            _pointLightStruct.lightPosition = point_light_position;
            _pointLightStruct.lightRadius = radius;
            _pointLightStruct.lightIntensity = glm::float4(glm::float3(color.r, color.g, color.b), point_light_intensity);
            _pointLightStruct.shadowType = 0;
            _pointLightStruct.falloff    = 1.0f / radius;

            _pointLightStruct.pointLightShadowmap = HLSL::PointLightShadowmap {};

            _pointLightUniform.pointLightStructs[i] = _pointLightStruct;
        }
        _frameUniforms->pointLightUniform = _pointLightUniform;

        HLSL::SpotLightUniform _spotLightUniform;
        _spotLightUniform.spotLightCounts = render_scene->m_spot_light_list.size();
        for (uint32_t i = 0; i < render_scene->m_spot_light_list.size(); i++)
        {
            glm::float3 spot_light_position  = render_scene->m_spot_light_list[i].m_position;
            glm::float3 spot_light_direction = render_scene->m_spot_light_list[i].m_direction;
            float   spot_light_intensity = render_scene->m_spot_light_list[i].m_intensity;

            float radius = render_scene->m_spot_light_list[i].m_radius;
            Color color  = render_scene->m_spot_light_list[i].m_color;

            HLSL::SpotLightStruct _spotLightStruct;
            _spotLightStruct.lightPosition = spot_light_position;
            _spotLightStruct.lightRadius   = radius;
            _spotLightStruct.lightIntensity = glm::float4(glm::float3(color.r, color.g, color.b), spot_light_intensity);
            _spotLightStruct.lightDirection = spot_light_direction;
            _spotLightStruct.inner_radians  = MoYu::degreesToRadians(render_scene->m_spot_light_list[i].m_inner_degree);
            _spotLightStruct.outer_radians  = MoYu::degreesToRadians(render_scene->m_spot_light_list[i].m_outer_degree);
            _spotLightStruct.shadowType   = render_scene->m_spot_light_list[i].m_shadowmap;
            _spotLightStruct.falloff        = 1.0f / radius;

            HLSL::SpotLightShadowmap _spotLightSgadowmap;
            _spotLightSgadowmap.shadowmap_size = glm::uvec2(render_scene->m_spot_light_list[i].m_shadowmap_size.x,
                                                             render_scene->m_spot_light_list[i].m_shadowmap_size.y);
            _spotLightSgadowmap.light_proj_view = render_scene->m_spot_light_list[i].m_shadow_view_proj_mat;

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

    void RenderResource::InitDefaultTextures()
    {
        auto linkedDevice = m_Device->GetLinkedDevice();

        char _WhiteColor[4] = {(char)255, (char)255, (char)255, (char)255};
        char _BlackColor[4] = {(char)0, (char)0, (char)0, (char)255};
        char _RedColor[4]   = {(char)255, (char)0, (char)0, (char)255};
        char _GreenColor[4] = {(char)0, (char)255, (char)0, (char)255};
        char _BlueColor[4]  = {(char)0, (char)0, (char)255, (char)255};

        #define CreateDefault(color, name) \
    RHI::D3D12Texture::Create2D(linkedDevice, \
                                1, \
                                1, \
                                1, \
                                DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, \
                                RHI::RHISurfaceCreateFlags::RHISurfaceCreateFlagNone, \
                                1, \
                                name, \
                                D3D12_RESOURCE_STATE_COMMON, \
                                std::nullopt, \
                                D3D12_SUBRESOURCE_DATA {color, sizeof(char) * 4, sizeof(char) * 4});\

        _Default2TexMap[White] = CreateDefault(_WhiteColor, L"WhiteTex");
        _Default2TexMap[Black] = CreateDefault(_BlackColor, L"BlackTex");
        _Default2TexMap[Red]   = CreateDefault(_RedColor, L"RedTex");
        _Default2TexMap[Green] = CreateDefault(_GreenColor, L"GreenTex");
        _Default2TexMap[Blue]  = CreateDefault(_BlueColor, L"BlueTex");

        _Default2TexMap[BaseColor] =
            RHI::D3D12Texture::Create2D(linkedDevice,
                                        1,
                                        1,
                                        1,
                                        DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                        RHI::RHISurfaceCreateFlags::RHISurfaceCreateFlagNone,
                                        1,
                                        L"DefaultBaseColor",
                                        D3D12_RESOURCE_STATE_COMMON,
                                        std::nullopt,
                                        D3D12_SUBRESOURCE_DATA {_WhiteColor, sizeof(char) * 4, sizeof(char) * 4});


        float _MetalAndRoughnessColor[4] = {1, 1, 0, 0};
        D3D12_SUBRESOURCE_DATA _MetalAndRoughData =
            D3D12_SUBRESOURCE_DATA {_MetalAndRoughnessColor, sizeof(float) * 4, sizeof(float) * 4};
        _Default2TexMap[MetallicAndRoughness] =
            RHI::D3D12Texture::Create2D(linkedDevice,
                                        1,
                                        1,
                                        1,
                                        DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
                                        RHI::RHISurfaceCreateFlags::RHISurfaceCreateFlagNone,
                                        1,
                                        L"DefaultMetallicAndRoughness",
                                        D3D12_RESOURCE_STATE_COMMON,
                                        std::nullopt,
                                        _MetalAndRoughData);

        float _NormalColor[4] = {0.5, 0.5, 1, 0};
        D3D12_SUBRESOURCE_DATA _NormalData =
            D3D12_SUBRESOURCE_DATA {_NormalColor, sizeof(float) * 4, sizeof(float) * 4};
        _Default2TexMap[TangentNormal] =
            RHI::D3D12Texture::Create2D(linkedDevice,
                                        1,
                                        1,
                                        1,
                                        DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM,
                                        RHI::RHISurfaceCreateFlags::RHISurfaceCreateFlagNone,
                                        1,
                                        L"DefaultNormal",
                                        D3D12_RESOURCE_STATE_COMMON,
                                        std::nullopt,
                                        _NormalData);
        
    }

    void RenderResource::ReleaseAllTextures()
    {
        _Default2TexMap.clear();
        _Image2TexMap.clear();
    }

    std::shared_ptr<RHI::D3D12Texture> RenderResource::SceneImageToTexture(const SceneImage& _sceneimage)
    {
        if (_sceneimage.m_image_file == "")
            return nullptr;

        if (_Image2TexMap.find(_sceneimage) != _Image2TexMap.end())
            return _Image2TexMap[_sceneimage];

        DXGI_FORMAT m_format   = _sceneimage.m_is_srgb ? DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
        bool        m_is_srgb  = _sceneimage.m_is_srgb;
        bool        m_gen_mips = _sceneimage.m_auto_mips;

        std::shared_ptr<MoYu::MoYuScratchImage> _image_data = loadImage(_sceneimage.m_image_file);

        uint32_t m_width = _image_data->GetMetadata().width;
        uint32_t m_height = _image_data->GetMetadata().height;
        void*    m_pixels = _image_data->GetPixels();

        std::shared_ptr<RHI::D3D12Texture> _tex =
            createTex2D(m_width, m_height, m_pixels, m_format, m_is_srgb, m_gen_mips, false);

        _Image2TexMap[_sceneimage] = _tex;

        return _tex;
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
            is_uniform_same &= m_mat_data.m_reflectance_factor == m_cached_mat_data.m_reflectance_factor;
            is_uniform_same &= m_mat_data.m_clearcoat_factor == m_cached_mat_data.m_clearcoat_factor;
            is_uniform_same &= m_mat_data.m_clearcoat_roughness_factor == m_cached_mat_data.m_clearcoat_roughness_factor;
            is_uniform_same &= m_mat_data.m_anisotropy_factor == m_cached_mat_data.m_anisotropy_factor;
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
                material_uniform_buffer_info.baseColorFactor   = m_mat_data.m_base_color_factor;
                material_uniform_buffer_info.metallicFactor    = m_mat_data.m_metallic_factor;
                material_uniform_buffer_info.roughnessFactor   = m_mat_data.m_roughness_factor;
                material_uniform_buffer_info.reflectanceFactor = m_mat_data.m_reflectance_factor;

                material_uniform_buffer_info.clearCoatFactor          = m_mat_data.m_clearcoat_factor;
                material_uniform_buffer_info.clearCoatRoughnessFactor = m_mat_data.m_clearcoat_roughness_factor;
                material_uniform_buffer_info.anisotropyFactor         = m_mat_data.m_anisotropy_factor;

                material_uniform_buffer_info.base_color_tilling = m_mat_data.m_base_color_texture_file.m_tilling;
                material_uniform_buffer_info.metallic_roughness_tilling = m_mat_data.m_metallic_roughness_texture_file.m_tilling;
                material_uniform_buffer_info.normal_tilling     = m_mat_data.m_normal_texture_file.m_tilling;
                material_uniform_buffer_info.occlusion_tilling  = m_mat_data.m_occlusion_texture_file.m_tilling;
                material_uniform_buffer_info.emissive_tilling   = m_mat_data.m_emissive_texture_file.m_tilling;

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
                auto _tex = SceneImageToTexture(base_color_image);
                now_material.base_color_texture_image = _tex == nullptr ? _Default2TexMap[BaseColor] : _tex;
            }
        }
        {
            if (!(m_mat_data.m_metallic_roughness_texture_file.m_image == m_cached_mat_data.m_metallic_roughness_texture_file.m_image) || !has_initialized)
            {
                SceneImage metallic_roughness_image = m_mat_data.m_metallic_roughness_texture_file.m_image;
                if (now_material.metallic_roughness_texture_image != nullptr)
                    now_material.metallic_roughness_texture_image = nullptr;
                auto _tex = SceneImageToTexture(metallic_roughness_image);
                now_material.metallic_roughness_texture_image = _tex == nullptr ? _Default2TexMap[MetallicAndRoughness] : _tex;
            }
        }
        {
            if (!(m_mat_data.m_normal_texture_file.m_image == m_cached_mat_data.m_normal_texture_file.m_image) || !has_initialized)
            {
                SceneImage normal_image = m_mat_data.m_normal_texture_file.m_image;
                if (now_material.normal_texture_image != nullptr)
                    now_material.normal_texture_image = nullptr;
                auto _tex = SceneImageToTexture(normal_image);
                now_material.normal_texture_image = _tex == nullptr ? _Default2TexMap[TangentNormal] : _tex;
            }
        }
        {
            if (!(m_mat_data.m_occlusion_texture_file.m_image == m_cached_mat_data.m_occlusion_texture_file.m_image) || !has_initialized)
            {
                SceneImage occlusion_image = m_mat_data.m_occlusion_texture_file.m_image;
                if (now_material.occlusion_texture_image != nullptr)
                    now_material.occlusion_texture_image = nullptr;
                auto _tex = SceneImageToTexture(occlusion_image);
                now_material.occlusion_texture_image = _tex == nullptr ? _Default2TexMap[White] : _tex;
            }
        }
        {
            if (!(m_mat_data.m_emissive_texture_file.m_image == m_cached_mat_data.m_emissive_texture_file.m_image) || !has_initialized)
            {
                SceneImage emissive_image = m_mat_data.m_emissive_texture_file.m_image;
                if (now_material.emissive_texture_image != nullptr)
                    now_material.emissive_texture_image = nullptr;
                auto _tex = SceneImageToTexture(emissive_image);
                now_material.emissive_texture_image = _tex == nullptr ? _Default2TexMap[Black] : _tex;
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

        if (buffer_data != nullptr)
        {
            void* bufferPtr = dynamicBuffer->GetCpuVirtualAddress<void>();
            memcpy(bufferPtr, buffer_data, buffer_size);
        }
        return dynamicBuffer;
    }

    std::shared_ptr<RHI::D3D12Buffer> RenderResource::createDynamicBuffer(std::shared_ptr<MoYu::MoYuScratchBuffer>& buffer_data)
    {
        return createDynamicBuffer(buffer_data->GetBufferPointer(), buffer_data->GetBufferSize(), buffer_data->GetBufferSize());
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

    std::shared_ptr<RHI::D3D12Buffer> RenderResource::createStaticBuffer(std::shared_ptr<MoYu::MoYuScratchBuffer>& buffer_data, bool raw, bool batch)
    {
        return createStaticBuffer(buffer_data->GetBufferPointer(), buffer_data->GetBufferSize(), buffer_data->GetBufferSize(), raw, batch);
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

    std::shared_ptr<RHI::D3D12Texture> RenderResource::createTex2D(std::shared_ptr<MoYu::MoYuScratchImage>& tex2d_data, DXGI_FORMAT format, bool is_srgb, bool genMips, bool batch)
    {
        return createTex2D(tex2d_data->GetMetadata().width, tex2d_data->GetMetadata().height, tex2d_data->GetPixels(), format, is_srgb, genMips, batch);
    }
    
    std::shared_ptr<RHI::D3D12Texture> RenderResource::createCubeMap(std::array<std::shared_ptr<MoYu::MoYuScratchImage>, 6>& cube_maps, DXGI_FORMAT format, bool is_srgb, bool isReadWrite, bool genMips, bool batch)
    {
        if (batch)
            this->startUploadBatch();

        // assume all textures have same width, height and format
        uint32_t cubemap_miplevels = 1;
        if (genMips)
            cubemap_miplevels = static_cast<uint32_t>(std::floor(std::log2(
                                    std::max(cube_maps[0]->GetMetadata().width, cube_maps[0]->GetMetadata().height)))) +
                                1;

        UINT width  = cube_maps[0]->GetMetadata().width;
        UINT height = cube_maps[0]->GetMetadata().height;
        
        RHI::RHISurfaceCreateFlags texflags = RHI::RHISurfaceCreateFlagNone;
        if (isReadWrite)
            texflags |= RHI::RHISurfaceCreateRandomWrite;
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
            resourceInitData.pData      = cube_maps[i]->GetPixels();
            resourceInitData.RowPitch   = cube_maps[i]->GetMetadata().width * bytesPerPixel;
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

    std::shared_ptr<RHI::D3D12Texture> RenderResource::createTex(std::shared_ptr<MoYu::MoYuScratchImage> scratch_image, bool batch)
    {
        if (batch)
            this->startUploadBatch();

        DirectX::ScratchImage& _sratchimage = *scratch_image;

        DirectX::TexMetadata _texMetaData = _sratchimage.GetMetadata();

        std::shared_ptr<RHI::D3D12Texture> _rhiTex = nullptr;

        if (_texMetaData.dimension == DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE2D)
        {
            if (_texMetaData.arraySize == 1)
            {
                _rhiTex = RHI::D3D12Texture::Create2D(m_Device->GetLinkedDevice(),
                                                      _texMetaData.width,
                                                      _texMetaData.height,
                                                      _texMetaData.mipLevels,
                                                      _texMetaData.format,
                                                      RHI::RHISurfaceCreateFlags::RHISurfaceCreateMipmap);

                m_ResourceUpload->Transition(_rhiTex->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
                
                int subresourceNumber = _texMetaData.mipLevels;

                D3D12_SUBRESOURCE_DATA resourceInitDatas[12];
                for (size_t i = 0; i < subresourceNumber; i++)
                {
                    const DirectX::Image* _image = _sratchimage.GetImage(i, 0, 0);
                    D3D12_SUBRESOURCE_DATA _resourceInitData;
                    _resourceInitData.pData = _image->pixels;
                    _resourceInitData.RowPitch = _image->rowPitch;
                    _resourceInitData.SlicePitch = _image->slicePitch;
                    resourceInitDatas[i] = _resourceInitData;
                }

                m_ResourceUpload->Upload(_rhiTex->GetResource(), 0, resourceInitDatas, subresourceNumber);

                m_ResourceUpload->Transition(_rhiTex->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
            }
            else
            {
                if (_texMetaData.IsCubemap())
                {
                    // TODO: arrysize > 6
                    _rhiTex = RHI::D3D12Texture::CreateCubeMap(m_Device->GetLinkedDevice(),
                                                               _texMetaData.width,
                                                               _texMetaData.height,
                                                               _texMetaData.mipLevels,
                                                               _texMetaData.format,
                                                               RHI::RHISurfaceCreateFlags::RHISurfaceCreateMipmap);
                }
                else
                {
                    _rhiTex = RHI::D3D12Texture::Create2DArray(m_Device->GetLinkedDevice(),
                                                               _texMetaData.width,
                                                               _texMetaData.height,
                                                               _texMetaData.arraySize,
                                                               _texMetaData.mipLevels,
                                                               _texMetaData.format,
                                                               RHI::RHISurfaceCreateFlags::RHISurfaceCreateMipmap);
                }
                m_ResourceUpload->Transition(_rhiTex->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
                
                D3D12_SUBRESOURCE_DATA resourceInitDatas[12*6];

                int subresourceNumber = _texMetaData.arraySize * _texMetaData.mipLevels;

                for (size_t i = 0; i < _texMetaData.arraySize; i++)
                {
                    for (size_t j = 0; j < _texMetaData.mipLevels; j++)
                    {
                        int _curSubIndex = D3D12CalcSubresource(j, i, 0, _texMetaData.mipLevels,_texMetaData.arraySize );

                        const DirectX::Image*  _image = _sratchimage.GetImage(j, i, 0);
                        D3D12_SUBRESOURCE_DATA _resourceInitData;
                        _resourceInitData.pData      = _image->pixels;
                        _resourceInitData.RowPitch   = _image->rowPitch;
                        _resourceInitData.SlicePitch = _image->slicePitch;
                        resourceInitDatas[_curSubIndex] = _resourceInitData;
                    }
                }

                m_ResourceUpload->Upload(_rhiTex->GetResource(), 0, resourceInitDatas, subresourceNumber);

                m_ResourceUpload->Transition(_rhiTex->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
            }
        }
        else if (_texMetaData.dimension == DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE3D)
        {
            _rhiTex = RHI::D3D12Texture::Create3D(m_Device->GetLinkedDevice(),
                                                  _texMetaData.width,
                                                  _texMetaData.height,
                                                  _texMetaData.depth,
                                                  _texMetaData.mipLevels,
                                                  _texMetaData.format,
                                                  RHI::RHISurfaceCreateFlags::RHISurfaceCreateMipmap);

            m_ResourceUpload->Transition(_rhiTex->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
                
            int mipLevelNum = _texMetaData.mipLevels;
            int arraysliceNum = _texMetaData.arraySize;
            int subresourceNumber = mipLevelNum * arraysliceNum;

            D3D12_SUBRESOURCE_DATA resourceInitDatas[1024];
            for (size_t i = 0; i < arraysliceNum; i++)
            {
                for (size_t j = 0; j < mipLevelNum; j++)
                {
                    const DirectX::Image* _image = _sratchimage.GetImage(j, 0, i);
                    D3D12_SUBRESOURCE_DATA _resourceInitData;
                    _resourceInitData.pData = _image->pixels;
                    _resourceInitData.RowPitch = _image->rowPitch;
                    _resourceInitData.SlicePitch = _image->slicePitch;
                    resourceInitDatas[i * arraysliceNum + j] = _resourceInitData;
                }
            }
            m_ResourceUpload->Upload(_rhiTex->GetResource(), 0, resourceInitDatas, subresourceNumber);

            m_ResourceUpload->Transition(_rhiTex->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
        }

        if (batch)
            this->endUploadBatch();

        return _rhiTex;
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
        InternalVertexBuffer vertex_buffer = createVertexBuffer<D3D12MeshVertexPositionNormalTangentTexture>(
            mesh_data.m_static_mesh_data.m_InputElementDefinition, mesh_data.m_static_mesh_data.m_vertex_buffer);

        InternalIndexBuffer index_buffer = createIndexBuffer<uint32_t>(mesh_data.m_static_mesh_data.m_index_buffer);

        return InternalMesh {false, mesh_data.m_axis_aligned_box, index_buffer, vertex_buffer};
    }

} // namespace MoYu
