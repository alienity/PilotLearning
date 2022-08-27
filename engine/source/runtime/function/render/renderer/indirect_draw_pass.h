#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class IndirectDrawPass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc depthBufferTexDesc;
        };

        struct DrawInputParameters : public PassInput
        {
            uint32_t numMeshes;
            uint32_t commandBufferCounterOffset;

            std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;

            std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle       backBufColor;
            RHI::D3D12RenderTargetView* backBufRtv;

            RHI::RgResourceHandle backBufDepth;
            RHI::RgResourceHandle backBufDsv;
        };

    public:
        ~IndirectDrawPass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void update(RHI::D3D12CommandContext& context,
                    RHI::RenderGraph&         graph,
                    DrawInputParameters&      passInput,
                    DrawOutputParameters&     passOutput);
        void destroy() override final;

    private:
        std::shared_ptr<RHI::D3D12DynamicDescriptor<D3D12_SHADER_RESOURCE_VIEW_DESC>> pD3D12SRVDescriptor;

        RHI::RgTextureDesc depthBufferTexDesc;
	};
}

