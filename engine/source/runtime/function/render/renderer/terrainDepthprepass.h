#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/pass_helper.h"

namespace MoYu
{
    class TerrainDepthPrePass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc depthTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle terrainHeightmapHandle;
            RHI::RgResourceHandle terrainNormalmapHandle;
            RHI::RgResourceHandle terrainRenderDataHandle;
            RHI::RgResourceHandle terrainMatPropertyHandle;
            RHI::RgResourceHandle culledPatchListBufferHandle;
            RHI::RgResourceHandle mainCamVisCmdSigHandle;
        };

        struct DrawOutput : public PassOutput
        {
            RHI::RgResourceHandle depthBufferHandle;
        };

    public:
        ~TerrainDepthPrePass() { destroy(); }

        void prepareMatBuffer(std::shared_ptr<RenderResource> render_resource);

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutput& passOutput);
        void destroy() override final;

        RHI::RgTextureDesc depthDesc;   // float

    private:
        Shader drawDepthVS;
        std::shared_ptr<RHI::D3D12RootSignature> pDrawDepthSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pDrawDepthPSO;
        std::shared_ptr<RHI::D3D12CommandSignature> pDepthCommandSignature;
	};
}

