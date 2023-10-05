#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

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

        struct DrawOutputParameters : public PassOutput
        {
            DrawOutputParameters()
            {
                albedoHandle.Invalidate();
                normalHandle.Invalidate();
                clearCoatNormalHandle.Invalidate();
                emissiveHandle.Invalidate();
                metallic_Roughness_Reflectance_AO_Handle.Invalidate();
                clearCoat_ClearCoatRoughness_Anisotropy_Handle.Invalidate();
                depthHandle.Invalidate();
            }

            RHI::RgResourceHandle albedoHandle;
            RHI::RgResourceHandle normalHandle;
            RHI::RgResourceHandle clearCoatNormalHandle;
            RHI::RgResourceHandle emissiveHandle;
            RHI::RgResourceHandle metallic_Roughness_Reflectance_AO_Handle;
            RHI::RgResourceHandle clearCoat_ClearCoatRoughness_Anisotropy_Handle;
            RHI::RgResourceHandle depthHandle;
        };

    public:
        ~IndirectGBufferPass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph&         graph,
                    DrawInputParameters&      passInput,
                    DrawOutputParameters&     passOutput);
        void destroy() override final;

    protected:
        bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);

    private:
        Shader indirectGBufferVS;
        Shader indirectGBufferPS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirectGBufferSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirectGBufferPSO;

        RHI::RgTextureDesc albedoTexDesc;                               // float4
        RHI::RgTextureDesc normalTexDesc;                               // float3
        RHI::RgTextureDesc clearCoatNormalTexDesc;                      // float3
        RHI::RgTextureDesc emissiveDesc;                                // float4
        RHI::RgTextureDesc metallic_Roughness_Reflectance_AO_Desc;      // float4
        RHI::RgTextureDesc clearCoat_ClearCoatRoughness_AnisotropyDesc; // float3
        RHI::RgTextureDesc depthTexDesc;                                // float
	};
}

