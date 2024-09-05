#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    struct SSROutput
    {
        SSROutput()
        {
            motionVectorHandle.Invalidate();
            depthHandle.Invalidate();
        }

        RHI::RgResourceHandle motionVectorHandle;
        RHI::RgResourceHandle depthHandle;
    };

    class SSRPass : public RenderPass
	{
    public:
        struct SSRInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc    m_ColorTexDesc;
            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle worldNormalHandle;
            RHI::RgResourceHandle depthTextureHandle;
            RHI::RgResourceHandle motionVectorHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle ssrOutHandle;
        };

    public:
        ~SSRPass() { destroy(); }

        void initialize(const SSRInitInfo& init_info);
        void prepareMetaData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

        std::shared_ptr<RHI::D3D12Texture> getSsrAccum(bool needPrev = false);

    private:
        int passIndex;

        Shader SSRTraceCS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSRTraceSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSRTracePSO;

        Shader SSRReprojectCS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSRReprojectSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSRReprojectPSO;

        Shader SSRAccumulateCS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSRAccumulateSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSRAccumulatePSO;

        RHI::RgTextureDesc colorTexDesc;
        RHI::RgTextureDesc SSRHitPointTextureDesc;
        //RHI::RgTextureDesc SSRAccumTextureDesc;
        //RHI::RgTextureDesc SSRLightingTextureDesc;

        std::shared_ptr<RHI::D3D12Texture> m_owenScrambled256Tex;
        std::shared_ptr<RHI::D3D12Texture> m_scramblingTile8SPP;
        std::shared_ptr<RHI::D3D12Texture> m_rankingTile8SPP;

        std::shared_ptr<RHI::D3D12Buffer> pSSRConstBuffer;;

        std::shared_ptr<RHI::D3D12Texture> pSSRAccumTexture[2];
        std::shared_ptr<RHI::D3D12Texture> pSSRLightingTexture;
	};
}

