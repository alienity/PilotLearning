#pragma once

#include "runtime/core/base/macro.h"
#include "runtime/function/render/renderer_moyu.h"
#include "runtime/function/render/render_scene.h"

namespace Pilot
{

	class DeferredRenderer final : public Renderer
	{
    public:
        DeferredRenderer(RHI::D3D12Device* Device, ShaderCompiler* Compiler, RHI::D3D12SwapChain* SwapChain);

		virtual ~DeferredRenderer();

		virtual void OnRender(RHI::D3D12CommandContext& Context);

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

        int ViewMode = 0;
	};


}
