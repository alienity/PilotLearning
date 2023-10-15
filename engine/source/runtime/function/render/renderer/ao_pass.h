#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    // https://blog.csdn.net/dengyibing/article/details/115421596
    class AOPass : public RenderPass
	{
    public:
        struct AOInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            DrawInputParameters()
            {
                perframeBufferHandle.Invalidate();
                worldNormalHandle.Invalidate();
                depthHandle.Invalidate();
            }

            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle worldNormalHandle;
            RHI::RgResourceHandle depthHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                outputAOHandle.Invalidate();
            }

            RHI::RgResourceHandle outputAOHandle;
        };

    public:
        ~AOPass() { destroy(); }

        void initialize(const AOInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    protected:
        bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);

        RHI::RgTextureDesc colorTexDesc;

    private:
        Shader SSAOCS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSAOSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSAOPSO;

	};
}

