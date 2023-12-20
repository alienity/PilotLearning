#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_guid_allocator.h"
#include "runtime/function/render/terrain_render_helper.h"

#include <vector>

namespace MoYu
{
    class RenderResource;

    struct CachedMeshRenderer
    {
        SceneMeshRenderer cachedSceneMeshrenderer;
        InternalMeshRenderer internalMeshRenderer;
    };

    struct CachedTerrainRenderer
    {
        SceneTerrainRenderer cachedSceneTerrainRenderer;
        InternalTerrainRenderer internalTerrainRenderer;
    };

    class RenderScene
    {
    public:
        // skybox
        SkyboxConfigs m_skybox_map;

        // ibl
        IBLConfigs m_ibl_map;

        // all light
        InternalAmbientLight            m_ambient_light;
        InternalDirectionLight          m_directional_light;
        std::vector<InternalPointLight> m_point_light_list;
        std::vector<InternalSpotLight>  m_spot_light_list;

        // render entities
        std::vector<CachedMeshRenderer> m_mesh_renderers;

        // terrain entities
        std::vector<CachedTerrainRenderer> m_terrain_renderers;

        // camera
        InternalCamera m_camera;

        // update functions
        void updateLight(SceneLight sceneLight, SceneTransform sceneTransform);
        void updateMeshRenderer(SceneMeshRenderer sceneMeshRenderer, SceneTransform sceneTransform, std::shared_ptr<RenderResource> m_render_resource);
        void updateCamera(SceneCamera sceneCamera, SceneTransform sceneTransform);
        void updateTerrainRenderer(SceneTerrainRenderer sceneTerrainRenderer, SceneTransform sceneTransform, std::shared_ptr<RenderResource> m_render_resource);

        // remove component
        void removeLight(SceneLight sceneLight);
        void removeMeshRenderer(SceneMeshRenderer sceneMeshRenderer);
        void removeCamera(SceneCamera sceneCamera);
        void removeTerrainRenderer(SceneTerrainRenderer sceneTerrainRenderer);

        // update terrain mesh data
        void updateTerrainClipmap(glm::float3 cameraPos, RenderResource* m_render_resource);

    private:
        std::shared_ptr<TerrainRenderHelper> m_terrain_render_helper;
    };
} // namespace MoYu
