#pragma once

#include "runtime/function/render/render_pass.h"

#include "runtime/function/global/global_context.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace MoYu
{
    class ColorPyramidPass : public RenderPass
	{
    public:
        struct DrawPassInitInfo : public RenderPassInitInfo
        {
            RHI::RgTextureDesc albedoTexDesc;

            ShaderCompiler*       m_ShaderCompiler;
            std::filesystem::path m_ShaderRootPath;
        };

        struct DrawInputParameters : public PassInput
        {
            RHI::RgResourceHandle perframeBufferHandle;
            RHI::RgResourceHandle colorHandle;
        };

        struct DrawOutputParameters : public PassOutput
        {
            RHI::RgResourceHandle colorHandle;
            RHI::RgResourceHandle colorPyramidHandle;
        };

    public:
        ~ColorPyramidPass() { destroy(); }

        void initialize(const DrawPassInitInfo& init_info);
        void prepareMeshData(std::shared_ptr<RenderResource> render_resource);
        void update(RHI::RenderGraph& graph, DrawInputParameters& passInput, DrawOutputParameters& passOutput);
        void destroy() override final;

        bool initializeRenderTarget(RHI::RenderGraph& graph, DrawOutputParameters* drawPassOutput);
        void generateColorPyramid(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture);
        void generateColorPyramid(RHI::D3D12ComputeContext* context, RHI::D3D12Texture* srcTexture, int srcIndex);

    private:
        RHI::RgTextureDesc albedoTexDesc;
	};
}

