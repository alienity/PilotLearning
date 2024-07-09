#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/core/math/moyu_math2.h"

namespace MoYu
{
    void RenderScene::updateLight(SceneLight sceneLight, SceneTransform sceneTransform)
    {
        // remove old light
        if (sceneLight.m_identifier == m_ambient_light.m_identifier && sceneLight.m_light_type != LightType::AmbientLight)
        {
            m_ambient_light.m_identifier = _UndefCommonIdentifier;
        }
        if (sceneLight.m_identifier == m_directional_light.m_identifier && sceneLight.m_light_type != LightType::DirectionLight)
        {
            m_directional_light.m_identifier = _UndefCommonIdentifier;
        }
        for (auto iter = m_spot_light_list.begin(); iter != m_spot_light_list.end();)
        {
            if (iter->m_identifier == sceneLight.m_identifier)
            {
                iter = m_spot_light_list.erase(iter);
                break;
            }
            else
            {
                ++iter;
            }
        }
        for (auto iter = m_point_light_list.begin(); iter != m_point_light_list.end();)
        {
            if (iter->m_identifier == sceneLight.m_identifier)
            {
                iter = m_point_light_list.erase(iter);
                break;
            }
            else
            {
                ++iter;
            }
        }


        // add new light
        glm::float4x4 model_matrix = sceneTransform.m_transform_matrix;

        glm::float3     m_scale;
        glm::quat m_orientation;
        glm::float3     m_translation;
        glm::float3     m_skew;
        glm::float4     m_perspective;
        glm::decompose(model_matrix, m_scale, m_orientation, m_translation, m_skew, m_perspective);

        glm::mat3 RotationMatrix = glm::toMat3(m_orientation);

        glm::float3 forward = glm::float3(RotationMatrix * MYFloat3::Forward);
        glm::float3 up = glm::float3(RotationMatrix * MYFloat3::Up);
        glm::float3 right = glm::float3(RotationMatrix * MYFloat3::Right);

        glm::float3 direction = m_orientation * MYFloat3::Forward;

        if (sceneLight.m_light_type == LightType::AmbientLight)
        {
            m_ambient_light.m_color = sceneLight.ambient_light.m_color;

            m_ambient_light.m_identifier = sceneLight.m_identifier;
            m_ambient_light.m_position   = m_translation;

            m_ambient_light.position = m_translation;
            m_ambient_light.forward = forward;
            m_ambient_light.right = right;
            m_ambient_light.up = up;
        }
        else if (sceneLight.m_light_type == LightType::DirectionLight)
          {
            m_directional_light.m_position_delation = (m_translation - m_directional_light.position);
            m_directional_light.position = m_translation;
            m_directional_light.forward = forward;
            m_directional_light.right = right;
            m_directional_light.up = up;

            int   shadow_bounds_width  = sceneLight.direction_light.m_shadow_bounds.x;
            int   shadow_bounds_height = sceneLight.direction_light.m_shadow_bounds.y;
            float shadow_near_plane    = sceneLight.direction_light.m_shadow_near_plane;
            float shadow_far_plane     = sceneLight.direction_light.m_shadow_far_plane;

            m_directional_light.m_shadowOffset = glm::float4(0, 10, 50, 100);
            m_directional_light.m_shadowPowScale = glm::int4(0, 1, 2, 3);
            
            for (size_t i = 0; i < sceneLight.direction_light.m_cascade; i++)
            {
                int powScale = m_directional_light.m_shadowPowScale[i];
                float shadow_bounds_width_scale  = shadow_bounds_width << powScale;
                float shadow_bounds_height_scale = shadow_bounds_height << powScale;

                glm::float3 m_new_translation = m_translation;// -direction * m_directional_light.m_shadowOffset[i];
                
                glm::float4x4 dirLightViewMat = MYMatrix4x4::createLookAtMatrix(m_new_translation, m_new_translation + direction, MYFloat3::Up);
                glm::float4x4 dirLightProjMat = MYMatrix4x4::createOrthographic(shadow_bounds_width_scale, shadow_bounds_height_scale, shadow_near_plane, shadow_far_plane);
                glm::float4x4 dirLightViewProjMat = dirLightProjMat * dirLightViewMat;
                
                m_directional_light.m_shadow_view_mat[i] = dirLightViewMat;
                m_directional_light.m_shadow_proj_mats[i] = dirLightProjMat;
                m_directional_light.m_shadow_view_proj_mats[i] = dirLightViewProjMat;
            }
            m_directional_light.m_identifier           = sceneLight.m_identifier;
            m_directional_light.m_position             = m_translation;
            m_directional_light.m_direction            = direction;
            m_directional_light.m_color                = sceneLight.direction_light.m_color;
            m_directional_light.m_intensity            = sceneLight.direction_light.m_intensity;
            m_directional_light.m_shadowmap            = sceneLight.direction_light.m_shadowmap;
            m_directional_light.m_cascade              = sceneLight.direction_light.m_cascade;
            m_directional_light.m_shadow_bounds        = sceneLight.direction_light.m_shadow_bounds;
            m_directional_light.m_shadow_near_plane    = sceneLight.direction_light.m_shadow_near_plane;
            m_directional_light.m_shadow_far_plane     = sceneLight.direction_light.m_shadow_far_plane;
            m_directional_light.m_shadowmap_size       = sceneLight.direction_light.m_shadowmap_size;
        }
        else if (sceneLight.m_light_type == LightType::PointLight)
        {
            int index_finded = -1;
            for (int i = 0; i < m_point_light_list.size(); i++)
            {
                if (m_point_light_list[i].m_identifier == sceneLight.m_identifier)
                {
                    index_finded = i;
                    break;
                }
            }

            InternalPointLight* pInternalPointLight;

            if (index_finded == -1)
            {
                InternalPointLight& internalPointLight = m_point_light_list.emplace_back();
                pInternalPointLight = &internalPointLight;
            }
            else
            {
                pInternalPointLight = &m_point_light_list[index_finded];
            }

            pInternalPointLight->m_identifier = sceneLight.m_identifier;
            pInternalPointLight->m_position = m_translation;
            pInternalPointLight->m_color = sceneLight.point_light.m_color;
            pInternalPointLight->m_intensity = sceneLight.point_light.m_intensity;
            pInternalPointLight->m_radius = sceneLight.point_light.m_radius;

            pInternalPointLight->position = m_translation;
            pInternalPointLight->forward = forward;
            pInternalPointLight->right = right;
            pInternalPointLight->up = up;
        }
        else if (sceneLight.m_light_type == LightType::SpotLight)
        {
            int index_finded = -1;
            for (int i = 0; i < m_spot_light_list.size(); i++)
            {
                if (m_spot_light_list[i].m_identifier == sceneLight.m_identifier)
                {
                    index_finded = i;
                    break;
                }
            }

            float _spotOutRadians = MoYu::degreesToRadians(sceneLight.spot_light.m_outer_degree);
            float _spotNearPlane  = sceneLight.spot_light.m_shadow_near_plane;
            float _spotFarPlane   = sceneLight.spot_light.m_shadow_far_plane;
            
            glm::float4x4 spotLightViewMat = MYMatrix4x4::createLookAtMatrix(m_translation, m_translation + direction, MYFloat3::Up);
            glm::float4x4 spotLightProjMat = MYMatrix4x4::createPerspectiveFieldOfView(_spotOutRadians, 1, _spotNearPlane, _spotFarPlane);
            glm::float4x4 spotLightViewProjMat = spotLightProjMat * spotLightViewMat;

            InternalSpotLight* pInternalSpotLight;

            if (index_finded == -1)
            {
                InternalSpotLight& internalSpotLight = m_spot_light_list.emplace_back();
                pInternalSpotLight = &internalSpotLight;
            }
            else
            {
                pInternalSpotLight = &m_spot_light_list[index_finded];
            }

            pInternalSpotLight->m_position_delation = (m_translation - pInternalSpotLight->m_position);;

            pInternalSpotLight->m_identifier           = sceneLight.m_identifier;
            pInternalSpotLight->m_position             = m_translation;
            pInternalSpotLight->m_direction            = direction;
            pInternalSpotLight->m_shadow_view_mat      = spotLightViewMat;
            pInternalSpotLight->m_shadow_proj_mats     = spotLightProjMat;
            pInternalSpotLight->m_shadow_view_proj_mat = spotLightViewProjMat;
            pInternalSpotLight->m_color                = sceneLight.spot_light.m_color;
            pInternalSpotLight->m_intensity            = sceneLight.spot_light.m_intensity;
            pInternalSpotLight->m_radius               = sceneLight.spot_light.m_radius;
            pInternalSpotLight->m_inner_degree         = sceneLight.spot_light.m_inner_degree;
            pInternalSpotLight->m_outer_degree         = sceneLight.spot_light.m_outer_degree;
            pInternalSpotLight->m_shadowmap            = sceneLight.spot_light.m_shadowmap;
            pInternalSpotLight->m_shadow_bounds        = sceneLight.spot_light.m_shadow_bounds;
            pInternalSpotLight->m_shadow_near_plane    = sceneLight.spot_light.m_shadow_near_plane;
            pInternalSpotLight->m_shadow_far_plane     = sceneLight.spot_light.m_shadow_far_plane;
            pInternalSpotLight->m_shadowmap_size       = sceneLight.spot_light.m_shadowmap_size;

            pInternalSpotLight->position = m_translation;
            pInternalSpotLight->forward = forward;
            pInternalSpotLight->right = right;
            pInternalSpotLight->up = up;
        }
    }

    void RenderScene::updateMeshRenderer(SceneMeshRenderer sceneMeshRenderer, SceneTransform sceneTransform, std::shared_ptr<RenderResource> m_render_resource)
    {
        const glm::float4x4 model_matrix = sceneTransform.m_transform_matrix;
        const glm::float4x4 model_matrix_inverse = glm::inverse(model_matrix);

        int mesh_finded = -1;
        std::vector<CachedMeshRenderer>& _mesh_renderers = m_mesh_renderers;
        for (int j = 0; j < _mesh_renderers.size(); j++)
        {
            if (_mesh_renderers[j].cachedSceneMeshrenderer.m_identifier == sceneMeshRenderer.m_identifier)
            {
                mesh_finded = j;
                break;
            }
        }
        if (mesh_finded == -1)
        {
            CachedMeshRenderer cachedMeshRenderer;
            m_render_resource->updateInternalMeshRenderer(sceneMeshRenderer,
                                                          cachedMeshRenderer.cachedSceneMeshrenderer,
                                                          cachedMeshRenderer.internalMeshRenderer,
                                                          false);
            cachedMeshRenderer.internalMeshRenderer.model_matrix         = model_matrix;
            cachedMeshRenderer.internalMeshRenderer.model_matrix_inverse = model_matrix_inverse;
            cachedMeshRenderer.internalMeshRenderer.prev_model_matrix         = model_matrix;
            cachedMeshRenderer.internalMeshRenderer.prev_model_matrix_inverse = model_matrix_inverse;

            _mesh_renderers.push_back(cachedMeshRenderer);
        }
        else
        {
            glm::mat4x4 prev_model_matrix = _mesh_renderers[mesh_finded].internalMeshRenderer.model_matrix;
            glm::mat4x4 prev_model_matrix_inverse = _mesh_renderers[mesh_finded].internalMeshRenderer.model_matrix_inverse;


            m_render_resource->updateInternalMeshRenderer(sceneMeshRenderer,
                                                          _mesh_renderers[mesh_finded].cachedSceneMeshrenderer,
                                                          _mesh_renderers[mesh_finded].internalMeshRenderer,
                                                          true);
            _mesh_renderers[mesh_finded].internalMeshRenderer.prev_model_matrix = prev_model_matrix;
            _mesh_renderers[mesh_finded].internalMeshRenderer.prev_model_matrix_inverse = prev_model_matrix_inverse;
            _mesh_renderers[mesh_finded].internalMeshRenderer.model_matrix         = model_matrix;
            _mesh_renderers[mesh_finded].internalMeshRenderer.model_matrix_inverse = model_matrix_inverse;
        }
    }

    void RenderScene::updateCamera(SceneCamera sceneCamera, SceneTransform sceneTransform)
    {
        //const Matrix4x4 model_matrix = sceneTransform.m_transform_matrix;
        //const Matrix4x4 model_matrix_inverse = GLMUtil::inverseMat4x4(model_matrix);

        const glm::float3 position = sceneTransform.m_position;
        const glm::quat rotation = sceneTransform.m_rotation;

        const glm::float3 direction = rotation * MYFloat3::Forward;

        const glm::float4x4 viewMatrix = MYMatrix4x4::createLookAtMatrix(position, position + direction, MYFloat3::Up);
        const glm::float4x4 viewMatrixInv = glm::inverse(viewMatrix);

        glm::float4x4 projMatrix = MYMatrix4x4::Identity;
        if (sceneCamera.m_projType == CameraProjType::Perspective)
        {
            projMatrix = MYMatrix4x4::createPerspective(
                sceneCamera.m_width, sceneCamera.m_height, sceneCamera.m_znear, sceneCamera.m_zfar);
        }
        else
        {
            projMatrix = MYMatrix4x4::createOrthographic(
                sceneCamera.m_width, sceneCamera.m_height, sceneCamera.m_znear, sceneCamera.m_zfar);
        }

        m_camera.m_identifier = sceneCamera.m_identifier;
        m_camera.m_projType   = sceneCamera.m_projType;
        m_camera.m_position   = position;
        m_camera.m_rotation   = rotation;

        m_camera.m_width  = sceneCamera.m_width;
        m_camera.m_height = sceneCamera.m_height;
        m_camera.m_znear  = sceneCamera.m_znear;
        m_camera.m_zfar   = sceneCamera.m_zfar;
        m_camera.m_aspect = sceneCamera.m_width / sceneCamera.m_height;
        m_camera.m_fovY   = sceneCamera.m_fovY;

        m_camera.m_ViewMatrix    = viewMatrix;
        m_camera.m_ViewMatrixInv = viewMatrixInv;

        m_camera.m_ProjMatrix    = projMatrix;
        m_camera.m_ProjMatrixInv = glm::inverse(projMatrix);

        m_camera.m_ViewProjMatrix    = projMatrix * viewMatrix;
        m_camera.m_ViewProjMatrixInv = m_camera.m_ProjMatrixInv * m_camera.m_ViewMatrixInv;

    }

    void RenderScene::updateTerrainRenderer(SceneTerrainRenderer sceneTerrainRenderer, SceneTransform sceneTransform, std::shared_ptr<RenderResource> m_render_resource)
    {
        const glm::float4x4 model_matrix = sceneTransform.m_transform_matrix;
        const glm::float4x4 model_matrix_inverse = glm::inverse(model_matrix);

        int mesh_finded = -1;
        std::vector<CachedTerrainRenderer>& _mesh_renderers = m_terrain_renderers;
        for (int j = 0; j < _mesh_renderers.size(); j++)
        {
            if (_mesh_renderers[j].cachedSceneTerrainRenderer.m_identifier == sceneTerrainRenderer.m_identifier)
            {
                mesh_finded = j;
                break;
            }
        }
        if (m_terrain_render_helper == nullptr)
        {
            m_terrain_render_helper = std::make_shared<TerrainRenderHelper>();
        }
        if (mesh_finded == -1)
        {
            CachedTerrainRenderer cachedMeshRenderer;
            m_terrain_render_helper->updateInternalTerrainRenderer(m_render_resource.get(),
                                                                   sceneTerrainRenderer,
                                                                   cachedMeshRenderer.cachedSceneTerrainRenderer,
                                                                   cachedMeshRenderer.internalTerrainRenderer,
                                                                   model_matrix,
                                                                   model_matrix_inverse,
                                                                   model_matrix,
                                                                   model_matrix_inverse,
                                                                   false);

            _mesh_renderers.push_back(cachedMeshRenderer);
        }
        else
        {
            glm::mat4x4 prev_model_matrix = _mesh_renderers[mesh_finded].internalTerrainRenderer.model_matrix;
            glm::mat4x4 prev_model_matrix_inverse = _mesh_renderers[mesh_finded].internalTerrainRenderer.model_matrix_inverse;

            m_terrain_render_helper->updateInternalTerrainRenderer(
                m_render_resource.get(),
                sceneTerrainRenderer,
                _mesh_renderers[mesh_finded].cachedSceneTerrainRenderer,
                _mesh_renderers[mesh_finded].internalTerrainRenderer,
                model_matrix,
                model_matrix_inverse,
                prev_model_matrix,
                prev_model_matrix_inverse,
                true);
        }
    }

    void RenderScene::removeLight(SceneLight sceneLight)
    {
        if (sceneLight.m_light_type == LightType::AmbientLight)
        {
            m_ambient_light = {};
        }
        else if (sceneLight.m_light_type == LightType::DirectionLight)
        {
            m_directional_light = {};
        }
        else if (sceneLight.m_light_type == LightType::PointLight)
        {
            for (int i = 0; i < m_point_light_list.size(); i++)
            {
                if (m_point_light_list[i].m_identifier == sceneLight.m_identifier)
                {
                    m_point_light_list.erase(m_point_light_list.begin() + i);
                    break;
                }
            }
        }
        else if (sceneLight.m_light_type == LightType::SpotLight)
        {
            for (int i = 0; i < m_spot_light_list.size(); i++)
            {
                if (m_spot_light_list[i].m_identifier == sceneLight.m_identifier)
                {
                    m_spot_light_list.erase(m_spot_light_list.begin() + i);
                    break;
                }
            }
        }

    }

    void RenderScene::removeMeshRenderer(SceneMeshRenderer sceneMeshRenderer)
    {
        std::vector<CachedMeshRenderer>& _mesh_renderers = m_mesh_renderers;
        for (int j = 0; j < _mesh_renderers.size(); j++)
        {
            if (_mesh_renderers[j].cachedSceneMeshrenderer.m_identifier == sceneMeshRenderer.m_identifier)
            {
                _mesh_renderers.erase(_mesh_renderers.begin() + j);
                break;
            }
        }
    }

    void RenderScene::removeCamera(SceneCamera sceneCamera)
    {
        m_camera = {};
    }

    void RenderScene::removeTerrainRenderer(SceneTerrainRenderer sceneTerrainRenderer)
    {
        std::vector<CachedTerrainRenderer>& _mesh_renderers = m_terrain_renderers;
        for (int j = 0; j < _mesh_renderers.size(); j++)
        {
            if (_mesh_renderers[j].cachedSceneTerrainRenderer.m_identifier == sceneTerrainRenderer.m_identifier)
            {
                _mesh_renderers.erase(_mesh_renderers.begin() + j);
                break;
            }
        }
    }

    void RenderScene::updateTerrainClipmap(glm::float3 cameraPos, RenderResource* m_render_resource)
    {
        if (!m_terrain_renderers.empty())
        {
            InternalTerrain* internalTerrain = &m_terrain_renderers[0].internalTerrainRenderer.ref_terrain;

            m_terrain_render_helper->UpdateInternalTerrainClipmap(internalTerrain, cameraPos, m_render_resource);
        }
    }

}
