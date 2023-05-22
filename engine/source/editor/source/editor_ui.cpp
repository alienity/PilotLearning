#include "editor/include/editor_ui.h"

#include "editor/include/editor_global_context.h"
#include "editor/include/editor_input_manager.h"
#include "editor/include/editor_scene_manager.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/meta/reflection/reflection.h"

#include "runtime/platform/path/path.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/engine.h"

#include "runtime/function/framework/component/mesh/mesh_component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/component/camera/camera_component.h"
#include "runtime/function/framework/component/light/light_component.h"

#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/framework/material/material_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/glm_wrapper.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_glfw.h>
#include <stb_image.h>

#include "ImGuizmo.h"

namespace MoYu
{
    std::vector<std::pair<std::string, bool>> g_editor_node_state_array;
    int g_node_depth = -1;

    bool DrawVecControl(const std::string& label,
                        MoYu::Vector3&    values,
                        float              resetValue  = 0.0f,
                        float              columnWidth = 100.0f);

    bool DrawVecControl(const std::string& label,
                        MoYu::Quaternion& values,
                        float              resetValue  = 0.0f,
                        float              columnWidth = 100.0f);

    EditorUI::EditorUI()
    {
        const auto& asset_folder            = g_runtime_global_context.m_config_manager->getAssetFolder();
        m_editor_ui_creator["TreeNodePush"] = [this](const std::string& name, void* value_ptr) -> void {
            static ImGuiTableFlags flags      = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings;
            bool                   node_state = false;
            g_node_depth++;
            if (g_node_depth > 0)
            {
                if (g_editor_node_state_array[g_node_depth - 1].second)
                {
                    node_state = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
                }
                else
                {
                    g_editor_node_state_array.emplace_back(std::pair(name.c_str(), node_state));
                    return;
                }
            }
            else
            {
                node_state = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
            }
            g_editor_node_state_array.emplace_back(std::pair(name.c_str(), node_state));

            if (value_ptr != nullptr)
            {
                Component* p_component = static_cast<Component*>(value_ptr);
                m_editor_component_stack.push_back(p_component);
            }
        };
        m_editor_ui_creator["TreeNodePop"] = [this](const std::string& name, void* value_ptr) -> void {
            if (g_editor_node_state_array[g_node_depth].second)
            {
                ImGui::TreePop();
            }
            g_editor_node_state_array.pop_back();
            g_node_depth--;

            if (value_ptr != nullptr)
            {
                m_editor_component_stack.pop_back();
            }
        };
        m_editor_ui_creator["Transform"] = [this](const std::string& name, void* value_ptr) -> void {
            if (g_editor_node_state_array[g_node_depth].second)
            {
                Transform* trans_ptr = static_cast<Transform*>(value_ptr);

                Vector3 degrees_val;

                MoYu::Vector3 euler = trans_ptr->m_rotation.toTaitBryanAngles();

                degrees_val.x = MoYu::Radian(euler.x).valueDegrees();
                degrees_val.y = MoYu::Radian(euler.y).valueDegrees();
                degrees_val.z = MoYu::Radian(euler.z).valueDegrees();

                bool isDirty = false;

                isDirty |= DrawVecControl("Position", trans_ptr->m_position);
                isDirty |= DrawVecControl("Rotation", degrees_val);
                isDirty |= DrawVecControl("Scale", trans_ptr->m_scale);

                MoYu::Vector3 newEuler = Vector3(Math::degreesToRadians(degrees_val.x),
                                                  Math::degreesToRadians(degrees_val.y),
                                                  Math::degreesToRadians(degrees_val.z));
                trans_ptr->m_rotation   = MoYu::Quaternion::fromTaitBryanAngles(newEuler);

                g_editor_global_context.m_scene_manager->drawSelectedEntityAxis();

            if (isDirty)
                {
                    if (!m_editor_component_stack.empty())
                    {
                        Component* m_component_ptr = m_editor_component_stack.back();
                        m_component_ptr->setDirtyFlag(isDirty);
                    }
                }
            }
        };
        m_editor_ui_creator["int"] = [this](const std::string& name, void* value_ptr) -> void {
            bool isDirty = false;

            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                isDirty = ImGui::InputInt(label.c_str(), static_cast<int*>(value_ptr));
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    isDirty = ImGui::InputInt(full_label.c_str(), static_cast<int*>(value_ptr));
                }
            }

            if (isDirty)
            {
                if (!m_editor_component_stack.empty())
                {
                    Component* m_component_ptr = m_editor_component_stack.back();
                    m_component_ptr->setDirtyFlag(isDirty);
                }
            }
        };
        m_editor_ui_creator["float"] = [this](const std::string& name, void* value_ptr) -> void {
            bool isDirty = false;

            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                isDirty = ImGui::InputFloat(label.c_str(), static_cast<float*>(value_ptr));
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    isDirty = ImGui::InputFloat(full_label.c_str(), static_cast<float*>(value_ptr));
                }
            }

            if (isDirty)
            {
                if (!m_editor_component_stack.empty())
                {
                    Component* m_component_ptr = m_editor_component_stack.back();
                    m_component_ptr->setDirtyFlag(isDirty);
                }
            }
        };
        m_editor_ui_creator["bool"] = [this](const std::string& name, void* value_ptr) -> void {
            bool isDirty = false;

            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                isDirty = ImGui::Checkbox(label.c_str(), static_cast<bool*>(value_ptr));
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    ImGui::SameLine();
                    isDirty = ImGui::Checkbox(full_label.c_str(), static_cast<bool*>(value_ptr));
                }
            }

            if (isDirty)
            {
                if (!m_editor_component_stack.empty())
                {
                    Component* m_component_ptr = m_editor_component_stack.back();
                    m_component_ptr->setDirtyFlag(isDirty);
                }
            }
        };
        m_editor_ui_creator["Vector2"] = [this](const std::string& name, void* value_ptr) -> void {
            bool isDirty = false;

            Vector2* vec_ptr = static_cast<Vector2*>(value_ptr);
            float    val[2]  = {vec_ptr->x, vec_ptr->y};
            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                isDirty = ImGui::DragFloat2(label.c_str(), val);
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    isDirty = ImGui::DragFloat2(full_label.c_str(), val);
                }
            }
            vec_ptr->x = val[0];
            vec_ptr->y = val[1];

            if (isDirty)
            {
                if (!m_editor_component_stack.empty())
                {
                    Component* m_component_ptr = m_editor_component_stack.back();
                    m_component_ptr->setDirtyFlag(isDirty);
                }
            }
        };
        m_editor_ui_creator["Vector3"] = [this](const std::string& name, void* value_ptr) -> void {
            bool isDirty = false;

            Vector3* vec_ptr = static_cast<Vector3*>(value_ptr);
            float    val[3]  = {vec_ptr->x, vec_ptr->y, vec_ptr->z};
            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                isDirty = ImGui::DragFloat3(label.c_str(), val);
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    isDirty = ImGui::DragFloat3(full_label.c_str(), val);
                }
            }
            vec_ptr->x = val[0];
            vec_ptr->y = val[1];
            vec_ptr->z = val[2];

            if (isDirty)
            {
                if (!m_editor_component_stack.empty())
                {
                    Component* m_component_ptr = m_editor_component_stack.back();
                    m_component_ptr->setDirtyFlag(isDirty);
                }
            }
        };
        m_editor_ui_creator["Vector4"] = [this](const std::string& name, void* value_ptr) -> void {
            bool isDirty = false;

            Vector4* vec_ptr = static_cast<Vector4*>(value_ptr);
            float    val[4]  = {vec_ptr->x, vec_ptr->y, vec_ptr->z, vec_ptr->w};
            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                isDirty = ImGui::DragFloat4(label.c_str(), val);
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    isDirty = ImGui::DragFloat4(full_label.c_str(), val);
                }
            }
            vec_ptr->x = val[0];
            vec_ptr->y = val[1];
            vec_ptr->z = val[2];
            vec_ptr->w = val[3];

            if (isDirty)
            {
                if (!m_editor_component_stack.empty())
                {
                    Component* m_component_ptr = m_editor_component_stack.back();
                    m_component_ptr->setDirtyFlag(isDirty);
                }
            }
        };
        m_editor_ui_creator["Quaternion"] = [this](const std::string& name, void* value_ptr) -> void {
            bool isDirty = false;

            Quaternion* qua_ptr = static_cast<Quaternion*>(value_ptr);
            float       val[4]  = {qua_ptr->x, qua_ptr->y, qua_ptr->z, qua_ptr->w};
            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                isDirty = ImGui::DragFloat4(label.c_str(), val);
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    isDirty = ImGui::DragFloat4(full_label.c_str(), val);
                }
            }
            qua_ptr->x = val[0];
            qua_ptr->y = val[1];
            qua_ptr->z = val[2];
            qua_ptr->w = val[3];

            if (isDirty)
            {
                if (!m_editor_component_stack.empty())
                {
                    Component* m_component_ptr = m_editor_component_stack.back();
                    m_component_ptr->setDirtyFlag(isDirty);
                }
            }
        };
        m_editor_ui_creator["Color"] = [this](const std::string& name, void* value_ptr) -> void {
            bool isDirty = false;

            Color* color_ptr = static_cast<Color*>(value_ptr);
            float  col[4]   = {color_ptr->r, color_ptr->g, color_ptr->g, color_ptr->a};

            isDirty = ImGui::ColorEdit4("Color", col);

            color_ptr->r = col[0];
            color_ptr->g = col[1];
            color_ptr->b = col[2];
            color_ptr->a = col[3];

            if (isDirty)
            {
                if (!m_editor_component_stack.empty())
                {
                    Component* m_component_ptr = m_editor_component_stack.back();
                    m_component_ptr->setDirtyFlag(isDirty);
                }
            }
        };
        m_editor_ui_creator["std::string"] = [this, &asset_folder](const std::string& name, void* value_ptr) -> void {
            if (g_node_depth == -1)
            {
                std::string label = "##" + name;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::Text("%s", (*static_cast<std::string*>(value_ptr)).c_str());
            }
            else
            {
                if (g_editor_node_state_array[g_node_depth].second)
                {
                    std::string full_label = "##" + getLeafUINodeParentLabel() + name;
                    ImGui::Text("%s", (name + ":").c_str());
                    std::string value_str = *static_cast<std::string*>(value_ptr);
                    if (value_str.find_first_of('/') != std::string::npos)
                    {
                        std::filesystem::path value_path(value_str);
                        if (value_path.is_absolute())
                        {
                            value_path = Path::getRelativePath(asset_folder, value_path);
                        }
                        value_str = value_path.generic_string();
                        if (value_str.size() >= 2 && value_str[0] == '.' && value_str[1] == '.')
                        {
                            value_str.clear();
                        }
                    }
                    ImGui::Text("%s", value_str.c_str());
                }
            }
        };
        m_editor_ui_creator["CameraComponentRes"] = [this, &asset_folder](const std::string& name, void* value_ptr) -> void {
            if (g_editor_node_state_array[g_node_depth].second)
            {
                CameraComponentRes* ccres_ptr = static_cast<CameraComponentRes*>(value_ptr);

                int item_current_idx = -1;

                const char* parameter_items[] = {
                    "FirstPersonCameraParameter", "ThirdPersonCameraParameter", "FreeCameraParameter"};

                std::string type_name = ccres_ptr->m_parameter.getTypeName();
                for (size_t i = 0; i < IM_ARRAYSIZE(parameter_items); i++)
                {
                    if (strcmp(parameter_items[i], type_name.c_str()) == 0)
                    {
                        item_current_idx = i;
                        break;
                    }
                }

                int item_prev_idx = item_current_idx;

                const char* combo_preview_value = parameter_items[item_current_idx];
                if (ImGui::BeginCombo("CameraType", combo_preview_value))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(parameter_items); n++)
                    {
                        const bool is_selected = (item_current_idx == n);
                        if (ImGui::Selectable(parameter_items[n], is_selected))
                            item_current_idx = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                if (item_current_idx != item_prev_idx)
                {
                    PILOT_REFLECTION_DELETE(ccres_ptr->m_parameter);
                    if (item_current_idx == 0)
                    {
                        ccres_ptr->m_parameter = PILOT_REFLECTION_NEW(FirstPersonCameraParameter);
                    }
                    else if (item_current_idx == 1)
                    {
                        ccres_ptr->m_parameter = PILOT_REFLECTION_NEW(ThirdPersonCameraParameter);
                    }
                    else if (item_current_idx == 2)
                    {
                        ccres_ptr->m_parameter = PILOT_REFLECTION_NEW(FreeCameraParameter);
                    }
                }

                auto object_instance = Reflection::ReflectionInstance(
                    MoYu::Reflection::TypeMeta::newMetaFromName(ccres_ptr->m_parameter.getTypeName().c_str()),
                    ccres_ptr->m_parameter.operator->());
                createComponentUI(object_instance);
            }
        };
        m_editor_ui_creator["LightComponentRes"] = [this, &asset_folder](const std::string& name, void* value_ptr) -> void {
            bool isDirty = false;

            if (g_editor_node_state_array[g_node_depth].second)
            {
                LightComponentRes* l_res_ptr = static_cast<LightComponentRes*>(value_ptr);

                int item_current_idx = -1;

                const char* parameter_items[] = {
                    "DirectionalLightParameter", "PointLightParameter", "SpotLightParameter"};

                std::string type_name = l_res_ptr->m_parameter.getTypeName();
                for (size_t i = 0; i < IM_ARRAYSIZE(parameter_items); i++)
                {
                    if (strcmp(parameter_items[i], type_name.c_str()) == 0)
                    {
                        item_current_idx = i;
                        break;
                    }
                }

                int item_prev_idx = item_current_idx;

                const char* combo_preview_value = parameter_items[item_current_idx];
                if (ImGui::BeginCombo("LightType", combo_preview_value))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(parameter_items); n++)
                    {
                        const bool is_selected = (item_current_idx == n);
                        if (ImGui::Selectable(parameter_items[n], is_selected))
                            item_current_idx = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                if (item_current_idx != item_prev_idx)
                {
                    isDirty = true;

                    PILOT_REFLECTION_DELETE(l_res_ptr->m_parameter);
                    if (item_current_idx == 0)
                    {
                        l_res_ptr->m_parameter = PILOT_REFLECTION_NEW(DirectionalLightParameter);
                    }
                    else if (item_current_idx == 1)
                    {
                        l_res_ptr->m_parameter = PILOT_REFLECTION_NEW(PointLightParameter);
                    }
                    else if (item_current_idx == 2)
                    {
                        l_res_ptr->m_parameter = PILOT_REFLECTION_NEW(SpotLightParameter)
                    }

                    if (isDirty)
                    {
                        if (!m_editor_component_stack.empty())
                        {
                            Component* m_component_ptr = m_editor_component_stack.back();
                            m_component_ptr->setDirtyFlag(isDirty);
                        }
                    }
                }

                auto object_instance = Reflection::ReflectionInstance(
                    MoYu::Reflection::TypeMeta::newMetaFromName(l_res_ptr->m_parameter.getTypeName().c_str()),
                    l_res_ptr->m_parameter.operator->());
                createComponentUI(object_instance);
            }
        };
        m_editor_ui_creator["MeshComponentRes"] = [this, &asset_folder](const std::string& name, void* value_ptr) -> void {
            if (g_editor_node_state_array[g_node_depth].second)
            {

                MeshComponentRes* mesh_res_ptr = static_cast<MeshComponentRes*>(value_ptr);

                std::string drag_file_path;
                
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Drag mesh file to here <_<");
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MESH_FILE_PATH"))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(std::string));
                        std::string payload_filepath = *(std::string*)payload->Data;
                        drag_file_path = payload_filepath;
                    }
                    ImGui::EndDragDropTarget();
                }

                for (size_t i = 0; i < mesh_res_ptr->m_sub_meshes.size(); i++)
                {
                    Reflection::TypeMeta field_meta = Reflection::TypeMeta::newMetaFromName("SubMeshRes");
                    //auto child_instance = Reflection::ReflectionInstance(field_meta, (void*)&mesh_res_ptr->m_sub_meshes[i]);
                    m_editor_ui_creator["TreeNodePush"](field_meta.getTypeName(), nullptr);

                    //createLeafNodeUI(child_instance);
                    m_editor_ui_creator[field_meta.getTypeName()](std::string("SubMeshRes[%d]", i), (void*)&mesh_res_ptr->m_sub_meshes[i]);

                    m_editor_ui_creator["TreeNodePop"](field_meta.getTypeName(), nullptr);
                }

                if (!drag_file_path.empty())
                {
                    std::shared_ptr<GObject> selected_object = g_editor_global_context.m_scene_manager->getSelectedGObject().lock();
                    MeshComponent* curSelectedMeshComponent = selected_object->tryGetComponent<MeshComponent>("MeshComponent");
                    if (curSelectedMeshComponent != nullptr)
                    {
                        std::string relative_path = "asset/" + drag_file_path;
                        curSelectedMeshComponent->addNewMeshRes(relative_path);
                        
                        if (!m_editor_component_stack.empty())
                        {
                            Component* m_component_ptr = m_editor_component_stack.back();
                            m_component_ptr->setDirtyFlag(true);
                        }
                    }
                }
            }
        };
        m_editor_ui_creator["SubMeshRes"] = [this, &asset_folder](const std::string& name, void* value_ptr) -> void {
            if (g_editor_node_state_array[g_node_depth].second)
            {
                SubMeshRes* sub_mesh_res_ptr = static_cast<SubMeshRes*>(value_ptr);
                if (sub_mesh_res_ptr != nullptr)
                {
                    m_editor_ui_creator["std::string"](std::string("mesh_file_path"), (void*)&sub_mesh_res_ptr->m_obj_file_ref);
                    m_editor_ui_creator["Transform"](std::string("sub_mesh_transform"), (void*)&sub_mesh_res_ptr->m_transform);

                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "MaterialRes : ");
                        //ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), sub_mesh_res_ptr->m_material.c_str());

                        std::string m_new_material_path;

                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_FILE_PATH"))
                            {
                                IM_ASSERT(payload->DataSize == sizeof(std::string));
                                std::string payload_filepath = *(std::string*)payload->Data;
                                std::string material_file_path = "asset/" + payload_filepath;

                                m_new_material_path = material_file_path;
                            }
                            ImGui::EndDragDropTarget();
                        }

                        std::shared_ptr<GObject> selected_object = g_editor_global_context.m_scene_manager->getSelectedGObject().lock();
                        MeshComponent* curSelectedMeshComponent = selected_object->tryGetComponent<MeshComponent>("MeshComponent");

                        if (!m_new_material_path.empty())
                        {
                            curSelectedMeshComponent->updateMaterial(sub_mesh_res_ptr->m_obj_file_ref, m_new_material_path);
                            g_runtime_global_context.m_material_manager->setMaterialDirty(m_new_material_path, true);
                        }
                        
                        bool isDefaultMat = sub_mesh_res_ptr->m_material == _default_gold_material_path;

                        m_editor_ui_creator["TreeNodePush"]("MaterialRes", nullptr);
                        {
                            if (isDefaultMat)
                                ImGui::BeginDisabled(true);

                            MaterialRes material_res = g_runtime_global_context.m_material_manager->loadMaterialRes(sub_mesh_res_ptr->m_material);
                            MaterialRes old_material_res = material_res;
                            m_editor_ui_creator["MaterialRes"]("MaterialRes", &material_res);
                            /*
                            Reflection::TypeMeta field_meta = Reflection::TypeMeta::newMetaFromName("MaterialRes");
                            auto material_instance = Reflection::ReflectionInstance(field_meta, (void*)&material_res);
                            createComponentUI(material_instance);
                            */
                            if (old_material_res != material_res)
                            {
                                g_runtime_global_context.m_material_manager->saveMaterialRes(sub_mesh_res_ptr->m_material, material_res);
                                g_runtime_global_context.m_material_manager->setMaterialDirty(sub_mesh_res_ptr->m_material);
                            }

                            if (isDefaultMat)
                                ImGui::EndDisabled();
                        }

                        m_editor_ui_creator["TreeNodePop"]("MaterialRes", nullptr);
                    }
                }
            }
        };
        m_editor_ui_creator["MaterialRes"] = [this, &asset_folder](const std::string& name, void* value_ptr) -> void {
            if (g_editor_node_state_array[g_node_depth].second)
            {
                MaterialRes* mat_res_ptr = static_cast<MaterialRes*>(value_ptr);

                ImGui::Checkbox("IsBlend", &mat_res_ptr->m_blend);
                ImGui::Checkbox("IsDoubleSide", &mat_res_ptr->m_double_sided);
                ImGui::ColorEdit4("BaseColorFactor", mat_res_ptr->m_base_color_factor.ptr());
                ImGui::DragFloat("MetallicFactor", &mat_res_ptr->m_metallic_factor, 0.02f, 0.0f, 1.0f);
                ImGui::DragFloat("RoughnessFactor", &mat_res_ptr->m_roughness_factor, 0.02f, 0.0f, 1.0f);
                ImGui::DragFloat("NormalScale", &mat_res_ptr->m_normal_scale, 0.02f, 0.0f, 1.0f);
                ImGui::DragFloat("OcclusionStrength", &mat_res_ptr->m_occlusion_strength, 0.02f, 0.0f, 1.0f);
                ImGui::DragFloat3("OcclusionStrength", mat_res_ptr->m_emissive_factor.ptr(), 0.02f, 0.0f, 1.0f);

                m_editor_ui_creator["TextureFilePath"]("BaseColorTextureFile", &mat_res_ptr->m_base_colour_texture_file);
                m_editor_ui_creator["TextureFilePath"]("MetallicRoughnessTextureFile", &mat_res_ptr->m_metallic_roughness_texture_file);
                m_editor_ui_creator["TextureFilePath"]("NormalTextureFile", &mat_res_ptr->m_normal_texture_file);
                m_editor_ui_creator["TextureFilePath"]("OcclusionTextureFile", &mat_res_ptr->m_occlusion_texture_file);
                m_editor_ui_creator["TextureFilePath"]("EmissiveTextureFile", &mat_res_ptr->m_emissive_texture_file);
            }
        };

        m_editor_ui_creator["TextureFilePath"] = [this, &asset_folder](const std::string& name, void* value_ptr) -> void {
            if (g_editor_node_state_array[g_node_depth].second)
            {
                std::string* texture_path = static_cast<std::string*>(value_ptr);

                //ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), name.c_str());
                //ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), texture_path->c_str());
                static char str1[128];
                memset(str1, 0, 128);
                memcpy(str1, texture_path->c_str(), texture_path->size());
                ImGui::InputText(name.c_str(), str1, IM_ARRAYSIZE(str1), ImGuiInputTextFlags_ReadOnly);
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_FILE_PATH"))
                    {
                        IM_ASSERT(payload->DataSize == sizeof(std::string));
                        std::string payload_filepath  = *(std::string*)payload->Data;
                        std::string texture_file_path = "asset/" + payload_filepath;

                        *texture_path = texture_file_path;
                    }
                    ImGui::EndDragDropTarget();
                }
            }
        };
    }

    std::string EditorUI::getLeafUINodeParentLabel()
    {
        std::string parent_label;
        int         array_size = g_editor_node_state_array.size();
        for (int index = 0; index < array_size; index++)
        {
            parent_label += g_editor_node_state_array[index].first + "::";
        }
        return parent_label;
    }

    void EditorUI::showEditorUI()
    {
        showEditorMenu(&m_editor_menu_window_open);
        showEditorWorldObjectsWindow(&m_asset_window_open);
        showEditorGameWindow(&m_game_engine_window_open);
        showEditorFileContentWindow(&m_file_content_window_open);
        showEditorDetailWindow(&m_detail_window_open);
    }

    void EditorUI::showEditorMenu(bool* p_open)
    {
        ImGuiDockNodeFlags dock_flags   = ImGuiDockNodeFlags_DockSpace;
        ImGuiWindowFlags   window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
                                        ImGuiConfigFlags_NoMouseCursorChange | ImGuiWindowFlags_NoBringToFrontOnFocus;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(main_viewport->WorkPos, ImGuiCond_Always);
        std::array<int, 2> window_size = g_editor_global_context.m_window_system->getWindowSize();
        ImGui::SetNextWindowSize(ImVec2((float)window_size[0], (float)window_size[1]), ImGuiCond_Always);

        ImGui::SetNextWindowViewport(main_viewport->ID);

        ImGui::Begin("Editor menu", p_open, window_flags);

        ImGuiID main_docking_id = ImGui::GetID("Main Docking");
        if (ImGui::DockBuilderGetNode(main_docking_id) == nullptr)
        {
            ImGui::DockBuilderRemoveNode(main_docking_id);

            ImGui::DockBuilderAddNode(main_docking_id, dock_flags);
            ImGui::DockBuilderSetNodePos(main_docking_id,
                                         ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y + 18.0f));
            ImGui::DockBuilderSetNodeSize(main_docking_id,
                                          ImVec2((float)window_size[0], (float)window_size[1] - 18.0f));

            ImGuiID center = main_docking_id;
            ImGuiID left;
            ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &left);

            ImGuiID left_other;
            ImGuiID left_file_content = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.30f, nullptr, &left_other);

            ImGuiID left_game_engine;
            ImGuiID left_asset =
                ImGui::DockBuilderSplitNode(left_other, ImGuiDir_Left, 0.30f, nullptr, &left_game_engine);

            ImGui::DockBuilderDockWindow("World Objects", left_asset);
            ImGui::DockBuilderDockWindow("Components Details", right);
            ImGui::DockBuilderDockWindow("File Content", left_file_content);
            ImGui::DockBuilderDockWindow("Game Engine", left_game_engine);

            ImGui::DockBuilderFinish(main_docking_id);
        }

        ImGui::DockSpace(main_docking_id);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Menu"))
            {
                if (ImGui::MenuItem("Reload Current Level"))
                {
                    g_runtime_global_context.m_world_manager->reloadCurrentLevel();
                    g_runtime_global_context.m_render_system->clearForLevelReloading();
                    g_editor_global_context.m_scene_manager->onGObjectSelected(k_invalid_gobject_id);
                }
                if (ImGui::MenuItem("Save Current Level"))
                {
                    g_runtime_global_context.m_world_manager->saveCurrentLevel();
                }
                if (ImGui::MenuItem("Exit"))
                {
                    g_editor_global_context.m_engine_runtime->shutdownEngine();
                    exit(0);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window"))
            {
                ImGui::MenuItem("World Objects", nullptr, &m_asset_window_open);
                ImGui::MenuItem("Game", nullptr, &m_game_engine_window_open);
                ImGui::MenuItem("File Content", nullptr, &m_file_content_window_open);
                ImGui::MenuItem("Detail", nullptr, &m_detail_window_open);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();
    }

    void EditorUI::showEditorWorldObjectsWindow(bool* p_open)
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (!*p_open)
            return;

        if (!ImGui::Begin("World Objects", p_open, window_flags))
        {
            ImGui::End();
            return;
        }

        std::shared_ptr<Level> current_active_level =
            g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
        if (current_active_level == nullptr)
        {
            ImGui::End();
            return;
        }

        /*
        const LevelObjectsMap& all_gobjects = current_active_level->getAllGObjects();
        for (auto& id_object_pair : all_gobjects)
        {
            const GObjectID          object_id = id_object_pair.first;
            std::shared_ptr<GObject> object    = id_object_pair.second;
            const std::string        name      = object->getName();
            if (name.size() > 0)
            {
                if (ImGui::Selectable(name.c_str(),
                                      g_editor_global_context.m_scene_manager->getSelectedObjectID() == object_id))
                {
                    if (g_editor_global_context.m_scene_manager->getSelectedObjectID() != object_id)
                    {
                        g_editor_global_context.m_scene_manager->onGObjectSelected(object_id);
                    }
                    else
                    {
                        g_editor_global_context.m_scene_manager->onGObjectSelected(k_invalid_gobject_id);
                    }
                    break;
                }
            }
        }
        */

        std::weak_ptr<GObject> root_node_weak_ptr = current_active_level->getRootNode();
        if (!root_node_weak_ptr.expired())
        {
            std::shared_ptr<GObject> root_node_shared_ptr = root_node_weak_ptr.lock();
            std::vector<std::weak_ptr<GObject>> root_children = root_node_shared_ptr->getChildren();
            for (size_t i = 0; i < root_children.size(); i++)
            {
                std::weak_ptr<GObject> root_child = root_children[i];
                showEditorWorldObjectsRecursive(root_child);
            }
        }

        if (ImGui::BeginPopupContextWindow())
        {
            GObjectID selectedId = g_editor_global_context.m_scene_manager->getSelectedObjectID();
            if (ImGui::MenuItem(" Create Node "))
            {
                GObjectID newParentID = selectedId == k_invalid_gobject_id ? K_Root_Object_Id : selectedId;
                current_active_level->createGObject("New Node", newParentID);
            }
            if (ImGui::MenuItem(" Delete Node "))
            {
                //current_active_level->deleteGObject(g_editor_global_context.m_scene_manager->getSelectedObjectID());
                g_editor_global_context.m_scene_manager->onDeleteSelectedGObject();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void EditorUI::showEditorWorldObjectsRecursive(std::weak_ptr<GObject> object_weak_ptr)
    {
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        std::shared_ptr<GObject> object    = object_weak_ptr.lock();
        const GObjectID          object_id = object->getID();
        const std::string        name      = object->getName();

        if (g_editor_global_context.m_scene_manager->getSelectedObjectID() == object_id)
            node_flags |= ImGuiTreeNodeFlags_Selected;

        auto chilren_nodes = object->getChildren();

        if (chilren_nodes.empty())
            node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)object_id, node_flags, name.c_str());

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            if (g_editor_global_context.m_scene_manager->getSelectedObjectID() != object_id)
            {
                g_editor_global_context.m_scene_manager->onGObjectSelected(object_id);
            }
            else
            {
                g_editor_global_context.m_scene_manager->onGObjectSelected(k_invalid_gobject_id);
            }
        }

        if (node_open)
        {
            for (size_t i = 0; i < chilren_nodes.size(); i++)
            {
                showEditorWorldObjectsRecursive(chilren_nodes[i]);
            }
            if (!chilren_nodes.empty())
                ImGui::TreePop();
        }

    }

    void EditorUI::createComponentUI(Reflection::ReflectionInstance& instance)
    {
        Reflection::ReflectionInstance* reflection_instance;
        int count = instance.m_meta.getBaseClassReflectionInstanceList(reflection_instance, instance.m_instance);
        for (int index = 0; index < count; index++)
        {
            createComponentUI(reflection_instance[index]);
        }
        createLeafNodeUI(instance);

        if (count > 0)
            delete[] reflection_instance;
    }

    void EditorUI::createLeafNodeUI(Reflection::ReflectionInstance& instance)
    {
        Reflection::FieldAccessor* fields;
        int                        fields_number = instance.m_meta.getFieldsList(fields);

        for (size_t index = 0; index < fields_number; index++)
        {
            auto fields_count = fields[index];
            if (fields_count.isArrayType())
            {

                Reflection::ArrayAccessor array_accessor;
                if (Reflection::TypeMeta::newArrayAccessorFromName(fields_count.getFieldTypeName(), array_accessor))
                {
                    void* field_instance = fields_count.get(instance.m_instance);
                    int   array_count    = array_accessor.getSize(field_instance);
                    m_editor_ui_creator["TreeNodePush"](
                        std::string(fields_count.getFieldName()) + "[" + std::to_string(array_count) + "]", nullptr);
                    auto item_type_meta_item =
                        Reflection::TypeMeta::newMetaFromName(array_accessor.getElementTypeName());
                    auto item_ui_creator_iterator = m_editor_ui_creator.find(item_type_meta_item.getTypeName());
                    for (int index = 0; index < array_count; index++)
                    {
                        if (item_ui_creator_iterator == m_editor_ui_creator.end())
                        {
                            m_editor_ui_creator["TreeNodePush"]("[" + std::to_string(index) + "]", nullptr);
                            auto object_instance = Reflection::ReflectionInstance(
                                MoYu::Reflection::TypeMeta::newMetaFromName(item_type_meta_item.getTypeName().c_str()),
                                array_accessor.get(index, field_instance));
                            createComponentUI(object_instance);
                            m_editor_ui_creator["TreeNodePop"]("[" + std::to_string(index) + "]", nullptr);
                        }
                        else
                        {
                            if (item_ui_creator_iterator == m_editor_ui_creator.end())
                            {
                                continue;
                            }
                            m_editor_ui_creator[item_type_meta_item.getTypeName()](
                                "[" + std::to_string(index) + "]", array_accessor.get(index, field_instance));
                        }
                    }
                    m_editor_ui_creator["TreeNodePop"](fields_count.getFieldName(), nullptr);
                }
            }
            auto ui_creator_iterator = m_editor_ui_creator.find(fields_count.getFieldTypeName());
            if (ui_creator_iterator == m_editor_ui_creator.end())
            {
                Reflection::TypeMeta field_meta =
                    Reflection::TypeMeta::newMetaFromName(fields_count.getFieldTypeName());
                if (fields_count.getTypeMeta(field_meta))
                {
                    auto child_instance =
                        Reflection::ReflectionInstance(field_meta, fields_count.get(instance.m_instance));
                    m_editor_ui_creator["TreeNodePush"](field_meta.getTypeName(), nullptr);
                    createLeafNodeUI(child_instance);
                    m_editor_ui_creator["TreeNodePop"](field_meta.getTypeName(), nullptr);
                }
                else
                {
                    if (ui_creator_iterator == m_editor_ui_creator.end())
                    {
                        continue;
                    }
                    m_editor_ui_creator[fields_count.getFieldTypeName()](fields_count.getFieldName(),
                                                                         fields_count.get(instance.m_instance));
                }
            }
            else
            {
                m_editor_ui_creator[fields_count.getFieldTypeName()](fields_count.getFieldName(),
                                                                     fields_count.get(instance.m_instance));
            }
        }
        delete[] fields;
    }

    void EditorUI::showEditorDetailWindow(bool* p_open)
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (!*p_open)
            return;

        if (!ImGui::Begin("Components Details", p_open, window_flags))
        {
            ImGui::End();
            return;
        }

        std::shared_ptr<GObject> selected_object = g_editor_global_context.m_scene_manager->getSelectedGObject().lock();
        if (selected_object == nullptr)
        {
            ImGui::End();
            return;
        }

        const std::string& name = selected_object->getName();
        static char        cname[128];
        memset(cname, 0, 128);
        memcpy(cname, name.c_str(), name.size());

        ImGui::Text("Name");
        ImGui::SameLine();
        if (ImGui::InputText("##Name", cname, IM_ARRAYSIZE(cname), ImGuiInputTextFlags_EnterReturnsTrue))
            selected_object->setName(std::string(cname));

        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings;
        auto&& selected_object_components = selected_object->getComponents();
        for (auto component_ptr : selected_object_components)
        {
            m_editor_ui_creator["TreeNodePush"](("<" + component_ptr.getTypeName() + ">").c_str(), component_ptr.getPtr());
            auto object_instance = Reflection::ReflectionInstance(
                MoYu::Reflection::TypeMeta::newMetaFromName(component_ptr.getTypeName().c_str()),
                component_ptr.operator->());
            createComponentUI(object_instance);
            m_editor_ui_creator["TreeNodePop"](("<" + component_ptr.getTypeName() + ">").c_str(), component_ptr.getPtr());
        }

        ImGui::NewLine();

        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponentPopup");

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            if (ImGui::MenuItem("Camera Component"))
            {
                if (selected_object->tryGetComponentConst<const CameraComponent>("CameraComponent"))
                {
                    LOG_INFO("object {} already has Camera Component", selected_object->getName());
                }
                else
                {
                    auto camera_component = PILOT_REFLECTION_NEW(CameraComponent);
                    camera_component->reset();
                    camera_component->postLoadResource(selected_object);
                    selected_object->tryAddComponent(camera_component);

                    LOG_INFO("Add New Camera Component");
                }
            }

            if (ImGui::MenuItem("Light Component"))
            {
                if (selected_object->tryGetComponentConst<const LightComponent>("LightComponent"))
                {
                    LOG_INFO("object {} already has Light Component", selected_object->getName());
                }
                else
                {
                    auto light_component = PILOT_REFLECTION_NEW(LightComponent);
                    light_component->reset();
                    light_component->postLoadResource(selected_object);
                    selected_object->tryAddComponent(light_component);

                    LOG_INFO("Add New Light Component");
                }
            }

            if (ImGui::MenuItem("Mesh Component"))
            {
                if (selected_object->tryGetComponentConst<const MeshComponent>("MeshComponent"))
                {
                    LOG_INFO("object {} already has Mesh Component", selected_object->getName());
                }
                else
                {
                    auto mesh_component = PILOT_REFLECTION_NEW(MeshComponent);
                    mesh_component->reset();
                    mesh_component->postLoadResource(selected_object);
                    selected_object->tryAddComponent(mesh_component);

                    LOG_INFO("Add New Mesh Component");
                }
            }


            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void EditorUI::showEditorFileContentWindow(bool* p_open)
    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (!*p_open)
            return;

        if (!ImGui::Begin("File Content", p_open, window_flags))
        {
            ImGui::End();
            return;
        }

        static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                                       ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("File Content", 2, flags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            auto current_time = std::chrono::steady_clock::now();
            if (current_time - m_last_file_tree_update > std::chrono::seconds(1))
            {
                m_editor_file_service.buildEngineFileTree();
                m_last_file_tree_update = current_time;
            }
            m_last_file_tree_update = current_time;

            std::shared_ptr<EditorFileNode> editor_root_node = m_editor_file_service.getEditorRootNode();
            buildEditorFileAssetsUITree(editor_root_node);
            ImGui::EndTable();
        }

        // file image list

        ImGui::End();
    }

    void EditorUI::showEditorGameWindow(bool* p_open)
    {
        ImGuiIO&         io           = ImGui::GetIO();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (!*p_open)
            return;

        if (!ImGui::Begin("Game Engine", p_open, window_flags))
        {
            ImGui::End();
            return;
        }

        static bool trans_button_ckecked  = false;
        static bool rotate_button_ckecked = false;
        static bool scale_button_ckecked  = false;

        switch (g_editor_global_context.m_scene_manager->getEditorAxisMode())
        {
            case EditorAxisMode::TranslateMode:
                trans_button_ckecked  = true;
                rotate_button_ckecked = false;
                scale_button_ckecked  = false;
                break;
            case EditorAxisMode::RotateMode:
                trans_button_ckecked  = false;
                rotate_button_ckecked = true;
                scale_button_ckecked  = false;
                break;
            case EditorAxisMode::ScaleMode:
                trans_button_ckecked  = false;
                rotate_button_ckecked = false;
                scale_button_ckecked  = true;
                break;
            default:
                break;
        }

        if (ImGui::BeginMenuBar())
        {
            ImGui::Indent(10.f);
            drawAxisToggleButton("Trans", trans_button_ckecked, (int)EditorAxisMode::TranslateMode);
            ImGui::Unindent();

            ImGui::SameLine();

            drawAxisToggleButton("Rotate", rotate_button_ckecked, (int)EditorAxisMode::RotateMode);

            ImGui::SameLine();

            drawAxisToggleButton("Scale", scale_button_ckecked, (int)EditorAxisMode::ScaleMode);

            ImGui::SameLine();

            float indent_val = 0.0f;

            float x_scale, y_scale;
            glfwGetWindowContentScale(g_editor_global_context.m_window_system->getWindow(), &x_scale, &y_scale);
            float indent_scale = fmaxf(1.0f, fmaxf(x_scale, y_scale));

            //indent_val = g_editor_global_context.m_input_manager->getEngineWindowSize().x - 200.0f * indent_scale;
            indent_val = ImGui::GetContentRegionMax().x - 150.0f * indent_scale;

            ImGui::Indent(indent_val);
            if (g_is_editor_mode)
            {
                ImGui::PushID("Editor Mode");
                if (ImGui::Button("Editor Mode"))
                {
                    g_is_editor_mode = false;
                    g_editor_global_context.m_scene_manager->drawSelectedEntityAxis();
                    g_editor_global_context.m_input_manager->resetEditorCommand();
                    g_editor_global_context.m_window_system->setFocusMode(true);
                }
                ImGui::PopID();
            }
            else
            {
                if (ImGui::Button("Game Mode"))
                {
                    g_is_editor_mode = true;
                    g_editor_global_context.m_scene_manager->drawSelectedEntityAxis();
                    g_runtime_global_context.m_input_system->resetGameCommand();
                    g_editor_global_context.m_render_system->getRenderCamera()->setMainViewMatrix(
                        g_editor_global_context.m_scene_manager->getEditorCamera()->getViewMatrix());
                }
            }

            ImGui::Unindent();
            ImGui::EndMenuBar();
        }

        auto menu_bar_rect = ImGui::GetCurrentWindow()->MenuBarRect();

        Vector2 new_window_pos  = {0.0f, 0.0f};
        Vector2 new_window_size = {0.0f, 0.0f};
        new_window_pos.x        = ImGui::GetWindowPos().x;
        new_window_pos.y        = ImGui::GetWindowPos().y + menu_bar_rect.Min.y + 20;
        new_window_size.x       = ImGui::GetWindowSize().x;
        new_window_size.y       = ImGui::GetWindowSize().y - menu_bar_rect.Min.y - 20;

        if (!g_is_editor_mode)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.8f), "Press Left Alt key to display the mouse cursor!");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 0.8f), "Current editor camera move speed: [%f]", g_editor_global_context.m_input_manager->getCameraSpeed());
        }

        // if (new_window_pos != m_engine_window_pos || new_window_size != m_engine_window_size)
        {
            // g_runtime_global_context.m_render_system->updateEngineContentViewport(
            //     new_window_pos.x, new_window_pos.y, new_window_size.x, new_window_size.y);
            //g_runtime_global_context.m_render_system->updateEngineContentViewport(
            //    0, 0, new_window_size.x, new_window_size.y);

            float displayWidth, displayHeight;

            float windowAspect     = new_window_size.x / new_window_size.y;
            float backbufferAspect = handleWidth / handleHeight;

            if (backbufferAspect > windowAspect)
            {
                displayWidth = new_window_size.x;
                displayHeight = new_window_size.x / backbufferAspect;
            }
            else
            {
                displayHeight = new_window_size.y;
                displayWidth  = new_window_size.y * backbufferAspect;
            }

            //g_editor_global_context.m_input_manager->setEngineWindowPos(new_window_pos);
            //g_editor_global_context.m_input_manager->setEngineWindowSize(new_window_size);

            Vector2 cursor_offset = Vector2((new_window_size.x - displayWidth) * 0.5f, (new_window_size.y - displayHeight) * 0.5f);

            ImVec2 cilld_cur_pos = ImVec2(cursor_offset.x + new_window_pos.x, cursor_offset.y + new_window_pos.y);

            g_editor_global_context.m_input_manager->setEngineWindowPos(Vector2(cilld_cur_pos.x, cilld_cur_pos.y));
            g_editor_global_context.m_input_manager->setEngineWindowSize(Vector2(displayWidth, displayHeight));

            ImGui::SetCursorPosX(cursor_offset.x);
            ImGui::BeginChild("GameView", ImVec2(displayWidth, displayHeight), true, ImGuiWindowFlags_NoDocking);
            {
                ImGuizmo::SetDrawlist();

                ImVec2 window_pos = ImGui::GetWindowPos();

                ImGuizmo::SetRect(window_pos.x, window_pos.y, displayWidth, displayHeight);

                Matrix4x4 viewMatrix  = g_editor_global_context.m_scene_manager->getEditorCamera()->getViewMatrix();
                glm::mat4 _cameraView = GLMUtil::fromMat4x4(viewMatrix);
                Matrix4x4 projMatrix  = g_editor_global_context.m_scene_manager->getEditorCamera()->getPersProjMatrix();
                glm::mat4 _projMatrix = GLMUtil::fromMat4x4(projMatrix);
                glm::mat4 _identiyMatrix = GLMUtil::fromMat4x4(Matrix4x4::Identity);

                ImGuizmo::DrawGrid((const float*)&_cameraView, (const float*)&_projMatrix, (const float*)&_identiyMatrix, 100.f);

                // draw the real rendering image
                {
                    ImGui::Image((ImTextureID)handleOfGameView.ptr, ImVec2(displayWidth, displayHeight));
                }

                std::shared_ptr<GObject> selected_object =
                    g_editor_global_context.m_scene_manager->getSelectedGObject().lock();
                if (selected_object != nullptr)
                {
                    TransformComponent* trans_component_ptr = selected_object->getTransformComponent();
                    Matrix4x4 worldMatrix = trans_component_ptr->getMatrixWorld();
                    glm::mat4 _worldMatrix = GLMUtil::fromMat4x4(worldMatrix);

                    ImGuizmo::OPERATION op_type = ImGuizmo::OPERATION::TRANSLATE;
                    if (trans_button_ckecked)
                    {
                        op_type = ImGuizmo::OPERATION::TRANSLATE;
                    }
                    else if (rotate_button_ckecked)
                    {
                        op_type = ImGuizmo::OPERATION::ROTATE;
                    }
                    else if (scale_button_ckecked)
                    {
                        op_type = ImGuizmo::OPERATION::SCALE;
                    }
                    ImGuizmo::Manipulate((float*)&_cameraView, (const float*)&_projMatrix, op_type, ImGuizmo::MODE::LOCAL, (float*)&_worldMatrix);
                }
            }
            ImGui::EndChild();
        }

        ImGui::End();
    }

    void EditorUI::drawAxisToggleButton(const char* string_id, bool check_state, int axis_mode)
    {
        if (check_state)
        {
            ImGui::PushID(string_id);
            ImVec4 check_button_color = ImVec4(93.0f / 255.0f, 10.0f / 255.0f, 66.0f / 255.0f, 1.00f);
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(check_button_color.x, check_button_color.y, check_button_color.z, 0.40f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, check_button_color);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, check_button_color);
            ImGui::Button(string_id);
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
        else
        {
            if (ImGui::Button(string_id))
            {
                check_state = true;
                g_editor_global_context.m_scene_manager->setEditorAxisMode((EditorAxisMode)axis_mode);
                g_editor_global_context.m_scene_manager->drawSelectedEntityAxis();
            }
        }
    }

    void EditorUI::fileDragHelper(std::shared_ptr<EditorFileNode> node)
    {
        if (strcmp(node->m_file_type.c_str(), "obj") == 0)
        {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("MESH_FILE_PATH", &node->m_relative_path, sizeof(std::string));
                ImGui::Text("Drag Mesh %s", node->m_file_name.c_str());
                ImGui::EndDragDropSource();
            }
        }
        if (node->m_file_type == "jpg" || node->m_file_type == "png" || node->m_file_type == "tga")
        {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("TEXTURE_FILE_PATH", &node->m_relative_path, sizeof(std::string));
                ImGui::Text("Drag Texture %s", node->m_file_name.c_str());
                ImGui::EndDragDropSource();
            }
        }
        if (node->m_file_name.find(".material") != std::string::npos)
        {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("MATERIAL_FILE_PATH", &node->m_relative_path, sizeof(std::string));
                ImGui::Text("Drag Material %s", node->m_file_name.c_str());
                ImGui::EndDragDropSource();
            }
        }
        if (node->m_file_name.find(".material") != std::string::npos)
        {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("MATERIAL_FILE_PATH", &node->m_relative_path, sizeof(std::string));
                ImGui::Text("Drag Material %s", node->m_file_name.c_str());
                ImGui::EndDragDropSource();
            }
        }
        if (strcmp(node->m_file_type.c_str(), "Folder") == 0)
        {
            std::shared_ptr<EditorFileNode> cur_sel_file_node = m_editor_file_service.getSelectedEditorNode();

            bool is_cur_node = cur_sel_file_node != nullptr && cur_sel_file_node == node;

            if (is_cur_node)
            {
                bool should_create_material = false;

                if (ImGui::BeginPopupContextWindow())
                {
                    if (ImGui::MenuItem(" Create Material "))
                    {
                        should_create_material = true;
                    }
                    ImGui::EndPopup();
                }

                if (should_create_material)
                {
                    if (!ImGui::IsPopupOpen("CreateNewMaterial?"))
                    {
                        ImGui::OpenPopup("CreateNewMaterial?");
                    }
                }

                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |
                                         ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize;
                if (ImGui::BeginPopupModal("CreateNewMaterial?", NULL, flags))
                {
                    static char cname[128] = "temp.material.json";
                    ImGui::InputText("##Name", cname, IM_ARRAYSIZE(cname));
                    if (ImGui::Button("Save"))
                    {
                        // create default gold material
                        std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
                        ASSERT(asset_manager);

                        MaterialRes default_material_res = {};
                        default_material_res.m_base_colour_texture_file = _default_color_texture_path;
                        default_material_res.m_metallic_roughness_texture_file = _default_metallic_roughness_texture_path;
                        default_material_res.m_normal_texture_file = _default_normal_texture_path;

                        std::string material_path = "asset/" + node->m_relative_path + "/" + std::string(cname);
                        asset_manager->saveAsset(default_material_res, material_path);

                        m_last_file_tree_update = std::chrono::time_point<std::chrono::steady_clock>();

                        memset(cname, 0, 128);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Close"))
                    {
                        memset(cname, 0, 128);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
        }
    }

    void EditorUI::buildEditorFileAssetsUITree(std::shared_ptr<EditorFileNode> node)
    {
        ImGuiTreeNodeFlags node_flags =
            ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

        std::shared_ptr<EditorFileNode> cur_sel_file_node = m_editor_file_service.getSelectedEditorNode();

        bool is_cur_node = cur_sel_file_node != nullptr && cur_sel_file_node == node;

        if (is_cur_node)
        {
            node_flags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        const bool is_folder = (node->m_child_nodes.size() > 0);
        if (!is_folder)
            node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        bool open = ImGui::TreeNodeEx(node->m_file_name.c_str(), node_flags);

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            if (!is_cur_node)
            {
                m_editor_file_service.setSelectedEditorNode(node);
            }
            else
            {
                m_editor_file_service.setSelectedEditorNode(nullptr);
            }
        }

        fileDragHelper(node);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::TextUnformatted(node->m_file_type.c_str());

        if (is_folder && open)
        {
            for (int child_n = 0; child_n < node->m_child_nodes.size(); child_n++)
                buildEditorFileAssetsUITree(node->m_child_nodes[child_n]);
            ImGui::TreePop();
        }


    }

    /*
    void EditorUI::onFileContentItemClicked(EditorFileNode* node)
    {
        if (node->m_file_type != "object")
            return;

        std::shared_ptr<Level> level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
        if (level == nullptr)
            return;

        const unsigned int new_object_index = ++m_new_object_index_map[node->m_file_name];

        ObjectInstanceRes new_object_instance_res;
        new_object_instance_res.m_name =
            "New_" + Path::getFilePureName(node->m_file_name) + "_" + std::to_string(new_object_index);
        new_object_instance_res.m_definition =
            g_runtime_global_context.m_asset_manager->getFullPath(node->m_file_path).generic_string();

        std::weak_ptr<GObject>   new_gobject_weak   = level->instantiateObject(new_object_instance_res);
        std::shared_ptr<GObject> new_gobject_shared = new_gobject_weak.lock();
        size_t                   new_gobject_id     = new_gobject_shared->getID();

        if (new_gobject_id != k_invalid_gobject_id)
        {
            g_editor_global_context.m_scene_manager->onGObjectSelected(new_gobject_id);
        }
    }
    */

    inline void windowContentScaleUpdate(float scale)
    {

        // TOOD: Reload fonts if DPI scale is larger than previous font loading DPI scale
    }

    inline void windowContentScaleCallback(GLFWwindow* window, float x_scale, float y_scale)
    {
        windowContentScaleUpdate(fmaxf(x_scale, y_scale));
    }

    void EditorUI::initialize(WindowUIInitInfo init_info)
    {
        std::shared_ptr<ConfigManager> config_manager = g_runtime_global_context.m_config_manager;
        ASSERT(config_manager);

        // create imgui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // set ui content scale
        float x_scale, y_scale;
        glfwGetWindowContentScale(init_info.window_system->getWindow(), &x_scale, &y_scale);
        float content_scale = fmaxf(1.0f, fmaxf(x_scale, y_scale));
        windowContentScaleUpdate(content_scale);
        glfwSetWindowContentScaleCallback(init_info.window_system->getWindow(), windowContentScaleCallback);

        // load font for imgui
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigDockingAlwaysTabBar         = true;
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.Fonts->AddFontFromFileTTF(
            config_manager->getEditorFontPath().generic_string().data(), content_scale * 16, nullptr, nullptr);
        io.Fonts->Build();

        ImGuiStyle& style     = ImGui::GetStyle();
        style.WindowPadding   = ImVec2(1.0, 0);
        style.FramePadding    = ImVec2(14.0, 2.0f);
        style.ChildBorderSize = 0.0f;
        style.FrameRounding   = 5.0f;
        style.FrameBorderSize = 1.5f;

        // set imgui color style
        //setUIColorStyle();
        ImGui::StyleColorsDark();

        // setup window icon
        GLFWimage   window_icon[2];
        std::string big_icon_path_string   = config_manager->getEditorBigIconPath().generic_string();
        std::string small_icon_path_string = config_manager->getEditorSmallIconPath().generic_string();
        window_icon[0].pixels =
            stbi_load(big_icon_path_string.data(), &window_icon[0].width, &window_icon[0].height, 0, 4);
        window_icon[1].pixels =
            stbi_load(small_icon_path_string.data(), &window_icon[1].width, &window_icon[1].height, 0, 4);
        glfwSetWindowIcon(init_info.window_system->getWindow(), 2, window_icon);
        stbi_image_free(window_icon[0].pixels);
        stbi_image_free(window_icon[1].pixels);

        // initialize imgui vulkan render backend
        init_info.render_system->initializeUIRenderBackend(this);
    }

    void EditorUI::setUIColorStyle()
    {
        ImGuiStyle* style  = &ImGui::GetStyle();
        ImVec4*     colors = style->Colors;

        colors[ImGuiCol_Text]                  = ImVec4(0.4745f, 0.4745f, 0.4745f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.0078f, 0.0078f, 0.0078f, 1.00f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border]                = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.047f, 0.047f, 0.047f, 0.5411f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.196f, 0.196f, 0.196f, 0.40f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.294f, 0.294f, 0.294f, 0.67f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.0039f, 0.0039f, 0.0039f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.0039f, 0.0039f, 0.0039f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]             = ImVec4(93.0f / 255.0f, 10.0f / 255.0f, 66.0f / 255.0f, 1.00f);
        colors[ImGuiCol_SliderGrab]            = colors[ImGuiCol_CheckMark];
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.3647f, 0.0392f, 0.2588f, 0.50f);
        colors[ImGuiCol_Button]                = ImVec4(0.0117f, 0.0117f, 0.0117f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.0235f, 0.0235f, 0.0235f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.0353f, 0.0196f, 0.0235f, 1.00f);
        colors[ImGuiCol_Header]                = ImVec4(0.1137f, 0.0235f, 0.0745f, 0.588f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(5.0f / 255.0f, 5.0f / 255.0f, 5.0f / 255.0f, 1.00f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
        colors[ImGuiCol_Separator]             = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);
        colors[ImGuiCol_SeparatorHovered]      = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 1.00f);
        colors[ImGuiCol_SeparatorActive]       = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 1.00f);
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab]                   = ImVec4(6.0f / 255.0f, 6.0f / 255.0f, 8.0f / 255.0f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 150.0f / 255.0f);
        colors[ImGuiCol_TabActive]             = ImVec4(47.0f / 255.0f, 6.0f / 255.0f, 29.0f / 255.0f, 1.0f);
        colors[ImGuiCol_TabUnfocused]          = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 25.0f / 255.0f);
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(6.0f / 255.0f, 6.0f / 255.0f, 8.0f / 255.0f, 200.0f / 255.0f);
        colors[ImGuiCol_DockingPreview]        = ImVec4(47.0f / 255.0f, 6.0f / 255.0f, 29.0f / 255.0f, 0.7f);
        colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.20f, 0.20f, 0.20f, 0.00f);
        colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]     = ImVec4(2.0f / 255.0f, 2.0f / 255.0f, 2.0f / 255.0f, 1.0f);
        colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    void EditorUI::preRender()
    {
        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();

        showEditorUI();
        
        // Rendering
        ImGui::EndFrame();
        ImGui::Render();
    }

    void EditorUI::setGameView(D3D12_GPU_DESCRIPTOR_HANDLE handle, uint32_t width, uint32_t height)
    {
        handleWidth      = width;
        handleHeight     = height;
        handleOfGameView = handle;
    }

    bool DrawVecControl(const std::string& label, MoYu::Vector3& values, float resetValue, float columnWidth)
    {
        Vector3 prevVal = values;

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2 {0, 0});

        float  lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.8f, 0.1f, 0.15f, 1.0f});
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.2f, 0.45f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.3f, 0.55f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.2f, 0.45f, 0.2f, 1.0f});
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.1f, 0.25f, 0.8f, 1.0f});
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);
        ImGui::PopID();

        if (prevVal != values)
            return true;
        return false;
    }

    bool DrawVecControl(const std::string& label, MoYu::Quaternion& values, float resetValue, float columnWidth)
    {
        Quaternion prevVal = values;

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2 {0, 0});

        float  lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.8f, 0.1f, 0.15f, 1.0f});
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.2f, 0.45f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.3f, 0.55f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.2f, 0.45f, 0.2f, 1.0f});
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.1f, 0.25f, 0.8f, 1.0f});
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.5f, 0.25f, 0.5f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.6f, 0.35f, 0.6f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.5f, 0.25f, 0.5f, 1.0f});
        if (ImGui::Button("W", buttonSize))
            values.w = resetValue;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##W", &values.w, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);
        ImGui::PopID();

        if (prevVal != values)
            return true;
        return false;
    }
} // namespace MoYu
