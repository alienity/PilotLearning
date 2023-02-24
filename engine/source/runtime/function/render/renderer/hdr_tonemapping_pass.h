#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class HDRToneMappingPass : public RenderPass
	{
    public:
        struct HDRToneMappingInitInfo : public RenderPassInitInfo
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
                //inputExposureHandle.Invalidate();
                //inputBloomHandle.Invalidate();
                inputSceneColorHandle.Invalidate();
            }

            //RHI::RgResourceHandle inputExposureHandle;
            //RHI::RgResourceHandle inputBloomHandle;
            RHI::RgResourceHandle inputSceneColorHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                outputLumaHandle.Invalidate();
                outputPostEffectsHandle.Invalidate();
            }

            RHI::RgResourceHandle outputLumaHandle;
            RHI::RgResourceHandle outputPostEffectsHandle;
        };

    public:
        ~HDRToneMappingPass() { destroy(); }

        void initialize(const HDRToneMappingInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        Shader ToneMapCS;
        std::shared_ptr<RHI::D3D12RootSignature> pToneMapCSSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pToneMapCSPSO;
        std::shared_ptr<RHI::D3D12Buffer> p_ExposureBuffer;

        RHI::RgTextureDesc m_LumaBufferDesc;

        struct _UnbindIndexBuffer
        {
            //uint32_t m_ExposureIndex;
            //uint32_t m_BloomIndex;
            uint32_t m_DstColorIndex;
            uint32_t m_SrcColorIndex;
            uint32_t m_OutLumaIndex;
        };

        //std::shared_ptr<RHI::D3D12Buffer> p_UnbindIndexBuffer;
	};
}

