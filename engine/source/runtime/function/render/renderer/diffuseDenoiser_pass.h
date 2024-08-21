#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class DiffuseDenoisePass : public RenderPass
	{
    public:
        struct DDInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc    m_ColorTexDesc;
            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle colorPyramidHandle;
            RHI::RgResourceHandle depthPyramidHandle;
            RHI::RgResourceHandle lastDepthPyramidHandle;
            RHI::RgResourceHandle normalBufferHandle;
            RHI::RgResourceHandle cameraMotionVectorHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle ssrOutHandle;
        };

    public:
        ~DiffuseDenoisePass() { destroy(); }

        void initialize(const DDInitInfo& init_info);
        void prepareMetaData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        Shader BilateralFilterSingleCS;
        Shader BilateralFilterColorCS;

        std::shared_ptr<RHI::D3D12RootSignature> pBilateralFilterSingleSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pBilateralFilterSinglePSO;

        std::shared_ptr<RHI::D3D12RootSignature> pBilateralFilterColorSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pBilateralFilterColorPSO;

        RHI::RgTextureDesc colorTexDesc;
	};
}

