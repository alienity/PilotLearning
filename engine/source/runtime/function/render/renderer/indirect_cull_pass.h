#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Pilot
{
    class IndirectCullPass : public RenderPass
	{
    public:
        struct IndirectCullResultBuffer
        {
            std::shared_ptr<RHI::D3D12Buffer> p_IndirectCommandBuffer;
        };

    public:
        void initialize(const RenderPassInitInfo& init_info) override final;
        void prepareMeshData(std::shared_ptr<RenderResourceBase>& render_resource);
        void cullMeshs(IndirectCullResultBuffer& indirectCullResult);

        void destroy() override final;

    private:

        std::uint64_t totalCommandBufferSizeInBytes =
            HLSL::MeshLimit * sizeof(HLSL::CommandSignatureParams);
        std::uint64_t commandBufferCounterOffset =
            D3D12RHIUtils::AlignUp(totalCommandBufferSizeInBytes, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

        std::shared_ptr<RHI::D3D12Buffer>              p_IndirectCommandBuffer;
        std::shared_ptr<RHI::D3D12UnorderedAccessView> p_IndirectCommandBufferUav;

        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;

        uint32_t numPointLights = 0;
        uint32_t numMaterials   = 0;
        uint32_t numMeshes      = 0;

        HLSL::MeshPerframeStorageBufferObject* pPerframeObj = nullptr;
        HLSL::MaterialInstance*                pMaterialObj = nullptr;
        HLSL::MeshInstance*                    pMeshesObj   = nullptr;
	};
}
