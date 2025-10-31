cmake_minimum_required(VERSION 3.10.2)
project(vulkan-engine CXX)

add_library(app_glue_src STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

set_target_properties(app_glue_src PROPERTIES LINKER_LANGUAGE "C")

add_library(vulkan-engine SHARED
    main.cpp
)

target_include_directories(vulkan-engine PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)

target_link_libraries(vulkan-engine
    app_glue_src
    android
    EGL
    GLESv3
    vulkan
    log
)
