set(cityhash_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/cityhash)

file(GLOB city_sources CONFIGURE_DEPENDS  
"${cityhash_SOURCE_DIR_}/city.h" 
"${cityhash_SOURCE_DIR_}/city.cc")

add_library(cityhash STATIC ${city_sources})
target_include_directories(cityhash PUBLIC ${cityhash_SOURCE_DIR_})