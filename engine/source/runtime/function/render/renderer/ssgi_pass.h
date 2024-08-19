#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class SSGIPass : public RenderPass
	{
    public:
        struct SSGIInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc    m_ColorTexDesc;
            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle worldNormalHandle;
            RHI::RgResourceHandle mrraMapHandle;
            RHI::RgResourceHandle maxDepthPtyramidHandle;
            RHI::RgResourceHandle lastFrameColorHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle ssrOutHandle;
        };

    public:
        ~SSGIPass() { destroy(); }

        void initialize(const SSGIInitInfo& init_info);
        void prepareMetaData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

        std::shared_ptr<RHI::D3D12Texture> getCurTemporalResult();
        std::shared_ptr<RHI::D3D12Texture> getPrevTemporalResult();

    private:
        int passIndex;

        Shader SSTraceGICS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSTraceGISignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSTraceGIPSO;

        Shader SSReporjectGICS;
        std::shared_ptr<RHI::D3D12RootSignature> pSSReporjectGISignature;
        std::shared_ptr<RHI::D3D12PipelineState> pSSReporjectGIPSO;

        RHI::RgTextureDesc colorTexDesc;
	};
}

