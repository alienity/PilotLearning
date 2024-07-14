#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/xeGTAO.h"

namespace MoYu
{
    // https://github.com/GameTechDev/XeGTAO
    class GTAOPass : public RenderPass
	{
    public:
        struct AOInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;
            RHI::RgTextureDesc depthTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            DrawInputParameters()
            {
                perframeBufferHandle.Invalidate();
                packedNormalHandle.Invalidate();
                depthHandle.Invalidate();
            }

            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle packedNormalHandle;
            RHI::RgResourceHandle depthHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                workingEdgeHandle.Invalidate();
                workingAOHandle.Invalidate();
                workingViewDepthHandle.Invalidate();
                outputAOHandle.Invalidate();
            }

            RHI::RgResourceHandle workingEdgeHandle;
            RHI::RgResourceHandle workingAOHandle;
            RHI::RgResourceHandle workingViewDepthHandle;
            RHI::RgResourceHandle outputAOHandle;
        };

    public:
        ~GTAOPass() { destroy(); }

        void initialize(const AOInitInfo& init_info);
        void updateConstantBuffer(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

    protected:
        bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);

        RHI::RgTextureDesc colorTexDesc;
        RHI::RgTextureDesc depthTexDesc;

        RHI::RgTextureDesc workingEdgeDesc;
        RHI::RgTextureDesc workingAODesc;

        XeGTAO::GTAOConstants gtaoConstants;
        XeGTAO::GTAOSettings  gtaoSettings;

        int frameCounter = 0;

        std::shared_ptr<RHI::D3D12Buffer> pGTAOConstants;

    private:
        Shader DepthPrefilterCS;
        std::shared_ptr<RHI::D3D12RootSignature> pDepthPrefilterSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pDepthPrefilterPSO;

        Shader GTAOCS;
        std::shared_ptr<RHI::D3D12RootSignature> pGTAOSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pGTAOPSO;

        Shader DenoiseCS;
        std::shared_ptr<RHI::D3D12RootSignature> pDenoiseSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pDenoisePSO;
	};
}

