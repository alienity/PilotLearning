#pragma once

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_pass_base.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/function/render/renderer/renderer_registry.h"

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

    struct PassInput
    {};

    struct PassOutput
    {};

    class RenderPass : public RenderPassBase
    {
    public:
        GlobalRenderResource*      m_global_render_resource {nullptr};

        void initialize(const RenderPassInitInfo* init_info) override;
        void postInitialize() override;

        virtual void update(RHI::D3D12CommandContext& context,
                            RHI::RenderGraph&         graph,
                            PassInput&                passInput,
                            PassOutput&               passOutput);

        virtual void destroy();

        static VisiableNodes m_visiable_nodes;

    private:
    };
} // namespace Piccolo
