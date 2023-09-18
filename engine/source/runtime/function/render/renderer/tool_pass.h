#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class ToolPass : public RenderPass
    {
    public:
        struct ToolPassInitInfo : public RenderPassInitInfo
        {};

        struct ToolInputParameters : public PassInput
        {
            std::shared_ptr<RHI::D3D12Texture> p_IBLSpecular;
        };

        struct ToolOutputParameters : public PassOutput
        {
            std::shared_ptr<RHI::D3D12Texture> p_Radians;
            std::shared_ptr<RHI::D3D12Texture> p_DFG;
            std::shared_ptr<RHI::D3D12Texture> p_LD;
        };

    public:
        ~ToolPass() { destroy(); }

        void initialize(const ToolPassInitInfo& init_info);
        void editorUpdate(RHI::D3D12CommandContext* context, ToolInputParameters& passInput, ToolOutputParameters& passOutput);
        void update(RHI::RenderGraph& graph, ToolInputParameters& passInput, ToolOutputParameters& passOutput);
        void destroy() override final;

    private:
        std::shared_ptr<RHI::D3D12Texture> p_DFG;
        std::shared_ptr<RHI::D3D12Texture> p_LD;
        std::shared_ptr<RHI::D3D12Texture> p_DiffuseRadians;
    };
}

