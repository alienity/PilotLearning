#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    enum DepthMipGenerateMode
    {
        AverageType,
        MaxType,
        MinType
    };

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
            RHI::RgResourceHandle averageDepthPyramidHandle;
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
        void generateMipmapForDepthPyramid(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, DepthMipGenerateMode mode = DepthMipGenerateMode::AverageType);
        void generateMipmapForDepthPyramid(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, int srcIndex, DepthMipGenerateMode mode = DepthMipGenerateMode::AverageType);

        std::shared_ptr<RHI::D3D12Texture> GetDepthPyramid(DepthMipGenerateMode mode = DepthMipGenerateMode::AverageType, bool lastFrame = false);

    private:
        int passIndex;

        // used for depth
        struct DepthPyramidStruct
        {
            std::shared_ptr<RHI::D3D12Texture> pMinDpethPyramid;
            std::shared_ptr<RHI::D3D12Texture> pMaxDpethPyramid;
            std::shared_ptr<RHI::D3D12Texture> pAverageDpethPyramid;
        };
        DepthPyramidStruct mDepthPyramid[2];

        RHI::RgTextureDesc albedoTexDesc;
        RHI::RgTextureDesc depthTexDesc;

	};
}

