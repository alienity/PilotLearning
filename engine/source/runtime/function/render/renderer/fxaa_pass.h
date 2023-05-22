#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class FXAAPass : public RenderPass
	{
    public:
        struct FXAAInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc       m_ColorTexDesc;
            EngineConfig::FXAAConfig m_FXAAConfig;
            ShaderCompiler*          m_ShaderCompiler;
            std::filesystem::path    m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            DrawInputParameters()
            {
                inputSceneColorHandle.Invalidate();
                inputLumaColorHandle.Invalidate();
            }

            RHI::RgResourceHandle inputSceneColorHandle;
            RHI::RgResourceHandle inputLumaColorHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                outputColorHandle.Invalidate();
            }

            RHI::RgResourceHandle outputColorHandle;
        };

    public:
        ~FXAAPass() { destroy(); }

        void initialize(const FXAAInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        Shader FXAAToLuminanceCS;
        Shader FXAALuminanceCS;
        Shader FXAAGreenCS;

        std::shared_ptr<RHI::D3D12RootSignature> pFXAAToLuminanceSignature;
        std::shared_ptr<RHI::D3D12RootSignature> pFXAALuminanceSignature;
        std::shared_ptr<RHI::D3D12RootSignature> pFXAAGreenSignature;

        std::shared_ptr<RHI::D3D12PipelineState> pFXAAToLuminancePSO;
        std::shared_ptr<RHI::D3D12PipelineState> pFXAALuminancePSO;
        std::shared_ptr<RHI::D3D12PipelineState> pFXAAGreenPSO;

        RHI::RgTextureDesc mTmpColorTexDesc;

	};
}

