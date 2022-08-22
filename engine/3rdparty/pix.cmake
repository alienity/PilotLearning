add_library(pix STATIC IMPORTED GLOBAL)

set_property(TARGET pix PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/WinPixEventRuntime/bin/x64/WinPixEventRuntime.lib)
set_property(TARGET pix PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/WinPixEventRuntime/Include/WinPixEventRuntime)