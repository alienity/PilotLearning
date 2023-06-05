#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_guid_allocator.h"

#include <optional>
#include <vector>

namespace MoYu
{
    class RenderResource;
    class RenderCamera;
    class RenderSystem;

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

    public:

        // update all objects for indirect cull in each frame
        void updateAllObjects(std::shared_ptr<RenderResource> render_resource, std::shared_ptr<RenderCamera> camera);

        // set visible nodes ptr in render pass
        void setVisibleNodesReference();

        void deleteMeshRendererByGObjectID(GObjectID go_id, GComponentID com_id);

        void deleteLightByGObjectID(GObjectID go_id, GComponentID com_id);

        void clearForLevelReloading();
    private:
        friend class RenderSystem;

        //GuidAllocator<SceneMesh> m_mesh_asset_id_allocator;
        //GuidAllocator<ScenePBRMaterial> m_material_asset_id_allocator;
        
    };
} // namespace MoYu
