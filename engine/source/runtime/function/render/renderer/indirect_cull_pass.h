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
        ~IndirectCullPass() { destroy(); }

        void initialize(const RenderPassInitInfo& init_info) override final;
        void prepareMeshData(std::shared_ptr<RenderResourceBase> render_resource);
        void cullMeshs(RHI::D3D12CommandContext& context,
                       RHI::RenderGraphRegistry& registry,
                       IndirectCullResultBuffer& indirectCullResult);

        void destroy() override final;

    private:

        std::uint64_t totalCommandBufferSizeInBytes =
            HLSL::MeshLimit * sizeof(HLSL::CommandSignatureParams);
        std::uint64_t commandBufferCounterOffset =
            D3D12RHIUtils::AlignUp(totalCommandBufferSizeInBytes, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

        std::shared_ptr<RHI::D3D12Buffer>              p_IndirectCommandBuffer;
        std::shared_ptr<RHI::D3D12UnorderedAccessView> p_IndirectCommandBufferUav;

        // 因为每帧都需要上传，所以这里就直接申请一份内存
        std::shared_ptr<RHI::D3D12Buffer> pUploadPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pUploadMaterialBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pUploadMeshBuffer;

        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;

        std::shared_ptr<RHI::D3D12ShaderResourceView> pPerframeBufferView;
        std::shared_ptr<RHI::D3D12ShaderResourceView> pMaterialBufferView;
        std::shared_ptr<RHI::D3D12ShaderResourceView> pMeshBufferView;

        uint32_t numPointLights = 0;
        uint32_t numMaterials   = 0;
        uint32_t numMeshes      = 0;

        HLSL::MeshPerframeStorageBufferObject* pPerframeObj = nullptr;
        HLSL::MaterialInstance*                pMaterialObj = nullptr;
        HLSL::MeshInstance*                    pMeshesObj   = nullptr;
	};
}

