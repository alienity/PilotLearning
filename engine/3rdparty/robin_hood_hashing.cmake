file(GLOB robin_hood_hashing_resource CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/robin_hood_hashing/*.h")
add_library(robin_hood_hashing INTERFACE ${robin_hood_hashing_resource})
target_include_directories(robin_hood_hashing INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/robin_hood_hashing)