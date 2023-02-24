#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class ExposurePass : public RenderPass
	{
    public:
        struct ExposureInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc       m_ColorTexDesc;
            EngineConfig::HDRConfig  m_HDRConfig;
            ShaderCompiler*          m_ShaderCompiler;
            std::filesystem::path    m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            DrawInputParameters()
            {
                inputColorHandle.Invalidate();
            }

            RHI::RgResourceHandle inputColorHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                targetColorHandle.Invalidate();
            }

            RHI::RgResourceHandle targetColorHandle;
        };

    public:
        ~ExposurePass() { destroy(); }

        void initialize(const ExposureInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:

        RHI::RgTextureDesc mTmpColorTexDesc;

        std::shared_ptr<RHI::D3D12Buffer> p_ExposureBuffer;

	};
}

