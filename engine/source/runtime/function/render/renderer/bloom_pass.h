#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class BloomPass : public RenderPass
	{
    public:
        struct BloomInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc       m_ColorTexDesc;
            ShaderCompiler*          m_ShaderCompiler;
            std::filesystem::path    m_ShaderRootPath;
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
                outputBloomHandle.Invalidate();
            }

            RHI::RgResourceHandle outputLumaLRHandle;
            RHI::RgResourceHandle outputBloomHandle;
        };

    public:
        ~BloomPass() { destroy(); }

        void initialize(const BloomInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    private:
        void BlurBuffer(RHI::RenderGraph&     graph,
                        RHI::RgResourceHandle buffer[2],
                        RHI::RgResourceHandle lowerResBuf,
                        float                 upsampleBlendFactor);

    private:
        Shader                                   BloomExtractAndDownsampleHdrCS;
        std::shared_ptr<RHI::D3D12RootSignature> pBloomExtractAndDownsampleHdrCSSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pBloomExtractAndDownsampleHdrCSPSO;

        Shader                                   DownsampleBloom4CS;
        std::shared_ptr<RHI::D3D12RootSignature> pDownsampleBloom4CSSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pDownsampleBloom4CSPSO;

        Shader                                   DownsampleBloom2CS;
        std::shared_ptr<RHI::D3D12RootSignature> pDownsampleBloom2CSSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pDownsampleBloom2CSPSO;

        Shader                                   BlurCS;
        std::shared_ptr<RHI::D3D12RootSignature> pBlurCSSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pBlurCSPSO;

        Shader                                   UpsampleAndBlurCS;
        std::shared_ptr<RHI::D3D12RootSignature> pUpsampleAndBlurCSSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pUpsampleAndBlurCSPSO;

        RHI::RgTextureDesc m_aBloomUAV1Desc[2]; // 640x384 (1/3)
        RHI::RgTextureDesc m_aBloomUAV2Desc[2]; // 320x192 (1/6)
        RHI::RgTextureDesc m_aBloomUAV3Desc[2]; // 160x96  (1/12)
        RHI::RgTextureDesc m_aBloomUAV4Desc[2]; // 80x48   (1/24)
        RHI::RgTextureDesc m_aBloomUAV5Desc[2]; // 40x24   (1/48)
        RHI::RgTextureDesc m_LumaLRDesc;

	};
}

