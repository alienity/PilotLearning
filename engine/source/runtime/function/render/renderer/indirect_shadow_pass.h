#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    struct DirectionShadowmapStruct
    {
        GObjectID    m_gobject_id {K_Invalid_Object_Id};
        GComponentID m_gcomponent_id {K_Invalid_Component_Id};

        Vector2 m_shadowmap_size;

        std::shared_ptr<RHI::D3D12Texture> p_LightShadowmap;

        void Reset() 
        {
            m_gobject_id = K_Invalid_Object_Id;
            m_gcomponent_id = K_Invalid_Component_Id;
            p_LightShadowmap = nullptr;
        }
    };

    struct SpotShadowmapStruct
    {
        GObjectID    m_gobject_id {K_Invalid_Object_Id};
        GComponentID m_gcomponent_id {K_Invalid_Component_Id};

        uint32_t m_spot_index;
        Vector2  m_shadowmap_size;

        std::shared_ptr<RHI::D3D12Texture> p_LightShadowmap;
        
        void Reset()
        {
            m_gobject_id     = K_Invalid_Object_Id;
            m_gcomponent_id  = K_Invalid_Component_Id;
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
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle meshBufferHandle;
            RHI::RgResourceHandle materialBufferHandle;
            RHI::RgResourceHandle dirIndirectSortBufferHandle;
            std::vector<RHI::RgResourceHandle> spotsIndirectSortBufferHandles;

            inline void Init()
            {
                perframeBufferHandle.Invalidate();
                meshBufferHandle.Invalidate();
                materialBufferHandle.Invalidate();
                dirIndirectSortBufferHandle.Invalidate();
                spotsIndirectSortBufferHandles = {};
            }
        };

        struct ShadowOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle directionalShadowmapHandle;
            std::vector<RHI::RgResourceHandle> spotShadowmapHandle;

            inline void Init()
            {
                directionalShadowmapHandle.Invalidate();
                spotShadowmapHandle = {};
            }
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
        DirectionShadowmapStruct         directionalShadowmap;
        std::vector<SpotShadowmapStruct> spotShadowmaps;
	};
}

