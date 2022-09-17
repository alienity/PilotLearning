#pragma once

#include <memory>
#include <directx/d3d12.h>

namespace Pilot
{
    class WindowSystem;
    class RenderSystem;

    struct WindowUIInitInfo
    {
        std::shared_ptr<WindowSystem> window_system;
        std::shared_ptr<RenderSystem> render_system;
    };

    class WindowUI
    {
    public:
        virtual void initialize(WindowUIInitInfo init_info) = 0;
        virtual void preRender() = 0;
        virtual void setGameView(D3D12_GPU_DESCRIPTOR_HANDLE handle, uint32_t width, uint32_t height) = 0;
    };
} // namespace Pilot
