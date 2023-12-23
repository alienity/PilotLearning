#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"
#include "runtime/function/render/renderer/pass_helper.h"

namespace MoYu
{
    class IndirectTerrainShadowPass : public RenderPass
	{
    public:
        struct ShadowPassInitInfo : public RenderPassInitInfo
        {
            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct ShadowInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle terrainHeightmapHandle;

            RHI::RgResourceHandle transformBufferHandle;
            std::vector<RHI::RgResourceHandle> dirCommandSigHandle;
        };

        struct ShadowOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle directionalShadowmapHandle = RHI::_DefaultRgResourceHandle;
        };

    public:
        ~IndirectTerrainShadowPass() { destroy(); }

        void prepareShadowmaps(std::shared_ptr<RenderResource> render_resource);

        void initialize(const ShadowPassInitInfo& init_info);
        void update(RHI::RenderGraph&         graph,
                    ShadowInputParameters&      passInput,
                    ShadowOutputParameters&     passOutput);
        void destroy() override final;

    private:
        glm::float2 m_shadowmap_size;
        int m_casccade;

        Shader indirectTerrainShadowmapVS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirectTerrainShadowmapSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirectTerrainShadowmapPSO;
        std::shared_ptr<RHI::D3D12CommandSignature> pIndirectTerrainShadowmapCommandSignature;
	};
}

