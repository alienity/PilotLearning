#pragma once

#include "editor/include/axis.h"

#include "runtime/core/math/moyu_math2.h"
#include "runtime/core/base/robin_hood.h"

#include "runtime/function/framework/object/object.h"
#include "runtime/function/ui/window_ui.h"

#include "editor/include/editor_file_service.h"

#include <chrono>
#include <map>
#include <vector>

namespace MoYu
{
    class PilotEditor;
    class WindowSystem;
    class RenderSystem;

    class EditorUI : public WindowUI
    {
    public:
        EditorUI();

    private:
        //void        onFileContentItemClicked(EditorFileNode* node);
        void        buildEditorFileAssetsUITree(std::shared_ptr<EditorFileNode> node);
        void        drawAxisToggleButton(const char* string_id, bool check_state, int axis_mode);
        void        createComponentUI(MoYu::Component* component);
        //void        createLeafNodeUI(Reflection::ReflectionInstance& instance);
        //std::string getLeafUINodeParentLabel();

        void showEditorUI();
        void showEditorMenu(bool* p_open);
        void showEditorWorldObjectsWindow(bool* p_open);
        void showEditorFileContentWindow(bool* p_open);
        void showEditorGameWindow(bool* p_open);
        void showEditorDetailWindow(bool* p_open);

        void setUIColorStyle();

        void showEditorWorldObjectsRecursive(std::weak_ptr<GObject> node_obj);

        void fileDragHelper(std::shared_ptr<EditorFileNode> node);

    public:
        virtual void initialize(WindowUIInitInfo init_info) override final;
        virtual void preRender() override final;
        virtual void setGameView(D3D12_GPU_DESCRIPTOR_HANDLE handle, uint32_t width, uint32_t height) override final;

    private:
        robin_hood::unordered_map<std::string, std::function<void(std::string, bool&, void*)>> m_editor_ui_creator;
        //std::unordered_map<std::string, unsigned int>                                   m_new_object_index_map;
        EditorFileService                                                               m_editor_file_service;
        std::chrono::time_point<std::chrono::steady_clock>                              m_last_file_tree_update;

        //std::vector<Component*> m_editor_component_stack;

        float handleWidth;
        float handleHeight;
        D3D12_GPU_DESCRIPTOR_HANDLE handleOfGameView;

        bool m_editor_menu_window_open       = true;
        bool m_asset_window_open             = true;
        bool m_game_engine_window_open       = true;
        bool m_file_content_window_open      = true;
        bool m_detail_window_open            = true;
        bool m_scene_lights_window_open      = true;
        bool m_scene_lights_data_window_open = true;
    };
} // namespace MoYu
