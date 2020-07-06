cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

project(platform LANGUAGES CXX)

add_library(platform 
STATIC
    include/platform/build_target.h
    include/platform/enums.h
    include/platform/Extent.h
    include/platform/Window.h
    include/platform/Library.h
)

target_include_directories(platform
PUBLIC
    include
PRIVATE
    include/platform
    src
)

target_link_libraries(platform
PUBLIC
    prebuilt
)

set_target_properties(platform
PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
    target_sources(platform
    PRIVATE
        include/platform/osx/Osx_window.h
        include/platform/posix/Posix_library.h
        src/osx/Osx_window.mm
        src/posix/Posix_library.cpp
    )

    target_compile_options(platform
    PUBLIC
        -x objective-c++)

    target_link_libraries(platform
    PUBLIC
        "-framework Cocoa"
        "-framework CoreVideo"
    )

    set_target_properties(platform
    PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )
elseif(CMAKE_SYSTEM_NAME MATCHES iOS)
    target_sources(platform
    PRIVATE
        include/platform/ios/Ios_window.h
        src/ios/Ios_window.mm
    )

    target_compile_options(platform
    PUBLIC
        -x objective-c++
    )

    target_link_libraries(platform
    PUBLIC
        "-framework UIKit"
    )

    set_target_properties(platform
    PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    )
elseif(CMAKE_SYSTEM_NAME MATCHES Android)
    add_library(native_app_glue
    STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.h
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    )

    target_include_directories(native_app_glue
    PUBLIC
        ${ANDROID_NDK}/sources/android/native_app_glue
    )

    target_sources(platform
    PRIVATE
        include/platform/android/Android_window.h
        include/platform/posix/Posix_library.h
        src/android/Android_window.cpp
        src/posix/Posix_library.cpp
    )

    target_link_libraries(platform
    PUBLIC
        android
        log
        native_app_glue
    )
elseif(CMAKE_SYSTEM_NAME MATCHES Windows)
    target_sources(platform
    PRIVATE
        include/platform/windows/Windows_window.h
        include/platform/windows/Windows_library.h
        src/windows/Windows_window.cpp
        src/windows/Windows_library.cpp
    )

    target_compile_definitions(platform
    PUBLIC
        NOMINMAX
        UNICODE
        WIN32_LEAN_AND_MEAN
    )
endif()

if(BUILD_TESTING)
    add_executable(platform_cts
        test/main.cpp
        test/window_cts.cpp
    )

    target_link_libraries(platform_cts
    PRIVATE
        platform
    )

    set_target_properties(platform_cts
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS ON
    )

    enable_testing()
    add_test(platform_cts platform_cts)
endif()
