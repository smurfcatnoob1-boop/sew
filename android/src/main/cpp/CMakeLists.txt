cmake_minimum_required(VERSION 3.18)

# NDK Native Glue (Android NativeActivity bağlayıcısı)
add_library(
    app_glue
    STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

# Vulkan Motoru (Kaynak Dosyalarımız)
add_library(
    vulkan-engine
    SHARED
    main.cpp
)

# Vulkan 1.2 ve diğer NDK kütüphanelerini bağlama
target_link_libraries( 
    vulkan-engine
    android
    app_glue
    log
    GLESv2 
    Vulkan::Vulkan
)

# Vulkan'ın cihazda bulunmasını zorlama
find_package(Vulkan REQUIRED)
