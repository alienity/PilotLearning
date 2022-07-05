set(TARGET_NAME PilotTest)

file(GLOB EDITOR_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADERS} ${EDITOR_SOURCES} ${EDITOR_RESOURCE})

add_executable(${TARGET_NAME} ${EDITOR_HEADERS} ${EDITOR_SOURCES} ${EDITOR_RESOURCE})

add_compile_definitions("MOYU_ROOT_DIR=${BINARY_ROOT_DIR}")

add_definitions(-DUNICODE -D_UNICODE)

target_include_directories(
  ${TARGET_NAME} 
  PUBLIC $<BUILD_INTERFACE:${THIRD_PARTY_DIR}/stb>
)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "MoYuTest")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_link_libraries(${TARGET_NAME} PilotRuntime)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E make_directory "${BINARY_ROOT_DIR}"
COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>/" "${BINARY_ROOT_DIR}"
COMMAND ${CMAKE_COMMAND} -E remove_directory "${BINARY_ROOT_DIR}/${ENGINE_ASSET_DIR}"
COMMAND ${CMAKE_COMMAND} -E copy_directory "${ENGINE_ROOT_DIR}/${ENGINE_ASSET_DIR}" "${BINARY_ROOT_DIR}/${ENGINE_ASSET_DIR}"
)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy 
	${THIRD_PARTY_DIR}/dxc_2021_12_08/bin/x64/dxcompiler.dll
	${THIRD_PARTY_DIR}/dxc_2021_12_08/bin/x64/dxil.dll
	${THIRD_PARTY_DIR}/WinPixEventRuntime.1.0.220124001/bin/x64/WinPixEventRuntime.dll
	${THIRD_PARTY_DIR}/Microsoft.Direct3D.DirectStorage.1.0.2/native/bin/x64/dstorage.dll
	${THIRD_PARTY_DIR}/Microsoft.Direct3D.DirectStorage.1.0.2/native/bin/x64/dstoragecore.dll
	$<TARGET_FILE_DIR:${TARGET_NAME}>
	DEPENDS ${TARGET_NAME}
)
