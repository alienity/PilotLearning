#pragma once

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



    private:

    };
} // namespace Pilot
