#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class IndirectCullPass : public RenderPass
	{
    public:
        struct IndirectCullParams
        {
            uint32_t numMeshes;
            uint32_t commandBufferCounterOffset;

            std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;
            std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;

            std::shared_ptr<RHI::D3D12Buffer>              p_IndirectCommandBuffer;
            std::shared_ptr<RHI::D3D12UnorderedAccessView> p_IndirectCommandBufferUav;

            std::shared_ptr<RHI::D3D12Buffer>              p_IndirectShadowmapCommandBuffer;
            std::shared_ptr<RHI::D3D12UnorderedAccessView> p_IndirectShadowmapCommandBufferUav;
        };

    public:
        ~IndirectCullPass() { destroy(); }

        void initialize(const RenderPassInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResourceBase> render_resource, uint32_t& numMeshes);
        void cullMeshs(RHI::D3D12CommandContext& context,
                       RHI::RenderGraphRegistry& registry,
                       IndirectCullParams&       indirectCullParams);

        void destroy() override final;

    private:

        // 因为每帧都需要上传，所以这里就直接申请一份内存
        std::shared_ptr<RHI::D3D12Buffer> pUploadPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pUploadMaterialBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pUploadMeshBuffer;

        HLSL::MeshPerframeStorageBufferObject* pPerframeObj = nullptr;
        HLSL::MaterialInstance*                pMaterialObj = nullptr;
        HLSL::MeshInstance*                    pMeshesObj   = nullptr;
	};
}

