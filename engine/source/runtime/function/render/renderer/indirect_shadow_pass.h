#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class IndirectShadowPass : public RenderPass
	{
    public:
        struct ShadowPassInitInfo : public RenderPassInitInfo
        {};

        struct ShadowInputParameters : public PassInput
        {
            uint32_t commandBufferCounterOffset;

            std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;

            std::shared_ptr<RHI::D3D12Buffer> p_IndirectShadowmapCommandBuffer;
        };

        struct ShadowOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle shadowmapTextureHandle;
            RHI::RgResourceHandle shadowmapDSVHandle; 
            RHI::RgResourceHandle shadowmapSRVHandle;
        };

    public:
        ~IndirectShadowPass() { destroy(); }

        void prepareShadowmaps(std::shared_ptr<RenderResourceBase> render_resource);

        void initialize(const ShadowPassInitInfo& init_info);
        void update(RHI::D3D12CommandContext& context,
                    RHI::RenderGraph&         graph,
                    ShadowInputParameters&      passInput,
                    ShadowOutputParameters&     passOutput);
        void destroy() override final;

    private:
        std::shared_ptr<RHI::D3D12Texture>            p_DirectionLightShadowmap;
        std::shared_ptr<RHI::D3D12DepthStencilView>   p_DirectionLightShadowmapDSV;
        std::shared_ptr<RHI::D3D12ShaderResourceView> p_DirectionLightShadowmapSRV;

	};
}

