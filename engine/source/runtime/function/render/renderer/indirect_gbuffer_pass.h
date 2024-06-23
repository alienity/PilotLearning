#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/renderer/pass_helper.h"

namespace MoYu
{
    // https://therealmjp.github.io/posts/bindless-texturing-for-deferred-rendering-and-decals/
    class IndirectGBufferPass : public RenderPass
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
            RHI::RgResourceHandle renderDataPerDrawHandle;
            RHI::RgResourceHandle propertiesPerMaterialHandle;
            RHI::RgResourceHandle opaqueDrawHandle;
        };

    public:
        ~IndirectGBufferPass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, GBufferOutput& passOutput);
        void destroy() override final;

        bool initializeRenderTarget(RHI::RenderGraph& graph, GBufferOutput* drawPassOutput);

        RHI::RgTextureDesc gbufferDesc; // float4
        RHI::RgTextureDesc gbuffer0Desc; // float4
        RHI::RgTextureDesc gbuffer1Desc; // float4
        RHI::RgTextureDesc gbuffer2Desc; // float4
        RHI::RgTextureDesc gbuffer3Desc; // float4
        RHI::RgTextureDesc depthDesc;   // float

    private:
        Shader drawGBufferVS;
        Shader drawGBufferPS;
        std::shared_ptr<RHI::D3D12RootSignature> pDrawGBufferSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pDrawGBufferPSO;
        std::shared_ptr<RHI::D3D12CommandSignature> pGBufferCommandSignature;
	};
}

