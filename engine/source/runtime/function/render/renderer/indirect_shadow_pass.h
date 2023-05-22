#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    struct DirectionShadowmapStruct
    {
        GObjectID    m_gobject_id {K_Invalid_Object_Id};
        GComponentID m_gcomponent_id {k_invalid_gcomponent_id};

        Vector2 m_shadowmap_size;

        std::shared_ptr<RHI::D3D12Texture> p_LightShadowmap;

        void Reset() 
        {
            m_gobject_id = K_Invalid_Object_Id;
            m_gcomponent_id = k_invalid_gcomponent_id;
            p_LightShadowmap = nullptr;
        }
    };

    struct SpotShadowmapStruct
    {
        GObjectID    m_gobject_id {K_Invalid_Object_Id};
        GComponentID m_gcomponent_id {k_invalid_gcomponent_id};

        uint32_t m_spot_index;
        Vector2  m_shadowmap_size;

        std::shared_ptr<RHI::D3D12Texture> p_LightShadowmap;
        
        void Reset()
        {
            m_gobject_id     = K_Invalid_Object_Id;
            m_gcomponent_id  = k_invalid_gcomponent_id;
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
            //std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
            //std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;
            //std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;
            //std::shared_ptr<RHI::D3D12Buffer> p_DirectionalCommandBuffer;
            //std::vector<std::shared_ptr<RHI::D3D12Buffer>> p_SpotCommandBuffer;

            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle meshBufferHandle;
            RHI::RgResourceHandle materialBufferHandle;
            RHI::RgResourceHandle dirIndirectSortBufferHandle;
            std::vector<RHI::RgResourceHandle> spotsIndirectSortBufferHandles;

        };

        struct ShadowOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle directionalShadowmapHandle;
            std::vector<RHI::RgResourceHandle> spotShadowmapHandle;
        };

    public:
        ~IndirectShadowPass() { destroy(); }

        void prepareShadowmaps(std::shared_ptr<RenderResourceBase> render_resource);

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

