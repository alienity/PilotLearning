#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_entity.h"
#include "runtime/function/render/render_guid_allocator.h"
#include "runtime/function/render/render_object.h"

#include <optional>
#include <vector>

namespace Pilot
{
    class RenderResource;
    class RenderCamera;
    class RenderSystem;

    class RenderScene
    {
    public:
        // all light
        AmbientLightDesc            m_ambient_light;
        DirectionLightDesc          m_directional_light;
        std::vector<PointLightDesc> m_point_light_list;
        std::vector<SpotLightDesc>  m_spot_light_list;

        // render entities
        std::vector<RenderEntity>   m_render_entities;

    public:
        // all objects (updated per frame)
        std::vector<RenderMeshNode> m_all_mesh_nodes;

        // update all objects for indirect cull in each frame
        void updateAllObjects(std::shared_ptr<RenderResource> render_resource, std::shared_ptr<RenderCamera> camera);

        // set visible nodes ptr in render pass
        void setVisibleNodesReference();

        void deleteEntityByGObjectID(GObjectID go_id);
        void deleteEntityByGObjectID(GObjectID go_id, GComponentID com_id);
        void deleteEntityByGObjectID(GObjectID go_id, GComponentID com_id, uint32_t mesh_asset_id);

        void deleteDirectionLightByGObjectID(GObjectID go_id);
        void deletePointLightByGObjectID(GObjectID go_id);
        void deleteSpotLightByGObjectID(GObjectID go_id);

        void clearForLevelReloading();

        GuidAllocator<MeshSourceDesc>&     getMeshAssetIdAllocator() { return m_mesh_asset_id_allocator; };
        GuidAllocator<MaterialSourceDesc>& getMaterialAssetdAllocator() { return m_material_asset_id_allocator; };

    private:
        friend class RenderSystem;

        GuidAllocator<MeshSourceDesc>        m_mesh_asset_id_allocator;
        GuidAllocator<MaterialSourceDesc>    m_material_asset_id_allocator;
        
    };
} // namespace Piccolo
