set(third_party_folder "ThirdParty")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(NOT TARGET imgui)
    include(imgui.cmake)
    set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder}/imgui)
endif()

if(NOT TARGET DirectX-Headers)
    add_subdirectory(DirectX-Headers)
    set_target_properties(DirectX-Headers PROPERTIES FOLDER ${third_party_folder}/DirectX-Headers)
    set_target_properties(DirectX-Guids PROPERTIES FOLDER ${third_party_folder}/DirectX-Guids)
endif()

if(NOT TARGET DirectXTK12)
    add_subdirectory(DirectXTK12-jun2022)
    set_target_properties(DirectXTK12 PROPERTIES FOLDER ${third_party_folder}/DirectXTK12)
endif()

if(NOT TARGET glm)
    option(BUILD_STATIC_LIBS "" ON)
    option(BUILD_TESTING "" OFF)
    option(GLM_TEST_ENABLE "" OFF)
    add_subdirectory(glm)
    set_target_properties(glm_static PROPERTIES FOLDER ${third_party_folder}/glm)
endif()

if(NOT TARGET glfw)
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)
    add_subdirectory(glfw)
    set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder}/glfw)
    set_target_properties(update_mappings PROPERTIES FOLDER ${third_party_folder}/glfw)
endif()

if(NOT TARGET spdlog)
	option(SPDLOG_WCHAR_SUPPORT "" ON)
    option(SPDLOG_BUILD_EXAMPLE "" OFF)
    option(SPDLOG_INSTALL "" OFF)
    add_subdirectory(spdlog)
    set_target_properties(spdlog PROPERTIES FOLDER ${third_party_folder}/spdlog)
endif()

if(NOT TARGET stb)
    include(stb.cmake)
    set_target_properties(stb PROPERTIES FOLDER ${third_party_folder}/stb)
endif()

if(NOT TARGET tinyobjloader)
    add_subdirectory(tinyobjloader)
    set_target_properties(tinyobjloader PROPERTIES FOLDER ${third_party_folder}/tinyobjloader)
    set_target_properties(uninstall PROPERTIES FOLDER ${third_party_folder}/tinyobjloader)
endif()

if(NOT TARGET assimp)
    option(BUILD_SHARED_LIBS "" OFF)
    option(ASSIMP_BUILD_ZLIB "" ON)
    option(ASSIMP_BUILD_TESTS "" OFF)
    option(ASSIMP_INJECT_DEBUG_POSTFIX "" OFF)
    add_subdirectory(assimp-5.2.4)
    set_target_properties(uninstall PROPERTIES FOLDER ${third_party_folder}/assimp-5.2.4)
    set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder}/assimp-5.2.4)
    set_target_properties(assimp_cmd PROPERTIES FOLDER ${third_party_folder}/assimp-5.2.4)
    set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER ${third_party_folder}/assimp-5.2.4)
    set_target_properties(zlibstatic PROPERTIES FOLDER ${third_party_folder}/assimp-5.2.4)
endif()

if(NOT TARGET Jolt)
    option(TARGET_HELLO_WORLD "" OFF)
    option(TARGET_PERFORMANCE_TEST "" OFF)
    option(TARGET_SAMPLES "" OFF)
    option(TARGET_UNIT_TESTS "" OFF)
    option(TARGET_VIEWER "" OFF)

    if(ENABLE_PHYSICS_DEBUG_RENDERER)
        option(TARGET_TEST_FRAMEWORK "" ON)
    else()
        option(TARGET_TEST_FRAMEWORK "" OFF)
    endif()

    add_subdirectory(JoltPhysics/Build)

    if(ENABLE_PHYSICS_DEBUG_RENDERER)
        set_target_properties(Jolt TestFramework
            PROPERTIES 
            FOLDER ${third_party_folder}/JoltPhysics
            MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    else()
        set_target_properties(Jolt
            PROPERTIES 
            FOLDER ${third_party_folder}/JoltPhysics
            MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    endif()

endif()

if(NOT TARGET dxc)
    include(dxc.cmake)
    set_target_properties(dxc PROPERTIES FOLDER ${third_party_folder}/dxc)
endif()

if(NOT TARGET pix)
    include(pix.cmake)
    set_target_properties(pix PROPERTIES FOLDER ${third_party_folder}/pix)
endif()

if(NOT TARGET DirectStorage)
    include(DirectStorage.cmake)
    set_target_properties(DirectStorage PROPERTIES FOLDER ${third_party_folder}/DirectStorage)
endif()

if(NOT TARGET cityhash)
    include(cityhash.cmake)
    set_target_properties(cityhash PROPERTIES FOLDER ${third_party_folder}/cityhash)
endif()