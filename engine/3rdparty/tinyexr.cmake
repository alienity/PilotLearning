set(tinyexr_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/tinyexr-1.0.5)

file(GLOB miniz_sources CONFIGURE_DEPENDS  
"${tinyexr_SOURCE_DIR_}/deps/miniz/miniz.h" 
"${tinyexr_SOURCE_DIR_}/deps/miniz/miniz.c")

add_library(miniz STATIC ${tinyexr_SOURCE_DIR_}/deps/miniz/miniz.c)
target_include_directories(miniz PUBLIC ${tinyexr_SOURCE_DIR_}/deps/miniz)
list(APPEND TINYEXR_EXT_LIBRARIES miniz)

file(GLOB tinyexr_sources CONFIGURE_DEPENDS  
"${tinyexr_SOURCE_DIR_}/tinyexr.h" 
"${tinyexr_SOURCE_DIR_}/tinyexr.cc")

add_library(tinyexr STATIC ${tinyexr_sources})

target_include_directories(tinyexr PRIVATE ${tinyexr_SOURCE_DIR_}/deps/miniz)

target_link_libraries(tinyexr ${TINYEXR_EXT_LIBRARIES})

target_include_directories(tinyexr PUBLIC ${tinyexr_SOURCE_DIR_})