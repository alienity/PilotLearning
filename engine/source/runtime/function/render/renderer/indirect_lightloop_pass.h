#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    // https://therealmjp.github.io/posts/bindless-texturing-for-deferred-rendering-and-decals/
    class IndirectLightLoopPass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle albedoHandle;
            RHI::RgResourceHandle worldNormalHandle;
            RHI::RgResourceHandle worldTangentHandle;
            RHI::RgResourceHandle materialNormalHandle;
            RHI::RgResourceHandle emissiveHandle;
            RHI::RgResourceHandle metallic_Roughness_Reflectance_AO_Handle;
            RHI::RgResourceHandle clearCoat_ClearCoatRoughness_Anisotropy_Handle;
            RHI::RgResourceHandle gbufferDepthHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                colorHandle.Invalidate();
            }

            RHI::RgResourceHandle colorHandle;
        };

    public:
        ~IndirectLightLoopPass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph&         graph,
                    DrawInputParameters&      passInput,
                    DrawOutputParameters&     passOutput);
        void destroy() override final;

    protected:
        bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);

        RHI::RgTextureDesc colorTexDesc;

    private:
        Shader indirectLightLoopCS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirectLightLoopSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirectLightLoopPSO;
	};
}

