#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    struct TAAOutput
    {
        TAAOutput()
        {
            motionVectorHandle.Invalidate();
            depthHandle.Invalidate();
        }

        RHI::RgResourceHandle motionVectorHandle;
        RHI::RgResourceHandle depthHandle;
    };

    class TAAPass : public RenderPass
	{
    public:
        struct TAAInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc       m_ColorTexDesc;
            ShaderCompiler*          m_ShaderCompiler;
            std::filesystem::path    m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            DrawInputParameters()
            {
                perframeBufferHandle.Invalidate();
                colorBufferHandle.Invalidate();
                depthBufferHandle.Invalidate();
                motionVectorHandle.Invalidate();
            }

            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle colorBufferHandle;
            RHI::RgResourceHandle depthBufferHandle;
            RHI::RgResourceHandle motionVectorHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                aaOutHandle.Invalidate();
            }

            RHI::RgResourceHandle aaOutHandle;
        };

    public:
        ~TAAPass() { destroy(); }

        void initialize(const TAAInitInfo& init_info);
        void prepareTAAMetaData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        RHI::RgTextureDesc colorDesc;

        Shader TemporalAntiAliasingCS;
        std::shared_ptr<RHI::D3D12RootSignature> pTemporalAntiAliasingSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pTemporalAntiAliasingPSO;

        std::shared_ptr<RHI::D3D12Texture> historyTexture[2];

        int indexRead;
        int indexWrite;
	};
}

