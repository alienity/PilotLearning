#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class IndirectDrawTransparentPass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;
            RHI::RgTextureDesc depthTexDesc;

            ShaderCompiler* m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle renderDataPerDrawHandle;
            RHI::RgResourceHandle propertiesPerMaterialHandle;
            RHI::RgResourceHandle transparentDrawHandle;

            // shadowmap input
            std::vector<RHI::RgResourceHandle> directionalShadowmapTexHandles;
            std::vector<RHI::RgResourceHandle> spotShadowmapTexHandles;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                renderTargetColorHandle.Invalidate();
                renderTargetDepthHandle.Invalidate();
            }

            RHI::RgResourceHandle renderTargetColorHandle;
            RHI::RgResourceHandle renderTargetDepthHandle;
        };

    public:
        ~IndirectDrawTransparentPass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph&         graph,
                    DrawInputParameters&      passInput,
                    DrawOutputParameters&     passOutput);
        void destroy() override final;

    protected:
        bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);

    private:
        RHI::RgTextureDesc colorTexDesc;
        RHI::RgTextureDesc depthTexDesc;

        Shader indirectDrawTransparentVS;
        Shader indirectDrawTransparentPS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirectDrawTransparentSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirectDrawTransparentPSO;
        std::shared_ptr<RHI::D3D12CommandSignature> pIndirectDrawTransparentCommandSignature;
	};
}

