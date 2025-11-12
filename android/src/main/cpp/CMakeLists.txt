# CMake Versiyonu
cmake_minimum_required(VERSION 3.10)

# Proje Adı
project(vulkan-engine C CXX)

# Kütüphaneyi oluşturma
# Düzeltme: Dosya yolunu sadece "main.cpp" olarak güncelliyoruz.
add_library(vulkan-engine
    SHARED
    main.cpp 
)

# KRİTİK: Header Dosyası Yollarını Tanımlama
target_include_directories(vulkan-engine PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
    ../../../ndk_includes/android 
)

# Derleme bayrakları
target_compile_options(vulkan-engine PRIVATE
    -std=c++17
    -Wall
    -Wextra
)

# NDK Modüllerini Bağlama (app_glue, android, log, vulkan)
target_link_libraries(vulkan-engine
    android_native_app_glue
    android
    log
    EGL
    GLESv3
    vulkan
)
