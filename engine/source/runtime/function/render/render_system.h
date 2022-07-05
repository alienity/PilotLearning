#pragma once

#include "runtime/function/render/render_swap_context.h"

#include <array>
#include <memory>
#include <optional>

namespace Pilot
{
    class WindowSystem;
    class RendererManager;

    struct RenderSystemInitInfo
    {
        std::shared_ptr<WindowSystem> window_system;
    };

    struct EngineContentViewport
    {
        float x { 0.f};
        float y { 0.f};
        float width { 0.f};
        float height { 0.f};
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

    private:
        
        RenderSwapContext m_swap_context;

        std::shared_ptr<RenderCamera> m_render_camera;

        std::shared_ptr<RendererManager> m_renderer_manager;
    };
} // namespace Pilot
