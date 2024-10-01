#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/pass_helper.h"

namespace MoYu
{
    // https://therealmjp.github.io/posts/bindless-texturing-for-deferred-rendering-and-decals/
    class IndirectTerrainGBufferPass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc colorTexDesc;
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

    public:
        ~IndirectTerrainGBufferPass() { destroy(); }

        void prepareMatBuffer(std::shared_ptr<RenderResource> render_resource);

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, GBufferOutput& passOutput);
        void destroy() override final;

    private:
        Shader terrainGBufferVS;
        Shader terrainGBufferPS;
        std::shared_ptr<RHI::D3D12RootSignature> pTerrainGBufferSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pTerrainGBufferPSO;
        std::shared_ptr<RHI::D3D12CommandSignature> pTerrainGBufferCommandSignature;

        RHI::RgTextureDesc gbufferDesc; // float4
        RHI::RgTextureDesc gbuffer0Desc; // float4
        RHI::RgTextureDesc gbuffer1Desc; // float4
        RHI::RgTextureDesc gbuffer2Desc; // float4
        RHI::RgTextureDesc gbuffer3Desc; // float4
        RHI::RgTextureDesc depthDesc;   // float
	};
}

