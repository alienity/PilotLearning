set(TARGET_NAME image)
set(PUBLIC_HDR_DIR include)

# ==================================================================================================
# Sources and headers
# ==================================================================================================
set(PUBLIC_HDRS
        include/image/ColorTransform.h
        include/image/ImageOps.h
        include/image/ImageSampler.h
        include/image/LinearImage.h
)

set(SRCS
        src/ImageOps.cpp
        src/ImageSampler.cpp
        src/LinearImage.cpp
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

target_link_libraries(${TARGET_NAME} PilotRuntime)
