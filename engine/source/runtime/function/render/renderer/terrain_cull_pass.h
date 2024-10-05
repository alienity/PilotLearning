#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"
#include "runtime/function/render/rhi/hlsl_data_types.h"

namespace MoYu
{
    struct TerrainDirShadowmapCommandBuffer
    {
        std::vector<std::shared_ptr<RHI::D3D12Buffer>> m_VisableClipmapBuffers;
        SceneCommonIdentifier m_identifier;
        
        void Reset()
        {
            m_identifier = SceneCommonIdentifier();
            for (int i = 0; i < m_VisableClipmapBuffers.size(); i++)
            {
                m_VisableClipmapBuffers[i] = nullptr;
            }
            m_VisableClipmapBuffers.clear();
        }
    };

    struct TerrainCullInitInfo : public RenderPassInitInfo
    {
        RHI::RgTextureDesc colorTexDesc;
        RHI::RgTextureDesc depthTexDesc;

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

        struct TerrainCullInput
        {
            TerrainCullInput()
            {
                perframeBufferHandle.Invalidate();
                hizDepthBufferHandle.Invalidate();
            }

            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle hizDepthBufferHandle;
        };

        struct TerrainCullOutput
        {
            RHI::RgResourceHandle terrainHeightmapHandle;
            RHI::RgResourceHandle terrainNormalmapHandle;
            RHI::RgResourceHandle maxHeightmapPyramidHandle;
            RHI::RgResourceHandle minHeightmapPyramidHandle;
            RHI::RgResourceHandle terrainRenderDataHandle;
            RHI::RgResourceHandle terrainMatPropertyHandle;

            RHI::RgResourceHandle mainCamVisPatchListHandle;
            RHI::RgResourceHandle mainCamVisCmdSigBufferHandle;

            std::vector<RHI::RgResourceHandle> dirConsBufferHandles;
            std::vector<RHI::RgResourceHandle> dirVisPatchListHandles;
            std::vector<RHI::RgResourceHandle> dirVisCmdSigBufferHandles;
        };

        struct DepthCullIndexInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle minDepthPyramidHandle;
        };

    public:
        ~IndirectTerrainCullPass() { destroy(); }

        void initialize(const TerrainCullInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, TerrainCullInput& passInput, TerrainCullOutput& passOutput);
        
        void destroy() override final;
    private:
        struct DepthCullInput
        {
            DepthCullInput()
            {
                perframeBufferHandle.Invalidate();
                minDepthPyramidHandle.Invalidate();
                terrainPatchNodeHandle.Invalidate();
                inputIndexBufferHandle.Invalidate();
                nonVisiableIndexBufferHandle.Invalidate();
                visiableIndexBufferHandle.Invalidate();
            }

            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle minDepthPyramidHandle;
            RHI::RgResourceHandle terrainPatchNodeHandle;
            RHI::RgResourceHandle inputIndexBufferHandle;

            RHI::RgResourceHandle nonVisiableIndexBufferHandle;
            RHI::RgResourceHandle visiableIndexBufferHandle;
        };

        struct DepthCommandSigGenInput
        {
            DepthCommandSigGenInput()
            {
                inputIndexBufferHandle.Invalidate();
                terrainCommandSigBufHandle.Invalidate();
            }

            RHI::RgResourceHandle inputIndexBufferHandle;

            RHI::RgResourceHandle terrainCommandSigBufHandle;
        };

        bool initializeRenderTarget(RHI::RenderGraph& graph, TerrainCullOutput* drawPassOutput);
        void generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, bool genMin);
        void generateMipmapForTerrainHeightmap(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, int srcIndex, bool genMin);

        RHI::RgBufferDesc traverseDispatchArgsBufferDesc;
        RHI::RgBufferDesc buildPatchArgsBufferDesc;
        
        bool iMinMaxHeightReady;

        RHI::RgTextureDesc colorTexDesc;
        RHI::RgTextureDesc depthTexDesc;

        /*
         * 对于WorldLodParams
         * - nodeSize为Node的边长(米)
         * - patchExtent等于nodeSize/16
         * - nodeCount等于WorldSize/nodeSize
         * - sectorCountPerNode等于2^lod
         */
        glm::float4 worldLODParams[MAX_TERRAIN_LOD + 1];
        int nodeIDOffsetLOD[MAX_TERRAIN_LOD + 1];

        std::shared_ptr<RHI::D3D12Texture> pMinHeightMap; // RG32
        std::shared_ptr<RHI::D3D12Texture> pMaxHeightMap; // RG32
        std::shared_ptr<RHI::D3D12Texture> pQuadTreeMap; // R16

        std::shared_ptr<RHI::D3D12Texture> pLodMap; // R8, 160x160

        std::shared_ptr<RHI::D3D12Buffer> TempNodeList[2]; // uint2, 代表当前LOD下Node的二维索引
        std::shared_ptr<RHI::D3D12Buffer> FinalNodeList; // uint3, 其中z表示Node的LOD，xy代表二维索引
        std::shared_ptr<RHI::D3D12Buffer> NodeDescriptors; // uint, branch

        std::shared_ptr<RHI::D3D12Buffer> pTerrainRenderDataBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pTerrainMatPropertiesBuffer;

        // 相机视锥内的CommandSignature
        std::shared_ptr<RHI::D3D12Buffer> camUploadPatchCmdSigBuffer;

        std::shared_ptr<RHI::D3D12Buffer> CulledPatchListBuffer;
        std::shared_ptr<RHI::D3D12Buffer> mTerrainConsBuffer; // 纯地形绘制常量
        std::shared_ptr<RHI::D3D12Buffer> camPatchCmdSigBuffer;

        std::vector<std::shared_ptr<RHI::D3D12Buffer>> CulledDirPatchListBuffers; // For DirectionalLight
        std::vector<std::shared_ptr<RHI::D3D12Buffer>> mTerrainDirConsBuffers; // For DirectionalLight
        std::vector<std::shared_ptr<RHI::D3D12Buffer>> dirPatchCmdSigBuffers; // For DirectionalLight

        Shader InitQuadTreeCS;
        std::shared_ptr<RHI::D3D12RootSignature> pInitQuadTreeSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pInitQuadTreePSO;

        Shader TraverseQuadTreeCS;
        std::shared_ptr<RHI::D3D12RootSignature> pTraverseQuadTreeSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pTraverseQuadTreePSO;

        Shader BuildLodMapCS;
        std::shared_ptr<RHI::D3D12RootSignature> pBuildLodMapSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pBuildLodMapPSO;

        Shader BuildPatchesCS;
        std::shared_ptr<RHI::D3D12RootSignature> pBuildPatchesSignature;
        std::shared_ptr<RHI::D3D12PipelineState> pBuildPatchesPSO;

	};
}

