#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"

namespace MoYu
{

    struct TerrainSpotShadowmapCommandBuffer
    {
        std::shared_ptr<RHI::D3D12Buffer> m_DrawCallCommandBuffer;
        SceneCommonIdentifier m_identifier;
        uint32_t m_lightIndex;

        void Reset()
        {
            m_identifier = SceneCommonIdentifier();
            m_lightIndex = 0;
            m_DrawCallCommandBuffer = nullptr;
        }
    };

    struct TerrainDirShadowmapCommandBuffer
    {
        std::vector<std::shared_ptr<RHI::D3D12Buffer>> m_DrawCallCommandBuffers;
        SceneCommonIdentifier m_identifier;
        
        void Reset()
        {
            m_identifier = SceneCommonIdentifier();
            for (size_t i = 0; i < m_DrawCallCommandBuffers.size(); i++)
            {
                m_DrawCallCommandBuffers[i] = nullptr;
            }
            m_DrawCallCommandBuffers.clear();
        }
    };

    struct TerrainCullInitInfo : public RenderPassInitInfo
    {
        RHI::RgTextureDesc colorTexDesc;

        ShaderCompiler*       m_ShaderCompiler;
        std::filesystem::path m_ShaderRootPath;
    };

    class IndirectTerrainCullPass : public RenderPass
	{
    public:
        struct DrawCallCommandBufferHandle
        {
            RHI::RgResourceHandle indirectIndexBufferHandle = RHI::_DefaultRgResourceHandle;
        };

        struct TerrainCullInput : public PassInput
        {
            TerrainCullInput() { perframeBufferHandle.Invalidate(); }

            RHI::RgResourceHandle perframeBufferHandle;
        };

        struct TerrainCullOutput : public PassOutput
        {
            TerrainCullOutput() { outputTerrainIndexHandle.Invalidate(); }

            RHI::RgResourceHandle outputTerrainIndexHandle;

            DrawCallCommandBufferHandle terrainDrawHandle;

            std::vector<DrawCallCommandBufferHandle> directionShadowmapHandles;
            std::vector<DrawCallCommandBufferHandle> spotShadowmapHandles;
        };

    public:
        ~IndirectTerrainCullPass() { destroy(); }

        void initialize(const TerrainCullInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, TerrainCullInput& passInput, TerrainCullOutput& passOutput);

        void destroy() override final;

    private:
        bool initializeRenderTarget(RHI::RenderGraph& graph, TerrainCullOutput* drawPassOutput);

        RHI::RgBufferDesc grabDispatchArgsBufferDesc;

        // used for later draw call
        std::shared_ptr<RHI::D3D12Buffer> terrainPatchNodeIndexBuffer;

        // used for shadowmap drawing
        TerrainDirShadowmapCommandBuffer               dirShadowmapCommandBuffers;
        std::vector<TerrainSpotShadowmapCommandBuffer> spotShadowmapCommandBuffer;

        Shader indirecTerrainPatchNodesGenCS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirecTerrainPatchNodesGenSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirecTerrainPatchNodesGenPSO;
	};
}

