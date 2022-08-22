#pragma once

#include "runtime/core/base/macro.h"
#include "runtime/function/render/renderer_moyu.h"
#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/renderer/ui_pass.h"
#include "runtime/function/render/renderer/indirect_cull_pass.h"
#include "runtime/function/render/renderer/indirect_draw_pass.h"

namespace Pilot
{
	class DeferredRenderer final : public Renderer
	{
    public:
        DeferredRenderer(RendererInitParams& renderInitParams);

        virtual void Initialize();
        virtual void InitializeUIRenderBackend(WindowUI* window_ui);
        virtual void PreparePassData(std::shared_ptr<RenderResourceBase> render_resource);

		virtual ~DeferredRenderer();

		virtual void OnRender(RHI::D3D12CommandContext& Context);

        std::shared_ptr<UIPass> mUIPass;
        std::shared_ptr<IndirectCullPass> mIndirectCullPass;
        std::shared_ptr<IndirectDrawPass> mIndirectDrawPass;

	private:
        WindowSystem* windowSystem;
        int           ViewMode = 0;
	};


}
