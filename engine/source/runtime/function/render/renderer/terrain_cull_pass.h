#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/render/rhi/rendergraph/RenderGraphCommon.h"
#include "runtime/function/render/rhi/hlsl_data_types.h"

namespace MoYu
{
    struct TerrainClipMeshCommandSigParams
    {
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };

    struct TerrainToDrawCommandSignatureParams
    {
        uint32_t                     ClipIndex;
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };

    struct TerrainDirShadowmapCommandBuffer
    {
        std::vector<std::shared_ptr<RHI::D3D12Buffer>> m_visableClipMeshTransformCommandSigBuffer;
        SceneCommonIdentifier m_identifier;
        
        void Reset()
        {
            m_identifier = SceneCommonIdentifier();
            for (int i = 0; i < m_visableClipMeshTransformCommandSigBuffer.size(); i++)
            {
                m_visableClipMeshTransformCommandSigBuffer[i] = nullptr;
            }
            m_visableClipMeshTransformCommandSigBuffer.clear();
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

        struct TerrainCullInput
        {
            TerrainCullInput()
            {
                perframeBufferHandle.Invalidate();
            }

            RHI::RgResourceHandle perframeBufferHandle;
        };

        struct TerrainCullOutput
        {
            RHI::RgResourceHandle terrainHeightmapHandle;
            RHI::RgResourceHandle terrainNormalmapHandle;
            RHI::RgResourceHandle maxHeightmapPyramidHandle;
            RHI::RgResourceHandle minHeightmapPyramidHandle;

            RHI::RgResourceHandle terrainCommandSigHandle;
            RHI::RgResourceHandle transformBufferHandle;
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

        RHI::RgBufferDesc grabDispatchArgsBufferDesc;

        // terrain heightmap min max heightmap
        std::shared_ptr<RHI::D3D12Texture> terrainMinHeightMap;
        std::shared_ptr<RHI::D3D12Texture> terrainMaxHeightMap;

        bool isTerrainMinMaxHeightReady;

        // main camera instance CommandSignature Buffer
        std::shared_ptr<RHI::D3D12Buffer> terrainClipMeshUploadCommandSigBuffer;
        std::shared_ptr<RHI::D3D12Buffer> clipmapTransformUploadCommandSigBuffer;

        // 5中clipmap的meshtype
        std::shared_ptr<RHI::D3D12Buffer> terrainClipMeshCommandSigBuffer;
        // terrain的clipmap的transform
        std::shared_ptr<RHI::D3D12Buffer> clipmapTransformCommandSigBuffer;

        // 相机视锥内的clipmap
        std::shared_ptr<RHI::D3D12Buffer> cameraVisableClipMeshTransformCommandSigBuffer;
        // 方向光的多级clipmap
        TerrainDirShadowmapCommandBuffer dirVisableClipMeshTransformCommandSigBuffer;


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

