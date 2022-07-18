#pragma once

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_resource.h"

#include <memory>
#include <vector>

namespace Pilot
{
    
    struct VisiableNodes
    {
        std::vector<RenderMeshNode>*              p_directional_light_visible_mesh_nodes {nullptr};
        std::vector<RenderMeshNode>*              p_point_lights_visible_mesh_nodes {nullptr};
        std::vector<RenderMeshNode>*              p_main_camera_visible_mesh_nodes {nullptr};
        RenderAxisNode*                           p_axis_node {nullptr};
        std::vector<RenderParticleBillboardNode>* p_main_camera_visible_particlebillboard_nodes {nullptr};
    };

    class RenderPass
    {
    public:
        
        static VisiableNodes m_visiable_nodes;

    private:
    };
} // namespace Piccolo
