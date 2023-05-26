#pragma once

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_pass_base.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/function/render/renderer/renderer_registry.h"

#include <memory>
#include <vector>

namespace MoYu
{
    
    struct VisiableNodes
    {
        std::vector<RenderMeshNode>* p_all_mesh_nodes {nullptr};
        AmbientLightDesc*            p_ambient_light {nullptr};
        DirectionLightDesc*          p_directional_light {nullptr};
        std::vector<PointLightDesc>* p_point_light_list {nullptr};
        std::vector<SpotLightDesc>*  p_spot_light_list {nullptr};
    };

    class RenderPass : public RenderPassBase
    {
    public:
        GlobalRenderResource* m_global_render_resource {nullptr};

        static VisiableNodes m_visiable_nodes;

    private:
    };
} // namespace MoYu
