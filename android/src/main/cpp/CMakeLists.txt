cmake_minimum_required(VERSION 3.10.2)
project(vulkan-engine CXX)

# NDK'dan gelen android_native_app_glue kaynak kodunu ekliyoruz
add_library(app_glue_src STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

add_library(vulkan-engine SHARED
    main.cpp
)

# app_glue'nun include klasörünü vulkan-engine'e ekliyoruz
target_include_directories(vulkan-engine PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)

# app_glue, log ve vulkan kütüphanelerini linkliyoruz
# app_glue.a yerine app_glue_src'yi STATIC olarak bağlayarak C/C++ uyumluluğunu çözebiliriz.
target_link_libraries(vulkan-engine
    app_glue_src  # app_glue'nun doğrudan kaynak kodunu bağla
    android
    EGL
    GLESv3
    vulkan
    log
)
