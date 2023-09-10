set(TARGET_NAME imageio)
set(PUBLIC_HDR_DIR include)

# ==================================================================================================
# Sources and headers
# ==================================================================================================
set(PUBLIC_HDRS
        include/imageio/BasisEncoder.h
        include/imageio/HDRDecoder.h
        include/imageio/ImageDecoder.h
        include/imageio/ImageDiffer.h
        include/imageio/ImageEncoder.h
)

set(SRCS
        src/BasisEncoder.cpp
        src/HDRDecoder.cpp
        src/ImageDecoder.cpp
        src/ImageDiffer.cpp
        src/ImageEncoder.cpp
)

# ==================================================================================================
# Include and target definitions
# ==================================================================================================
include_directories(${PUBLIC_HDR_DIR})

add_library(${TARGET_NAME} STATIC ${PUBLIC_HDRS} ${SRCS})

add_definitions(-DUNICODE -D_UNICODE)

target_include_directories(${TARGET_NAME} PUBLIC ${PUBLIC_HDR_DIR})

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "${TARGET_NAME}")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "${ToolRoot}")

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_link_libraries(${TARGET_NAME} PilotRuntime image)

# target_link_libraries(${TARGET} PUBLIC image math png tinyexr utils z basis_encoder)

