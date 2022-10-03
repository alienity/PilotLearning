#pragma once

#include "runtime/core/base/macro.h"
#include "runtime/function/render/renderer_moyu.h"
#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/renderer/ui_pass.h"
#include "runtime/function/render/renderer/indirect_cull_pass.h"
#include "runtime/function/render/renderer/indirect_shadow_pass.h"
#include "runtime/function/render/renderer/indirect_draw_pass.h"
#include "runtime/function/render/renderer/indirect_display_pass.h"

namespace Pilot
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

		virtual void OnRender(RHI::D3D12CommandContext& Context);

    public:
        uint32_t    backBufferWidth;
        uint32_t    backBufferHeight;
        DXGI_FORMAT backBufferFormat;
        DXGI_FORMAT depthBufferFormat;

        DXGI_FORMAT pipleineColorFormat;
        DXGI_FORMAT pipleineDepthFormat;

        std::shared_ptr<UIPass>             mUIPass;
        std::shared_ptr<IndirectCullPass>   mIndirectCullPass;
        std::shared_ptr<IndirectShadowPass> mIndirectShadowPass;
        std::shared_ptr<IndirectDrawPass>   mIndirectDrawPass;
        std::shared_ptr<DisplayPass>        mDisplayPass;
        
        std::shared_ptr<RHI::D3D12Buffer>              p_IndirectCommandBuffer;
        std::shared_ptr<RHI::D3D12UnorderedAccessView> p_IndirectCommandBufferUav;

        std::shared_ptr<RHI::D3D12Buffer> pPerframeBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMaterialBuffer;
        std::shared_ptr<RHI::D3D12Buffer> pMeshBuffer;

        std::shared_ptr<RHI::D3D12Texture>            p_RenderTargetTex;
        std::shared_ptr<RHI::D3D12ShaderResourceView> p_RenderTargetTexSRV;
        std::shared_ptr<RHI::D3D12RenderTargetView>   p_RenderTargetTexRTV;

        uint32_t numPointLights = 0;
        uint32_t numMaterials   = 0;
        uint32_t numMeshes      = 0;

        std::uint64_t totalCommandBufferSizeInBytes = HLSL::MeshLimit * sizeof(HLSL::CommandSignatureParams);
        std::uint64_t commandBufferCounterOffset =
            D3D12RHIUtils::AlignUp(totalCommandBufferSizeInBytes, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

	private:
        WindowSystem*  windowSystem;
        int            ViewMode = 0;
	};


}
