#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class ExtractLumaPass : public RenderPass
	{
    public:
        struct ExtractLumaInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc    m_ColorTexDesc;
            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            DrawInputParameters()
            {
                inputSceneColorHandle.Invalidate();
                inputExposureHandle.Invalidate();
            }

            RHI::RgResourceHandle inputSceneColorHandle;
            RHI::RgResourceHandle inputExposureHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                outputLumaLRHandle.Invalidate();
            }

            RHI::RgResourceHandle outputLumaLRHandle;
        };

    public:
        ~ExtractLumaPass() { destroy(); }

        void initialize(const ExtractLumaInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        Shader                                   ExtractLumaCS;
        std::shared_ptr<RHI::D3D12RootSignature> pExtractLumaCSSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pExtractLumaCSPSO;
        
        RHI::RgTextureDesc m_LumaLRDesc;

	};
}

