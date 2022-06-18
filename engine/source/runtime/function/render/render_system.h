#pragma once

#include "runtime/function/render/render_swap_context.h"

#include <array>
#include <memory>
#include <optional>

namespace Pilot
{
    class WindowSystem;

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
        //RENDER_PIPELINE_TYPE m_render_pipeline_type {RENDER_PIPELINE_TYPE::DEFERRED_PIPELINE};

        RenderSwapContext m_swap_context;
    };
} // namespace Pilot
