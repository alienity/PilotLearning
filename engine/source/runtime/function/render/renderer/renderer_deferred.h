#pragma once

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/renderer_moyu.h"
#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/renderer/tool_pass.h"
#include "runtime/function/render/renderer/ui_pass.h"
#include "runtime/function/render/renderer/indirect_cull_pass.h"
#include "runtime/function/render/renderer/terrain_cull_pass.h"
#include "runtime/function/render/renderer/indirect_shadow_pass.h"
#include "runtime/function/render/renderer/indirect_terrain_shadow_pass.h"
#include "runtime/function/render/renderer/indirect_draw_pass.h"
#include "runtime/function/render/renderer/skybox_pass.h"
#include "runtime/function/render/renderer/indirect_draw_transparent_pass.h"
#include "runtime/function/render/renderer/ssr_pass.h"
#include "runtime/function/render/renderer/ao_pass.h"
#include "runtime/function/render/renderer/gtao_pass.h"
#include "runtime/function/render/renderer/postprocess_passes.h"
#include "runtime/function/render/renderer/indirect_display_pass.h"
#include "runtime/function/render/renderer/indirect_gbuffer_pass.h"
#include "runtime/function/render/renderer/indirect_depth_pyramid_pass.h"
#include "runtime/function/render/renderer/color_pyramid_pass.h"
#include "runtime/function/render/renderer/indirect_terrain_gbuffer_pass.h"
#include "runtime/function/render/renderer/indirect_lightloop_pass.h"
#include "runtime/function/render/renderer/volume_light_pass.h"

namespace MoYu
{


	class DeferredRenderer final : public Renderer
	{
    public:
        DeferredRenderer(RendererInitParams& renderInitParams);

        virtual void Initialize();
        void         InitGlobalBuffer();
        void         InitPass();
        virtual void InitializeUIRenderBackend(WindowUI* window_ui);
        virtual void PreparePassData(std::shared_ptr<RenderResource> render_resource);

		virtual ~DeferredRenderer();

		virtual void OnRender(RHI::D3D12CommandContext* Context);

        virtual void PreRender(double deltaTime) override;
        virtual void PostRender(double deltaTime) override;

        std::shared_ptr<RHI::D3D12Texture> GetCurrentFrameColorPyramid();
        std::shared_ptr<RHI::D3D12Texture> GetLastFrameColorPyramid();

    public:
        uint32_t    backBufferWidth;
        uint32_t    backBufferHeight;
        DXGI_FORMAT backBufferFormat;
        DXGI_FORMAT depthBufferFormat;

        DXGI_FORMAT pipleineColorFormat;
        DXGI_FORMAT pipleineDepthFormat;

        std::shared_ptr<ToolPass> mToolPass;

        std::shared_ptr<UIPass>                      mUIPass;
        std::shared_ptr<IndirectCullPass>            mIndirectCullPass;
        std::shared_ptr<IndirectTerrainCullPass>     mTerrainCullPass;
        std::shared_ptr<IndirectShadowPass>          mIndirectShadowPass;
        std::shared_ptr<IndirectTerrainShadowPass>   mIndirectTerrainShadowPass;
        std::shared_ptr<IndirectGBufferPass>         mIndirectGBufferPass;
        std::shared_ptr<IndirectTerrainGBufferPass>  mIndirectTerrainGBufferPass;
        std::shared_ptr<IndirectLightLoopPass>       mIndirectLightLoopPass;
        std::shared_ptr<IndirectDrawPass>            mIndirectOpaqueDrawPass;
        std::shared_ptr<DepthPyramidPass>            mDepthPyramidPass;
        std::shared_ptr<ColorPyramidPass>            mColorPyramidPass;
        std::shared_ptr<SkyBoxPass>                  mSkyBoxPass;
        std::shared_ptr<IndirectDrawTransparentPass> mIndirectTransparentDrawPass;
        std::shared_ptr<AOPass>                      mAOPass;
        std::shared_ptr<GTAOPass>                    mGTAOPass;
        std::shared_ptr<VolumeLightPass>             mVolumeLightPass;
        std::shared_ptr<SSRPass>                     mSSRPass;
        std::shared_ptr<PostprocessPasses>           mPostprocessPasses;
        std::shared_ptr<DisplayPass>                 mDisplayPass;
        
        std::shared_ptr<RHI::D3D12Texture> p_RenderTargetTex;

        std::shared_ptr<RHI::D3D12Texture> p_ColorPyramidRTs[2];

	private:
        WindowSystem*  windowSystem;
        int            ViewMode = 0;
	};


}
