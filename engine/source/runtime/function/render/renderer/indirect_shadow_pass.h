#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"

namespace MoYu
{
    struct DirectionShadowmapStruct
    {
        SceneCommonIdentifier m_identifier {UndefCommonIdentifier};
        glm::float2 m_shadowmap_bounds;
        glm::float2 m_shadowmap_size;
        uint32_t m_casccade;

        std::shared_ptr<RHI::D3D12Texture> p_LightShadowmap;

        void Reset() 
        {
            m_identifier       = UndefCommonIdentifier;
            p_LightShadowmap   = nullptr;
            m_shadowmap_bounds = MYFloat2::One;
            m_shadowmap_size   = MYFloat2::One;
            m_casccade         = 4;
        }
    };

    struct SpotShadowmapStruct
    {
        SceneCommonIdentifier m_identifier {UndefCommonIdentifier};
        uint32_t m_spot_index;
        glm::float2  m_shadowmap_size;
        std::shared_ptr<RHI::D3D12Texture> p_LightShadowmap;
        
        void Reset()
        {
            m_identifier     = UndefCommonIdentifier;
            p_LightShadowmap = nullptr;
        }
    };

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

    private:
        DirectionShadowmapStruct m_DirectionalShadowmap;
        std::vector<SpotShadowmapStruct> m_SpotShadowmaps;
	};
}

