# CMake Versiyonu
cmake_minimum_required(VERSION 3.10)

# Proje Adı
project(vulkan-engine C CXX)

# Kütüphaneyi oluşturma
add_library(vulkan-engine
    SHARED
    android/src/main/cpp/main.cpp
)

# Derleme bayrakları
target_compile_options(vulkan-engine PRIVATE 
    -std=c++17 
    -Wall 
    -Wextra
)

# KRİTİK: NDK Modüllerini İçeri Aktarma (Gradle/AGP'nin işini yapmasını sağlar)
find_package(AndroidNdkModules)
android_ndk_import_module_native_app_glue()


# Gerekli kütüphaneleri bağlama (Linking)
target_link_libraries(vulkan-engine
    # NDK modülünü linkle (Gradle/AGP bu kütüphanenin yolunu otomatik bulur)
    android_native_app_glue
    
    # Vulkan için temel kütüphaneler
    android
    log
    EGL
    GLESv3
    vulkan
)

# KRİTİK: Include yolunu manuel olarak ekle (header bulma sorununu çözer)
target_include_directories(vulkan-engine PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue 
)

