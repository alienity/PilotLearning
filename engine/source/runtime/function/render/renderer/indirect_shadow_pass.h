#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"
#include "runtime/function/render/renderer/shadow_helper.h"

namespace MoYu
{
    class IndirectShadowPass : public RenderPass
	{
    public:
        struct ShadowPassInitInfo : public RenderPassInitInfo
        {};

        struct ShadowInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle        = RHI::_DefaultRgResourceHandle;
            RHI::RgResourceHandle meshBufferHandle            = RHI::_DefaultRgResourceHandle;
            RHI::RgResourceHandle materialBufferHandle        = RHI::_DefaultRgResourceHandle;

            std::vector<RHI::RgResourceHandle> dirIndirectSortBufferHandles;

            std::vector<RHI::RgResourceHandle> spotsIndirectSortBufferHandles;
        };

        struct ShadowOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle directionalShadowmapHandle = RHI::_DefaultRgResourceHandle;

            std::vector<RHI::RgResourceHandle> spotShadowmapHandle;
        };

    public:
        ~IndirectShadowPass() { destroy(); }

        void prepareShadowmaps(std::shared_ptr<RenderResource> render_resource);

        void initialize(const ShadowPassInitInfo& init_info);
        void update(RHI::RenderGraph&         graph,
                    ShadowInputParameters&      passInput,
                    ShadowOutputParameters&     passOutput);
        void destroy() override final;

    //private:
        DirectionShadowmapStruct m_DirectionalShadowmap;
        std::vector<SpotShadowmapStruct> m_SpotShadowmaps;
	};
}

