cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

add_library(sc
STATIC
    include/sc/enums.h
    include/sc/Preamble.h
    include/sc/Spirv_compiler.h
    include/sc/Spirv_reflector.h
    include/sc/Glsl_compiler.h
    include/sc/Msl_compiler.h
    src/std_lib.h
    src/Includer.h
    src/Preamble.cpp
    src/Spirv_compiler.cpp
    src/Spirv_reflector.cpp
    src/Glsl_compiler.cpp
    src/Msl_compiler.cpp
    src/Includer.cpp
)

target_include_directories(sc
PUBLIC
    include
PRIVATE
    include/sc
    src
)

target_link_libraries(sc
PUBLIC
    prebuilt
PRIVATE
    platform
)

set_target_properties(sc
PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)

add_executable(sc_demo
    demo/sc_demo.cpp
)

target_link_libraries(sc_demo
PUBLIC
    sc
)

set_target_properties(sc_demo
PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)
