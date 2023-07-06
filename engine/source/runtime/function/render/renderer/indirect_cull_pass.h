#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    struct DrawCallCommandBuffer
    {
        std::shared_ptr<RHI::D3D12Buffer> p_IndirectIndexCommandBuffer;
        std::shared_ptr<RHI::D3D12Buffer> p_IndirectSortCommandBuffer;

        void ResetBuffer()
        {
            p_IndirectIndexCommandBuffer = nullptr;
            p_IndirectSortCommandBuffer  = nullptr;
        }
    };

    struct ShadowmapCommandBuffer : public DrawCallCommandBuffer
    {
        SceneCommonIdentifier m_identifier;
        uint32_t m_lightIndex;

        void Reset()
        {
            m_identifier = SceneCommonIdentifier();
            ResetBuffer();
        }
    };

    class IndirectCullPass : public RenderPass
	{
    public:
        struct DrawCallCommandBufferHandle
        {
            RHI::RgResourceHandle indirectIndexBufferHandle;
            RHI::RgResourceHandle indirectSortBufferHandle;

            inline void Init()
            {
                indirectIndexBufferHandle.Invalidate();
                indirectSortBufferHandle.Invalidate();
            }
        };

        struct IndirectCullOutput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle materialBufferHandle;
            RHI::RgResourceHandle meshBufferHandle;

            DrawCallCommandBufferHandle opaqueDrawHandle;
            DrawCallCommandBufferHandle transparentDrawHandle;
            DrawCallCommandBufferHandle dirShadowmapHandle;
            std::vector<DrawCallCommandBufferHandle> spotShadowmapHandles;

            inline void Init()
            {
                perframeBufferHandle.Invalidate();
                materialBufferHandle.Invalidate();
                meshBufferHandle.Invalidate();
                opaqueDrawHandle.Init();
                transparentDrawHandle.Init();
                dirShadowmapHandle.Init();
                spotShadowmapHandles = {};
            }
        };

    public:
        ~IndirectCullPass() { destroy(); }

        void initialize(const RenderPassInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResource> render_resource);
        //void cullMeshs(RHI::D3D12CommandContext* context, RHI::RenderGraphRegistry* registry, IndirectCullOutput& indirectCullOutput);
        void update(RHI::RenderGraph& cullingGraph, IndirectCullOutput& indirectCullOutput);

        void destroy() override final;

    private:
        void prepareBuffer();

        void bitonicSort(RHI::D3D12ComputeContext*      context,
                         RHI::D3D12Buffer*              keyIndexList,
                         RHI::D3D12Buffer*              countBuffer,
                         RHI::D3D12Buffer*              sortDispatchArgBuffer,
                         bool                           isPartiallyPreSorted,
                         bool                           sortAscending);

        void grabObject(RHI::D3D12ComputeContext* context,
                        RHI::D3D12Buffer*         meshBuffer,
                        RHI::D3D12Buffer*         indirectIndexBuffer,
                        RHI::D3D12Buffer*         indirectSortBuffer,
                        RHI::D3D12Buffer*         grabDispatchArgBuffer);

    private:

        // for upload
        std::shared_ptr<RHI::D3D12Buffer> pUploadPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pUploadMaterialBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pUploadMeshBuffer;

        // used for later pass computation
        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;

        /*
        // for sort
        std::shared_ptr<RHI::D3D12Buffer> pSortDispatchArgs;
        // for grab
        std::shared_ptr<RHI::D3D12Buffer> pGrabDispatchArgs;
        */
        RHI::RgBufferDesc sortDispatchArgsBufferDesc;
        RHI::RgBufferDesc grabDispatchArgsBufferDesc;

        // used for later draw call
        DrawCallCommandBuffer commandBufferForOpaqueDraw;
        DrawCallCommandBuffer commandBufferForTransparentDraw;

        // used for shadowmap drawing
        ShadowmapCommandBuffer dirShadowmapCommandBuffer;
        std::vector<ShadowmapCommandBuffer> spotShadowmapCommandBuffer;
	};
}

