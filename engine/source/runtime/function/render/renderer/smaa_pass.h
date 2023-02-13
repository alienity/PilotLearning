#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class SMAAPass : public RenderPass
	{
    public:
        struct SMAAInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc       m_ColorTexDesc;
            EngineConfig::SMAAConfig m_SMAAConfig;
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
        ~SMAAPass() { destroy(); }

        void initialize(const SMAAInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:

        RHI::RgTextureDesc mTmpColorTexDesc;
	};
}

