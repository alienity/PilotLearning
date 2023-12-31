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
            RHI::RgTextureDesc albedoTexDesc;
            RHI::RgTextureDesc depthTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle meshBufferHandle;
            RHI::RgResourceHandle materialBufferHandle;
            RHI::RgResourceHandle opaqueDrawHandle;
        };

    public:
        ~IndirectGBufferPass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, GBufferOutput& passOutput);
        void destroy() override final;

        bool initializeRenderTarget(RHI::RenderGraph& graph, GBufferOutput* drawPassOutput);

        RHI::RgTextureDesc albedoDesc;                                  // float4
        RHI::RgTextureDesc worldNormalDesc;                             // float3
        //RHI::RgTextureDesc clearCoatNormalDesc;                       // float3
        RHI::RgTextureDesc motionVectorDesc;                            // float
        RHI::RgTextureDesc metallic_Roughness_Reflectance_AO_Desc;      // float4
        RHI::RgTextureDesc clearCoat_ClearCoatRoughness_AnisotropyDesc; // float3
        RHI::RgTextureDesc depthDesc;                                   // float

    private:
        Shader indirectGBufferVS;
        Shader indirectGBufferPS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirectGBufferSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirectGBufferPSO;
        std::shared_ptr<RHI::D3D12CommandSignature> pIndirectGBufferCommandSignature;
	};
}

