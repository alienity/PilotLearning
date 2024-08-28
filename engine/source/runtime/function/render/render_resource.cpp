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
        std::shared_ptr<MoYu::MoYuScratchImage> _PreIntegratedFGD_GGXDisneyDiffuse = loadImage(level_resource_desc.m_ibl_map._PreIntegratedFGD_GGXDisneyDiffuse);
        std::shared_ptr<MoYu::MoYuScratchImage> _PreIntegratedFGD_CharlieAndFabric = loadImage(level_resource_desc.m_ibl_map._PreIntegratedFGD_CharlieAndFabric);

        // load bluenoise texture
        std::shared_ptr<MoYu::MoYuScratchImage> _bluenoise_map = loadImage(level_resource_desc.m_bluenoises.m_bluenoise_map);

        std::shared_ptr<MoYu::MoYuScratchImage> _owenScrambled256Tex = loadImage(level_resource_desc.m_bluenoises.m_owenScrambled256Tex);
        std::shared_ptr<MoYu::MoYuScratchImage> _scramblingTile8SPP = loadImage(level_resource_desc.m_bluenoises.m_scramblingTile8SPP);
        std::shared_ptr<MoYu::MoYuScratchImage> _rankingTile8SPP = loadImage(level_resource_desc.m_bluenoises.m_rankingTile8SPP);
        std::shared_ptr<MoYu::MoYuScratchImage> _scramblingTex = loadImage(level_resource_desc.m_bluenoises.m_scramblingTex);

        startUploadBatch();
        {
            // create irradiance cubemap
            auto irridiance_tex = createCubeMap(irradiance_maps, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, true, false, false, false);
            m_render_scene->m_skybox_map.m_skybox_irradiance_map = irridiance_tex;

            // create specular cubemap
            auto specular_tex = createCubeMap(specular_maps, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, false, false, false, false);
            m_render_scene->m_skybox_map.m_skybox_specular_map = specular_tex;

            // create volume cloud maps
            auto weather2d_tex = createTex(weather2d_map, L"weather2d");
            auto cloud3d_tex = createTex(cloud3d_map, L"cloud3d");
            auto worley3d_tex = createTex(worley3d_map, L"worley3d");
            m_render_scene->m_cloud_map.m_weather2D = weather2d_tex;
            m_render_scene->m_cloud_map.m_cloud3D = cloud3d_tex;
            m_render_scene->m_cloud_map.m_worley3D = worley3d_tex;

            // create ibl dfg
            auto dfg_tex = createTex(_dfg_map, L"ibl_dfg");
            m_render_scene->m_ibl_map.m_dfg = dfg_tex;

            // create ibl ld
            auto ld_tex = createTex(_ld_map, L"ibl_ld");
            m_render_scene->m_ibl_map.m_ld = ld_tex;

            // create ibl radians
            auto radians_tex = createTex(_radians_map, L"ibl_radians");
            m_render_scene->m_ibl_map.m_radians = radians_tex;

            auto preIntegratedFGD_GGXDisneyDiffuse = createTex(_dfg_map/*_PreIntegratedFGD_GGXDisneyDiffuse*/, L"PreIntegratedFGD_GGXDisneyDiffuse");
            m_render_scene->m_ibl_map._PreIntegratedFGD_GGXDisneyDiffuseIndex = preIntegratedFGD_GGXDisneyDiffuse;

            auto PreIntegratedFGD_CharlieAndFabric = createTex(_dfg_map/*_PreIntegratedFGD_CharlieAndFabric*/, L"PreIntegratedFGD_CharlieAndFabric");
            m_render_scene->m_ibl_map._PreIntegratedFGD_CharlieAndFabricIndex = PreIntegratedFGD_CharlieAndFabric;
            
            // create blue noise
            m_render_scene->m_bluenoise_map.m_bluenoise_64x64_uni = createTex(_bluenoise_map, L"blue_noise");
            m_render_scene->m_bluenoise_map.m_owenScrambled256Tex = createTex(_owenScrambled256Tex, L"owenScrambled256Tex");
            m_render_scene->m_bluenoise_map.m_scramblingTile8SPP = createTex(_scramblingTile8SPP, L"scramblingTile8SPP");
            m_render_scene->m_bluenoise_map.m_rankingTile8SPP = createTex(_rankingTile8SPP, L"rankingTile8SPP");
            m_render_scene->m_bluenoise_map.m_scramblingTex = createTex(_scramblingTex, L"scramblingTex");
        }
        endUploadBatch();

        return true;
    }

    void RenderResource::updateFrameUniforms(RenderScene* render_scene, RenderCamera* camera)
    {
        glm::float4x4 view_matrix     = camera->getViewMatrix();
        glm::float4x4 proj_matrix     = camera->getProjMatrix();
        glm::float3   camera_position = camera->position();
        
        float _cn = camera->m_nearClipPlane;
        float _cf = camera->m_farClipPlane;

        int _cw = camera->m_pixelWidth;
        int _ch = camera->m_pixelHeight;

        double _t = g_SystemTime.GetTimeSecs();
        double _pt = g_SystemTime.GetPrevTimeSecs();
        double _dt = g_SystemTime.GetDeltaTimeSecs();

        glm::float4x4 unjitter_proj_matrix = camera->getProjMatrix(true);
        
        // FrameUniforms
        HLSL::FrameUniforms* _frameUniforms = &m_FrameUniforms;

        // Camera Uniform
        HLSL::CameraDataBuffer _frameCameraData;
        _frameCameraData.viewFromWorldMatrix  = view_matrix;
        _frameCameraData.worldFromViewMatrix  = glm::inverse(_frameCameraData.viewFromWorldMatrix);
        _frameCameraData.clipFromViewMatrix   = proj_matrix;
        _frameCameraData.viewFromClipMatrix   = glm::inverse(_frameCameraData.clipFromViewMatrix);
        _frameCameraData.clipFromWorldMatrix  = proj_matrix * view_matrix;
        _frameCameraData.worldFromClipMatrix  = glm::inverse(_frameCameraData.clipFromWorldMatrix);
        _frameCameraData.clipTransform        = glm::float4(1, 1, 1, 1);
        _frameCameraData._WorldSpaceCameraPos = camera_position;
        // update camera taa parameters
        _frameCameraData.unJitterProjectionMatrix = unjitter_proj_matrix;
        _frameCameraData.unJitterProjectionMatrixInv = glm::inverse(unjitter_proj_matrix);

        _frameCameraData._ZBufferParams = CalculateZBufferParams(_cn, _cf);

        HLSL::CameraDataBuffer _prevFrameCameraData = _frameUniforms->cameraUniform._CurFrameUniform;

        HLSL::CameraUniform _cameraUniform;
        _cameraUniform._PrevFrameUniform = _prevFrameCameraData;
        _cameraUniform._CurFrameUniform = _frameCameraData;
        _cameraUniform._Resolution = glm::float4(_cw, _ch, 1.0f / _cw, 1.0f / _ch);
        _cameraUniform._LogicalViewportScale = glm::float2(1.0f, 1.0f);
        _cameraUniform._LogicalViewportOffset = glm::float2(0.0f, 0.0f);
        _cameraUniform._ZBufferParams = CalculateZBufferParams(_cn, _cf);
        _cameraUniform._CameraNear = _cn;
        _cameraUniform._CameraFar = _cf;
        _cameraUniform.unity_MotionVectorsParams = glm::float4(0, 1, 0, 1);

        _frameUniforms->cameraUniform = _cameraUniform;


        // Base Uniform
        HLSL::BaseUniform _baseUniform;
        _baseUniform._ScreenSize = glm::float4(_cw, _ch, 1.0f / _cw, 1.0f / _ch);
        _baseUniform._PostProcessScreenSize = glm::float4(_cw, _ch, 1.0f / _cw, 1.0f / _ch);
        _baseUniform._RTHandleScale = glm::float4(1.0f);
        _baseUniform._RTHandleScaleHistory = glm::float4(1.0f);
        _baseUniform._RTHandlePostProcessScale = glm::float4(1.0f);
        _baseUniform._RTHandlePostProcessScaleHistory = glm::float4(1.0f);
        _baseUniform._DynamicResolutionFullscreenScale = glm::float4(1.0f);
        _baseUniform._Time = glm::float4(_t / 20, _t, _t * 2, _t * 3); // (t/20, t, t*2, t*3)
        _baseUniform._SinTime = glm::float4(glm::sin(_t / 8), glm::sin(_t / 4), glm::sin(_t / 2), glm::sin(_t)); // sin(t/8), sin(t/4), sin(t/2), sin(t)
        _baseUniform._CosTime = glm::float4(glm::cos(_t / 8), glm::cos(_t / 4), glm::cos(_t / 2), glm::cos(_t)); // cos(t/8), cos(t/4), cos(t/2), cos(t)
        _baseUniform._DeltaTime = glm::float4(_dt, 1.0f / _dt, _dt, 1.0f / _dt); // dt, 1/dt, smoothdt, 1/smoothdt
        _baseUniform._TimeParameters = glm::float4(_t, glm::sin(_t), glm::cos(_t), 0); // t, sin(t), cos(t)
        _baseUniform._LastTimeParameters = glm::float4(_pt, glm::sin(_pt), glm::cos(_pt), 0); // t, sin(t), cos(t)
        _baseUniform.temporalNoise = glm::float4(0);

        _baseUniform.needsAlphaChannel = 1;
        _baseUniform.lodBias = 0;
        _baseUniform.refractionLodOffset = 0;
        _baseUniform.baseReserved0 = 0;

        _baseUniform._IndirectDiffuseLightingMultiplier = 0.3f;
        _baseUniform._IndirectDiffuseLightingLayers = 255;
        _baseUniform._ReflectionLightingMultiplier = 1;
        _baseUniform._ReflectionLightingLayers = 255;

        _frameUniforms->baseUniform = _baseUniform;


        // terrain Uniform
        HLSL::TerrainUniform _terrainUniform;
        _terrainUniform.terrainSize = 1024;
        _terrainUniform.terrainMaxHeight = 1024;
        if (render_scene->m_terrain_renderers.size() != 0)
        {
            _terrainUniform.prevLocal2WorldMatrix = _frameUniforms->terrainUniform.local2WorldMatrix;
            _terrainUniform.local2WorldMatrix = render_scene->m_terrain_renderers[0].internalTerrainRenderer.model_matrix;
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

        _iblUniform._PreIntegratedFGD_GGXDisneyDiffuseIndex = render_scene->m_ibl_map._PreIntegratedFGD_GGXDisneyDiffuseIndex->GetDefaultSRV()->GetIndex();
        _iblUniform._PreIntegratedFGD_CharlieAndFabricIndex = render_scene->m_ibl_map._PreIntegratedFGD_CharlieAndFabricIndex->GetDefaultSRV()->GetIndex();
        
        for (size_t i = 0; i < 7; i++)
        {
            _iblUniform.iblSH[i] = EngineConfig::g_SHConfig._GSH[i];
            //_iblUniform.iblSH[i] = GLMUtil::fromVec4(render_scene->m_ibl_map.m_SH[i]);
        }

        _frameUniforms->iblUniform = _iblUniform;


        // Sun Uniform
        HLSL::VolumeCloudStruct volumeCloudUniform{};
        volumeCloudUniform.sunlight_direction = render_scene->m_directional_light.m_direction;
        _frameUniforms->volumeCloudUniform = volumeCloudUniform;

        // Exposure Uniform
        HLSL::ExposureUniform exposureUniform{};
        exposureUniform._ProbeExposureScale = 1.0f;
        _frameUniforms->exposureUniform = exposureUniform;


    }

    void RenderResource::updateLightUniforms(RenderScene* render_scene)
    {
        
    }

    bool RenderResource::updateInternalMeshRenderer(SceneMeshRenderer scene_mesh_renderer, SceneMeshRenderer& cached_mesh_renderer, InternalMeshRenderer& internal_mesh_renderer, bool has_initialized)
    {
        updateInternalMesh(scene_mesh_renderer.m_scene_mesh, cached_mesh_renderer.m_scene_mesh, internal_mesh_renderer.ref_mesh, has_initialized);
        updateInternalMaterial(scene_mesh_renderer.m_material, cached_mesh_renderer.m_material, internal_mesh_renderer.ref_material, has_initialized);

        cached_mesh_renderer = scene_mesh_renderer;

        return true;
    }

    std::shared_ptr<RHI::D3D12Texture> RenderResource::CreateTransientTexture(
        RHI::RHIRenderSurfaceBaseDesc desc, std::wstring name, D3D12_RESOURCE_STATES initState)
    {
        std::shared_ptr<RHI::D3D12Texture> nRT = nullptr;
        for (auto& element : _AvailableDescRTs)
        {
            if(element.desc == desc)
            {
                nRT = element.rt;
                nRT->SetResourceName(name);
                _AvailableDescRTs.remove(element);
                break;
            }            
        }

        if(nRT == nullptr)
        {
            nRT = RHI::D3D12Texture::Create(m_Device->GetLinkedDevice(), desc, name, initState);   
        }
        _AllocatedDescRTs.push_back(DescTexturePair{ desc, nRT });

        return nRT;
    }

    void RenderResource::ReleaseTransientResources()
    {
        for (auto& element : _AllocatedDescRTs)
        {
            _AvailableDescRTs.push_back(element);
        }
        _AllocatedDescRTs.clear();
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
        _AvailableDescRTs.clear();
        _AllocatedDescRTs.clear();
        
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

        StandardLightMaterial m_mat_data = scene_material.m_mat_data;
        StandardLightMaterial m_cached_mat_data = cached_material.m_mat_data;

        this->startUploadBatch();


        InternalStandardLightMaterial& now_material = internal_material.m_intenral_light_mat;
        {
#define UpdateInternalMaterialMapCheck(MapName) if (!(m_mat_data.MapName.m_image == m_cached_mat_data.MapName.m_image) || !has_initialized) { now_material.MapName = SceneImageToTexture(m_mat_data.MapName.m_image); }

            UpdateInternalMaterialMapCheck(_BaseColorMap)
            UpdateInternalMaterialMapCheck(_MaskMap)
            UpdateInternalMaterialMapCheck(_NormalMap)
            UpdateInternalMaterialMapCheck(_NormalMapOS)
            UpdateInternalMaterialMapCheck(_BentNormalMap)
            UpdateInternalMaterialMapCheck(_BentNormalMapOS)
            UpdateInternalMaterialMapCheck(_HeightMap)
            UpdateInternalMaterialMapCheck(_DetailMap)
            UpdateInternalMaterialMapCheck(_TangentMap)
            UpdateInternalMaterialMapCheck(_TangentMapOS)
            UpdateInternalMaterialMapCheck(_AnisotropyMap)
            UpdateInternalMaterialMapCheck(_SubsurfaceMaskMap)
            UpdateInternalMaterialMapCheck(_TransmissionMaskMap)
            UpdateInternalMaterialMapCheck(_ThicknessMap)
            UpdateInternalMaterialMapCheck(_IridescenceThicknessMap)
            UpdateInternalMaterialMapCheck(_IridescenceMaskMap)
            UpdateInternalMaterialMapCheck(_CoatMaskMap)
            UpdateInternalMaterialMapCheck(_EmissiveColorMap)
            UpdateInternalMaterialMapCheck(_TransmittanceColorMap)

#undef UpdateInternalMaterialMapCheck
        }
        {
#define UpdateInternalMaterialAssign(Name) now_material.Name = m_mat_data.Name;

            UpdateInternalMaterialAssign(_BaseColor)
            now_material._BaseColorMap_ST = glm::float4(m_mat_data._BaseColorMap.m_tilling, m_mat_data._BaseColorMap.m_offset);
            now_material._BaseColorMap_TexelSize = glm::float4(
                1.0f / now_material._BaseColorMap->GetWidth(),
                1.0f / now_material._BaseColorMap->GetHeight(),
                now_material._BaseColorMap->GetWidth(),
                now_material._BaseColorMap->GetHeight());
            now_material._BaseColorMap_MipInfo = glm::float4(1, 1, 1, 1);
            UpdateInternalMaterialAssign(_Metallic)
            UpdateInternalMaterialAssign(_Smoothness)
            UpdateInternalMaterialAssign(_MetallicRemapMin)
            UpdateInternalMaterialAssign(_MetallicRemapMax)
            UpdateInternalMaterialAssign(_SmoothnessRemapMin)
            UpdateInternalMaterialAssign(_SmoothnessRemapMax)
            UpdateInternalMaterialAssign(_AlphaRemapMin)
            UpdateInternalMaterialAssign(_AlphaRemapMax)
            UpdateInternalMaterialAssign(_AORemapMin)
            UpdateInternalMaterialAssign(_AORemapMax)
            UpdateInternalMaterialAssign(_NormalScale)
            UpdateInternalMaterialAssign(_HeightAmplitude)
            UpdateInternalMaterialAssign(_HeightCenter)
            UpdateInternalMaterialAssign(_HeightMapParametrization)
            UpdateInternalMaterialAssign(_HeightOffset)
            UpdateInternalMaterialAssign(_HeightMin)
            UpdateInternalMaterialAssign(_HeightMax)
            UpdateInternalMaterialAssign(_HeightTessAmplitude)
            UpdateInternalMaterialAssign(_HeightTessCenter)
            UpdateInternalMaterialAssign(_HeightPoMAmplitude)
            now_material._DetailMap_ST = glm::float4(m_mat_data._DetailMap.m_tilling, m_mat_data._DetailMap.m_offset);
            UpdateInternalMaterialAssign(_DetailAlbedoScale)
            UpdateInternalMaterialAssign(_DetailNormalScale)
            UpdateInternalMaterialAssign(_DetailSmoothnessScale)
            UpdateInternalMaterialAssign(_Anisotropy)
            UpdateInternalMaterialAssign(_DiffusionProfileHash)
            UpdateInternalMaterialAssign(_SubsurfaceMask)
            UpdateInternalMaterialAssign(_TransmissionMask)
            UpdateInternalMaterialAssign(_Thickness)
            UpdateInternalMaterialAssign(_ThicknessRemap)
            UpdateInternalMaterialAssign(_IridescenceThicknessRemap)
            UpdateInternalMaterialAssign(_IridescenceThickness)
            UpdateInternalMaterialAssign(_IridescenceMask)
            UpdateInternalMaterialAssign(_CoatMask)
            UpdateInternalMaterialAssign(_EnergyConservingSpecularColor)
            UpdateInternalMaterialAssign(_SpecularOcclusionMode)
            UpdateInternalMaterialAssign(_EmissiveColor)
            UpdateInternalMaterialAssign(_AlbedoAffectEmissive)
            UpdateInternalMaterialAssign(_EmissiveIntensityUnit)
            UpdateInternalMaterialAssign(_UseEmissiveIntensity)
            UpdateInternalMaterialAssign(_EmissiveIntensity)
            UpdateInternalMaterialAssign(_EmissiveExposureWeight)
            UpdateInternalMaterialAssign(_UseShadowThreshold)
            UpdateInternalMaterialAssign(_AlphaCutoffEnable)
            UpdateInternalMaterialAssign(_AlphaCutoff)
            UpdateInternalMaterialAssign(_AlphaCutoffShadow)
            UpdateInternalMaterialAssign(_AlphaCutoffPrepass)
            UpdateInternalMaterialAssign(_AlphaCutoffPostpass)
            UpdateInternalMaterialAssign(_TransparentDepthPrepassEnable)
            UpdateInternalMaterialAssign(_TransparentBackfaceEnable)
            UpdateInternalMaterialAssign(_TransparentDepthPostpassEnable)
            UpdateInternalMaterialAssign(_TransparentSortPriority)
            UpdateInternalMaterialAssign(_RefractionModel)
            UpdateInternalMaterialAssign(_Ior)
            UpdateInternalMaterialAssign(_TransmittanceColor)
            UpdateInternalMaterialAssign(_ATDistance)
            UpdateInternalMaterialAssign(_TransparentWritingMotionVec)
            UpdateInternalMaterialAssign(_SurfaceType)
            UpdateInternalMaterialAssign(_BlendMode)
            UpdateInternalMaterialAssign(_SrcBlend)
            UpdateInternalMaterialAssign(_DstBlend)
            UpdateInternalMaterialAssign(_AlphaSrcBlend)
            UpdateInternalMaterialAssign(_AlphaDstBlend)
            UpdateInternalMaterialAssign(_EnableFogOnTransparent)
            UpdateInternalMaterialAssign(_DoubleSidedEnable)
            UpdateInternalMaterialAssign(_DoubleSidedNormalMode)
            UpdateInternalMaterialAssign(_DoubleSidedConstants)
            UpdateInternalMaterialAssign(_DoubleSidedGIMode)
            UpdateInternalMaterialAssign(_UVBase)
            UpdateInternalMaterialAssign(_ObjectSpaceUVMapping)
            UpdateInternalMaterialAssign(_TexWorldScale)
            UpdateInternalMaterialAssign(_UVMappingMask)
            UpdateInternalMaterialAssign(_NormalMapSpace)
            UpdateInternalMaterialAssign(_MaterialID)
            UpdateInternalMaterialAssign(_TransmissionEnable)
            UpdateInternalMaterialAssign(_PPDMinSamples)
            UpdateInternalMaterialAssign(_PPDMaxSamples)
            UpdateInternalMaterialAssign(_PPDLodThreshold)
            UpdateInternalMaterialAssign(_PPDPrimitiveLength)
            UpdateInternalMaterialAssign(_PPDPrimitiveWidth)
            UpdateInternalMaterialAssign(_InvPrimScale)
            UpdateInternalMaterialAssign(_UVDetailsMappingMask)
            UpdateInternalMaterialAssign(_UVDetail)
            UpdateInternalMaterialAssign(_LinkDetailsWithBase)
            UpdateInternalMaterialAssign(_EmissiveColorMode)
            UpdateInternalMaterialAssign(_UVEmissive)

#undef UpdateInternalMaterialAssign
        }

        this->endUploadBatch();

        internal_material.m_intenral_light_mat = now_material;

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

    std::shared_ptr<RHI::D3D12Texture> RenderResource::createTex(std::shared_ptr<MoYu::MoYuScratchImage> scratch_image, std::wstring name, bool batch)
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
                                                      RHI::RHISurfaceCreateFlags::RHISurfaceCreateMipmap, 1, name);

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
                                                               RHI::RHISurfaceCreateFlags::RHISurfaceCreateMipmap, 1, name);
                }
                else
                {
                    _rhiTex = RHI::D3D12Texture::Create2DArray(m_Device->GetLinkedDevice(),
                                                               _texMetaData.width,
                                                               _texMetaData.height,
                                                               _texMetaData.arraySize,
                                                               _texMetaData.mipLevels,
                                                               _texMetaData.format,
                                                               RHI::RHISurfaceCreateFlags::RHISurfaceCreateMipmap, 1, name);
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
                                                  RHI::RHISurfaceCreateFlags::RHISurfaceCreateMipmap, 1, name);

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
        InternalVertexBuffer vertex_buffer = createVertexBuffer<D3D12MeshVertexStandard>(
            mesh_data.m_static_mesh_data.m_InputElementDefinition, mesh_data.m_static_mesh_data.m_vertex_buffer);

        InternalIndexBuffer index_buffer = createIndexBuffer<uint32_t>(mesh_data.m_static_mesh_data.m_index_buffer);

        return InternalMesh {false, mesh_data.m_axis_aligned_box, index_buffer, vertex_buffer};
    }

} // namespace MoYu
