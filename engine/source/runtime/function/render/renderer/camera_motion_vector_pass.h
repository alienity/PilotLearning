#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/pass_helper.h"

namespace MoYu
{
    class CameraMotionVectorPass : public RenderPass
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
            RHI::RgResourceHandle depthBufferHandle;
        };

        struct DrawOutputParameters : public PassInput
        {
            RHI::RgResourceHandle motionVectorHandle;
        };

    public:
        ~CameraMotionVectorPass() { destroy(); }

        void prepareMatBuffer(std::shared_ptr<RenderResource> render_resource);

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:

        RHI::RgTextureDesc motionVectorDesc;

        Shader cameraMotionVectorVS;
        Shader cameraMotionVectorPS;
        std::shared_ptr<RHI::D3D12RootSignature> pCameraMotionVectorSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pCameraMotionVectorPSO;
	};
}

