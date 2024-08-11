#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/pass_helper.h"

namespace MoYu
{
    class IndirectMotionVectorPass : public RenderPass
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
            RHI::RgResourceHandle renderDataPerDrawHandle;
            RHI::RgResourceHandle propertiesPerMaterialHandle;
            RHI::RgResourceHandle depthBufferHandle;
            RHI::RgResourceHandle opaqueDrawHandle;
        };

        struct DrawOutputParameters : public PassInput
        {
            RHI::RgResourceHandle motionVectorHandle;
            RHI::RgResourceHandle depthBufferHandle;
        };

    public:
        ~IndirectMotionVectorPass() { destroy(); }

        void prepareMatBuffer(std::shared_ptr<RenderResource> render_resource);

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

        //bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);

    private:

        RHI::RgTextureDesc motionVectorDesc;

        std::shared_ptr<RHI::D3D12Texture> pMotionVectorRT;

        Shader drawMotionVectorVS;
        Shader drawMotionVectorPS;
        std::shared_ptr<RHI::D3D12RootSignature> pMotionVectorSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pMotionVectorPSO;
        std::shared_ptr<RHI::D3D12CommandSignature> pMotionVectorCommandSignature;
	};
}

