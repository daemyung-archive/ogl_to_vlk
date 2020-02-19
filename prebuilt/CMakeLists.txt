add_library(prebuilt INTERFACE)

target_include_directories(prebuilt
INTERFACE
    include
)

target_link_directories(prebuilt
INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
