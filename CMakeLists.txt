# CMake Versiyonu
cmake_minimum_required(VERSION 3.10)

# Proje Adı
project(vulkan-engine C CXX)

# Kütüphaneyi oluşturma
add_library(vulkan-engine
    SHARED
    android/src/main/cpp/main.cpp
)

# KRİTİK: Header Dosyası Yolunu Zorla Tanımlama
# ANDROID_NDK değişkeni, Gradle tarafından CMake'e her zaman iletilir.
target_include_directories(vulkan-engine PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue 
)

# Derleme bayrakları
target_compile_options(vulkan-engine PRIVATE 
    -std=c++17 
    -Wall 
    -Wextra
)

# NDK Modüllerini Bağlama
# Gradle/AGP bu linkleme ismini (android_native_app_glue) otomatik olarak bulur ve çözer.
target_link_libraries(vulkan-engine
    android_native_app_glue
    android
    log
    EGL
    GLESv3
    vulkan
)

