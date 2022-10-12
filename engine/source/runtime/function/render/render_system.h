#pragma once

#include "runtime/function/render/render_entity.h"
#include "runtime/function/render/render_guid_allocator.h"
#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_type.h"

#include <array>
#include <memory>
#include <optional>

namespace Pilot
{
    class WindowSystem;
    class RenderResourceBase;
    class RendererManager;
    class RenderScene;
    class RenderCamera;
    class WindowUI;

    struct EngineContentViewport;

    struct RenderSystemInitInfo
    {
        std::shared_ptr<WindowSystem> window_system;
    };

    class RenderSystem
    {
    public:
        RenderSystem() = default;
        ~RenderSystem();

        void initialize(RenderSystemInitInfo init_info);
        void tick();

        void                          swapLogicRenderData();
        RenderSwapContext&            getSwapContext();
        std::shared_ptr<RenderCamera> getRenderCamera() const;

        void      initializeUIRenderBackend(WindowUI* window_ui);
        uint32_t  getGuidOfPickedMesh(const Vector2& picked_uv);
        
        EngineContentViewport getEngineContentViewport() const;

        GuidAllocator<MeshSourceDesc>&   getMeshAssetIdAllocator();

        void clearForLevelReloading();

    private:
        RenderSwapContext m_swap_context;

        std::shared_ptr<RenderCamera>       m_render_camera;
        std::shared_ptr<RenderScene>        m_render_scene;
        std::shared_ptr<RenderResourceBase> m_render_resource;
        std::shared_ptr<RendererManager>    m_renderer_manager;

        void processSwapData();
    };
} // namespace Pilot
