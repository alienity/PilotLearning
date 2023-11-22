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
        void generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, bool genMin);
        void generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, int srcIndex, bool genMin);

        struct MipGenInBuffer
        {
            glm::uint   SrcMipLevel;  // Texture level of source mip
            glm::uint   NumMipLevels; // Number of OutMips to write: [1, 4]
            glm::float2 TexelSize;    // 1.0 / OutMip1.Dimensions

            glm::uint SrcIndex;
            glm::uint OutMip1Index;
            glm::uint OutMip2Index;
            glm::uint OutMip3Index;
            glm::uint OutMip4Index;
        };

        RHI::RgBufferDesc grabDispatchArgsBufferDesc;

        // terrain heightmap min max heightmap
        std::shared_ptr<RHI::D3D12Texture> terrainMinHeightMap;
        std::shared_ptr<RHI::D3D12Texture> terrainMaxHeightMap;

        // used for later draw call
        std::shared_ptr<RHI::D3D12Buffer> terrainPatchNodeBuffer;
        // used to mark terrain patch node that are visiable to main camera
        std::shared_ptr<RHI::D3D12Buffer> patchNodeVisiableToMainCameraIndexBuffer;

        // main camera instance CommandSignature Buffer
        std::shared_ptr<RHI::D3D12Buffer> terrainUploadCommandSigBuffer;
        std::shared_ptr<RHI::D3D12Buffer> terrainCommandSigBuffer;
        int terrainInstanceCountOffset;

        // used for shadowmap drawing
        TerrainDirShadowmapCommandBuffer               dirShadowmapCommandBuffers;
        std::vector<TerrainSpotShadowmapCommandBuffer> spotShadowmapCommandBuffer;

        Shader indirecTerrainPatchNodesGenCS;
        std::shared_ptr<RHI::D3D12RootSignature> pIndirecTerrainPatchNodesGenSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pIndirecTerrainPatchNodesGenPSO;

        Shader patchNodeVisiableIndexGenCS;
        std::shared_ptr<RHI::D3D12RootSignature> pPatchNodeVisiableIndexGenSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pPatchNodeVisiableIndexGenPSO;
	};
}

