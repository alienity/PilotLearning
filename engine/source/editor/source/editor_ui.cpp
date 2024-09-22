#include "editor/include/editor_ui.h"

#include "editor/include/editor_global_context.h"
#include "editor/include/editor_input_manager.h"
#include "editor/include/editor_scene_manager.h"

#include "runtime/core/base/macro.h"

#include "runtime/platform/path/path.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/engine.h"

#include "runtime/function/framework/component/mesh/mesh_renderer_component.h"
#include "runtime/function/framework/component/terrain/terrain_component.h"
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

#include "runtime/core/math/moyu_math2.h"

#include "imgui.h"
#include "imgui_widgets.cpp"
#include "imgui_internal.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_glfw.h"

#include "stb_image.h"

#include "ImGuizmo.h"

namespace MoYu
{
    bool DrawVecControl(const std::string& label, glm::float3& values, float resetValue = 0.0f, float columnWidth = 100.0f);
    bool DrawVecControl(const std::string& label, glm::quat& values, float resetValue = 0.0f, float columnWidth = 100.0f);

    EditorUI::EditorUI()
    {
        const auto& asset_folder = g_runtime_global_context.m_config_manager->getAssetFolder();

        m_editor_ui_creator["Transform"] = [this](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            Transform* trans_ptr = static_cast<Transform*>(value_ptr);

            glm::vec3 euler;
            glm::mat4 _quat = glm::mat4_cast(trans_ptr->m_rotation);
            glm::extractEulerAngleYXZ(_quat, euler.y, euler.x, euler.z);

            glm::float3 degrees_val = {};
            degrees_val.x = MoYu::radiansToDegrees(euler.x);
            degrees_val.y = MoYu::radiansToDegrees(euler.y);
            degrees_val.z = MoYu::radiansToDegrees(euler.z);

            bool isDirty = false;

            isDirty |= DrawVecControl("Position", trans_ptr->m_position);
            isDirty |= DrawVecControl("Rotation", degrees_val);
            isDirty |= DrawVecControl("Scale", trans_ptr->m_scale);

            glm::float3 newEuler = {};
            newEuler.x = MoYu::degreesToRadians(degrees_val.x);
            newEuler.y = MoYu::degreesToRadians(degrees_val.y);
            newEuler.z = MoYu::degreesToRadians(degrees_val.z);

            trans_ptr->m_rotation = glm::eulerAngleYXZ(newEuler.y, newEuler.x, newEuler.z);

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["int"] = [this](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            std::string label = "##" + name;
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            isDirty = ImGui::InputInt(label.c_str(), static_cast<int*>(value_ptr));

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["float"] = [this](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            std::string label = "##" + name;
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            isDirty = ImGui::InputFloat(label.c_str(), static_cast<float*>(value_ptr));

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["bool"] = [this](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            std::string label = "##" + name;
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            isDirty = ImGui::Checkbox(label.c_str(), static_cast<bool*>(value_ptr));

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["glm::float2"] = [this](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            glm::float2* vec_ptr = static_cast<glm::float2*>(value_ptr);
            float val[2] = {vec_ptr->x, vec_ptr->y};
            
            std::string label   = "##" + name;
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            isDirty    = ImGui::DragFloat2(label.c_str(), val);

            vec_ptr->x = val[0];
            vec_ptr->y = val[1];

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["glm::int2"] = [this](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            glm::int2* vec_ptr = static_cast<glm::int2*>(value_ptr);
            int val[2]  = {vec_ptr->x, vec_ptr->y};

            std::string label = "##" + name;
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            isDirty = ImGui::DragInt2(label.c_str(), val);

            vec_ptr->x = val[0];
            vec_ptr->y = val[1];

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["glm::float3"] = [this](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            glm::float3* vec_ptr = static_cast<glm::float3*>(value_ptr);
            float    val[3]  = {vec_ptr->x, vec_ptr->y, vec_ptr->z};

            std::string label = "##" + name;
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            isDirty = ImGui::DragFloat3(label.c_str(), val);

            vec_ptr->x = val[0];
            vec_ptr->y = val[1];
            vec_ptr->z = val[2];

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["glm::float4"] = [this](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;
            
            glm::float4* vec_ptr = static_cast<glm::float4*>(value_ptr);
            float val[4] = {vec_ptr->x, vec_ptr->y, vec_ptr->z, vec_ptr->w};
            
            std::string label = "##" + name;
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            isDirty = ImGui::DragFloat4(label.c_str(), val);

            vec_ptr->x = val[0];
            vec_ptr->y = val[1];
            vec_ptr->z = val[2];
            vec_ptr->w = val[3];

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["Quaternion"] = [this](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            glm::quat* qua_ptr = static_cast<glm::quat*>(value_ptr);
            float val[4] = {qua_ptr->x, qua_ptr->y, qua_ptr->z, qua_ptr->w};

            std::string label = "##" + name;
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            isDirty = ImGui::DragFloat4(label.c_str(), val);

            qua_ptr->x = val[0];
            qua_ptr->y = val[1];
            qua_ptr->z = val[2];
            qua_ptr->w = val[3];

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["Color"] = [this](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            Color* color_ptr = static_cast<Color*>(value_ptr);
            float  col[4]   = {color_ptr->r, color_ptr->g, color_ptr->b, color_ptr->a};

            is_dirty |= ImGui::ColorEdit4("Color", col);

            color_ptr->r = col[0];
            color_ptr->g = col[1];
            color_ptr->b = col[2];
            color_ptr->a = col[3];
        };
        m_editor_ui_creator["FirstPersonCameraParameter"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            FirstPersonCameraParameter* cam_ptr = static_cast<FirstPersonCameraParameter*>(value_ptr);

            m_editor_ui_creator["float"]("m_fovY", isDirty, &cam_ptr->m_fovY);
            m_editor_ui_creator["float"]("m_vertical_offset", isDirty, &cam_ptr->m_vertical_offset);
            m_editor_ui_creator["int"]("m_width", isDirty, &cam_ptr->m_width);
            m_editor_ui_creator["int"]("m_height", isDirty, &cam_ptr->m_height);
            m_editor_ui_creator["float"]("m_nearZ", isDirty, &cam_ptr->m_nearZ);
            m_editor_ui_creator["float"]("m_farZ", isDirty, &cam_ptr->m_farZ);

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["ThirdPersonCameraParameter"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            ThirdPersonCameraParameter* cam_ptr = static_cast<ThirdPersonCameraParameter*>(value_ptr);

            m_editor_ui_creator["float"]("m_fovY", isDirty, &cam_ptr->m_fovY);
            m_editor_ui_creator["float"]("m_horizontal_offset", isDirty, &cam_ptr->m_horizontal_offset);
            m_editor_ui_creator["float"]("m_vertical_offset", isDirty, &cam_ptr->m_vertical_offset);
            m_editor_ui_creator["Quaternion"]("m_cursor_pitch", isDirty, &cam_ptr->m_cursor_pitch);
            m_editor_ui_creator["Quaternion"]("m_cursor_yaw", isDirty, &cam_ptr->m_cursor_yaw);
            m_editor_ui_creator["int"]("m_width", isDirty, &cam_ptr->m_width);
            m_editor_ui_creator["int"]("m_height", isDirty, &cam_ptr->m_height);
            m_editor_ui_creator["float"]("m_nearZ", isDirty, &cam_ptr->m_nearZ);
            m_editor_ui_creator["float"]("m_farZ", isDirty, &cam_ptr->m_farZ);

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["FreeCameraParameter"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            FreeCameraParameter* cam_ptr = static_cast<FreeCameraParameter*>(value_ptr);

            m_editor_ui_creator["bool"]("m_perspective", isDirty, &cam_ptr->m_perspective);
            m_editor_ui_creator["float"]("m_fovY", isDirty, &cam_ptr->m_fovY);
            m_editor_ui_creator["float"]("m_speed", isDirty, &cam_ptr->m_speed);
            m_editor_ui_creator["int"]("m_width", isDirty, &cam_ptr->m_width);
            m_editor_ui_creator["int"]("m_height", isDirty, &cam_ptr->m_height);
            m_editor_ui_creator["float"]("m_nearZ", isDirty, &cam_ptr->m_nearZ);
            m_editor_ui_creator["float"]("m_farZ", isDirty, &cam_ptr->m_farZ);

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["CameraComponentRes"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            CameraComponentRes* ccres_ptr = static_cast<CameraComponentRes*>(value_ptr);

            int item_current_idx = -1;

            const char* parameter_items[] = {FirstPersonCameraParameterName, ThirdPersonCameraParameterName, FreeCameraParameterName};

            std::string type_name = ccres_ptr->m_CamParamName;
            for (size_t i = 0; i < IM_ARRAYSIZE(parameter_items); i++)
            {
                if (strcmp(parameter_items[i], type_name.c_str()) == 0)
                {
                    item_current_idx = i;
                    break;
                }
            }

            if (item_current_idx == -1)
            {
                item_current_idx = 0;
                isDirty = true;
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
                isDirty = true;

                ccres_ptr->m_CamParamName = parameter_items[item_current_idx];
                if (item_current_idx == 0)
                {
                    ccres_ptr->m_FirstPersonCamParam = FirstPersonCameraParameter {};
                }
                else if (item_current_idx == 1)
                {
                    ccres_ptr->m_ThirdPersonCamParam = ThirdPersonCameraParameter {};
                }
                else if (item_current_idx == 2)
                {
                    ccres_ptr->m_FreeCamParam = FreeCameraParameter {};
                }
            }

            if (item_current_idx == 0)
            {
                m_editor_ui_creator["FirstPersonCameraParameter"](FirstPersonCameraParameterName, isDirty, &ccres_ptr->m_FirstPersonCamParam);
            }
            else if (item_current_idx == 1)
            {
                m_editor_ui_creator["ThirdPersonCameraParameter"](ThirdPersonCameraParameterName, isDirty, &ccres_ptr->m_ThirdPersonCamParam);
            }
            else if (item_current_idx == 2)
            {
                m_editor_ui_creator["FreeCameraParameter"](FreeCameraParameterName, isDirty, &ccres_ptr->m_FreeCamParam);
            }

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["DirectionLightParameter"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            DirectionLightParameter* light_ptr = static_cast<DirectionLightParameter*>(value_ptr);

            m_editor_ui_creator["Color"]("color", isDirty, &light_ptr->color);
            m_editor_ui_creator["float"]("intensity", isDirty, &light_ptr->intensity);
            m_editor_ui_creator["bool"]("shadows", isDirty, &light_ptr->shadows);
            m_editor_ui_creator["glm::float2"]("shadow_bounds", isDirty, &light_ptr->shadow_bounds);
            m_editor_ui_creator["float"]("shadow_near_plane", isDirty, &light_ptr->shadow_near_plane);
            m_editor_ui_creator["float"]("shadow_far_plane", isDirty, &light_ptr->shadow_far_plane);
            m_editor_ui_creator["glm::float2"]("shadowmap_size", isDirty, &light_ptr->shadowmap_size);

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["PointLightParameter"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            PointLightParameter* light_ptr = static_cast<PointLightParameter*>(value_ptr);

            m_editor_ui_creator["Color"]("color", isDirty, &light_ptr->color);
            m_editor_ui_creator["float"]("intensity", isDirty, &light_ptr->intensity);
            m_editor_ui_creator["float"]("falloff_radius", isDirty, &light_ptr->falloff_radius);

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["SpotLightParameter"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            SpotLightParameter* light_ptr = static_cast<SpotLightParameter*>(value_ptr);

            m_editor_ui_creator["Color"]("color", isDirty, &light_ptr->color);
            m_editor_ui_creator["float"]("intensity", isDirty, &light_ptr->intensity);
            m_editor_ui_creator["float"]("falloff_radius", isDirty, &light_ptr->falloff_radius);
            m_editor_ui_creator["float"]("spotAngle", isDirty, &light_ptr->spotAngle);
            m_editor_ui_creator["float"]("innerSpotPercent", isDirty, &light_ptr->innerSpotPercent);
            m_editor_ui_creator["bool"]("shadows", isDirty, &light_ptr->shadows);
            m_editor_ui_creator["glm::float2"]("shadow_bounds", isDirty, &light_ptr->shadow_bounds);
            m_editor_ui_creator["float"]("shadow_near_plane", isDirty, &light_ptr->shadow_near_plane);
            m_editor_ui_creator["float"]("shadow_far_plane", isDirty, &light_ptr->shadow_far_plane);
            m_editor_ui_creator["glm::float2"]("shadowmap_size", isDirty, &light_ptr->shadowmap_size);

            is_dirty |= isDirty;
        };  
        m_editor_ui_creator["LightComponentRes"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            ImGui::Indent();

            LightComponentRes* l_res_ptr = static_cast<LightComponentRes*>(value_ptr);

            int item_current_idx = -1;

            const char* parameter_items[] = {DirectionLightParameterName, PointLightParameterName, SpotLightParameterName};

            std::string type_name = l_res_ptr->m_LightParamName;
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

                l_res_ptr->m_LightParamName = parameter_items[item_current_idx];
                if (item_current_idx == 0)
                {
                    l_res_ptr->m_DirectionLightParam = DirectionLightParameter {};
                }
                else if (item_current_idx == 1)
                {
                    l_res_ptr->m_PointLightParam = PointLightParameter {};
                }
                else if (item_current_idx == 2)
                {
                    l_res_ptr->m_SpotLightParam = SpotLightParameter {};
                }
            }

            if (item_current_idx == 0)
            {
                m_editor_ui_creator[DirectionLightParameterName](DirectionLightParameterName, isDirty, &l_res_ptr->m_DirectionLightParam);
            }
            else if (item_current_idx == 1)
            {
                m_editor_ui_creator[PointLightParameterName](PointLightParameterName, isDirty, &l_res_ptr->m_PointLightParam);
            }
            else if (item_current_idx == 2)
            {
                m_editor_ui_creator[SpotLightParameterName](SpotLightParameterName, isDirty, &l_res_ptr->m_SpotLightParam);
            }

            ImGui::Unindent();

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["SceneMesh"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            SceneMesh* mesh_ptr = static_cast<SceneMesh*>(value_ptr);
            
            //m_editor_ui_creator["bool"]("m_is_mesh_data", isDirty, &mesh_ptr->m_is_mesh_data);

            //ImGui::Text(mesh_ptr->m_sub_mesh_file.c_str());
            static char str1[128];
            memset(str1, 0, 128);
            memcpy(str1, mesh_ptr->m_sub_mesh_file.c_str(), mesh_ptr->m_sub_mesh_file.size());
            ImGui::InputText(name.c_str(), str1, IM_ARRAYSIZE(str1), ImGuiInputTextFlags_ReadOnly);
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MESH_FILE_PATH"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(std::string));
                    std::string payload_filepath = *(std::string*)payload->Data;
                    std::string mesh_file_path   = "asset/" + payload_filepath;
                    mesh_ptr->m_sub_mesh_file    = mesh_file_path;

                    isDirty = true;
                }
                ImGui::EndDragDropTarget();
            }

            //ImGui::Text(mesh_ptr->m_mesh_data_path.c_str());

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["SceneMaterial"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            SceneMaterial* res_ptr = static_cast<SceneMaterial*>(value_ptr);

            ImGui::Text(res_ptr->m_shader_name.c_str());

            m_editor_ui_creator["StandardLightMaterial"]("m_mat_data", isDirty, &res_ptr->m_mat_data);

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["StandardLightMaterial"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            auto BlendModeFunc = [=](float& InBlendMode, bool& IsDirty) {
                const char* blendModeItems[] = { "Opaque", "Transparent" };
                int curItemIndex = (int)InBlendMode;
                if (ImGui::Combo("combo", &curItemIndex, blendModeItems, IM_ARRAYSIZE(blendModeItems))) IsDirty = true;
                InBlendMode = curItemIndex;
            };

            auto DoubleSliderFunc = [=](std::string TileName, float& InValue0, float& InValue1, float InMin, float InMax, bool& IsDirty) {
                float tmpRemapMinMax[2] = { InValue0 , InValue1 };
                if (ImGui::SliderFloat2(TileName.c_str(), tmpRemapMinMax, InMin, InMax))
                {
                    InValue0 = tmpRemapMinMax[0];
                    InValue1 = glm::min(glm::max(tmpRemapMinMax[0] + 0.01f, tmpRemapMinMax[1]), InMax);
                    IsDirty = true;
                }
            };

            StandardLightMaterial* mat_res_ptr = static_cast<StandardLightMaterial*>(value_ptr);

            BlendModeFunc(mat_res_ptr->_BlendMode, isDirty);

            if (ImGui::ColorEdit4("_BaseColor", &mat_res_ptr->_BaseColor.x)) isDirty = true;
            //if (ImGui::DragFloat("_Metallic", &mat_res_ptr->_Metallic, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_Smoothness", &mat_res_ptr->_Smoothness, 0.02f, 0.0f, 1.0f)) isDirty = true;

            DoubleSliderFunc("_MetallicRemapMinMax", mat_res_ptr->_MetallicRemapMin, mat_res_ptr->_MetallicRemapMax, 0.0f, 1.0f, isDirty);
            DoubleSliderFunc("_SmoothnessRemapMinMax", mat_res_ptr->_SmoothnessRemapMin, mat_res_ptr->_SmoothnessRemapMax, 0.0f, 1.0f, isDirty);
            DoubleSliderFunc("_AlphaRemapMinMax", mat_res_ptr->_AlphaRemapMin, mat_res_ptr->_AlphaRemapMax, 0.0f, 1.0f, isDirty);
            DoubleSliderFunc("_AORemapMinMax", mat_res_ptr->_AORemapMin, mat_res_ptr->_AORemapMax, 0.0f, 1.0f, isDirty);

            if (ImGui::DragFloat("_NormalScale", &mat_res_ptr->_NormalScale, 0.02f, 0.0f, 8.0f)) isDirty = true;
            //if (ImGui::DragFloat("_HeightAmplitude", &mat_res_ptr->_HeightAmplitude, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_HeightCenter", &mat_res_ptr->_HeightCenter, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragInt("_HeightMapParametrization", &mat_res_ptr->_HeightMapParametrization, 1, 0, 1)) isDirty = true;
            //if (ImGui::DragFloat("_HeightOffset", &mat_res_ptr->_HeightOffset, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //DoubleSliderFunc("_HeightMinMax", mat_res_ptr->_HeightMin, mat_res_ptr->_HeightMax, -10.0f, 10.0f, isDirty);
            //if (ImGui::DragFloat("_HeightTessCenter", &mat_res_ptr->_HeightTessCenter, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_HeightPoMAmplitude", &mat_res_ptr->_HeightPoMAmplitude, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_DetailAlbedoScale", &mat_res_ptr->_DetailAlbedoScale, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_DetailNormalScale", &mat_res_ptr->_DetailNormalScale, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_DetailSmoothnessScale", &mat_res_ptr->_DetailSmoothnessScale, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_Anisotropy", &mat_res_ptr->_Anisotropy, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_DiffusionProfileHash", &mat_res_ptr->_DiffusionProfileHash, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_SubsurfaceMask", &mat_res_ptr->_SubsurfaceMask, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_TransmissionMask", &mat_res_ptr->_TransmissionMask, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_Thickness", &mat_res_ptr->_Thickness, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat4("_ThicknessRemap", &mat_res_ptr->_ThicknessRemap.x, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat4("_IridescenceThicknessRemap", &mat_res_ptr->_IridescenceThicknessRemap.x, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_IridescenceThickness", &mat_res_ptr->_IridescenceThickness, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_IridescenceMask", &mat_res_ptr->_IridescenceMask, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_CoatMask", &mat_res_ptr->_CoatMask, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_EnergyConservingSpecularColor", &mat_res_ptr->_EnergyConservingSpecularColor, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragInt("_SpecularOcclusionMode", &mat_res_ptr->_SpecularOcclusionMode, 1, 0, 2)) isDirty = true;
            //if (ImGui::ColorEdit3("_EmissiveColor", &mat_res_ptr->_EmissiveColor.x)) isDirty = true;
            //if (ImGui::DragFloat("_AlbedoAffectEmissive", &mat_res_ptr->_AlbedoAffectEmissive, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragInt("_EmissiveIntensityUnit", &mat_res_ptr->_EmissiveIntensityUnit, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragInt("_UseEmissiveIntensity", &mat_res_ptr->_UseEmissiveIntensity, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_EmissiveIntensity", &mat_res_ptr->_EmissiveIntensity, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_EmissiveExposureWeight", &mat_res_ptr->_EmissiveExposureWeight, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_UseShadowThreshold", &mat_res_ptr->_UseShadowThreshold, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_AlphaCutoffEnable", &mat_res_ptr->_AlphaCutoffEnable, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_AlphaCutoff", &mat_res_ptr->_AlphaCutoff, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_AlphaCutoffShadow", &mat_res_ptr->_AlphaCutoffShadow, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_AlphaCutoffPrepass", &mat_res_ptr->_AlphaCutoffPrepass, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_AlphaCutoffPostpass", &mat_res_ptr->_AlphaCutoffPostpass, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_TransparentDepthPrepassEnable", &mat_res_ptr->_TransparentDepthPrepassEnable, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_TransparentBackfaceEnable", &mat_res_ptr->_TransparentBackfaceEnable, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_TransparentDepthPostpassEnable", &mat_res_ptr->_TransparentDepthPostpassEnable, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_TransparentSortPriority", &mat_res_ptr->_TransparentSortPriority, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragInt("_RefractionModel", &mat_res_ptr->_RefractionModel, 1, 0, 3)) isDirty = true;
            //if (ImGui::DragFloat("_Ior", &mat_res_ptr->_Ior, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::ColorEdit3("_TransmittanceColor", &mat_res_ptr->_TransmittanceColor.x)) isDirty = true;
            //if (ImGui::DragFloat("_ATDistance", &mat_res_ptr->_ATDistance, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_TransparentWritingMotionVec", &mat_res_ptr->_TransparentWritingMotionVec, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_SurfaceType", &mat_res_ptr->_SurfaceType, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_SrcBlend", &mat_res_ptr->_SrcBlend, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_DstBlend", &mat_res_ptr->_DstBlend, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_AlphaSrcBlend", &mat_res_ptr->_AlphaSrcBlend, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_AlphaDstBlend", &mat_res_ptr->_AlphaDstBlend, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_EnableFogOnTransparent", &mat_res_ptr->_EnableFogOnTransparent, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_DoubleSidedEnable", &mat_res_ptr->_DoubleSidedEnable, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_DoubleSidedNormalMode", &mat_res_ptr->_DoubleSidedNormalMode, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat4("_DoubleSidedConstants", &mat_res_ptr->_DoubleSidedConstants.x, 0.02f, -1.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_DoubleSidedGIMode", &mat_res_ptr->_DoubleSidedGIMode, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_UVBase", &mat_res_ptr->_UVBase, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_ObjectSpaceUVMapping", &mat_res_ptr->_ObjectSpaceUVMapping, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_TexWorldScale", &mat_res_ptr->_TexWorldScale, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::ColorEdit4("_UVMappingMask", &mat_res_ptr->_UVMappingMask.x)) isDirty = true;
            //if (ImGui::DragFloat("_NormalMapSpace", &mat_res_ptr->_NormalMapSpace, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragInt("_MaterialID", &mat_res_ptr->_MaterialID, 1, 0, 5)) isDirty = true;
            //if (ImGui::DragFloat("_TransmissionEnable", &mat_res_ptr->_TransmissionEnable, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_PPDMinSamples", &mat_res_ptr->_PPDMinSamples, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_PPDMaxSamples", &mat_res_ptr->_PPDMaxSamples, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_PPDLodThreshold", &mat_res_ptr->_PPDLodThreshold, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_PPDPrimitiveLength", &mat_res_ptr->_PPDPrimitiveLength, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_PPDPrimitiveWidth", &mat_res_ptr->_PPDPrimitiveWidth, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat4("_InvPrimScale", &mat_res_ptr->_InvPrimScale.x, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat4("_UVDetailsMappingMask", &mat_res_ptr->_UVDetailsMappingMask.x, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_UVDetail", &mat_res_ptr->_UVDetail, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_LinkDetailsWithBase", &mat_res_ptr->_LinkDetailsWithBase, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_EmissiveColorMode", &mat_res_ptr->_EmissiveColorMode, 0.02f, 0.0f, 1.0f)) isDirty = true;
            //if (ImGui::DragFloat("_UVEmissive", &mat_res_ptr->_UVEmissive, 0.02f, 0.0f, 1.0f)) isDirty = true;

            m_editor_ui_creator["MaterialImage"]("_BaseColorMap", isDirty, &mat_res_ptr->_BaseColorMap);
            m_editor_ui_creator["MaterialImage"]("_MaskMap", isDirty, &mat_res_ptr->_MaskMap);
            m_editor_ui_creator["MaterialImage"]("_NormalMap", isDirty, &mat_res_ptr->_NormalMap);
            //m_editor_ui_creator["MaterialImage"]("_NormalMapOS", isDirty, &mat_res_ptr->_NormalMapOS);
            //m_editor_ui_creator["MaterialImage"]("_BentNormalMap", isDirty, &mat_res_ptr->_BentNormalMap);
            //m_editor_ui_creator["MaterialImage"]("_BentNormalMapOS", isDirty, &mat_res_ptr->_BentNormalMapOS);
            m_editor_ui_creator["MaterialImage"]("_HeightMap", isDirty, &mat_res_ptr->_HeightMap);
            m_editor_ui_creator["MaterialImage"]("_DetailMap", isDirty, &mat_res_ptr->_DetailMap);
            //m_editor_ui_creator["MaterialImage"]("_TangentMap", isDirty, &mat_res_ptr->_TangentMap);
            //m_editor_ui_creator["MaterialImage"]("_TangentMapOS", isDirty, &mat_res_ptr->_TangentMapOS);
            m_editor_ui_creator["MaterialImage"]("_AnisotropyMap", isDirty, &mat_res_ptr->_AnisotropyMap);
            m_editor_ui_creator["MaterialImage"]("_SubsurfaceMaskMap", isDirty, &mat_res_ptr->_SubsurfaceMaskMap);
            m_editor_ui_creator["MaterialImage"]("_TransmissionMaskMap", isDirty, &mat_res_ptr->_TransmissionMaskMap);
            m_editor_ui_creator["MaterialImage"]("_ThicknessMap", isDirty, &mat_res_ptr->_ThicknessMap);
            m_editor_ui_creator["MaterialImage"]("_IridescenceThicknessMap", isDirty, &mat_res_ptr->_IridescenceThicknessMap);
            m_editor_ui_creator["MaterialImage"]("_IridescenceMaskMap", isDirty, &mat_res_ptr->_IridescenceMaskMap);
            m_editor_ui_creator["MaterialImage"]("_CoatMaskMap", isDirty, &mat_res_ptr->_CoatMaskMap);
            m_editor_ui_creator["MaterialImage"]("_EmissiveColorMap", isDirty, &mat_res_ptr->_EmissiveColorMap);
            m_editor_ui_creator["MaterialImage"]("_TransmittanceColorMap", isDirty, &mat_res_ptr->_TransmittanceColorMap);

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["MaterialImage"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            ImGui::Text(name.c_str());

            ImGui::PushID(name.c_str());

            MaterialImage* material_image_ptr = static_cast<MaterialImage*>(value_ptr);

            m_editor_ui_creator["SceneImage"]("m_Image", isDirty, &material_image_ptr->m_image);

            if (ImGui::DragFloat2("m_tilling", (float*)&material_image_ptr->m_tilling, 0.1f, 0.0001f))
                isDirty = true;
            if (ImGui::DragFloat2("m_offset", (float*)&material_image_ptr->m_offset, 0.1f, 0.0f))
                isDirty = true;

            ImGui::PopID();
            
            is_dirty |= isDirty;
        };
        m_editor_ui_creator["SceneImage"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            SceneImage* scene_image_ptr = static_cast<SceneImage*>(value_ptr);

            //ImGui::Indent();
            //ImGui::PushID(name.c_str());

            if (ImGui::Checkbox("m_is_srgb", &scene_image_ptr->m_is_srgb))
                isDirty = true;
            if (ImGui::Checkbox("m_auto_mips", &scene_image_ptr->m_auto_mips))
                isDirty = true;
            if (ImGui::InputInt("m_mip_levels", &scene_image_ptr->m_mip_levels, 1))
            {
                int _mipLevel = glm::clamp(scene_image_ptr->m_mip_levels, 0, 12);
                if (scene_image_ptr->m_mip_levels != _mipLevel)
                {
                    scene_image_ptr->m_mip_levels = _mipLevel;
                    isDirty = true;
                }
            }

            static char str1[128];
            memset(str1, 0, 128);
            memcpy(str1, scene_image_ptr->m_image_file.c_str(), scene_image_ptr->m_image_file.size());
            ImGui::InputText(name.c_str(), str1, IM_ARRAYSIZE(str1), ImGuiInputTextFlags_ReadOnly);
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_FILE_PATH"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(std::string));
                    std::string payload_filepath  = *(std::string*)payload->Data;
                    std::string texture_file_path = "asset/" + payload_filepath;
                    scene_image_ptr->m_image_file = texture_file_path;

                    isDirty = true;
                }
                ImGui::EndDragDropTarget();
            }

            //ImGui::PopID();
            //ImGui::Unindent();

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["MeshRendererComponent"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            if (ImGui::TreeNode(name.c_str()))
            {
                ImGui::Indent();

                MeshRendererComponent* mesh_renderer_ptr = static_cast<MeshRendererComponent*>(value_ptr);

                SceneMesh& _sceneMesh = mesh_renderer_ptr->getSceneMesh();
                SceneMaterial& _sceneMaterial = mesh_renderer_ptr->getSceneMaterial();

                m_editor_ui_creator["SceneMesh"]("m_scene_mesh", isDirty, &_sceneMesh);
                m_editor_ui_creator["SceneMaterial"]("m_material", isDirty, &_sceneMaterial);
                
                ImGui::Unindent();
                ImGui::TreePop();
            }

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["TerrainComponent"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            if (ImGui::TreeNode(name.c_str()))
            {
                ImGui::Indent();

                TerrainComponent* terrain_ptr = static_cast<TerrainComponent*>(value_ptr);

                m_editor_ui_creator["TerrainMesh"]("m_terrain_mesh", isDirty, &terrain_ptr->m_terrain);
                m_editor_ui_creator["TerrainMaterial"]("m_terrain_material", isDirty, &terrain_ptr->m_material);

                ImGui::Unindent();
                ImGui::TreePop();
            }

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["TerrainMesh"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            if (ImGui::TreeNode(name.c_str()))
            {
                ImGui::Indent();

                SceneTerrainMesh* terrain_mesh_ptr = static_cast<SceneTerrainMesh*>(value_ptr);

                m_editor_ui_creator["glm::int3"]("TerrainSize", isDirty, &terrain_mesh_ptr->terrain_size);
                m_editor_ui_creator["SceneImage"]("TerrainHeightMap", isDirty, &terrain_mesh_ptr->m_terrain_height_map);
                m_editor_ui_creator["SceneImage"]("TerrainNormalMap", isDirty, &terrain_mesh_ptr->m_terrain_normal_map);

                ImGui::Unindent();
                ImGui::TreePop();
            }

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["TerrainMaterial"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            if (ImGui::TreeNode(name.c_str()))
            {
                ImGui::Indent();

                TerrainMaterial* terrain_material_ptr = static_cast<TerrainMaterial*>(value_ptr);

                int base_tex_length = sizeof(terrain_material_ptr->m_base_texs) / sizeof(TerrainBaseTex);

                for (int i = 0; i < base_tex_length; i++)
                {
                    m_editor_ui_creator["MaterialImage"]("Albedo", isDirty, &terrain_material_ptr->m_base_texs[i].m_albedo_file);
                    m_editor_ui_creator["MaterialImage"]("ARM", isDirty, &terrain_material_ptr->m_base_texs[i].m_ao_roughness_metallic_file);
                    m_editor_ui_creator["MaterialImage"]("Displacement", isDirty, &terrain_material_ptr->m_base_texs[i].m_displacement_file);
                    m_editor_ui_creator["MaterialImage"]("Normal", isDirty, &terrain_material_ptr->m_base_texs[i].m_normal_file);
                }

                ImGui::Unindent();
                ImGui::TreePop();
            }

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["LightComponent"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            if (ImGui::TreeNode(name.c_str()))
            {
                LightComponent* light_ptr = static_cast<LightComponent*>(value_ptr);

                LightComponentRes& _lightComponentRes = light_ptr->getLightComponent();

                m_editor_ui_creator["LightComponentRes"]("m_light_res", isDirty, &_lightComponentRes);

                ImGui::TreePop();
            }
            
            is_dirty |= isDirty;
        };
        m_editor_ui_creator["CameraComponent"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            if (ImGui::TreeNode(name.c_str()))
            {
                CameraComponent* camera_ptr = static_cast<CameraComponent*>(value_ptr);

                CameraComponentRes& _cameraComponentRes = camera_ptr->getCameraComponent();

                m_editor_ui_creator["CameraComponentRes"]("m_camera_res", isDirty, &_cameraComponentRes);

                ImGui::TreePop();
            }

            is_dirty |= isDirty;
        };
        m_editor_ui_creator["TransformComponent"] = [this, &asset_folder](const std::string& name, bool& is_dirty, void* value_ptr) -> void {
            bool isDirty = false;

            if (ImGui::TreeNode(name.c_str()))
            {
                TransformComponent* transform_ptr = static_cast<TransformComponent*>(value_ptr);
                Transform& _transform =  transform_ptr->getTransform();
                m_editor_ui_creator["Transform"]("m_transform", isDirty, &_transform);

                ImGui::TreePop();
            }

            is_dirty |= isDirty;
        };
    }
    /*
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
    */
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
                    g_editor_global_context.m_scene_manager->onGObjectSelected(K_Invalid_Object_Id);
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

        std::weak_ptr<GObject> root_node_weak_ptr = current_active_level->getRootNode();
        if (!root_node_weak_ptr.expired())
        {
            std::shared_ptr<GObject> root_node_shared_ptr = root_node_weak_ptr.lock();
            std::vector<std::shared_ptr<GObject>> root_children = root_node_shared_ptr->getChildren();
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
                GObjectID newParentID = selectedId == K_Invalid_Object_Id ? K_Root_Object_Id : selectedId;
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
                g_editor_global_context.m_scene_manager->onGObjectSelected(K_Invalid_Object_Id);
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
    
    void EditorUI::createComponentUI(MoYu::Component* component)
    {
        bool _isDirty = false;

        std::string _typeName = component->getTypeName();

        m_editor_ui_creator[_typeName](_typeName, _isDirty, component);

        if (_isDirty)
            component->markDirty();

        /*
        Reflection::ReflectionInstance* reflection_instance;
        int count = instance.m_meta.getBaseClassReflectionInstanceList(reflection_instance, instance.m_instance);
        for (int index = 0; index < count; index++)
        {
            createComponentUI(reflection_instance[index]);
        }
        createLeafNodeUI(instance);

        if (count > 0)
            delete[] reflection_instance;
        */

    }
    /**/
    /*
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
    */

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
            createComponentUI(component_ptr.get());
            /*
            m_editor_ui_creator["TreeNodePush"](("<" + component_ptr->getTypeName() + ">").c_str(), component_ptr.getPtr());
            auto object_instance = Reflection::ReflectionInstance(
                MoYu::Reflection::TypeMeta::newMetaFromName(component_ptr.getTypeName().c_str()),
                component_ptr.operator->());
            createComponentUI(object_instance);
            m_editor_ui_creator["TreeNodePop"](("<" + component_ptr.getTypeName() + ">").c_str(), component_ptr.getPtr());
            */
        }

        ImGui::NewLine();

        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponentPopup");

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            if (ImGui::MenuItem("Camera Component"))
            {
                if (selected_object->tryGetComponent<CameraComponent>("CameraComponent"))
                {
                    LOG_INFO("object {} already has Camera Component", selected_object->getName());
                }
                else
                {
                    std::shared_ptr<CameraComponent> camera_component = std::make_shared<CameraComponent>();
                    selected_object->tryAddComponent(camera_component);
                    LOG_INFO("Add New Camera Component");
                }
            }

            if (ImGui::BeginMenu("Light Component"))
            {
                if (selected_object->tryGetComponent<LightComponent>("LightComponent"))
                {
                    LOG_INFO("object {} already has Light Component", selected_object->getName());
                }
                else
                {
                    static const char* param_names[] = {DirectionLightParameterName, PointLightParameterName, SpotLightParameterName};

                    for (int i = 0; i < IM_ARRAYSIZE(param_names); i++)
                    {
                        bool _param_toggle = false;
                        if (ImGui::MenuItem(param_names[i], "", &_param_toggle))
                        {
                            if (_param_toggle)
                            {
                                std::shared_ptr<LightComponent> light_component = std::make_shared<LightComponent>();
                                light_component->getLightComponent().m_LightParamName = param_names[i];
                                light_component->markDirty();
                                selected_object->tryAddComponent(light_component);

                                LOG_INFO(("Add New Light Component " + std::string(param_names[i])).c_str());
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Mesh Renderer Component"))
            {
                if (selected_object->tryGetComponent<MeshRendererComponent>("MeshRendererComponent"))
                {
                    LOG_INFO("object {} already has Mesh Renderer Component", selected_object->getName());
                }
                else
                {
                    static const DefaultMeshType param_types[] = {DefaultMeshType::Capsule,
                                                                  DefaultMeshType::Cone,
                                                                  DefaultMeshType::Convexmesh,
                                                                  DefaultMeshType::Cube,
                                                                  DefaultMeshType::Cylinder,
                                                                  DefaultMeshType::Sphere,
                                                                  DefaultMeshType::Triangle,
                                                                  DefaultMeshType::Square};

                    for (int i = 0; i < IM_ARRAYSIZE(param_types); i++)
                    {
                        std::string param_name = DefaultMeshTypeToName(param_types[i]);
                        bool _param_toggle = false;
                        if (ImGui::MenuItem(param_name.c_str(), "", &_param_toggle))
                        {
                            if (_param_toggle)
                            {
                                std::shared_ptr<MeshRendererComponent> mesh_renderer_component = std::make_shared<MeshRendererComponent>();
                                mesh_renderer_component->updateMeshRendererRes(DefaultMeshTypeToComponentRes(param_types[i]));
                                mesh_renderer_component->markDirty();
                                selected_object->tryAddComponent(mesh_renderer_component);

                                LOG_INFO(("Add New Mesh Renderer Component " + param_name).c_str());
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Terrain Renderer Component"))
            {
                if (selected_object->tryGetComponent<TerrainComponent>("TerrainComponent"))
                {
                    LOG_INFO("object {} already has Terrain Renderer Component", selected_object->getName());
                }
                else
                {
                    std::shared_ptr<TerrainComponent> terrain_component = std::make_shared<TerrainComponent>();
                    selected_object->tryAddComponent(terrain_component);
                    LOG_INFO("Add New Terrain Component");
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
        /**/
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
                    //g_editor_global_context.m_scene_manager->drawSelectedEntityAxis();
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
                    //g_editor_global_context.m_scene_manager->drawSelectedEntityAxis();
                    g_runtime_global_context.m_input_system->resetGameCommand();
                    g_editor_global_context.m_render_system->getRenderCamera()->setMainViewMatrix(
                        g_editor_global_context.m_scene_manager->getEditorCamera()->getViewMatrix());
                }
            }

            ImGui::Unindent();
            ImGui::EndMenuBar();
        }

        auto menu_bar_rect = ImGui::GetCurrentWindow()->MenuBarRect();

        glm::float2 new_window_pos  = {0.0f, 0.0f};
        glm::float2 new_window_size = {0.0f, 0.0f};
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

            glm::float2 cursor_offset = glm::float2((new_window_size.x - displayWidth) * 0.5f, (new_window_size.y - displayHeight) * 0.5f);

            ImVec2 cilld_cur_pos = ImVec2(cursor_offset.x + new_window_pos.x, cursor_offset.y + new_window_pos.y);

            g_editor_global_context.m_input_manager->setEngineWindowPos(glm::float2(cilld_cur_pos.x, cilld_cur_pos.y));
            g_editor_global_context.m_input_manager->setEngineWindowSize(glm::float2(displayWidth, displayHeight));

            ImGui::SetCursorPosX(cursor_offset.x);
            ImGui::BeginChild("GameView", ImVec2(displayWidth, displayHeight), true, ImGuiWindowFlags_NoDocking);
            {
                ImGuizmo::SetDrawlist();

                ImVec2 window_pos = ImGui::GetWindowPos();

                ImGuizmo::SetRect(window_pos.x, window_pos.y, displayWidth, displayHeight);

                glm::float4x4 viewMatrix  = g_editor_global_context.m_scene_manager->getEditorCamera()->getViewMatrix();
                glm::mat4  _cameraView = viewMatrix;
                glm::float4x4 projMatrix = g_editor_global_context.m_scene_manager->getEditorCamera()->getProjMatrix(true);
                glm::mat4  _projMatrix    = projMatrix;
                glm::mat4  _identiyMatrix = MYMatrix4x4::Identity;

                ImGuizmo::DrawGrid((const float*)&_cameraView, (const float*)&_projMatrix, (const float*)&_identiyMatrix, 100.f);

                // draw the real rendering image
                {
                    ImGui::Image((ImTextureID)handleOfGameView.ptr, ImVec2(displayWidth, displayHeight));
                }

                std::shared_ptr<GObject> selected_object = g_editor_global_context.m_scene_manager->getSelectedGObject().lock();
                if (selected_object != nullptr)
                {
                    TransformComponent* trans_component_ptr = selected_object->getTransformComponent().lock().get();
                    glm::float4x4 worldMatrix = trans_component_ptr->getMatrixWorld();
                    glm::mat4 _worldMatrix = worldMatrix;

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
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(check_button_color.x, check_button_color.y, check_button_color.z, 0.40f));
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
                //g_editor_global_context.m_scene_manager->drawSelectedEntityAxis();
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
        if (node->m_file_type == "jpg" || node->m_file_type == "png" || node->m_file_type == "tga" || node->m_file_type == "exr" || node->m_file_type == "dds")
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
                        //default_material_res.m_base_color_texture_file = _default_color_texture_path;
                        //default_material_res.m_metallic_roughness_texture_file = _default_metallic_roughness_texture_path;
                        //default_material_res.m_normal_texture_file = _default_normal_texture_path;

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
        /*
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
        */
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

    bool DrawVecControl(const std::string& label, glm::float3& values, float resetValue, float columnWidth)
    {
        glm::float3 prevVal = values;

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

    bool DrawVecControl(const std::string& label, glm::quat& values, float resetValue, float columnWidth)
    {
        glm::quat prevVal = values;

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
