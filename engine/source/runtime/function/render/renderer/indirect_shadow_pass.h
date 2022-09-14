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
        {
            RHI::RgTextureDesc depthBufferTexDesc;
        };

        struct ShadowInputParameters : public PassInput
        {
            uint32_t commandBufferCounterOffset;

            std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;

            std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer;
        };

        struct ShadowOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle backBufDepth;
            RHI::RgResourceHandle backBufDsv;
        };

    public:
        ~IndirectShadowPass() { destroy(); }

        void initialize(const ShadowPassInitInfo& init_info);
        void update(RHI::D3D12CommandContext& context,
                    RHI::RenderGraph&         graph,
                    ShadowInputParameters&      passInput,
                    ShadowOutputParameters&     passOutput);
        void destroy() override final;

    private:
        std::shared_ptr<RHI::D3D12DynamicDescriptor<D3D12_SHADER_RESOURCE_VIEW_DESC>> pD3D12SRVDescriptor;

        RHI::RgTextureDesc depthBufferTexDesc;
	};
}

