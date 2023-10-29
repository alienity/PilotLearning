#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    enum MipGenerateMode
    {
        Average,
        Max,
        Min
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
                mipGenMode = MipGenerateMode::Average;
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

    //protected:
    //    RHI::RgTextureDesc depthPyramidDesc;
    //    RHI::RgTextureDesc depthMinPyramidDesc;
    //    RHI::RgTextureDesc depthMaxPyramidDesc;

    private:
        Shader GenerateMipsLinearCS;
        Shader GenerateMipsLinearOddCS;
        Shader GenerateMipsLinearOddXCS;
        Shader GenerateMipsLinearOddYCS;

        Shader GenerateMaxMipsLinearCS;
        Shader GenerateMaxMipsLinearOddCS;
        Shader GenerateMaxMipsLinearOddXCS;
        Shader GenerateMaxMipsLinearOddYCS;

        Shader GenerateMinMipsLinearCS;
        Shader GenerateMinMipsLinearOddCS;
        Shader GenerateMinMipsLinearOddXCS;
        Shader GenerateMinMipsLinearOddYCS;

        std::shared_ptr<RHI::D3D12RootSignature> pGenerateMipsLinearSignature;

        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMipsLinearPSO;
        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMipsLinearOddPSO;
        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMipsLinearOddXPSO;
        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMipsLinearOddYPSO;

        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMaxMipsLinearPSO;
        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMaxMipsLinearOddPSO;
        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMaxMipsLinearOddXPSO;
        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMaxMipsLinearOddYPSO;

        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMinMipsLinearPSO;
        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMinMipsLinearOddPSO;
        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMinMipsLinearOddXPSO;
        std::shared_ptr<RHI::D3D12PipelineState> pGenerateMinMipsLinearOddYPSO;
	};
}

