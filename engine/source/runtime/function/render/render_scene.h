#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_guid_allocator.h"

#include <optional>
#include <vector>

namespace MoYu
{
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
    };
} // namespace MoYu
