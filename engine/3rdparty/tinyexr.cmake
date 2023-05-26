set(tinyexr_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/tinyexr-1.0.2)

file(GLOB tinyexr_sources CONFIGURE_DEPENDS  
"${tinyexr_SOURCE_DIR_}/tinyexr.h" 
"${tinyexr_SOURCE_DIR_}/tinyexr.cc")

add_library(tinyexr INTERFACE ${tinyexr_sources})
target_include_directories(tinyexr INTERFACE ${tinyexr_SOURCE_DIR_})