#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    struct DrawCallCommandBuffer
    {
        std::shared_ptr<RHI::D3D12Buffer>              p_IndirectCommandBuffer;
        std::shared_ptr<RHI::D3D12UnorderedAccessView> p_IndirectCommandBufferUav;
    };

    struct ShadowmapCommandBuffer : public DrawCallCommandBuffer
    {
        GObjectID    m_gobject_id {k_invalid_gobject_id};
        GComponentID m_gcomponent_id {k_invalid_gcomponent_id};
        uint32_t     m_lightIndex;

        void Reset()
        {
            m_gobject_id               = k_invalid_gcomponent_id;
            m_gcomponent_id            = k_invalid_gcomponent_id;
            p_IndirectCommandBufferUav = nullptr;
            p_IndirectCommandBuffer    = nullptr;
        }
    };

    class IndirectCullPass : public RenderPass
	{
    public:
        struct IndirectCullOutput
        {
            std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;

            std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer;
            std::shared_ptr<RHI::D3D12Buffer> p_DirShadowmapCommandBuffer;
            std::vector<std::shared_ptr<RHI::D3D12Buffer>> p_SpotShadowmapCommandBuffers;
        };

    public:
        ~IndirectCullPass() { destroy(); }

        void initialize(const RenderPassInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResourceBase> render_resource);
        void cullMeshs(RHI::D3D12CommandContext& context, RHI::RenderGraphRegistry& registry, IndirectCullOutput& indirectCullOutput);

        void destroy() override final;

    private:
        void prepareBuffer();

    private:

        // for upload
        std::shared_ptr<RHI::D3D12Buffer> pUploadPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pUploadMaterialBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pUploadMeshBuffer;

        // used for later pass computation
        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;

        // used for later draw call
        DrawCallCommandBuffer commandBufferForDraw;

        // used for shadowmap drawing
        ShadowmapCommandBuffer dirShadowmapCommandBuffer;
        std::vector<ShadowmapCommandBuffer> spotShadowmapCommandBuffer;

        HLSL::MeshPerframeStorageBufferObject* pPerframeObj = nullptr;
        HLSL::MaterialInstance*                pMaterialObj = nullptr;
        HLSL::MeshInstance*                    pMeshesObj   = nullptr;
	};
}

