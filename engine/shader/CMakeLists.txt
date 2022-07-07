set(TARGET_NAME ${SHADER_COMPILE_TARGET})

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.vert"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.frag"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.comp"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.geom"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.tesc"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.tese"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.mesh"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.task"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.rgen"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.rchit"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.rmiss"
                                            "${CMAKE_CURRENT_SOURCE_DIR}/hlsl/*.rcall")

SOURCE_GROUP("shaders" FILES ${SHADER_FILES})

set(SHADER_INCLUDE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/include)

include(${MOYU_ROOT_DIR}/cmake/ShaderCompile.cmake)

compile_shader(
  "${SHADER_FILES}"
  "${TARGET_NAME}"
  "${SHADER_INCLUDE_FOLDER}"
  "${GENERATED_SHADER_FOLDER}"
  "${glslangValidator_executable}")

# set_target_properties("${TARGET_NAME}" PROPERTIES FOLDER "Engine" )

# For each shader, we create a header file
# foreach(SHADER ${shaders})
#     # Prepare a header name and a global variable for this shader
#     get_filename_component(SHADER_NAME ${SHADER} NAME)
#     string(REPLACE "." "_" HEADER_NAME ${SHADER_NAME})
#     string(TOUPPER ${HEADER_NAME} GLOBAL_SHADER_VAR)

#     set(SHADER_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER}/${HEADER_NAME}.h")

#     add_custom_target(
#       ${HEADER_NAME}
#       # Compile any GLSL shader into SPIR-V shader
#       # -g -Od should only be used in debug version
#       COMMAND ${glslangValidator_executable} -I${SHADER_INCLUDE_FOLDER} -V100 -o ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER}/${SHADER_NAME}.spv ${SHADER}
#       # Make a C header file with the SPIR-V shader
#       COMMAND ${CMAKE_COMMAND} -DPATH="${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER}/${SHADER_NAME}.spv" -DHEADER="${SHADER_HEADER}" -DGLOBAL="${GLOBAL_SHADER_VAR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/embed_resource.cmake"
#       # Rebuild the header file if the shader is updated
#       DEPENDS ${SHADER}
#       COMMENT "Building ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER}/${SHADER_NAME}.spv and embedding it into ${SHADER_HEADER}"
#     )

#     set_target_properties(${HEADER_NAME} PROPERTIES FOLDER "Shaders/generated" )
#     # Add the custom target like a dependencies of the project
#     add_dependencies(${PILOT_NAME} ${HEADER_NAME})

#     message(STATUS "Generating build commands for ${SHADER}")
# endforeach()

