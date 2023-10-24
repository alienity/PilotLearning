#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class IndirectDepthPrePass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc depthTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle meshBufferHandle;
            RHI::RgResourceHandle materialBufferHandle;
            RHI::RgResourceHandle opaqueDrawHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                depthHandle.Invalidate();
            }

            RHI::RgResourceHandle depthHandle;
        };

    public:
        ~IndirectDepthPrePass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    protected:
        bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);

    private:
        Shader indirectDepthPrepassVS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirectDepthPrepassSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirectDepthPrepassPSO;
        std::shared_ptr<RHI::D3D12CommandSignature> pIndirectDepthPrepassCommandSignature;

        RHI::RgTextureDesc depthDesc; // float
	};
}

