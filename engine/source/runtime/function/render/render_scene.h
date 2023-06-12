#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_guid_allocator.h"

#include <vector>

namespace MoYu
{
    class RenderResource;

    struct CachedMeshRenderer
    {
        SceneMeshRenderer cachedSceneMeshrenderer;
        InternalMeshRenderer internalMeshRenderer;
    };

    class RenderScene
    {
    public:
        // all light
        InternalAmbientLight            m_ambient_light;
        InternalDirectionLight          m_directional_light;
        std::vector<InternalPointLight> m_point_light_list;
        std::vector<InternalSpotLight>  m_spot_light_list;

        // render entities
        std::vector<CachedMeshRenderer> m_mesh_renderers;

        // camera
        InternalCamera m_camera;

        // update functions
        void updateLight(SceneLight sceneLight, SceneTransform sceneTransform);
        void updateMeshRenderer(SceneMeshRenderer sceneMeshRenderer, SceneTransform sceneTransform, std::shared_ptr<RenderResource> m_render_resource);
        void updateCamera(SceneCamera sceneCamera, SceneTransform sceneTransform);

        // remove component
        void removeLight(SceneLight sceneLight);
        void removeMeshRenderer(SceneMeshRenderer sceneMeshRenderer);
        void removeCamera(SceneCamera sceneCamera);
    };
} // namespace MoYu
