#pragma once

#include "runtime/core/base/macro.h"
#include "runtime/function/render/renderer_moyu.h"
#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/renderer/ui_pass.h"

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

	private:
        struct CommandSignatureParams
        {
            std::uint32_t                MeshIndex;
            D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
            D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
            D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
        };

	    static constexpr std::uint64_t TotalCommandBufferSizeInBytes =
                RenderScene::MeshLimit * sizeof(CommandSignatureParams);
        static constexpr std::uint64_t CommandBufferCounterOffset =
            AlignUp(TotalCommandBufferSizeInBytes, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

        RHI::D3D12Buffer              IndirectCommandBuffer;
        RHI::D3D12UnorderedAccessView IndirectCommandBufferUav;

        WindowSystem*                 windowSystem;

        int ViewMode = 0;
	};


}
