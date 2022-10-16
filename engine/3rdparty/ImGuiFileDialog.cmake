set(ImGuiFileDialog_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiFileDialog-Lib_Only)

file(GLOB ImGuiFileDialogSources CONFIGURE_DEPENDS  "${ImGuiFileDialog_SOURCE_DIR_}/*.cpp" "${ImGuiFileDialog_SOURCE_DIR_}/*.h")
add_library(ImGuiFileDialog STATIC ${ImGuiFileDialogSources})
target_include_directories(ImGuiFileDialog PUBLIC $<BUILD_INTERFACE:${ImGuiFileDialog_SOURCE_DIR_}>)
target_link_libraries(ImGuiFileDialog PUBLIC imgui)