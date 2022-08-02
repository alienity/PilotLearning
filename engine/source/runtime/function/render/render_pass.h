#pragma once

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_pass_base.h"
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

    class RenderPass : public RenderPassBase
    {
    public:
        //struct FrameBufferAttachment
        //{
        //    VkImage        image;
        //    VkDeviceMemory mem;
        //    VkImageView    view;
        //    VkFormat       format;
        //};

        //struct Framebuffer
        //{
        //    int           width;
        //    int           height;
        //    VkFramebuffer framebuffer;
        //    VkRenderPass  render_pass;

        //    std::vector<FrameBufferAttachment> attachments;
        //};

        //struct Descriptor
        //{
        //    VkDescriptorSetLayout layout;
        //    VkDescriptorSet       descriptor_set;
        //};

        //struct RenderPipelineBase
        //{
        //    VkPipelineLayout layout;
        //    VkPipeline       pipeline;
        //};

        GlobalRenderResource*      m_global_render_resource {nullptr};

        //std::vector<Descriptor>         m_descriptor_infos;
        //std::vector<RenderPipelineBase> m_render_pipelines;
        //Framebuffer                     m_framebuffer;

        void initialize(const RenderPassInitInfo* init_info) override;
        void postInitialize() override;

        virtual void draw();

        //virtual VkRenderPass                       getRenderPass() const;
        //virtual std::vector<VkImageView>           getFramebufferImageViews() const;
        //virtual std::vector<VkDescriptorSetLayout> getDescriptorSetLayouts() const;

        static VisiableNodes m_visiable_nodes;

    private:
    };
} // namespace Piccolo
