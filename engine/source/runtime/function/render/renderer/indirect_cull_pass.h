#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"

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

    struct SpotShadowmapCommandBuffer
    {
        DrawCallCommandBuffer m_DrawCallCommandBuffer;
        SceneCommonIdentifier m_identifier;
        uint32_t m_lightIndex;

        void Reset()
        {
            m_identifier = SceneCommonIdentifier();
            m_lightIndex = 0;
            m_DrawCallCommandBuffer.ResetBuffer();
        }
    };

    struct DirShadowmapCommandBuffer
    {
        std::vector<DrawCallCommandBuffer> m_DrawCallCommandBuffer;
        SceneCommonIdentifier m_identifier;
        
        void Reset()
        {
            m_identifier = SceneCommonIdentifier();
            for (size_t i = 0; i < m_DrawCallCommandBuffer.size(); i++)
            {
                m_DrawCallCommandBuffer[i].ResetBuffer();
            }
            m_DrawCallCommandBuffer.clear();
        }
    };

    class IndirectCullPass : public RenderPass
	{
    public:
        struct IndirectCullInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc albedoTexDesc;
            RHI::RgTextureDesc depthTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawCallCommandBufferHandle
        {
            RHI::RgResourceHandle indirectIndexBufferHandle = RHI::_DefaultRgResourceHandle;
            RHI::RgResourceHandle indirectSortBufferHandle  = RHI::_DefaultRgResourceHandle;
        };

        struct IndirectCullOutput
        {
            RHI::RgResourceHandle perframeBufferHandle = RHI::_DefaultRgResourceHandle;
            RHI::RgResourceHandle renderDataPerDrawHandle = RHI::_DefaultRgResourceHandle;
            RHI::RgResourceHandle propertiesPerMaterialHandle = RHI::_DefaultRgResourceHandle;

            DrawCallCommandBufferHandle opaqueDrawHandle;
            DrawCallCommandBufferHandle transparentDrawHandle;
            
            std::vector<DrawCallCommandBufferHandle> directionShadowmapHandles;
            std::vector<DrawCallCommandBufferHandle> spotShadowmapHandles;
        };

    public:
        ~IndirectCullPass() { destroy(); }

        void initialize(const IndirectCullInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResource> render_resource);
        void inflatePerframeBuffer(std::shared_ptr<RenderResource> render_resource);
        //void cullMeshs(RHI::D3D12CommandContext* context, RHI::RenderGraphRegistry* registry, IndirectCullOutput& indirectCullOutput);
        void update(RHI::RenderGraph& cullingGraph, IndirectCullOutput& indirectCullOutput);

        void destroy() override final;

    private:
        void prepareBuffer();
        void prepareRenderTexture();

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
        std::shared_ptr<RHI::D3D12Buffer> pUploadFrameUniformBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pUploadRenderDataPerDrawBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pUploadPropertiesPerMaterialBuffer;

        // used for later pass computation
        std::shared_ptr<RHI::D3D12Buffer> pFrameUniformBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pRenderDataPerDrawBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pPropertiesPerMaterialBuffer;

        // desc
        RHI::RgTextureDesc albedoDesc;
        RHI::RgTextureDesc depthDesc;

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
        DirShadowmapCommandBuffer dirShadowmapCommandBuffers;
        std::vector<SpotShadowmapCommandBuffer> spotShadowmapCommandBuffer;
	};
}

