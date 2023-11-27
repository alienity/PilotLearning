#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"
#include "runtime/function/render/rhi/hlsl_data_types.h"

namespace MoYu
{
    struct TerrainCommandSignatureParams
    {
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };

    struct TerrainDrawCommandBuffer
    {
        std::shared_ptr<RHI::D3D12Buffer> m_PatchNodeVisiableIndexBuffer;
        std::shared_ptr<RHI::D3D12Buffer> m_TerrainCommandSignatureBuffer;
    };

    struct TerrainDirShadowmapCommandBuffer
    {
        std::vector<std::shared_ptr<RHI::D3D12Buffer>> m_PatchNodeVisiableIndexBuffers;
        std::vector<std::shared_ptr<RHI::D3D12Buffer>> m_TerrainCommandSignatureBuffers;
        SceneCommonIdentifier m_identifier;
        
        void Reset()
        {
            m_identifier = SceneCommonIdentifier();
            for (int i = 0; i < m_PatchNodeVisiableIndexBuffers.size(); i++)
            {
                m_PatchNodeVisiableIndexBuffers[i] = nullptr;
            }
            m_PatchNodeVisiableIndexBuffers.clear();
            for (int i = 0; i < m_TerrainCommandSignatureBuffers.size(); i++)
            {
                m_TerrainCommandSignatureBuffers[i] = nullptr;
            }
            m_TerrainCommandSignatureBuffers.clear();
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
            RHI::RgResourceHandle commandSigBufferHandle = RHI::_DefaultRgResourceHandle;
            RHI::RgResourceHandle indirectIndexBufferHandle = RHI::_DefaultRgResourceHandle;
        };

        struct TerrainCullInput : public PassInput
        {
            TerrainCullInput() { perframeBufferHandle.Invalidate(); }

            RHI::RgResourceHandle perframeBufferHandle;
        };

        struct TerrainCullOutput : public PassOutput
        {
            TerrainCullOutput() { terrainPatchNodeBufferHandle.Invalidate(); }

            RHI::RgResourceHandle terrainHeightmapHandle;
            RHI::RgResourceHandle terrainNormalmapHandle;
            RHI::RgResourceHandle maxHeightmapPyramidHandle;
            RHI::RgResourceHandle minHeightmapPyramidHandle;

            RHI::RgResourceHandle terrainPatchNodeBufferHandle;

            DrawCallCommandBufferHandle terrainDrawHandle;
            std::vector<DrawCallCommandBufferHandle> directionShadowmapHandles;
        };

    public:
        ~IndirectTerrainCullPass() { destroy(); }

        void initialize(const TerrainCullInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, TerrainCullInput& passInput, TerrainCullOutput& passOutput);

        void destroy() override final;

        std::shared_ptr<RHI::D3D12Texture> lastFrameMinDepthPyramid;
        std::shared_ptr<RHI::D3D12Texture> lastFrameMaxDepthPyramid;
    private:
        bool initializeRenderTarget(RHI::RenderGraph& graph, TerrainCullOutput* drawPassOutput);
        void generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, bool genMin);
        void generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, int srcIndex, bool genMin);

        RHI::RgBufferDesc grabDispatchArgsBufferDesc;

        // terrain heightmap min max heightmap
        std::shared_ptr<RHI::D3D12Texture> terrainMinHeightMap;
        std::shared_ptr<RHI::D3D12Texture> terrainMaxHeightMap;

        bool isTerrainMinMaxHeightReady;

        // used for later draw call
        std::shared_ptr<RHI::D3D12Buffer> terrainPatchNodeBuffer;

        // 用于主相机绘制的buffer
        TerrainDrawCommandBuffer mainCameraCommandBuffer;
        // 用于方向光绘制的buffer
        TerrainDirShadowmapCommandBuffer dirShadowmapCommandBuffers;

        // main camera instance CommandSignature Buffer
        std::shared_ptr<RHI::D3D12Buffer> terrainUploadCommandSigBuffer;
        int terrainInstanceCountOffset;

        Shader indirecTerrainPatchNodesGenCS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirecTerrainPatchNodesGenSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirecTerrainPatchNodesGenPSO;

        Shader patchNodeVisToMainCamIndexGenCS;
        std::shared_ptr<RHI::D3D12RootSignature> pPatchNodeVisToMainCamIndexGenSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pPatchNodeVisToMainCamIndexGenPSO;

        Shader patchNodeVisToDirCascadeIndexGenCS;
        std::shared_ptr<RHI::D3D12RootSignature> pPatchNodeVisToDirCascadeIndexGenSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pPatchNodeVisToDirCascadeIndexGenPSO;
	};
}

