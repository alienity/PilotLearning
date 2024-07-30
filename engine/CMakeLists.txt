
set(ENGINE_ROOT_DIR "${MOYU_ROOT_DIR}/engine")
set(THIRD_PARTY_DIR "${ENGINE_ROOT_DIR}/3rdparty")
set(ENGINE_ASSET_DIR "/asset")
set(ENGINE_SHADER_DIR "/shader")
set(ENGINE_INIT_CONFIG "${ENGINE_ROOT_DIR}/MoYuEditor.ini")

set(THIRD_PARTY_DLL 
${THIRD_PARTY_DIR}/dxc_2023_08_14/bin/x64/dxcompiler.dll
${THIRD_PARTY_DIR}/dxc_2023_08_14/bin/x64/dxil.dll
${THIRD_PARTY_DIR}/WinPixEventRuntime/bin/x64/WinPixEventRuntime.dll
${THIRD_PARTY_DIR}/DirectStorage/native/bin/x64/dstorage.dll
${THIRD_PARTY_DIR}/DirectStorage/native/bin/x64/dstoragecore.dll
${THIRD_PARTY_DIR}/embree-4.3.3.x64.windows/bin/embree4.dll
${THIRD_PARTY_DIR}/embree-4.3.3.x64.windows/bin/tbb12.dll)

option(ENABLE_PHYSICS_DEBUG_RENDERER "Enable Physics Debug Renderer" OFF)

if(ENABLE_PHYSICS_DEBUG_RENDERER)
  set(JOLT_ASSET_DIR "/jolt-asset")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options("/MP")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MoYuEditor)
endif()

set(wil_include ${THIRD_PARTY_DIR}/wil/include)
set(directx_headers_include ${THIRD_PARTY_DIR}/DirectX-Headers/include)
set(dxc_headers_include ${THIRD_PARTY_DIR}/dxc_2023_08_14/inc)
# set(vulkan_include ${THIRD_PARTY_DIR}/VulkanSDK/include)

# if(WIN32)
#   set(vulkan_lib ${THIRD_PARTY_DIR}/VulkanSDK/lib/Win32/vulkan-1.lib)
#   set(glslangValidator_executable ${THIRD_PARTY_DIR}/VulkanSDK/bin/Win32/glslangValidator.exe)
#   add_compile_definitions("PILOT_VK_LAYER_PATH=${THIRD_PARTY_DIR}/VulkanSDK/bin/Win32")
# else()
#   message(FATAL_ERROR "Unknown Platform")
# endif()


# set(SHADER_COMPILE_TARGET PilotShaderCompile)
add_subdirectory(shader)

add_subdirectory(3rdparty)

add_subdirectory(source/runtime)
add_subdirectory(source/editor)
add_subdirectory(source/tools)
# add_subdirectory(source/meta_parser)
# add_subdirectory(source/test)

# set(CODEGEN_TARGET "PilotPreCompile")
# include(source/precompile/precompile.cmake)
# set_target_properties("${CODEGEN_TARGET}" PROPERTIES FOLDER "Engine" )

# add_dependencies(PilotRuntime "${CODEGEN_TARGET}")
# add_dependencies("${CODEGEN_TARGET}" "MoYuParser")
