#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class DepthPyramidPass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc albedoTexDesc;
            RHI::RgTextureDesc depthTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle depthHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle minDepthPtyramidHandle;
            RHI::RgResourceHandle maxDepthPtyramidHandle;
        };

    public:
        ~DepthPyramidPass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

        bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);
        void generateMipmapForDepthPyramid(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, bool genMin);
        void generateMipmapForDepthPyramid(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, int srcIndex, bool genMin);

        std::shared_ptr<RHI::D3D12Texture> GetLastFrameMinDepthPyramid();
        std::shared_ptr<RHI::D3D12Texture> GetLastFrameMaxDepthPyramid();

        std::shared_ptr<RHI::D3D12Texture> GetCurrentFrameMinDepthPyramid();
        std::shared_ptr<RHI::D3D12Texture> GetCurrentFrameMaxDepthPyramid();

    private:
        int passIndex;

        // used for depth
        struct DepthPyramidStruct
        {
            std::shared_ptr<RHI::D3D12Texture> pMinDpethPyramid;
            std::shared_ptr<RHI::D3D12Texture> pMaxDpethPyramid;
        };
        DepthPyramidStruct mDepthPyramid[2];

        //bool isDepthMinMaxHeightReady;

        RHI::RgTextureDesc albedoTexDesc;
        RHI::RgTextureDesc depthTexDesc;

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

