#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    enum HistoryRejectionFlags
    {
        Depth = 0x1,
        Reprojection = 0x2,
        PreviousDepth = 0x4,
        Position = 0x8,
        Normal = 0x10,
        Motion = 0x20,
        Combined = Depth | Reprojection | PreviousDepth | Position | Normal | Motion,
        CombinedNoMotion = Depth | Reprojection | PreviousDepth | Position | Normal
    };

    struct TemporalFilterParameters
    {
        bool singleChannel;
        float historyValidity;
        bool occluderMotionRejection;
        bool receiverMotionRejection;
        bool exposureControl;
        float resolutionMultiplier;
        float historyResolutionMultiplier;
    };

    struct ShaderSignaturePSO
    {
        Shader m_Kernal;
        std::shared_ptr<RHI::D3D12RootSignature> pKernalSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pKernalPSO;
    };

    class TemporalFilterPass : public RenderPass
	{
    public:
        struct TemporalFilterInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc    m_ColorTexDesc;
            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle depthStencilBuffer;
            RHI::RgResourceHandle normalBuffer;
            RHI::RgResourceHandle motionVectorBuffer;
            RHI::RgResourceHandle velocityBuffer;
            RHI::RgResourceHandle noisyBuffer;
            RHI::RgResourceHandle validationBuffer;
            RHI::RgResourceHandle historyBuffer;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle outputBuffer;
        };

    public:
        ~TemporalFilterPass() { destroy(); }

        void initialize(const TemporalFilterInitInfo& init_info);
        void prepareMetaData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

        void Denoise(RHI::RenderGraph& graph);

        ShaderSignaturePSO mValidateHistory;

        ShaderSignaturePSO mTemporalAccumulationSingle;
        ShaderSignaturePSO mTemporalAccumulationColor;
        ShaderSignaturePSO mCopyHistory;

    private:

        RHI::RgTextureDesc colorTexDesc;
	};
}

