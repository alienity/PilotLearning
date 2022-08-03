#include "runtime/function/render/renderer/ui_pass.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/ui/window_ui.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_dx12.h>

#include <cassert>

namespace Pilot
{
    void UIPass::initialize(const RenderPassInitInfo* init_info)
    {
        RenderPass::initialize(nullptr);
    }

    void UIPass::initializeUIRenderBackend(WindowUI* window_ui)
    {
        m_window_ui = window_ui;

        ImGui_ImplGlfw_InitForOther(m_vulkan_rhi->m_window, true);

        // may be different from the real swapchain image count
        // see ImGui_ImplVulkanH_GetMinImageCountFromPresentMode
        init_info.MinImageCount = 3;
        init_info.ImageCount    = 3;
        ImGui_ImplVulkan_Init(&init_info, m_framebuffer.render_pass);

        uploadFonts();
    }



}
