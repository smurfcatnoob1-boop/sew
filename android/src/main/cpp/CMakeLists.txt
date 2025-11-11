cmake_minimum_required(VERSION 3.22.1)

project(vulkan-engine CXX)

# KRİTİK DÜZELTME: include_directories'i ayrı bir komut olarak add_library'nin dışına alıyoruz.
include_directories(/sources/android/native_app_glue)

# NDK'nın .c kaynak dosyasını kullanarak kendi app-glue kütüphanemizi oluşturuyoruz
add_library(app-glue STATIC
    /sources/android/native_app_glue/android_native_app_glue.c
)

# Ana kütüphaneyi tanımlama
add_library(vulkan-engine SHARED
    main.cpp
)

# Bağlama (Linking) - İkinci target_link_libraries çağrısı hala hatalı,
# ancak şimdilik sadece add_library hatasını çözmek için eski yapıyı koruyoruz.
target_link_libraries(vulkan-engine
target_link_libraries(native_app android log)
    -Wl,--whole-archive
    app-glue
    -Wl,--no-whole-archive

    # Gerekli NDK ve Vulkan kütüphaneleri
    android
    log
    vulkan
)
