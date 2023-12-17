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
            RHI::RgTextureDesc albedoDesc;
            RHI::RgTextureDesc depthDesc;
            RHI::RgTextureDesc worldNormalDesc;                             // float3
            RHI::RgTextureDesc worldTangentDesc;                            // float4
            RHI::RgTextureDesc matNormalDesc;                               // float3
            RHI::RgTextureDesc metallic_Roughness_Reflectance_AO_Desc;      // float4
            RHI::RgTextureDesc clearCoat_ClearCoatRoughness_AnisotropyDesc; // float3
            RHI::RgTextureDesc motionVectorDesc;                            // float4

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle terrainHeightmapHandle;
            RHI::RgResourceHandle terrainNormalmapHandle;

            RHI::RgResourceHandle terrainCommandSigHandle;
            RHI::RgResourceHandle transformBufferHandle;
        };

    public:
        ~IndirectTerrainGBufferPass() { destroy(); }

        void prepareMatBuffer(std::shared_ptr<RenderResource> render_resource);

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, GBufferOutput& passOutput);
        void destroy() override final;

    private:
        int passIndex;

        Shader indirectTerrainGBufferVS;
        Shader indirectTerrainGBufferPS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirectTerrainGBufferSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirectTerrainGBufferPSO;
        std::shared_ptr<RHI::D3D12CommandSignature> pIndirectTerrainGBufferCommandSignature;

        struct MaterialIndexStruct
        {
            int albedoIndex;
            int armIndex;
            int displacementIndex;
            int normalIndex;
        };

        struct MaterialTillingStruct
        {
            glm::float2 albedoTilling;
            glm::float2 armTilling;
            glm::float2 displacementTilling;
            glm::float2 normalTilling;
        };

        std::shared_ptr<RHI::D3D12Buffer> pMatTextureIndexBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMatTextureTillingBuffer;

        RHI::RgTextureDesc albedoDesc;                                  // float4
        RHI::RgTextureDesc depthDesc;                                   // float
        RHI::RgTextureDesc worldNormalDesc;                             // float3
        RHI::RgTextureDesc worldTangentDesc;                            // float4
        RHI::RgTextureDesc matNormalDesc;                               // float3
        RHI::RgTextureDesc motionVectorDesc;                            // float4
        RHI::RgTextureDesc metallic_Roughness_Reflectance_AO_Desc;      // float4
        RHI::RgTextureDesc clearCoat_ClearCoatRoughness_AnisotropyDesc; // float3
	};
}

