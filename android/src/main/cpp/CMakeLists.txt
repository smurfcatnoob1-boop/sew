cmake_minimum_required(VERSION 3.22.1)

project(vulkan-engine CXX)

# KRİTİK DÜZELTME 1: include_directories'i ayrı bir komut olarak add_library'nin dışına alıyoruz.
include_directories(/sources/android/native_app_glue)

# Düzeltilmiş add_library komutu. Sadece kaynak dosya listeleniyor.
add_library(app-glue STATIC
    /sources/android/native_app_glue/android_native_app_glue.c
)

# Ana kütüphaneyi tanımlama
add_library(vulkan-engine SHARED
    main.cpp
)

# Kütüphane Yolları: app-glue'nun header dosyalarını bulması için ana kütüphaneye yolu ekliyoruz
target_include_directories(vulkan-engine PRIVATE
    /sources/android/native_app_glue
)

# Bağlama (Linking) - KRİTİK DÜZELTME 3: Tek bir komut ve doğru format
target_link_libraries(vulkan-engine
    app-glue
    -Wl,--whole-archive
    -Wl,--no-whole-archive
    
    android
    log
    EGL
    GLESv3
    vulkan 
)
