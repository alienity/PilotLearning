set(ImGuizmo_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo)

file(GLOB ImGuizmo_SOURCE_DIR__sources CONFIGURE_DEPENDS  "${ImGuizmo_SOURCE_DIR_}/*.cpp" "${ImGuizmo_SOURCE_DIR_}/*.h")
add_library(ImGuizmo STATIC ${ImGuizmo_SOURCE_DIR__sources})
target_include_directories(ImGuizmo PUBLIC $<BUILD_INTERFACE:${ImGuizmo_SOURCE_DIR_}>)
target_link_libraries(ImGuizmo PUBLIC imgui)