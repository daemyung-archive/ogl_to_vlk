add_library(prebuilt INTERFACE)

target_include_directories(prebuilt
INTERFACE
    include
)

target_link_directories(prebuilt
INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_compile_definitions(prebuilt
INTERFACE
    FMT_HEADER_ONLY
)

target_link_libraries(prebuilt
INTERFACE
    glslang
    SPIRV
    HLSL
    SPVRemapper
    OSDependent
    OGLCompiler
    SPIRV-Tools-opt
    SPIRV-Tools-link
    SPIRV-Tools-reduce
    SPIRV-Tools
    spirv-cross-core
    spirv-cross-util
    spirv-cross-reflect
    spirv-cross-glsl
    spirv-cross-hlsl
    spirv-cross-msl
    spirv-cross-c
    spirv-cross-cpp
)
