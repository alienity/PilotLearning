#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    struct DirectionShadowmapStruct
    {
        GObjectID    m_gobject_id {k_invalid_gobject_id};
        GComponentID m_gcomponent_id {k_invalid_gcomponent_id};

        Vector2 m_shadowmap_size;

        std::shared_ptr<RHI::D3D12Texture>            p_LightShadowmap;
        std::shared_ptr<RHI::D3D12DepthStencilView>   p_LightShadowmapDSV;
        std::shared_ptr<RHI::D3D12ShaderResourceView> p_LightShadowmapSRV;

        void Reset() 
        {
            m_gobject_id = k_invalid_gobject_id;
            m_gcomponent_id = k_invalid_gcomponent_id;
            p_LightShadowmap = nullptr;
            p_LightShadowmapDSV = nullptr;
            p_LightShadowmapSRV = nullptr;
        }
    };

    struct SpotShadowmapStruct
    {
        GObjectID    m_gobject_id {k_invalid_gobject_id};
        GComponentID m_gcomponent_id {k_invalid_gcomponent_id};

        uint32_t m_spot_index;
        Vector2  m_shadowmap_size;

        std::shared_ptr<RHI::D3D12Texture>            p_LightShadowmap;
        std::shared_ptr<RHI::D3D12DepthStencilView>   p_LightShadowmapDSV;
        std::shared_ptr<RHI::D3D12ShaderResourceView> p_LightShadowmapSRV;

        void Reset()
        {
            m_gobject_id        = k_invalid_gobject_id;
            m_gcomponent_id     = k_invalid_gcomponent_id;
            p_LightShadowmap    = nullptr;
            p_LightShadowmapDSV = nullptr;
            p_LightShadowmapSRV = nullptr;
        }
    };

    class IndirectShadowPass : public RenderPass
	{
    public:
        struct ShadowmapRGHandle
        {
            RHI::RgResourceHandle shadowmapTextureHandle;
            RHI::RgResourceHandle shadowmapDSVHandle;
            RHI::RgResourceHandle shadowmapSRVHandle;
        };
        
        struct ShadowPassInitInfo : public RenderPassInitInfo
        {};

        struct ShadowInputParameters : public PassInput
        {
            std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;

            std::shared_ptr<RHI::D3D12Buffer> p_DirectionalCommandBuffer;
            std::vector<std::shared_ptr<RHI::D3D12Buffer>> p_SpotCommandBuffer;
        };

        struct ShadowOutputParameters : public PassOutput
        {
            ShadowmapRGHandle directionalShadowmapRGHandle;
            std::vector<ShadowmapRGHandle> spotShadowmapRGHandle;
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
        DirectionShadowmapStruct         directionalShadowmap;
        std::vector<SpotShadowmapStruct> spotShadowmaps;
	};
}

