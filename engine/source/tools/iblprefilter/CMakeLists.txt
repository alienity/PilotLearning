set(TARGET_NAME IBLPrefilter)

file(GLOB EDITOR_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.h)
file(GLOB EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp)
file(GLOB EDITOR_RESOURCE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resource/*)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADERS} ${EDITOR_SOURCES} ${EDITOR_RESOURCE})

add_library(${TARGET_NAME} STATIC ${EDITOR_HEADERS} ${EDITOR_SOURCES} ${EDITOR_RESOURCE})

add_definitions(-DUNICODE -D_UNICODE)

set(moyu_ibl_headers_include ${CMAKE_CURRENT_SOURCE_DIR}/sources)

target_include_directories(
  ${TARGET_NAME}
  PUBLIC $<BUILD_INTERFACE:${moyu_ibl_headers_include}>)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "${TARGET_NAME}")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "${ToolRoot}")

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_link_libraries(${TARGET_NAME} PilotRuntime IBL image imageio)
