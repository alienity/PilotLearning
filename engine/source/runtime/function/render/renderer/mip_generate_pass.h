#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    enum MipGenerateMode
    {
        AverageType,
        MaxType,
        MinType
    };

    class MipGeneratePass : public RenderPass
	{
    public:
        struct DrawInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            DrawInputParameters()
            {
                texHandle.Invalidate();
                mipGenMode = MipGenerateMode::AverageType;
            }

            RHI::RgResourceHandle texHandle;
            MipGenerateMode       mipGenMode;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                outputPyramidHandle.Invalidate();
            }

            RHI::RgResourceHandle outputPyramidHandle;
        };

    public:
        ~MipGeneratePass() { destroy(); }

        void initialize(const DrawInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:

	};
}

