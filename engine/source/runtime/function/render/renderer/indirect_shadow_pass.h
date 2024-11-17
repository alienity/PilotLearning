#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"
#include "runtime/function/render/renderer/pass_helper.h"

namespace MoYu
{
    class IndirectShadowPass : public RenderPass
	{
    public:
        struct ShadowPassInitInfo : public RenderPassInitInfo
        {};

        struct ShadowInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle = RHI::_DefaultRgResourceHandle;
            RHI::RgResourceHandle renderDataPerDrawHandle = RHI::_DefaultRgResourceHandle;
            RHI::RgResourceHandle propertiesPerMaterialHandle = RHI::_DefaultRgResourceHandle;

            std::vector<RHI::RgResourceHandle> dirIndirectSortBufferHandles;
            std::vector<RHI::RgResourceHandle> spotsIndirectSortBufferHandles;
        };

        struct ShadowOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle directionalCascadeShadowmapHandle;
            std::vector<RHI::RgResourceHandle> spotShadowmapHandle;
        };

    public:
        ~IndirectShadowPass() { destroy(); }

        void prepareShadowmaps(std::shared_ptr<RenderResource> render_resource);

        void initialize(const ShadowPassInitInfo& init_info);
        void update(RHI::RenderGraph& graph, const ShadowInputParameters& passInput, ShadowOutputParameters& passOutput);
        void destroy() override final;
    
        DirectionShadowmapStruct getDirShadowmapStruct();
    public:
        DirectionShadowmapStruct m_DirectionalShadowmap;
        std::vector<SpotShadowmapStruct> m_SpotShadowmaps;
	};
}

