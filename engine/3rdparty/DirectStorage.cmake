add_library(DirectStorage STATIC IMPORTED GLOBAL)

set_property(TARGET DirectStorage PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Microsoft.Direct3D.DirectStorage.1.0.2/native/lib/x64/dstorage.lib)
set_property(TARGET DirectStorage PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/Microsoft.Direct3D.DirectStorage.1.0.2/native/include)