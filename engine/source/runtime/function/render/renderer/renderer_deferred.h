#pragma once

#include "runtime/core/base/macro.h"
#include "runtime/function/render/renderer_moyu.h"
#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/renderer/ui_pass.h"
#include "runtime/function/render/renderer/indirect_cull_pass.h"
#include "runtime/function/render/renderer/indirect_shadow_pass.h"
#include "runtime/function/render/renderer/indirect_draw_pass.h"
#include "runtime/function/render/renderer/skybox_pass.h"
#include "runtime/function/render/renderer/indirect_draw_transparent_pass.h"
#include "runtime/function/render/renderer/postprocess_passes.h"
#include "runtime/function/render/renderer/indirect_display_pass.h"

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
        virtual void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource);

		virtual ~DeferredRenderer();

		virtual void OnRender(RHI::D3D12CommandContext* Context);

    public:
        uint32_t    backBufferWidth;
        uint32_t    backBufferHeight;
        DXGI_FORMAT backBufferFormat;
        DXGI_FORMAT depthBufferFormat;

        DXGI_FORMAT pipleineColorFormat;
        DXGI_FORMAT pipleineDepthFormat;

        std::shared_ptr<UIPass>                      mUIPass;
        std::shared_ptr<IndirectCullPass>            mIndirectCullPass;
        std::shared_ptr<IndirectShadowPass>          mIndirectShadowPass;
        std::shared_ptr<IndirectDrawPass>            mIndirectOpaqueDrawPass;
        std::shared_ptr<SkyBoxPass>                  mSkyBoxPass;
        std::shared_ptr<IndirectDrawTransparentPass> mIndirectTransparentDrawPass;
        std::shared_ptr<PostprocessPasses>           mPostprocessPasses;
        std::shared_ptr<DisplayPass>                 mDisplayPass;
        
        std::shared_ptr<RHI::D3D12Texture> p_RenderTargetTex;

	private:
        WindowSystem*  windowSystem;
        int            ViewMode = 0;
	};


}
